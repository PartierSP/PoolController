#include <DS3231.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <uEEPROMLib.h>
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

//uEEPROMLib eeprom(0x57);
uEEPROMLib eeprom;

char ssid[]=SSID_NAME;
char pass[]=NET_PASSWD;

int Tier_1_Savings=0;
int Tier_2_Savings=0;
int Tier_3_Savings=0;

int Tier_1_Used=0;
int Tier_2_Used=0;
int Tier_3_Used=0;

int Tier_Mode=0;
int Last_Min=0;

float Tier_1_Rate;
float Tier_2_Rate;
float Tier_3_Rate;

int Wattage;

uint32_t Starttime;

WiFiServer server(80);
DS3231 Clock;
RTClib myRTC;
bool century = false;
int int_8[4];
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

byte ToU[8][3];     //Time of Use   Rows 0~3=Weekdays, Rows 4~7=Weekends
                    //              Col 2=Rate, Col 0=Hour & Col1=Mins for start time.

void setup() {
  Serial.begin(115200);
  pinMode(Pool_Pin, OUTPUT);
  digitalWrite(Pool_Pin, 1);

  Wire.begin();

  delay(500);

  DateTime now = myRTC.now();
  Starttime=now.unixtime();

  for(int i=0;i<8;i++){
    for(int x=0;x<3;x++){
      Program[i][x]=eeprom.eeprom_read(i*3+x);
      ToU[i][x]=eeprom.eeprom_read(i*3+x+24);
    }
  }
  Tier_1_Rate=eeprom.eeprom_read(48);
  Tier_2_Rate=eeprom.eeprom_read(52);
  Tier_3_Rate=eeprom.eeprom_read(56);
  Wattage=eeprom.eeprom_read(60)*5;

  Serial.println(Tier_1_Rate);
  Serial.println(Tier_2_Rate);
  Serial.println(Tier_3_Rate);

  Tier_1_Rate=Tier_1_Rate/(float)1000;
  Tier_2_Rate=Tier_2_Rate/(float)1000;
  Tier_3_Rate=Tier_3_Rate/(float)1000;

  Tier_1_Savings=getint(61);
  Tier_2_Savings=getint(65);
  Tier_3_Savings=getint(69);
  Tier_1_Used=getint(73);
  Tier_2_Used=getint(77);
  Tier_3_Used=getint(81);
   
  
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
  int a;
  int b;
  
  CheckOutput();
  UpdateOutput();

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
        } else if(strcmp("RESET",desc)==0){
          Serial.println("Resetting accumulated values.");
          Tier_1_Savings=0;
          Tier_2_Savings=0;
          Tier_3_Savings=0;
          Tier_1_Used=0;
          Tier_2_Used=0;
          Tier_3_Used=0;
          toint8(Tier_1_Savings);
          saveint(61);
          toint8(Tier_2_Savings);
          saveint(65);
          toint8(Tier_3_Savings);
          saveint(69);
          toint8(Tier_1_Used);
          saveint(73);
          toint8(Tier_2_Used);
          saveint(77);
          toint8(Tier_3_Used);
          saveint(81);
        } else if(strcmp("TIER1RATE",desc)==0){
          Serial.print("Tier Rate 1 is now settings to: ");
          Serial.println(value);
          i=atoi(value);
          Serial.println(i);
          eeprom.eeprom_write(48,i/10);
          Tier_1_Rate=(float)i/(float)10000;
        } else if(strcmp("TIER2RATE",desc)==0){
          Serial.print("Tier Rate 2 is now settings to: ");
          Serial.println(value);
          i=atoi(value);
          Serial.println(i);
          eeprom.eeprom_write(52,i/10);
          Tier_2_Rate=(float)i/(float)10000;
        } else if(strcmp("TIER3RATE",desc)==0){
          Serial.print("Tier Rate 3 is now settings to: ");
          Serial.println(value);
          i=atoi(value);
          Serial.println(i);
          eeprom.eeprom_write(56,i/10);
          Tier_3_Rate=(float)i/(float)10000;
        } else if(strcmp("WATTAGE",desc)==0){
          Serial.print("Wattage is now settings to: ");
          Serial.println(value);
          Wattage=atoi(value);
          eeprom.eeprom_write(60,Wattage/5);
        } else {
          String s_desc=convertToString(desc,sizeof(desc));
          if(s_desc.startsWith("prog5B")){
            row=(int)desc[6]-48;
            col=(int)desc[11]-48;
            ToU[row][col]=atoi(value);
            eeprom.eeprom_write(row*3+col+24,ToU[row][col]);
            Serial.print("Row: ");
            Serial.print(row);
            Serial.print(" Col: ");
            Serial.print(col);
            Serial.print(" Value: ");
            Serial.println(ToU[row][col]);
          }
        }
        desc[0]='\0';
        value[0]='\0';
      }
      idx++;
    }
    updatestatus();
    client.flush();
    client.print(header);
    client.print(html_1);
    client.print(html_status);
    client.print("<h3>Time of Use</h3>");
    client.print(html_5);
    client.print("Tier");
    client.print(html_9);
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
        client.print("] min=");
        if(x==2){
          client.print("1");
        }else{
          client.print("0");
        }
        client.print(" max=");
        client.print(Max[x+3]);
        client.print(" step=");
        client.print(Step[x]);
        client.print(" value=");
        client.print(ToU[i][x]);
        client.print(" class='w3-input w3-border-0'></td>");
      }
      client.print("</tr>");
    }
    client.print(html_10);
    client.print(Tier_1_Rate*100);
    client.print(html_13);
    client.print(html_11);
    client.print(Tier_2_Rate*100);
    client.print(html_13);
    client.print(html_12);
    client.print(Tier_3_Rate*100);
    client.print(html_13);
    client.print(html_14);
    client.print(Wattage);
    client.print(html_13);
    client.print(html_8);
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
          eeprom.eeprom_write(row*3+col,Program[row][col]);
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
    updatestatus();
    client.flush();
    client.print(header);
    client.print(html_1);
    client.print(html_status);
    client.print(html_5);
    client.print("Power");
    client.print(html_9);
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

    UpdateOutput();
    
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
  float kwSavings;
  float Savings;
  float kwUsed;
  float Used;
  
  getdatetime();
  DateTime now = myRTC.now();

  i=digitalRead(Pool_Pin);
  rssi = WiFi.RSSI();
  sgnl=map(rssi, MIN_VAL, MAX_VAL, 0, 100); 

  html_status="<div class='w3-panel w3-card-4 w3-white w3-round-large w4-padding w3-center'><table class='w3-table w3-bordered'><tr><th style='width:25%'>Time:</th><td>";
  html_status.concat(curdatetime);
  html_status.concat("</td><th>Temperature:</th><td>");
  html_status.concat(Clock.getTemperature());
  html_status.concat("<sup>o</sup>C</td></tr><tr><th>Pump:</th><td colspan='4' class='w3-text-");
  if(i==0){
    html_status.concat("green'><b>On");
  }else{
    html_status.concat("red'><b>Off");
  }
  html_status.concat("</b></td></tr><tr><th>Mode:</th><td colspan='4'>");
  switch(function){
    case 0:
      if(ManOveride==true){
        html_status.concat("Temporary ON</td></tr><tr><th>Remaining:</th><td colspan='4'><div class='w3-container w3-center w3-green' style='width:");
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
        html_status.concat("<tr><th>Pump Power: </th><td colspan='4'><div class='w3-container w3-center w3-green' style='width:");
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
  html_status.concat("<tr><th>Uptime: </th><td>");
  html_status.concat((float)(now.unixtime() - Starttime) / 86400L);
  html_status.concat(" days</td><th>Log Length:</th><td colspan='2'>");
  html_status.concat((float)((Tier_1_Used + Tier_2_Used + Tier_3_Used + Tier_1_Savings + Tier_2_Savings + Tier_3_Savings)/(float)1440));
  html_status.concat(" days</td></tr><tr><th>Prg Line: </th><td>");
  html_status.concat(line);
  html_status.concat("</td><th>Current Tier:</th><td colspan='2'>Tier ");
  html_status.concat(Tier_Mode+1);
  html_status.concat(" - $");
  switch(Tier_Mode){
    case 0:
      html_status.concat(Tier_1_Rate);
      break;
    case 1:
      html_status.concat(Tier_2_Rate);
      break;
    case 2:
      html_status.concat(Tier_3_Rate);
      break;
  }
  html_status.concat("/kWhr");
  html_status.concat("</td></tr><tr><th>Signal Strength:</th><td colspan='4'><div class='w3-container w3-center w3-");
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
  html_status.concat("%</div></td></tr><tr><td></td><th>Tier 1</th><th>Tier 2</th><th>Tier 3</th><th>Total</th></tr><tr><th>Used:</th><td>");
  kwUsed=Tier_1_Used * Wattage / (float)60000;
  html_status.concat(kwUsed);
  html_status.concat(" kWHr<br>$ ");
  Used=Tier_1_Used * Tier_1_Rate * Wattage / 60000;
  html_status.concat(Used);
  html_status.concat("</td><td>");
  kwUsed=Tier_2_Used * Wattage / (float)60000;
  html_status.concat(kwUsed);
  html_status.concat(" kWHr<br>$ ");
  Used=Tier_2_Used * Tier_2_Rate * Wattage / 60000;
  html_status.concat(Used);
  html_status.concat("</td><td>");
  kwUsed=Tier_3_Used * Wattage / (float)60000;
  html_status.concat(kwUsed);
  html_status.concat(" kWHr<br>$ ");
  Used=Tier_3_Used * Tier_3_Rate * Wattage / 60000;
  html_status.concat(Used);
  html_status.concat("</td><td>");
  kwUsed=(Tier_1_Used + Tier_2_Used + Tier_3_Used) * Wattage / (float)60000;
  html_status.concat(kwUsed);
  html_status.concat(" kWHr<br>$ ");
  Used=((Tier_1_Used * Tier_1_Rate) + (Tier_2_Used * Tier_2_Rate) + (Tier_3_Used * Tier_3_Rate)) * Wattage / 60000;
  html_status.concat(Used);
  html_status.concat("</td></tr><tr><th>Savings:</th><td>");
  kwSavings=Tier_1_Savings * Wattage / (float)60000;
  html_status.concat(kwSavings);
  html_status.concat(" kWHr<br>$ ");
  Savings=Tier_1_Savings * Tier_1_Rate * Wattage / 60000;
  html_status.concat(Savings);
  html_status.concat("</td><td>");
  kwSavings=Tier_2_Savings * Wattage / (float)60000;
  html_status.concat(kwSavings);
  html_status.concat(" kWHr<br>$ ");
  Savings=Tier_2_Savings * Tier_2_Rate * Wattage / 60000;
  html_status.concat(Savings);
  html_status.concat("</td><td>");
  kwSavings=Tier_3_Savings * Wattage / (float)60000;
  html_status.concat(kwSavings);
  html_status.concat(" kWHr<br>$ ");
  Savings=Tier_3_Savings * Tier_3_Rate * Wattage / 60000;
  html_status.concat(Savings);
  html_status.concat("</td><td>");
  kwSavings=(Tier_1_Savings + Tier_2_Savings + Tier_3_Savings) * Wattage / (float)60000;
  html_status.concat(kwSavings);
  html_status.concat(" kWHr<br>$ ");
  Savings=((Tier_1_Savings * Tier_1_Rate) + (Tier_2_Savings * Tier_2_Rate) + (Tier_3_Savings * Tier_3_Rate)) * Wattage / 60000;
  html_status.concat(Savings);
  html_status.concat("</td></tr></table><br></div>");
  
  
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
  Tier_Mode=ToU[3+x][2]-1;
  line=3;
  for(i=0+x;i<4+x;i++){
    if((Program[i][0]*100)+Program[i][1]<=product){
      mode=Program[i][2];
      line=i;
    }
    if((ToU[i][0]*100)+ToU[i][1]<=product){
      Tier_Mode=ToU[i][2]-1;
    }
  }
}

void UpdateOutput(){

  int i;
  
  Minute=Clock.getMinute();
  i=Schedule[mode][Minute];

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
  if(Minute!=Last_Min){
    if(digitalRead(Pool_Pin)==true){
      switch(Tier_Mode){
        case 0:
          Tier_1_Savings++;
          break;
        case 1:
          Tier_2_Savings++;
          break;
        case 2:
          Tier_3_Savings++;
          break;
      }    
    }else{
      switch(Tier_Mode){
        case 0:
          Tier_1_Used++;
          break;
        case 1:
          Tier_2_Used++;
          break;
        case 2:
          Tier_3_Used++;
          break;
      }    
    }
    Last_Min=Minute;

    if(Minute==0){
      //top of hour, backup power usage
      toint8(Tier_1_Savings);
      saveint(61);
      toint8(Tier_2_Savings);
      saveint(65);
      toint8(Tier_3_Savings);
      saveint(69);
      toint8(Tier_1_Used);
      saveint(73);
      toint8(Tier_2_Used);
      saveint(77);
      toint8(Tier_3_Used);
      saveint(81);
    }

  }
}

void toint8 (int i){
  int_8[0]=( i >>  0) & 0xFF;
  int_8[1]=( i >>  8) & 0xFF;
  int_8[2]=( i >> 16) & 0xFF;
  int_8[3]=( i >> 24) & 0xFF;
}

void saveint(int address){
  eeprom.eeprom_write(address+0,int_8[0]);
  eeprom.eeprom_write(address+1,int_8[1]);
  eeprom.eeprom_write(address+2,int_8[2]);
  eeprom.eeprom_write(address+3,int_8[3]);
}

int getint(int address){
  int i;

  i=eeprom.eeprom_read(address+3);
  i=i << 8;
  i=i+eeprom.eeprom_read(address+2);
  i=i << 8;
  i=i+eeprom.eeprom_read(address+1);
  i=i << 8;
  i=i+eeprom.eeprom_read(address+0);

  return i;

}
