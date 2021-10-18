#include <DS3231.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <Wire.h>

//const char WiFiPassword[] = "12345678";
//const char AP_NameChar[] = "LEDControl";

char ssid[]="SmartRG-85ea";
char pass[]="8d549a5c26";

WiFiServer server(80);
DS3231 Clock;
bool century = false;
bool h12Flag;
bool pmFlag;

String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
String html_1 = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'/><meta charset='utf-8'><style>body {font-size:140%;} #main {display: table; margin: auto;  padding: 0 10px 0 10px; } h2,{text-align:center; } .onbutton { padding:10px 10px 10px 10px; width:100%;  background-color: #4CAF50; font-size: 120%;} .offbutton { padding:10px 10px 10px 10px; width:100%;  background-color: #af4c4c; font-size: 120%;}</style><title>Pool Control</title></head><body><div id='main'><h2>Pool Control</h2>";
String html_2 = "<form method='get'><input type='hidden' value='1' name='BUMP'><input type='hidden' value='0' name='na'><input type='submit' value='Bump'></form></p><p><form method='get' onsubmit='synctime()'><div id='time'></div><input type='hidden' value='0' name='na'><input type='submit' value='Sync Time'></form></p><p><script>function synctime(){let currentDate=new Date();let cDoW=currentDate.getDay();let cDay=currentDate.getDate();let cMonth=currentDate.getMonth()+1;let cYear=currentDate.getYear()-100;let cHour=currentDate.getHours();let cMins=currentDate.getMinutes();let cSec=currentDate.getSeconds();document.getElementById('time').innerHTML='<input type=hidden name=SETDOW value='+cDoW+'><input type=hidden name=SETDATE value='+cDay+'><input type=hidden name=SETMNTH value='+cMonth+'><input type=hidden name=SETYEAR value='+cYear+'><input type=hidden name=SETHR value='+cHour+'><input type=hidden name=SETMIN value='+cMins+'><input type=hidden name=SETSEC value='+cSec+'>';}</script>";
String html_3 = "";
String html_4 = "<p><a href='index'>Main Page</a> - <a href='confg'>Configuration Page</a> - <a href='sched'>Schedual Page</a></p></body></html>";
String html_5 = "<form method='get'><table><tr><th>Day</th><th colspan=2>Time</th><th>Power</th></tr>";
String html_6 = "<td rowspan=4><input type=text value=Weekend disabled></td>";
String html_7 = "<td rowspan=4><input type=text value=Weekday disabled></td>";
String html_8 = "</table><input type=hidden value=0 name=na><input type=submit value='Update Schedule'></form>";

String html_status = "";
String curdatetime = "";
String request = "";

String DoW[7]={"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
int Max[3]={23, 55, 4};
int Step[3]={1,5,1};

int Pool_Pin = D3;

int Program[8][3]; //program schedule.  Rows 0~3=Weekdays, Rows 4~7=Weekends
                   //                   Col 0=Powerlevel, Col 1=Hour & Col2=Mins for start time.

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
  
  // put your main code here, to run repeatedly:
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
//    client.print(html_3);  // Send debug info to client.
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
//    client.print(html_3);  // Send debug info to client.
    client.print(html_5);
    for(int i=0;i<8;i++){
      client.print("<tr>");
      if(i==0){
        client.print(html_6);
      }else if(i==4){
        client.print(html_7);
      }
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
  
  delay(5);
}

void updatestatus(){
  int i;
  
  getdatetime();
  i=digitalRead(Pool_Pin);

  html_status="<table><tr><th>Time:</th><td>";
  html_status.concat(curdatetime);
  html_status.concat("</td></tr><tr><th>Pump:</th><td>");
  if(i==0){
    html_status.concat("On");
  }else{
    html_status.concat("Off");
  }
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
