void handleSendCommands() {
String aux;
int protocol = -1;
unsigned long code = 0;
unsigned int length = 0;
String sCode = "";

  DebugLn("handleSendCommands");

  aux = server.arg("protocol");
  if (aux.length()) {
    protocol=aux.toInt();
  }
  aux = server.arg("length");
  if (aux.length()) {
    length=aux.toInt();
  }
  aux = server.arg("code");
  if (aux.length()) {
    code=strtoul(aux.c_str(), NULL, 10);
  }
  aux = server.arg("hexcode");
  if (aux.length()) {
    code=strtoul(aux.c_str(), NULL, 16);
  }
  aux = server.arg("bitcode");
  if (aux.length()) {
    sCode=aux;
  }

  if ((protocol > 0) && (length > 0) && (code != 0) ) {

    setTXmode();

    mySwitch.setProtocol(protocol);
    mySwitch.send(code, length);
    server.send(200, "text/html", "sent code 0x" + String(code,HEX) + ", length " + String(length) + ", protocol " + String(protocol));
    DebugLn("sent code 0x" + String(code,HEX) + ", length " + String(length) + ", protocol " + String(protocol));
  }
  else if ( (protocol > 0) && (sCode.length() > 0 )) {

    setTXmode();

    mySwitch.setProtocol(protocol);
    mySwitch.send(sCode.c_str());
    server.send(200, "text/html", "sent code " + sCode + ", protocol " + String(protocol));
    DebugLn("sent code " + sCode + ", protocol " + String(protocol));
  }
  else {
      server.send(200, "text/html", "usage: &emsp; http://ip_address/send?protocol=xx&code=cccccccc&length=nn, <br/>&emsp; http://ip_address/send?protocol=xx&hexcode=hhhhhh&length=nn  <br/>&emsp; or http://ip_address/send?protocol=xx&bitcode=bbbbbbbbbbbbbbbbbbbb" );
  }

}

void handleSendTxData () {
String aux;
int protocol = -1;
unsigned int txindex = -1;

  DebugLn("handleSendTxData");

  aux = server.arg("txindex");
  if (aux.length()) {
    txindex=aux.toInt();
    sendTxData(txindex-1);
    server.send(200, "text/html", "sent TxData[" + String(txindex) + "]");
  }
  else {
    server.send(200, "text/html", "Usage: &emsp;http://ip_address/sendtxdata?txindex=nn");
  }

}

void handleRxData() {
String aux;
int protocol = -1;
unsigned long code = 0;
unsigned int rxindex = -1;
unsigned int txindex = -1;

  DebugLn("handleRxData");

  aux = server.arg("rxindex");
  if (aux.length()) {
    rxindex=aux.toInt();
  }
  aux = server.arg("protocol");
  if (aux.length()) {
    protocol=aux.toInt();
  }
  aux = server.arg("code");
  if (aux.length()) {
    code=strtoul(aux.c_str(), NULL, 10);
  }
  aux = server.arg("hexcode");
  if (aux.length()) {
    code=strtoul(aux.c_str(), NULL, 16);
  }
  aux = server.arg("bitcode");
  if (aux.length()) {
    code=strtoul(aux.c_str(), NULL, 2);
  }
  aux = server.arg("txindex");
  if (aux.length()) {
    txindex=aux.toInt();
  }


  if ((protocol > 0) && (code != 0) && (rxindex > 0) && (rxindex < RXCODE_SIZE) && (txindex > 0)) {
    rxindex = rxindex - 1;
    settings.data.rxData[rxindex].protocol = protocol;
    settings.data.rxData[rxindex].code = code;
    settings.data.rxData[rxindex].txIndex = txindex;
    server.send(200, "text/html", "RXdata[" + String(rxindex+1) + "]: code 0x" + String(code,HEX) + ", protocol " + String(protocol) + ", TXindex " + String(txindex));
    DebugLn("RXdata[" + String(rxindex+1) + "]: code 0x" + String(code,HEX) + ", protocol " + String(protocol) + ", TXindex " + String(txindex));
    settings.Save();
  }
  else {
      server.send(200, "text/html", "usage: &emsp; http://ip_address/rxdata?rxindex=nn&protocol=xx&code=cccccccc&txindex=nn, <br/>&emsp; http://ip_address/rxdata?rxindex=nn&protocol=xx&hexcode=hhhhhh&txindex=nn  <br/>&emsp; or http://ip_address/rxdata?rxindex=nn&protocol=xx&bitcode=bbbbbbbbbbbbbbbbbbbb&txindex=nn" );
  }

}

