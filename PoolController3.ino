#include <DS3231.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "config.h"
#include "constants.h"
#include "html.h"

//The config.h file is used to configure the network SSID and password.  It should
//contain the following two lines (subsituting your SSID name and password for
//myssid mypassword).  Quotes must be included.
//
//  #define SSID_NAME "myssid"
//  #define NET_PASSWD "mypassword"
//

char ssid[]=SSID_NAME;
char pass[]=NET_PASSWD;

WiFiServer server(80);
DS3231 Clock;
bool century = false;
bool h12Flag;
bool pmFlag;
byte Hour;
byte Minute;
byte mode;
byte line;
bool ManOveride;
byte ManOvOff;
byte function = 0;  // Which is running. 0 - Automatic or TempManOveride
                    //                   1 - Force on
                    //                   2 - Force off

String html_status = "";
String curdatetime = "";
String request = "";

byte Program[8][3]; //program schedule.  Rows 0~3=Weekdays, Rows 4~7=Weekends
                    //                   Col 2=Powerlevel, Col 0=Hour & Col1=Mins for start time.

void setup() {
  Serial.begin(115200);
  pinMode(Pool_Pin, OUTPUT);
  digitalWrite(Pool_Pin, 1);

  Wire.begin();

  EEPROM.begin(32);
  for(int i=0;i<8;i++){
    for(int x=0;x<4;x++){
      Program[i][x]=EEPROM.read(i*4+x);
    }
  }

  ManOveride=false;

  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid,pass);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(Pool_Pin, 0);
    delay(5);
    digitalWrite(Pool_Pin, 1);
    delay(500);
  }
  Serial.println("");
  Serial.println("[CONNECTED]");
  Serial.print("[IP ");
  Serial.print(WiFi.localIP());
  Serial.println("]");

//  boolean conn = WiFi.softAP(AP_NameChar, WiFiPassword);
  server.begin();
}

