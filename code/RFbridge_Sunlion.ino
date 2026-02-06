/*
  RF 433 bridge

  This sketch receives RF433 codes from remote wall switches
  and sends RF433 codes for switch on/off ligth on Sulion fans

  uses a modified https://github.com/sui77/rc-switch/ library
  with added Sulion protocol 
  { 250, { 1,  39 }, {  1,  3 }, {  3,  1 }, false }     // protocol 13 (Sulion)

  prepared for module LOLIN(WEMOS) D1 R2 & mini (ESP8266)
  
*/
///////////////////////////////////////////////////////////////
//           History                                         //
///////////////////////////////////////////////////////////////
//  1.0 first version
//      standalone version for test purposes with fixed codes
//      and no wifi connection
//
//  2.0 added WiFi connection for:
//        1) receive toggle commands using URL: http://ip_address/toggle?room=room_name
//        2) receive commands thru MQTT (optional, needs MQTT broker defined in configuration)
//        3) OTA update
//
//  2.1 program continues execution if not WiFi connected
//      added http "send" command (see mainPage.h)
//
//  2.2 reset program after 10 hours (RESETTIME)
//      moved "output.ino" to "output.h" and change Serial output by Debug macros
//      added MQTT command base_topic/send with payload {"protocol": pp, "length": ll, "code": nnnnnn}
//
//  3.0 parametrizable commands
//      removed "toggle" commands and hard-coded commands
//      created txData and rxData matrixs to store RX and TX commands
//      added http "rxdata", "txdata", "listdata", "rxclear" and "txclear" commnds (see mainPage.h)
//      added "pulselength" to MQTT command base_topic/send with payload {"protocol": pp, "length": ll, "code": nnnnnn, "pulselength": xx}
//
//  3.1 added web page to list, modify txData and rXData parameters (updatePage.h)
//      added html sendtxdata http://ip_address/sendtxdata?txindex=n
//      added MQTT command sendtxdata with payload txindex
//      size of rxData[] and txData[] in settings can be modified by #define
//
//  3.2 If WiFi no connected, retry after 1 hour
//
//  3.3 toggle all ligths if it is hard reset (resetReason = 0)
//
//  3.4 added /setup and handlesetup pages in normal operation
//      corrected error in retries in WiFi coonection
//
//  3.4.1 enable TX and RX only when performing the function,
//      keeping disabled the alternate function
//
//  3.5 change TX/RX board to CC1101, using ELECHOUSE_CC1101_SRC_DRV library
//      (https://github.com/LSatan/SmartRC-CC1101-Driver-Lib)
//
//  5.0 Rebuild of RFbridge (from v3.5), only for Sunlion fans
//      keep internal status of lights and fans speed
//      device table stores only base code (4 first hex digits), as
//        last 2 digits are defined for fan or light control
//      receive commands and send status also via MQTT
//
//  5.1 added additional set mqtt simple commands (topic/set_light/code ON/OFF)
// 
///////////////////////////////////////////////////////////////

/****************************************************************************
                CC 1101 wiring

    1     GND
    2     VCC (3.3 v)
    3     GD00          ->    D1  (GPIO 05)
    4     CSN           ->    D8  (GPIO 15)
    5     SCK           ->    D5  (GPIO 14)
    6     MOSI          ->    D7  (GPIO 13)
    7     MISO/GD01     ->    D6  (GPIO 12)
    8     GD02          ->    D2  (GPIO 04)

****************************************************************************/
    
#include <ESP8266WiFi.h>    
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <PubSubClient.h>  // v 2.8

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>   // 1.0.12

#include <ELECHOUSE_CC1101_SRC_DRV.h> // v 2.5.7
#include <RCSwitch.h>  // V 2.6.4 (modified to add Sulion code as protocol 13)

#define FVERSION  "v5.1"

#define RDEBUG

#include "Rdebug.h"
#include "settings.h"
#include "output.h"

RCSwitch mySwitch = RCSwitch();

static const RCSwitch::Protocol sulionProtocol =  { 250, { 1,  39 }, {  1,  3 }, {  3,  1 }, false };

// Codes 
#define   LIGHT_CODE      0x02
#define   FAN_OFF         0x01
#define   FAN_SPEED_BASE  0x02
#define   SULION_PROTOCOL 13

//Global Variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
rst_info      *myResetInfo;
String        httpUpdateResponse;
const char*   ap_ssid = "ESP-RF433";
const char*   ap_password = ""; //"12345678";
int           ap_setup_done = 0;
unsigned int  count = 0;
String        ipaddress;
bool          WiFiconnected;
bool          RXmode = false;
bool          TXmode = false;

//Struct to store setting in EEPROM
Settings    settings;

