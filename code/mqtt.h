///////////////////////////////////////////////////////////
// MQTT functions
///////////////////////////////////////////////////////////

#include <PubSubClient.h>  // v 2.8
WiFiClient espClient;
PubSubClient client(espClient);

String      base_topic;

#include <ArduinoJson.h>    // v 6.21.3
StaticJsonDocument<256> jsonDoc;
DeserializationError error;


/**************************************************
 * Callback function for MQTT
 * ***********************************************/
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
char* buff;
const char* nomsg="";

  Debug("MQTT message received [");
  Debug(String(topic));
  Debug("] : ");

  buff = (char*)malloc(length+1);
  memcpy(buff, payload, length);
  buff[length] = '\0';
  DebugLn("[" + String(buff) + "]");

  // is payload no NULL?
  if (length > 0) { 
    if (String(topic) == (base_topic + "/send")) {
      // received mqtt message with topic "send" and payload {"protocol": pp, "length": ll, "code": nnnnnn}
      error = deserializeJson(jsonDoc, buff, length);
      if (! error) {
        String aux;
        int protocol = -1;
        unsigned long code = 0;
        unsigned int length = 0;
        unsigned int pulselength = 0;
        aux = String(jsonDoc["protocol"]);
        if (aux.length()) {
          protocol=aux.toInt();
        }
        aux = String(jsonDoc["length"]);
        if (aux.length()) {
          length=aux.toInt();
        }
        aux = String(jsonDoc["code"]);
        if (aux.length()) {
          code=strtoul(aux.c_str(), NULL, 10);
        }
        aux = String(jsonDoc["pulselength"]);
        if (aux.length()) {
          pulselength=aux.toInt();
        }
        if ((protocol > 0) && (length > 0) && (code != 0) ) {

          setTXmode();

          mySwitch.setProtocol(protocol);
          if (pulselength > 0) mySwitch.setPulseLength(pulselength);
          mySwitch.send(code, length);
          DebugLn("sent code 0x" + String(code,HEX) + ", length " + String(length) + ", protocol " + String(protocol) + ", pulselength " + String(pulselength));

        }

      }
    }
    else if (String(topic) == (base_topic + "/sendtxdata")) {
      // received mqtt message with topic "send" and payload txindex
      sendTxData (atoi(buff)-1);
    } 
    else if (String(topic) == (base_topic + "/cmd")) {
      // received a command for control lights
      error = deserializeJson(jsonDoc, buff, length);
      if (! error) {
        String aux;
        unsigned long device;
        String  light;
        int speed = -1;
        aux = String(jsonDoc["device"]);
        if (aux.length()) {
          device=strtoul(aux.c_str(), NULL, 16);
        }
        aux = String(jsonDoc["light"]);
        if (aux.length()) {
          light = aux;
        }
        aux = String(jsonDoc["speed"]);
        if (aux.length()) {
          speed=aux.toInt();
        }
        for (int i = 0; i < TXCODE_SIZE; i++) {
          if ( device == (settings.data.txData[i].code & 0xFFFF00) ) {
            // device is in table
            if (light != "") {
              // light command received
              bool lightStatus;
              if (light == "ON") {
                lightStatus = true;
              }
              else {
                lightStatus = false;
              }
              if (lightStatus != settings.data.lightStatus[i]) {
                // toggle light
                DebugLn("/cmd, toggle light[" + String(i) + "]");
                transmitCode(settings.data.txData[i].protocol, device + LIGHT_CODE, settings.data.txData[i].dataLength, settings.data.txData[i].pulseLength);
                settings.data.lightStatus[i] = lightStatus;
                // send mqtt with status
                mqtt_send("light_" + String(device, HEX),(settings.data.lightStatus[i] ? "ON" : "OFF"), true);

              }
            }
            if (speed >= 0) {
              // speed command received
              if ((speed == 0) && (settings.data.fanSpeed[i] > 0) ) {
                // stop fan
                transmitCode(settings.data.txData[i].protocol, device + FAN_OFF, settings.data.txData[i].dataLength, settings.data.txData[i].pulseLength);
                // mySwitch.send(device + FAN_OFF, settings.data.txData[i].dataLength);
                settings.data.fanSpeed[i] = 0;
                // send mqtt with status
                mqtt_send("speed_" + String(device, HEX), String(speed), true);
              }
              else if (speed > 0 && speed <= 6) {
                transmitCode(settings.data.txData[i].protocol, device + FAN_SPEED_BASE + speed, settings.data.txData[i].dataLength, settings.data.txData[i].pulseLength);
                // mySwitch.send(device + FAN_SPEED_BASE + speed, settings.data.txData[i].dataLength);
                settings.data.fanSpeed[i] = speed;
                // send mqtt with status
                mqtt_send("speed_" + String(device, HEX), String(speed), true);
              }
            }
          }
        }
      }
    } 
    else if (String(topic).indexOf("set_light") > 0) {
      // received light set command (topic/set_light/code)
      int codePos = String(topic).indexOf("set_light") + 10;
      unsigned long device=strtoul(String(topic).substring(codePos).c_str(), NULL, 16);
      DebugLn("set_light: 0x" + String(device, HEX));
      for (int i = 0; i < TXCODE_SIZE; i++) {
        if ( device == (settings.data.txData[i].code & 0xFFFF00) ) {
          // toggle light
          DebugLn("/set_light, toggle light[" + String(i) + "]");
          bool lightStatus = (String(buff) == "ON" ? true : false);
          if (lightStatus != settings.data.lightStatus[i]) {
            // toggle if status change
            transmitCode(settings.data.txData[i].protocol, device + LIGHT_CODE, settings.data.txData[i].dataLength, settings.data.txData[i].pulseLength);
          }
          settings.data.lightStatus[i] = lightStatus;
          // send mqtt with status
          mqtt_send("light_" + String(device, HEX),(settings.data.lightStatus[i] ? "ON" : "OFF"), true);
        }
      }
    }
    else if (String(topic).indexOf("set_fanspeed") > 0) {
      // received fan speed command (topic/set_fanspeed/code)
      int codePos = String(topic).indexOf("set_fanspeed") + 13;
      unsigned long device=strtoul(String(topic).substring(codePos).c_str(), NULL, 16);
      DebugLn("set_fanspeed: 0x" + String(device, HEX));
      for (int i = 0; i < TXCODE_SIZE; i++) {
        if ( device == (settings.data.txData[i].code & 0xFFFF00) ) {
          // set fan speed
          int speed = String(buff).toInt();
          if (speed >= 0) {
            // speed command received
            if ((speed == 0) && (settings.data.fanSpeed[i] > 0) ) {
              // stop fan
              transmitCode(settings.data.txData[i].protocol, device + FAN_OFF, settings.data.txData[i].dataLength, settings.data.txData[i].pulseLength);
              // mySwitch.send(device + FAN_OFF, settings.data.txData[i].dataLength);
              settings.data.fanSpeed[i] = 0;
              // send mqtt with status
              mqtt_send("speed_" + String(device, HEX), String(speed), true);
            }
            else if (speed > 0 && speed <= 6) {
              transmitCode(settings.data.txData[i].protocol, device + FAN_SPEED_BASE + speed, settings.data.txData[i].dataLength, settings.data.txData[i].pulseLength);
              // mySwitch.send(device + FAN_SPEED_BASE + speed, settings.data.txData[i].dataLength);
              settings.data.fanSpeed[i] = speed;
              // send mqtt with status
              mqtt_send("speed_" + String(device, HEX), String(speed), true);
            }
          }
        }
      }
    }
    else if (String(topic).indexOf("light_") > 0) {
      // received status of light
      int codePos = String(topic).indexOf("light_") + 6;
      unsigned long device=strtoul(String(topic).substring(codePos).c_str(), NULL, 16);
      DebugLn("light status: 0x" + String(device, HEX));
      for (int i = 0; i < TXCODE_SIZE; i++) {
        if ( device == (settings.data.txData[i].code & 0xFFFF00) ) {
          // found device
          settings.data.lightStatus[i] = (String(buff) == "ON" ? true : false);
        }
      }
    }
    else if (String(topic).indexOf("speed_") > 0) {
      // received fan speed status
      int codePos = String(topic).indexOf("speed_") + 6;
      unsigned long device=strtoul(String(topic).substring(codePos).c_str(), NULL, 16);
      DebugLn("fan speed status: 0x" + String(device, HEX));
      for (int i = 0; i < TXCODE_SIZE; i++) {
        if ( device == (settings.data.txData[i].code & 0xFFFF00) ) {
          // found device
          settings.data.fanSpeed[i] = String(buff).toInt();
        }
      }
    }
  }
  
  free(buff);
}