void loop() {
  int idx;
  char c;
  char desc[16]="";
  char value[16]="";
  int editvalue=0;
  int i;
  int col;
  int row;
  
  CheckOutput();
  Minute=Clock.getMinute();
  i=Schedule[mode][Minute];
//  if(i==digitalRead(Pool_Pin)){
//    Serial.print("Running Mode: ");
//    Serial.println(mode);
//  }
  switch(function){
    case 0:
      if(ManOveride==true){
        if(Minute==ManOvOff){
          digitalWrite(Pool_Pin, true);
          ManOveride=false;
        }else{
          digitalWrite(Pool_Pin,false);
        }
      }else{
        digitalWrite(Pool_Pin, !i);
      }
      break;
    case 1:
      digitalWrite(Pool_Pin, 0);
      break;
    case 2:
      digitalWrite(Pool_Pin, 1);
      break;
  }

  delay(50);

  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  Serial.println("New client");
  request=client.readStringUntil('\r');
  Serial.println(request);
  if(request.indexOf("confg")>0){
    //Config page requested
    idx=request.indexOf("confg")+5;
    while(idx<request.length()){
      c=request[idx];
      if(isalnum(c)){
        if(editvalue==1){
          append(value,c);
        }else{
          append(desc,c);
        }
      }else if(c=='='){
        editvalue=1;
      }else if(c=='&'){
        editvalue=0;

       Serial.print(desc);
        Serial.print("=");
        Serial.println(value);
        if(strcmp("SETMIN",desc)==0){
          Serial.print("Setting minuites to: ");
          Serial.println(value);
          i=atoi(value);
          Clock.setMinute(i);
        } else if(strcmp("SETHR",desc)==0){
          Serial.print("Setting hours to: ");
          Serial.println(value);
          i=atoi(value);
          Clock.setHour(i);
        } else if(strcmp("SETSEC",desc)==0){
          Serial.print("Setting seconds to: ");
          Serial.println(value);
          i=atoi(value);
          Clock.setSecond(i);
        } else if(strcmp("SETMNTH",desc)==0){
          Serial.print("Setting month to: ");
          Serial.println(value);
          i=atoi(value);
          Clock.setMonth(i);
        } else if(strcmp("SETDATE",desc)==0){
          Serial.print("Setting day of the month to: ");
          Serial.println(value);
          i=atoi(value);
          Clock.setDate(i);
        } else if(strcmp("SETDOW",desc)==0){
          Serial.print("Setting day of the week to: ");
          Serial.println(value);
          i=atoi(value);
          if (i==0){
            i=7; //Javascript sets Sunday to 0, DS3231 doesn't like that.
          }
          Clock.setDoW(i);
        } else if(strcmp("SETYEAR",desc)==0){
          Serial.print("Setting year to: ");
          Serial.println(value);
          i=atoi(value);
          Clock.setYear(i);
        } else if(strcmp("BUMP",desc)==0){
          Serial.println("Bumping!");
          delay(100);
          i=atoi(value);
          if(i==1){
            i=digitalRead(Pool_Pin);
            Serial.println(!i);
            digitalWrite(Pool_Pin,!i);
            delay(500);
            Serial.println(i);
            digitalWrite(Pool_Pin,i);
          }
        }
      }
      idx++;
    }
    updatestatus();
    client.flush();
    client.print(header);
    client.print(html_1);
    client.print(html_status);
    client.print(html_2);
    client.print(html_4);

  }else if(request.indexOf("sched")>0){
    //Schedual page requested
    idx=request.indexOf("sched")+5;
    while(idx<request.length()){
      c=request[idx];
      if(isalnum(c)){
        if(editvalue==1){
          append(value,c);
        }else{
          append(desc,c);
        }
      }else if(c=='='){
        editvalue=1;
      }else if(c=='&'){
        editvalue=0;

        Serial.print(desc);
        Serial.print("=");
        Serial.println(value);

        String s_desc=convertToString(desc,sizeof(desc));
        if(s_desc.startsWith("prog5B")){
          row=(int)desc[6]-48;
          col=(int)desc[11]-48;
          Program[row][col]=atoi(value);
          EEPROM.put(row*4+col,Program[row][col]);
          Serial.print("Row: ");
          Serial.print(row);
          Serial.print(" Col: ");
          Serial.print(col);
          Serial.print(" Value: ");
          Serial.println(Program[row][col]);
        }
        
        desc[0]='\0';
        value[0]='\0';
      }
      idx++;
    }
    EEPROM.commit();
    updatestatus();
    client.flush();
    client.print(header);
    client.print(html_1);
    client.print(html_status);
    client.print(html_5);
    for(int i=0;i<8;i++){
      client.print("<tr>");
      if(i==0){
        client.print(html_6);
      }else if(i==4){
        client.print(html_7);
      }
      client.print("<td><input type=text value=");
      client.print(i);
      client.print(" disabled class='w3-input w3-border-0'></td>");
      for(int x=0;x<3;x++){
        client.print("<td");
        if(x<2){
          client.print(" class='w3-pale-green'");
        }
        client.print("><input type=number name=prog[");
        client.print(i);
        client.print("][");
        client.print(x);
        client.print("] min=0 max=");
        client.print(Max[x]);
        client.print(" step=");
        client.print(Step[x]);
        client.print(" value=");
        client.print(Program[i][x]);
        client.print(" class='w3-input w3-border-0'></td>");
      }
      client.print("</tr>");
    }
    client.print(html_8);
    client.print(html_4);
    
    
  }else{
    //Index page requested
    if(request.indexOf("?")>0){
      idx=request.indexOf("?")+1;
      while(idx<request.length()){
        c=request[idx];
        if(isalnum(c)){
          if(editvalue==1){
            append(value,c);
          }else{
            append(desc,c);
          }
        }else if(c=='='){
          editvalue=1;
        }else if(c=='&'){
          editvalue=0;
  
          Serial.print(desc);
          Serial.print("=");
          Serial.println(value);
  
          if(strcmp("MANOVRD",desc)==0){
            Serial.println("Starting Manual Overide");
            if(Minute>0){
              ManOvOff=Minute-1;
            }else{
              ManOvOff=59;
            }
            ManOveride=true;
            function=0;
          } else if(strcmp("MANON",desc)==0){
            Serial.println("Manual ON");
            function=1;
            ManOveride=false;
          } else if(strcmp("MANOFF",desc)==0){
            Serial.println("Manual OFF");
            function=2;
            ManOveride=false;
          } else if(strcmp("AUTOON",desc)==0){
            Serial.println("Automatic Mode");
            function=0;
            ManOveride=false;
          }
          desc[0]='\0';
          value[0]='\0';
        }
        idx++;

      }
    }

    updatestatus();
    client.flush();
    client.print(header);
    client.print(html_1);
    client.print(html_status);
    client.print(html_3);
    client.print(html_4);
  }

  Serial.println("");

}