#define LEDON LOW
#define LEDOFF HIGH
# define SETUP_PIN D3  // GPIO0
# define RX_PIN  4     // GPIO D2
# define TX_PIN  5     // GPIO D1

#define   RESETTIME 10*60*60*1000
#define   RETRY_WIFI 60*60*1000

#ifdef ARDUINO_ESP8266_NODEMCU_ESP12E
# define LED_PIN D0    // GPIO16, alternate for ESP8266 built-in use D4 (GPIO2)
#endif
#ifdef  ARDUINO_ESP8266_WEMOS_D1MINI
# define LED_PIN D4    // GPIO2
#endif

/////////////////////////////////////////
// blink LED
/////////////////////////////////////////
void blinkLed(unsigned long time){
  digitalWrite(LED_PIN, LEDON);
  delay(time);
  digitalWrite(LED_PIN, LEDOFF);
  delay(time);
}

/////////////////////////////////////////
// set receive mode on CC1101 and rc-switch
/////////////////////////////////////////
void setRXmode(){

  if ((! RXmode) && (ELECHOUSE_cc1101.getMode() != 2) ) {
    // set CC1101 in receive mode
    if(TXmode) mySwitch.disableTransmit();      // set Transmit off
    ELECHOUSE_cc1101.SetRx();                   // set Receive on
    ELECHOUSE_cc1101.SetTx();                   // set Transmit (see: https://github.com/LSatan/SmartRC-CC1101-Driver-Lib/issues/64#issuecomment-889026058)
    ELECHOUSE_cc1101.SetRx();                   // set Receive on, again 
    while (ELECHOUSE_cc1101.getMode() != 2) {delay(50);}  // wait until CC1101 mode is RX (=2)
    mySwitch.enableReceive(RX_PIN);             // Receiver on
    mySwitch.resetAvailable();                  // reset available data & buffer
    TXmode = false;
    RXmode = true;
    // DebugLn("changed to RX mode");
  }

}

/////////////////////////////////////////
// set transmit mode on CC1101 and rc-switch
/////////////////////////////////////////
void setTXmode(){

  if ((! TXmode) && (ELECHOUSE_cc1101.getMode() != 1)) {
    // set CC1101 in Transmit mode
    if(RXmode) mySwitch.disableReceive();       // Receiver off
    ELECHOUSE_cc1101.SetTx();                   // set Transmit on
    while (ELECHOUSE_cc1101.getMode() != 1) {delay(50);}  // wait until CC1101 mode is tX (=1)
    mySwitch.enableTransmit(TX_PIN);            // Transmit on
    RXmode = false;
    TXmode = true;
    // DebugLn("changed to TX mode");
  }

}

/////////////////////////////////////////
//  transmit code with parameters
/////////////////////////////////////////
void transmitCode(int protocol, unsigned long code, unsigned int dataLength, unsigned int pulseLength) {

  DebugLn("transmitCode, protocol: " + String(protocol) + ", code: 0x" + String(code,HEX) + ", pulse length: " + String(pulseLength));

  if (protocol > 0 ) {
    setTXmode();   // set transmit mode
    mySwitch.setProtocol(protocol);
    if (pulseLength > 0) {
      // defined pulse length
      mySwitch.setPulseLength(pulseLength);
      DebugLn("Pulse length " + String(pulseLength));
    }
    mySwitch.send(code, dataLength);
    DebugLn("sent code 0x" + String(code,HEX) + ", length " + String(dataLength) + ", protocol " + String(protocol));
  }
  else {
    DebugLn("protocol not defined");
  }

}

// mqtt_send function declaration
void mqtt_send(String subtopic, String message, bool retain);

/////////////////////////////////////////
// send code stored on TX data
/////////////////////////////////////////
void sendTxData(int index){

  DebugLn("sendTxData, index " + String(index + 1));

  transmitCode(settings.data.txData[index].protocol, settings.data.txData[index].code, settings.data.txData[index].dataLength, settings.data.txData[index].pulseLength);

  settings.data.lightStatus[index] =  ! settings.data.lightStatus[index];
  // send MQTT status
  mqtt_send("light_" + String(settings.data.txData[index].code & 0xFFFF00, HEX),(settings.data.lightStatus[index] ? "ON" : "OFF"), true);

}

// Web server for receive commands
ESP8266WebServer server(80);
#include "initPage.h"
#include "mainPage.h"
#include "updatePage.h"

// MQTT functions
#include  "mqtt.h"