/*************************************************
 * MQTT reconnect function
 * **********************************************/
bool mqtt_reconnect() {
int res;

  if (!client.connected() ) {
    DebugLn("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = String(settings.data.name) + String(random(0xffff), HEX);
    // Attempt to connect

    if(strlen(settings.data.mqttuser)) {
      res = client.connect(clientId.c_str(), settings.data.mqttuser, settings.data.mqttpassword);
    }
    else {
      res = client.connect(clientId.c_str());
    }
    if (res) {
      DebugLn("MQTT connected");
      // once connect ....resubscribe
      base_topic = String(settings.data.mqtttopic) + "/" + String(settings.data.name);
      DebugLn("base topic: " + base_topic);
      client.subscribe((base_topic + "/#").c_str());
      DebugLn("subscribe: " + base_topic + "/#");
    } else {
      Debug ("MQTT reconnection failed, rc=");
      DebugLn (client.state());
    }
  }
  else // already connected
    res = true;
  return res;
}

/*************************************************
 * MQTT init function
 * **********************************************/
void mqtt_init() {

  client.setServer(settings.data.mqttbroker, settings.data.mqttport);
  DebugLn("setServer: " + String(settings.data.mqttbroker) + ", port: " +String(settings.data.mqttport));
  client.setCallback(mqtt_callback);
  mqtt_reconnect();

}

/*************************************************
 * MQTT send function
 * **********************************************/
void mqtt_send(String subtopic, String message, bool retain){

String topic = base_topic + "/" + subtopic;

  if (MQTTactive){
    // MQTT is active send message
    DebugLn("mqtt_send, topic: " + topic + ", payload: " + message);
    if(mqtt_reconnect() ) {
      // send data to topic
      client.publish(topic.c_str(), message.c_str(), retain);
      Debug("mqtt send [" );
      Debug(topic);
      Debug("]: ");
      DebugLn(message);
    }
 }

}
