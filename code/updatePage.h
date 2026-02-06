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

const char DATALIST_page[] PROGMEM = R"=====(
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
<form method="post" action="/updatedata">
<table>

<tr><td colspan=5 class=heading>TX data array</td></tr>
<tr><td></td><td>Protocol</td><td>code</td><td>code length</td><td>pulse length</td><td>toggle</td></tr>
@@TXROW@@

<tr><td colspan=5 class=heading>RX data array</td></tr>
<tr><td></td><td>protocol</td><td>code</td><td>tx cmd ptr</td></tr>
@@RXROW@@

</table>
<p/>
<input type="submit" value="Update">
</form>
<div class="update">@@UPDATERESPONSE@@</div>
</html>
)=====";

const char ROWTX[] PROGMEM = R"=====(
  <tr>
    <td>@@TXINDEX@@</td>
    <td><input type=text size=25 name="txprotocol@@N@@" value="@@TXPROTOCOL@@"></td>
    <td><input type=text size=26 name="txcode@@N@@" value="@@TXCODE@@"></td>
    <td><input type=text size=4 name="datalength@@N@@" value="@@DATALENGTH@@"></td>
    <td><input type=text size=4 name="pulselength@@N@@" value="@@PULSELENGTH@@"></td>
    <td><input type=checkbox name="toggle@@N@@" value=1 @@TOGGLE@@></td>
  </tr>
)=====";
const char ROWRX[] PROGMEM = R"=====(
  <tr>
    <td>@@RXINDEX@@</td>
    <td><input type=text size=25 name="rxprotocol@@N@@" value="@@RXPROTOCOL@@"></td>
    <td><input type=text size=26 name="rxcode@@N@@" value="@@RXCODE@@"></td>
    <td><input type=text size=4 name="txpointer@@N@@" value="@@TXPOINTER@@"></td>
  </tr>
)=====";

void handleUpdateForm() {
  DebugLn("handleUpdateForm");

  String txRow = "";
  for (int i = 0; i < TXCODE_SIZE; i++) {
    String row = FPSTR(ROWTX);
    int protocol = settings.data.txData[i].protocol;
    row.replace("@@TXINDEX@@",String(i+1));
    row.replace ("@@N@@", String(i));
    if (protocol > 0) {
      row.replace("@@TXPROTOCOL@@", String(settings.data.txData[i].protocol));
      row.replace("@@TXCODE@@", "0x" + String(settings.data.txData[i].code, HEX));
      row.replace("@@DATALENGTH@@" , String(settings.data.txData[i].dataLength));
      if (settings.data.txData[i].toggle) {
        row.replace("@@TOGGLE@@", "checked");
      }
      else {
        row.replace("@@TOGGLE@@", "");
      }
    }
    else {
      row.replace("@@TXPROTOCOL@@" , "");
      row.replace("@@TXCODE@@" , "");
      row.replace("@@DATALENGTH@@", "");
      row.replace("@@TOGGLE@@", "");
    } 
    unsigned int pulselength = settings.data.txData[i].pulseLength;
    if (pulselength != 0) {
      row.replace("@@PULSELENGTH@@", String(settings.data.txData[i].pulseLength));
    }
    else {
      row.replace("@@PULSELENGTH@@", "");
    }

    txRow = txRow + row;
  }


  String rxRow = "";
  for (int i = 0; i < RXCODE_SIZE; i++) {
    String row = FPSTR(ROWRX);
    int protocol = settings.data.rxData[i].protocol;
    row.replace("@@RXINDEX@@",String(i+1));
    row.replace ("@@N@@", String(i));
    if (protocol > 0) {
      row.replace("@@RXPROTOCOL@@", String(settings.data.rxData[i].protocol));
      row.replace("@@RXCODE@@" , "0x" + String(settings.data.rxData[i].code, HEX));
      row.replace("@@TXPOINTER@@" , String(settings.data.rxData[i].txIndex));
    }
    else {
      row.replace("@@RXPROTOCOL@@", "");
      row.replace("@@RXCODE@@", "");
      row.replace("@@TXPOINTER@@", "");
    }

    rxRow = rxRow + row;
  }

  String s = FPSTR(DATALIST_page);
  s.replace("@@VERSION@@",FVERSION);
  s.replace("@@TXROW@@", txRow);
  s.replace("@@RXROW@@", rxRow);
  s.replace("@@UPDATERESPONSE@@", httpUpdateResponse);
  httpUpdateResponse = "";
  server.send(200, "text/html", s);

}

void handleUpdateData() {
String aux;
int protocol;
unsigned long code;
unsigned int pulseLength;
unsigned int dataLength;
unsigned int txindex;
bool toggle;

  DebugLn("handleUpdateData");

  for (int i = 0; i < TXCODE_SIZE; i++) {
    String strI = String(i);
    protocol = -1;
    code = 0;
    pulseLength = 0;
    dataLength = 24;
    toggle = false;

    aux = server.arg("txprotocol" + strI);
    if (aux.length()) {
      protocol=aux.toInt();
    }
    aux = server.arg("pulselength" + strI);
    if (aux.length()) {
      pulseLength=aux.toInt();
    }
    aux = server.arg("txcode" + strI);
    if (aux.length()) {
      code=strtoul(aux.c_str(), NULL, 0);
    }
    aux = server.arg("datalength" + strI);
    if (aux.length()) {
      dataLength=aux.toInt();
    }
    aux = server.arg("toggle" + strI);
    if (aux == "1") {
      toggle = true;
    }
    else {
      toggle = false;
    }


    if ((protocol > 0) && (code != 0)) {
      settings.data.txData[i].protocol = protocol;
      settings.data.txData[i].pulseLength = pulseLength;
      settings.data.txData[i].dataLength = dataLength;
      settings.data.txData[i].code = code;
      settings.data.txData[i].toggle = toggle;
    }
    else {
      settings.data.txData[i].protocol = -1;
      settings.data.txData[i].pulseLength = 0;
      settings.data.txData[i].dataLength = 0;
      settings.data.txData[i].code = 0;
      settings.data.txData[i].toggle = false;
    }
  }

  for (int i = 0; i < RXCODE_SIZE; i++) {
    String strI = String(i);
    protocol = -1;
    code = 0;
    txindex = -1;

    aux = server.arg("rxprotocol" + strI);
    if (aux.length()) {
      protocol=aux.toInt();
    }
    aux = server.arg("rxcode" + strI);
    if (aux.length()) {
      code=strtoul(aux.c_str(), NULL, 0);
    }
    aux = server.arg("txpointer" + strI);
    if (aux.length()) {
      txindex=aux.toInt();
    }

    if ((protocol > 0) && (code != 0) && (txindex > 0)) {
      settings.data.rxData[i].protocol = protocol;
      settings.data.rxData[i].code = code;
      settings.data.rxData[i].txIndex = txindex;
    }
    else {
      settings.data.rxData[i].protocol = -1;
      settings.data.rxData[i].code = 0;
      settings.data.rxData[i].txIndex = -1;
    }
  }

  httpUpdateResponse = "The configuration is updated.";
  server.sendHeader("Location", "/updateform");
  server.send(302, "text/plain", "Moved");
  settings.Save();

}