/////////////////////////////////////////
// setup WiFi configuration
/////////////////////////////////////////
void firstSetup(){
  DebugLn("First Setup ->");
  DebugLn("Magic->"+String(settings.data.magic));
  DebugLn("Setting up AP");
  WiFi.mode(WIFI_AP);             //Only Access point
  WiFi.softAP(ap_ssid, NULL, 8);  //Start HOTspot removing password will disable security
  delay(50);
  server.on("/", handleSetup);
  server.on("/setup", handleSetup);
  server.on("/initForm", handleInitForm);
  DebugLn("Server Begin");
  server.begin(); 
  delay(100);
  do {

    server.handleClient(); 
    blinkLed(200);
    Debug(".");
  }
  while (!ap_setup_done);
  settings.data.magic[0] = MAGIC[0];
  settings.data.magic[1] = MAGIC[1];
  settings.data.magic[2] = MAGIC[2];
  settings.data.magic[3] = MAGIC[3];
  server.stop();
  WiFi.disconnect();
  settings.Save();
  DebugLn("First Setup Done");
}

/////////////////////////////////////////
// conection to WiFi
/////////////////////////////////////////
int setupSTA()
{
  int timeOut=0;

  for (int retry=0; retry<=3; retry++) {
    WiFi.disconnect();
    WiFi.hostname("ESP_" + String(settings.data.name)) ;
    WiFi.mode(WIFI_STA);
    DebugLn("Connecting to "+String(settings.data.ssid)+" Retry:"+String(retry));
    DebugLn("Connecting to "+String(settings.data.psk));
    
    if (String(settings.data.psk).length()) {
      WiFi.begin(String(settings.data.ssid), String(settings.data.psk));
    } else {
      WiFi.begin(String(settings.data.ssid));
    }
    
    timeOut=0;
    while (WiFi.status() != WL_CONNECTED) {
      if ((timeOut < 10) && WiFi.status() != WL_CONNECT_FAILED){ // if not timeout or failure, keep trying
        //delay(100);
        Debug(String(WiFi.status()));
        blinkLed(500);
        timeOut ++;
      } 
      else{
        timeOut = 0;
        DebugLn("-Wifi connection timeout");
        blinkLed(500);
        if (retry == 2) {
          WiFiconnected = false;
          return 0;
        }
        break;
      }
    }
    if (WiFi.status() == WL_CONNECTED)
      break;
  }  
  DebugLn(" Connected");
  // Print the IP address
  ipaddress = WiFi.localIP().toString();
  DebugLn(ipaddress); 
  DebugLn(WiFi.hostname().c_str());
  WiFiconnected = true;
  return 1;
}

/////////////////////////////////////////
// Arduino initialization code
/////////////////////////////////////////
void setup() {
  DebugStart();
  DebugLn("Setup ->");
  myResetInfo = ESP.getResetInfoPtr();
  uint32 resetReason = myResetInfo->reason;
  DebugLn("myResetInfo->reason "+String(resetReason));    // reason is uint32
                                                                 // 0 = power down
                                                                 // 1 = HW WDT reset
                                                                 // 2 = fatal exception
                                                                 // 3 = SW watchdog reset
                                                                 // 4 = SW reset
                                                                 // 5 = restart from deepsleep
                                                                 // 6 = reset button / hard reset
                                                                 
  settings.Load();

  // ******** initiallize GPIOs and wait for setup pin *****************
  pinMode(LED_PIN, OUTPUT);

  DebugLn("-> Check if SETUP_PIN is low");
  digitalWrite(LED_PIN, LEDON);
  // Wait up to 5s for SETUP_PIN to go low to enter AP/setup mode.
  pinMode(SETUP_PIN, INPUT);      //Configure setup pin as input
  digitalWrite(SETUP_PIN, HIGH);  //Enable internal pooling
  delay(5000);  
  // if NO MAGIC Or SETUP PIN enter hard config...
  if ((String(settings.data.magic) != MAGIC)   || !digitalRead(SETUP_PIN)){
    digitalWrite(LED_PIN, LEDOFF);
    firstSetup();
  }

  // NO SETUP, switch off LED
  digitalWrite(LED_PIN, LEDOFF);

  // *********** setup STA mode and connect to WiFi ************
  if (setupSTA() == 0) { // SetupSTA mode
    DebugLn("Wifi no connected");
    DebugLn("continue without WiFi ....");
  }

  if (WiFiconnected) {
    // ********** initialize OTA *******************
    ArduinoOTA.begin();

    // ********* initialize MQTT ******************
    if (strlen(settings.data.mqttbroker) > 0 ) {
      // MQTT broker defined, initialize MQTT
      mqtt_init();
      delay(100);
      mqtt_send("ipaddress",ipaddress,false);
    }

    //********** initialize web server for receiving commands
    digitalWrite(LED_PIN, LEDON);
    DebugLn("-> Initiate WebServer");
    server.on("/send", handleSendCommands);
    server.on("/sendtxdata", handleSendTxData);
    server.on("/txdata", handleTxData);
    server.on("/rxdata", handleRxData);
    server.on("/listdata", handleListData);
    server.on("/rxclear", handleClearRxData);
    server.on("/txclear", handleClearTxData);
    server.on("/updateform", handleUpdateForm);
    server.on("/updatedata", handleUpdateData);
    server.on("/setup", handleSetup);
    server.on("/initForm", handleInitForm);
    server.on("/", handleUsage);
    delay(100);
    server.begin(); 
    delay(500);
    digitalWrite(LED_PIN, LEDOFF);


  }
  // initialite RF433 CC1101 Settings:
    if (ELECHOUSE_cc1101.getCC1101()){       // Check the CC1101 Spi connection.
      DebugLn("cc1101 Connection OK");
  }
  else{
      DebugLn("cc1101 Connection Error");
  }
  ELECHOUSE_cc1101.Init();            // must be set to initialize the cc1101!
  ELECHOUSE_cc1101.setMHZ(433.92);    // Set basic frequency. 

  for (int i = 0; i < TXCODE_SIZE; i++) {
    settings.data.lightStatus[i] = 0;
    settings.data.fanSpeed[i] = 0;
    settings.data.prevFanSpeed[i] = 2;
  }

  // if is a power on reset, then toggle ligths
  DebugLn("reset reason: " + String(resetReason));
  if (resetReason == 6) {
    for (int i = 0; i < TXCODE_SIZE; i++) {
      if (settings.data.txData[i].toggle == 1) {
        // assume lighth is currently ON, sendTxData will toggle it and set status to OFF
        settings.data.lightStatus[i] = true;
        sendTxData(i);
        DebugLn("toggle[" + String(i+1) + "]");
      }
    }

  }

  DebugLn("end Setup");

}

