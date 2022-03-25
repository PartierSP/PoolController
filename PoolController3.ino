#include <DS3231.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "config.h"

//The config.h file is used to configure the network SSID and password.  It should
//contain the following two lines (subsituting your SSID name and password for
//myssid mypassword).  Quotes must be included.
//
//  #define SSID_NAME "myssid"
//  #define NET_PASSWD "mypassword"
//

//const char WiFiPassword[] = "12345678";
//const char AP_NameChar[] = "LEDControl";

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

String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
String html_1 = R"====(<!DOCTYPE html><html>
<head>
  <meta charset='utf-8'>
  <link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'> 
  <title>Pool Control</title>
</head>
<body class='w3-pale-blue'>
  <div class='w3-main'>
    <div class='w3-blue'>
      <div class='w3-container'>
        <h1>Pool Control</h1>
      </div>
    </div>
    <div class='w3-container'>
      <div class='w3-auto'>
        <h3>Status</h3>
)====";
String html_2 = R"====(
    <div class='w3-panel w3-card-4 w3-white w3-round-large'>
      <div class='w3-contatiner w3-center w3-padding'>
        <form method='get'><input type='hidden' value='1' name='MANOVRD'><input type='hidden' value='0' name='na'><input type='submit' value='Manual Overide' class='w3-button w3-blue w3-round-large'></form> - <form method='get'><input type='hidden' value='1' name='BUMP'><input type='hidden' value='0' name='na'><input type='submit' value='Bump' class='w3-button w3-blue w3-round-large'></form> - <form method='get' onsubmit='synctime()'><div id='time'></div><input type='hidden' value='0' name='na'><input type='submit' value='Sync Time' class='w3-button w3-blue w3-round-large'></form>
        <script>
          function synctime(){
            let currentDate=new Date();
            let cDoW=currentDate.getDay();
            let cDay=currentDate.getDate();
            let cMonth=currentDate.getMonth()+1;
            let cYear=currentDate.getYear()-100;
            let cHour=currentDate.getHours();
            let cMins=currentDate.getMinutes();
            let cSec=currentDate.getSeconds();
            
            document.getElementById('time').innerHTML='
              <input type=hidden name=SETDOW value='+cDoW+'>
              <input type=hidden name=SETDATE value='+cDay+'>
              <input type=hidden name=SETMNTH value='+cMonth+'>
              <input type=hidden name=SETYEAR value='+cYear+'>
              <input type=hidden name=SETHR value='+cHour+'>
              <input type=hidden name=SETMIN value='+cMins+'>
              <input type=hidden name=SETSEC value='+cSec+'>
            ';
          }
        </script>
      </div>
    </div>
)====";
String html_3 = "";
String html_4 = R"====(
       <div class='w3-panel w3-card-4 w3-white w3-round-large'>
          <div class='w3-container w3-center w3-padding'>
            <a href='index' class='w3-button w3-blue w3-round-large'>Main Page</a> - <a href='confg' class='w3-button w3-blue w3-round-large'>Configuration Page</a> - <a href='sched' class='w3-button w3-blue w3-round-large'>Schedual Page</a>
          </div>
        </div>
      </div>
    </div>
  </div>
</body>
</html>
)====";
String html_5 = "<form method='get'><table border=1 cellpadding=0 cellspacing=0><tr><th>Day</th><th>Line</th><th colspan=2>Time</th><th>Power</th></tr>";
String html_6 = "<td rowspan=4><input type=text value=Weekend disabled></td>";
String html_7 = "<td rowspan=4><input type=text value=Weekday disabled></td>";
String html_8 = "</table><input type=hidden value=0 name=na><input type=submit value='Update Schedule'></form>";

String html_status = "";
String curdatetime = "";
String request = "";

String DoW[7]={"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
int Max[3]={23, 55, 4};
int Step[3]={1,5,1};
int ModeDesc[5]={0,25,50,75,100};

int Pool_Pin = D4;

byte Program[8][3]; //program schedule.  Rows 0~3=Weekdays, Rows 4~7=Weekends
                   //                   Col 2=Powerlevel, Col 0=Hour & Col1=Mins for start time.

                   //Schedule: Rows 0~4 Requested power level (0=off, 1=25%, 2=50%, 3=75%, 4=100%)
                   //          Columns 0~59 Pump status for that minuite
bool Schedule[5][60]={
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};


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

  delay(50);

  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  Serial.println("New client");
  request=client.readStringUntil('\r');
  Serial.println(request);
  if(request.indexOf("confg")>0){
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

        html_3.concat(desc);
        html_3.concat("=");
        html_3.concat(value);
        html_3.concat("<br/>");
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
          Serial.print(value);
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
        } else if(strcmp("MANOVRD",desc)==0){
          Serial.println("Starting Manual Overide");
          if(Minute>0){
            ManOvOff=Minute-1;
          }else{
            ManOvOff=59;
          }
          ManOveride=true;
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
    client.print(html_2);
    client.print(html_4);
    html_3="";

  }else if(request.indexOf("sched")>0){
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

        html_3.concat(desc);
        html_3.concat("=");
        html_3.concat(value);
        html_3.concat("<br/>");
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
//    client.print(html_3);  // Send debug info to client.
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
      client.print(" disabled></td>");
      for(int x=0;x<3;x++){
        client.print("<td><input type=number name=prog[");
        client.print(i);
        client.print("][");
        client.print(x);
        client.print("] min=0 max=");
        client.print(Max[x]);
        client.print(" step=");
        client.print(Step[x]);
        client.print(" value=");
        client.print(Program[i][x]);
        client.print("></td>");
      }
      client.print("</tr>");
    }
    client.print(html_8);
    client.print(html_4);
    html_3="";
    
    
  }else{
    updatestatus();
    client.flush();
    client.print(header);
    client.print(html_1);
    client.print(html_status);
    client.print(html_4);
  }

  Serial.println("");

}

void updatestatus(){
  int i;
  int x;
  
  getdatetime();
  i=digitalRead(Pool_Pin);

  html_status="<table class='w3-table w3-bordered w3-card-4 w3-white w3-round-large'><tr><th style='width:25%'>Time:</th><td>";
  html_status.concat(curdatetime);
  html_status.concat("</td></tr><tr><th>Pump:</th><td class='w3-text-");
  if(i==0){
    html_status.concat("green'><b>On");
  }else{
    html_status.concat("red'><b>Off");
  }
  html_status.concat("</b></td></tr><tr><th>Mode:</th><td>");
  if(ManOveride==true){
    html_status.concat("Manual Overide</td></tr><tr><th>Remaining:</th><td><div class='w3-container w3-center w3-green' style='width:");
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
  html_status.concat("<tr><th>Prg Line: </th><td>");
  html_status.concat(line);
  html_status.concat("</td></tr></table>");
  
  
}

void getdatetime(){
  curdatetime="";
  curdatetime.concat(DoW[Clock.getDoW()]);
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
