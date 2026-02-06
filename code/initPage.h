/*  Copyright (C) 2016 Buxtronix and Alexander Pruss

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

const char INIT_page[] PROGMEM = R"=====(
<html>
<head>
 <meta name="viewport" content="initial-scale=1">
 <style>
 body {font-family: helvetica,arial,sans-serif;}
 table {border-collapse: collapse; border: 1px solid black;}
 td {padding: 0.25em;}
 .title { font-size: 2em; font-weight: bold;}
 .name {padding: 0.5em;}
 .heading {font-weight: bold; background: #c0c0c0; padding: 0.5em;}
 .update {color: #dd3333; font-size: 0.75em;}
 </style>
</head>
<div class=title>RF433 Bridge</div>
<div class=version>Firmware: @@VERSION@@</div>
<form method="post" action="/initForm">
<table>
<tr><td colspan=2 class=heading>WiFi Setup</td></tr>
<tr><td>SSID:</td><td><input type=text name="ssid" value="@@SSID@@"></td></tr>
<tr><td>PSK:</td><td><input type=password name="psk" value="@@PSK@@"></td></tr>
<tr><td>Name:</td><td><input type=text name="nodename" value="@@NODENAME@@"></td></tr>

<tr><td colspan=2 class=heading>MQTT Setup</td></tr>
<tr><td>MQTT broker:</td><td><input type=text size=35 name="mqttbroker" value="@@MQTTBROKER@@"></td></tr>
<tr><td>MQTT port:</td><td><input type=text size=35 name="mqttport" value="@@MQTTPORT@@"></td></tr>
<tr><td>MQTT user:</td><td><input type=text size=35 name="mqttuser" value="@@MQTTUSER@@"></td></tr>
<tr><td>MQTT passwd:</td><td><input type=text size=35 name="mqttpasswd" value="@@MQTTPASSWD@@"></td></tr>
<tr><td>MQTT topic:</td><td><input type=text size=35 name="mqtttopic" value="@@MQTTTOPIC@@"></td></tr>

</table>
<p/>
<input type="submit" value="Update">
</form>
<div class="update">@@UPDATERESPONSE@@</div>
</html>
)=====";


void handleSetup() {
  DebugLn("handlesetup");
  String s = FPSTR(INIT_page);
  s.replace("@@SSID@@", settings.data.ssid);
  s.replace("@@PSK@@", settings.data.psk);
  s.replace("@@NODENAME@@", settings.data.name);
  s.replace("@@VERSION@@",FVERSION);

  s.replace("@@MQTTBROKER@@",settings.data.mqttbroker);
  s.replace("@@MQTTPORT@@",String(settings.data.mqttport));
  s.replace("@@MQTTUSER@@",settings.data.mqttuser);
  s.replace("@@MQTTPASSWD@@",settings.data.mqttpassword);
  s.replace("@@MQTTTOPIC@@",settings.data.mqtttopic);

  s.replace("@@UPDATERESPONSE@@", httpUpdateResponse);
  httpUpdateResponse = "";
  server.send(200, "text/html", s);
}

void handleInitForm() {
String aux;

  DebugLn("handleInitForm");
  DebugLn("Mode ="+String(WiFi.status()));

  String t_ssid = server.arg("ssid");
  String t_psk = server.arg("psk");
  String t_name = server.arg("nodename");
  String(t_name).replace("+", "_");
  t_ssid.toCharArray(settings.data.ssid,SSID_LENGTH);
  t_psk.toCharArray(settings.data.psk,PSK_LENGTH);
  t_name.toCharArray(settings.data.name,NAME_LENGTH);

  aux = server.arg("mqttbroker");
  aux.toCharArray(settings.data.mqttbroker,128);
  aux = server.arg("mqttuser");
  aux.toCharArray(settings.data.mqttuser,30);
  aux = server.arg("mqttpasswd");
  aux.toCharArray(settings.data.mqttpassword,30);
  aux = server.arg("mqtttopic");
  aux.toCharArray(settings.data.mqtttopic,30);
  aux = server.arg("mqttport");
  if (aux.length()) {
    settings.data.mqttport=aux.toInt();
  }

  httpUpdateResponse = "The configuration was updated.";
  server.sendHeader("Location", "/setup");
  server.send(302, "text/plain", "Moved");
  settings.Save();
  ap_setup_done = 1;
}