void loop() {

long recCode;
int recProtocol;

  setRXmode();

  if (mySwitch.available()) {
    // print received data
    output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(),mySwitch.getReceivedProtocol());

    recProtocol = mySwitch.getReceivedProtocol();
    recCode = mySwitch.getReceivedValue();
    mySwitch.resetAvailable();

    for (int i = 0; i < RXCODE_SIZE; i++) {
      // check if received data is in received matrix
      if (settings.data.rxData[i].protocol == recProtocol && settings.data.rxData[i].code == recCode) {
        int txindex = settings.data.rxData[i].txIndex -1;
        sendTxData(txindex);
      }
    }

    if (recProtocol == SULION_PROTOCOL) {
      // received a protocol 13 code (Sulion fan)
      // store info in status array
      unsigned int statCode = recCode & 0xFF;
      unsigned int devCode = recCode & 0xFFFF00;
      DebugLn("received msg Sulion: 0x" + String(devCode, HEX) + ", 0x" + String(statCode, HEX));
      for (int i = 0; i < TXCODE_SIZE; i++) {
        if ( devCode == (settings.data.txData[i].code & 0xFFFF00) ) {
          // device identified
          if (statCode == 0x02) {
            // toggle light
            DebugLn("toggle light[" +String(i) + "], current status: " + String(settings.data.lightStatus[i]));
            settings.data.lightStatus[i] =  ! settings.data.lightStatus[i];
            DebugLn("toggle light[" +String(i) + "], new status: " + String(settings.data.lightStatus[i]));
            // send MQTT status
            mqtt_send("light_" + String(devCode,HEX),(settings.data.lightStatus[i] ? "ON" : "OFF"), true);
          }
          else if (statCode == 0x01) {
            // toggle fan
            if (settings.data.fanSpeed[i] > 0 ) {
              // fan on, set off
              settings.data.prevFanSpeed[i] = settings.data.fanSpeed[i];
              settings.data.fanSpeed[i] = 0;
            }
            else {
              if (settings.data.prevFanSpeed[i] == 0) settings.data.prevFanSpeed[i] = 2;
              settings.data.fanSpeed[i] = settings.data.prevFanSpeed[i];
            }
            // send MQTT speed 
            mqtt_send("speed_" + String(devCode,HEX), String(settings.data.fanSpeed[i]), true);
          }
          else if ((statCode > 0x02)  && (statCode <= 0x08)) {
            // new fan speed
            settings.data.fanSpeed[i] = statCode - 0x02;
            // send MQTT speed
            mqtt_send("speed_" + String(devCode,HEX), String(settings.data.fanSpeed[i]), true);
          }
        }
      }
    }

  }

  if (WiFiconnected) {
    // handle Web server
    server.handleClient(); 

    // handle OTA
    ArduinoOTA.handle();

    // handle MQTT if broker defined
    if (strlen(settings.data.mqttbroker) > 0 ) {
      client.loop();
    }

  }

  if (millis() > RESETTIME) {
    ESP.reset();
  }

  if ( !WiFiconnected && (millis() > RETRY_WIFI)) {
    ESP.reset();
  }

}
