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
    <h3>Functions</h3>
    <div class='w3-panel w3-card-4 w3-white w3-round-large w3-center'>
      <div class='w3-cell-row'> 
        <div class='w3-container w3-cell w3-mobile w3-padding'>
          <form method='get'><input type='hidden' value='1' name='BUMP'><input type='hidden' value='0' name='na'><input type='submit' value=' Bump ' class='w3-button w3-blue w3-round-large'></form>
        </div>
        <div class='w3-container w3-cell w3-mobile w3-padding'>
          <form method='get' onsubmit='synctime()'><div id='time'></div><input type='hidden' value='0' name='na'><input type='submit' value='Sync Time' class='w3-button w3-blue w3-round-large'></form>
        </div>
      </div>
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
          
          document.getElementById('time').innerHTML='<input type=hidden name=SETDOW value='
            +cDoW+'><input type=hidden name=SETDATE value='
            +cDay+'><input type=hidden name=SETMNTH value='
            +cMonth+'><input type=hidden name=SETYEAR value='
            +cYear+'><input type=hidden name=SETHR value='
            +cHour+'><input type=hidden name=SETMIN value='
            +cMins+'><input type=hidden name=SETSEC value='
            +cSec+'>';
        }
      </script>
    </div>
)====";
String html_3 = R"====(
    <h3>Mode</h3>
    <div class='w3-panel w3-card-4 w3-white w3-round-large w3-center'>
      <div class='w3-cell-row'>
        <div class='w3-container w3-cell w3-mobile w3-padding'>
          <form method='get'><input type='hidden' value='1' name='AUTOON'><input type='hidden' value='0' name='na'><input type='submit' value='Automatic' class='w3-button w3-green w3-round-large'></form>
        </div>
        <div class='w3-container w3-cell w3-mobile w3-padding'>
          <form method='get'><input type='hidden' value='1' name='MANON'><input type='hidden' value='0' name='na'><input type='submit' value='Manual ON' class='w3-button w3-green w3-round-large'></form>
        </div>
        <div class='w3-container w3-cell w3-mobile w3-padding'>
          <form method='get'><input type='hidden' value='1' name='MANOFF'><input type='hidden' value='0' name='na'><input type='submit' value='Manual OFF' class='w3-button w3-red w3-round-large'></form>
        </div>
        <div class='w3-container w3-cell w3-mobile w3-padding'>
          <form method='get'><input type='hidden' value='1' name='MANOVRD'><input type='hidden' value='0' name='na'><input type='submit' value='Temp. ON' class='w3-button w3-yellow w3-round-large'></form>
        </div>
      </div>
    </div>
)====";
String html_4 = R"====(
        <h3>Navigation</h3>
        <div class='w3-panel w3-card-4 w3-white w3-round-large w3-center'>
          <div class='w3-cell-row'>
            <div class='w3-container w3-cell w3-mobile w3-padding'>
              <a href='index' class='w3-button w3-blue w3-round-large'>Main Page</a>
            </div>
            <div class='w3-container w3-cell w3-mobile w3-padding'>
              <a href='confg' class='w3-button w3-blue w3-round-large'>Configuration Page</a>
            </div>
            <div class='w3-container w3-cell w3-mobile w3-padding'>
              <a href='sched' class='w3-button w3-blue w3-round-large'>Schedual Page</a>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</body>
</html>
)====";
String html_5 = R"====(        <div class='w3-panel w3-card-4 w3-white w3-round-large w4-padding w3-center'>
          <form method='get'><table class='w3-table w3-bordered w3-centered'><tr><th>Day</th><th>Line</th><th colspan=2 class='w3-pale-green'>Time</th><th>)====";
String html_6 = "<td rowspan=4><input type=text value=Weekend disabled class='w3-input w3-border-0'></td>";
String html_7 = "<td rowspan=4><input type=text value=Weekday disabled class='w3-input w3-border-0'></td>";
String html_8 = "</table><p><input type=hidden value=0 name=na><input type=submit value='Update' class='w3-button w3-blue w3-round-large'></p></form></div>";
String html_9 = "</th></tr>";
String html_10 = R"====(
            <tr><th colspan='5'>Rates &cent;/kWhr</th></tr>
            <tr><th colspan='2'>Tier 1</th><td colspan='3'><input type=number name=TIER1RATE min=0 max=25.5 step=0.1 value=)====";
String html_11 = R"====(
            <tr><th colspan='2'>Tier 2</th><td colspan='3'><input type=number name=TIER2RATE min=0 max=25.5 step=0.1 value=)====";
String html_12 = R"====(
            <tr><th colspan='2'>Tier 3</th><td colspan='3'><input type=number name=TIER3RATE min=0 max=25.5 step=0.1 value=)====";
String html_13 = " class='w3-input w3-border-0'></td></tr>";
String html_14 = "<tr><th colspan='2'>Pump Wattage (W)</th><td colspan='3'><input type=number name=WATTAGE min=0 max=1275 step=5 value=";