void handleTxData() {
String aux;
int protocol = -1;
unsigned long code = 0;
unsigned int pulseLength = 0;
unsigned int dataLength = 24;
unsigned int txindex = -1;
bool  toggle;

  DebugLn("handleTxData");

  aux = server.arg("txindex");
  if (aux.length()) {
    txindex=aux.toInt();
  }
  aux = server.arg("protocol");
  if (aux.length()) {
    protocol=aux.toInt();
  }
  aux = server.arg("pulselength");
  if (aux.length()) {
    pulseLength=aux.toInt();
  }
  aux = server.arg("code");
  if (aux.length()) {
    code=strtoul(aux.c_str(), NULL, 10);
  }
  aux = server.arg("hexcode");
  if (aux.length()) {
    code=strtoul(aux.c_str(), NULL, 16);
  }
  aux = server.arg("bitcode");
  if (aux.length()) {
    code=strtoul(aux.c_str(), NULL, 2);
    dataLength=aux.length();
  }
  aux = server.arg("datalength");
  if (aux.length()) {
    dataLength=aux.toInt();
  }
  aux = server.arg("toggle");
  if (aux.length()) {
    toggle=aux.toInt();
  }


  if ((protocol > 0) && (code != 0) && (txindex > 0) && (txindex < TXCODE_SIZE)) {
    txindex = txindex - 1;
    settings.data.txData[txindex].protocol = protocol;
    settings.data.txData[txindex].pulseLength = pulseLength;
    settings.data.txData[txindex].dataLength = dataLength;
    settings.data.txData[txindex].code = code;
    settings.data.txData[txindex].toggle = toggle;
    server.send(200, "text/html", "TXdata[" + String(txindex+1) + "]: code 0x" + String(code,HEX) + ", datalength " + String(dataLength) + ", protocol " + String(protocol) + ", pulselength " + String(pulseLength) + ", toggle " + String(toggle));
    DebugLn("TXdata[" + String(txindex+1) + "]: code 0x" + String(code,HEX) + ", datalength " + String(dataLength) + ", protocol " + String(protocol) + ", pulselength " + String(pulseLength) + ", toggle " + String(toggle));
    settings.Save();
  }
  else {
      server.send(200, "text/html", "usage: &emsp; http://ip_address/txdata?txindex=nn&protocol=xx&pulselength=xx&code=cccccccc&datalength=nn, <br/>&emsp; http://ip_address/txdata?txindex=nn&protocol=xx&pulselength=xx&hexcode=hhhhhh&datalength=nn&toggle=t  <br/>&emsp; or http://ip_address/txdata?txindex=nn&protocol=xx&pulselength=xx&bitcode=bbbbbbbbbbbbbbbbbbbb&toggle=t" );
  }

}

void handleListData () {
  int i;
  String response = "Stored protocol data <br/><br/>";

  DebugLn("handleListData");

  for (i = 0; i < TXCODE_SIZE; i++) {
    response = response + "Txdata[" + String(i+1) + "]: code 0x" + String(settings.data.txData[i].code,HEX) + ", datalength " + String(settings.data.txData[i].dataLength) + ", protocol " + String(settings.data.txData[i].protocol) + ", pulselength " + String(settings.data.txData[i].pulseLength) + ", toggle " +String(settings.data.txData[i].toggle) + "<br/>";
  }

  response = response + "<br/>";

  for (i = 0; i < RXCODE_SIZE; i++) {
    response = response + "RXdata[" + String(i+1) + "]: code 0x" + String(settings.data.rxData[i].code,HEX) + ", protocol " + String(settings.data.rxData[i].protocol) + ", txindex " + String(settings.data.rxData[i].txIndex) + "<br/>";
  }

  server.send(200, "text/html", response); 

}