void updatestatus(){
  int i;
  int x;
  long rssi;
  int sgnl;
  
  getdatetime();
  i=digitalRead(Pool_Pin);
  rssi = WiFi.RSSI();
  sgnl=map(rssi, MIN_VAL, MAX_VAL, 0, 100); 

  html_status="<div class='w3-panel w3-card-4 w3-white w3-round-large w4-padding w3-center'><table class='w3-table w3-bordered'><tr><th style='width:25%'>Time:</th><td>";
  html_status.concat(curdatetime);
  html_status.concat("</td></tr><tr><th>Pump:</th><td class='w3-text-");
  if(i==0){
    html_status.concat("green'><b>On");
  }else{
    html_status.concat("red'><b>Off");
  }
  html_status.concat("</b></td></tr><tr><th>Mode:</th><td>");
  switch(function){
    case 0:
      if(ManOveride==true){
        html_status.concat("Temporary ON</td></tr><tr><th>Remaining:</th><td><div class='w3-container w3-center w3-green' style='width:");
        i=ManOvOff-Minute;
        if(i<0){
          i=i+60;
        }
        x=i*5/3;
        html_status.concat(x);
        html_status.concat("%'>");
        html_status.concat(i);
        html_status.concat("mins</div></td></tr>");
      }else{
        html_status.concat("Automatic</td></tr>");
        html_status.concat("<tr><th>Power: </th><td><div class='w3-container w3-center w3-green' style='width:");
        html_status.concat(ModeDesc[mode]);
        html_status.concat("%'>");
        html_status.concat(ModeDesc[mode]);
        html_status.concat("%</div></td></tr>");
      }
      break;
    case 1:
      html_status.concat("Manual On</td></tr>");
      break;
    case 2:
      html_status.concat("Manual Off");
      break;
  }
  html_status.concat("<tr><th>Prg Line: </th><td>");
  html_status.concat(line);
  html_status.concat("</td></tr><tr><th>Signal Strength</th><td><div class='w3-container w3-center w3-");
  if (sgnl>30){
    html_status.concat("green");
  }else{
    if(sgnl<20){
      html_status.concat("red");
    }else{
      html_status.concat("orange");
    }
  }
  html_status.concat("' style='width:");
  html_status.concat(sgnl);
  html_status.concat("%'>");
  html_status.concat(sgnl);
  html_status.concat("%</div></td></tr></table><br></div>");
  
  
}

void getdatetime(){
  curdatetime="";
  curdatetime.concat(sDoW[Clock.getDoW()]);
  curdatetime.concat(" ");
  curdatetime.concat(Clock.getYear());
  curdatetime.concat("/");
  curdatetime.concat(Clock.getMonth(century));
  curdatetime.concat("/");
  curdatetime.concat(Clock.getDate());
  curdatetime.concat(" ");
  curdatetime.concat(Clock.getHour(h12Flag, pmFlag));
  curdatetime.concat(":");
  curdatetime.concat(Clock.getMinute());
  curdatetime.concat(":");
  curdatetime.concat(Clock.getSecond());
}

void append(char* s, char c)
{
  int len = strlen(s);
  s[len]=c;
  s[len+1]='\0';
}

String convertToString(char* a, int size)
{
  int i;
  String s = "";
  for (i = 0; i < size; i++) {
    s = s + a[i];
  }
  return s;
}

void CheckOutput(){
  int product;
  int i;
  int DoW;
  int x;

  Hour=Clock.getHour(h12Flag, pmFlag);
  Minute=Clock.getMinute();
  DoW=Clock.getDoW();

  if (DoW<6){
    x=4; //Weekday.  Skip Weekend schedual lines.
  }else{
    x=0; //Weekend.  Done skip past Weekend schedual lines.
  }

  product=(Hour*100)+Minute;

  mode=Program[3+x][2];
  line=3;
  for(i=0+x;i<4+x;i++){
    if((Program[i][0]*100)+Program[i][1]<=product){
      mode=Program[i][2];
      line=i;
    }
  }
}