void handleClearRxData () {
  int i;
  String response = "RX data cleared <br/><br/>";

  DebugLn("handleClearRxData");

  for (int i = 0; i < RXCODE_SIZE; i++) {
    settings.data.rxData[i].protocol = -1;
    settings.data.rxData[i].code = 0;
    settings.data.rxData[i].txIndex = -1;
  }
  settings.Save();

  for (i = 0; i < RXCODE_SIZE; i++) {
    response = response + "RXdata[" + String(i+1) + "]: code 0x" + String(settings.data.rxData[i].code,HEX) + ", protocol " + String(settings.data.rxData[i].protocol) + ", txindex " + String(settings.data.rxData[i].txIndex) + "<br/>";
  }

  server.send(200, "text/html", response); 

}

void handleClearTxData () {
  int i;
  String response = "TX data cleared <br/><br/>";

  DebugLn("handleClearTxData");

  for (int i = 0; i < TXCODE_SIZE; i++) {
    settings.data.txData[i].protocol = -1;
    settings.data.txData[i].pulseLength = 0;
    settings.data.txData[i].code = 0;
    settings.data.txData[i].dataLength = 0;
    settings.data.txData[i].toggle = false;
  }
  settings.Save();

  for (i = 0; i < TXCODE_SIZE; i++) {
    response = response + "Txdata[" + String(i+1) + "]: code 0x" + String(settings.data.txData[i].code,HEX) + ", datalength " + String(settings.data.txData[i].dataLength) + ", protocol " + String(settings.data.txData[i].protocol) + ", pulselength " + String(settings.data.txData[i].pulseLength) + ", toggle " + String(settings.data.txData[i].toggle) + "<br/>";
  }

  server.send(200, "text/html", response); 

}

void handleUsage()  {

  DebugLn("handleUsage");

  server.send(200, "text/html",
    String("<b>Usage:</b> <br/>")
    + "<b>help:</b> &emsp; http://ip_address/ <br/>"
    + "<b>send RF data:</b> &emsp; http://ip_address/send?protocol=xx&code=cccccccc&length=nn, <br/>&emsp; http://ip_address/send?protocol=xx&hexcode=hhhhhh&length=nn  <br/> &emsp; or http://ip_address/send?protocol=xx&bitcode=bbbbbbbbbbbbbbbbbbbb <br/>"
    + "<b>send Tx data:</b> &emsp; http://ip_address/sendtxdata?txindex=nn <br/>"
    + "<b>define Rx code:</b> &emsp; http://ip_address/rxdata?rxindex=nn&protocol=xx&code=cccccccc&txindex=nn, <br/>&emsp; http://ip_address/rxdata?rxindex=nn&protocol=xx&hexcode=hhhhhh&txindex=nn  <br/>&emsp; or http://ip_address/rxdata?rxindex=nn&protocol=xx&bitcode=bbbbbbbbbbbbbbbbbbbb&txindex=nn <br/>"
    + "<b>define Tx code:</b> &emsp; http://ip_address/txdata?txindex=nn&protocol=xx&pulselength=xx&code=cccccccc&datalength=nn&toggle=t, <br/>&emsp; http://ip_address/txdata?txindex=nn&protocol=xx&pulselength=xx&hexcode=hhhhhh&datalength=nn&toggle=t  <br/>&emsp; or http://ip_address/txdata?txindex=nn&protocol=xx&pulselength=xx&bitcode=bbbbbbbbbbbbbbbbbbbb&toggle=t <br/>"
    + "<b>list Tx/Rx tables:</b> &emsp; http://ip_address/listdata <br/>"
    + "<b>clear Tx table:</b> &emsp; http://ip_address/txclear <br/>"
    + "<b>clear Rx table:</b> &emsp; http://ip_address/rxclear <br/>"
    + "<b>show update form:</b> &emsp; http://ip_address/updateform <br/>"
  );

}