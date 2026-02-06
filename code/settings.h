/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <EEPROM.h> // v 1.0.1

#ifndef SETTINGS_H
#define SETTINGS_H

#define MAGIC_LENGTH 4
#define SSID_LENGTH 32
#define PSK_LENGTH 64
#define NAME_LENGTH 32
#define SERVER_LENGTH 256
#define ID_LENGTH 16
#define KEY_LENGTH 16

#define RXCODE_SIZE 10
#define TXCODE_SIZE 5

#define MAGIC "RFB\0"

char my_default_ssid[SSID_LENGTH] = "";
char my_default_psk[PSK_LENGTH] = "";

#define NODE_NAME "RF-BRIDGE"

struct TXcode {
  int            protocol;
  unsigned int   pulseLength;
  unsigned long  code;
  unsigned int   dataLength;
  bool           toggle;
};

struct RXcode {
  int            protocol;
  unsigned long  code;
  int            txIndex; 
};

class Settings {
  public:
  struct {
    char    magic[MAGIC_LENGTH];               // magic
    char    ssid[SSID_LENGTH];                 // SSID
    char    psk[PSK_LENGTH];                   // PASSWORD
    char    name[NAME_LENGTH];                 // NAME
    char    mqttbroker[SERVER_LENGTH];         // Address of MQTT broker server
    char    mqtttopic[NAME_LENGTH];            // MQTT topic for Jeedom
    char    mqttuser[NAME_LENGTH];             // MQTT account user
    char    mqttpassword[NAME_LENGTH];         // MQTT account password
    int     mqttport;                          // port of MQTT broker
    RXcode  rxData[RXCODE_SIZE];               // array to store RX messages
    TXcode  txData[TXCODE_SIZE];               // array to store TX messages
    bool    lightStatus[TXCODE_SIZE];          // array to store fan light status
    int     fanSpeed[TXCODE_SIZE];             // array to store fan speeds
    int     prevFanSpeed[TXCODE_SIZE];         // array to storte previous fan speed
    } data;
    
    Settings() {};
 
    void Load() {           // Carga toda la EEPROM en el struct
      EEPROM.begin(sizeof(data));
      EEPROM.get(0,data);  
      EEPROM.end();
     // Verify magic; // para saber si es la primera vez y carga los valores por defecto
     if (String(data.magic)!=MAGIC){
      data.magic[0]=0;
      data.ssid[0]=0;
      data.psk[0]=0;
      data.name[0]=0;
      data.mqttbroker[0]=0;
      strcpy(data.mqtttopic, "jeedom");
      data.mqttuser[0]=0;
      data.mqttpassword[0]=0;
      data.mqttport=1883;
      for (int i = 0; i < RXCODE_SIZE; i++) {
        data.rxData[i].protocol = -1;
        data.rxData[i].code = 0;
        data.rxData[i].txIndex = -1;
      }
      for (int i = 0; i < TXCODE_SIZE; i++) {
        data.txData[i].protocol = -1;
        data.txData[i].pulseLength = 0;
        data.txData[i].code = 0;
        data.txData[i].dataLength = 0;
        data.txData[i].toggle = false;
        data.lightStatus[i] = false;
        data.fanSpeed[i] = 0;
        data.prevFanSpeed[i] = 0;
      }
      Save();
     }
    };
      
    void Save() {
      EEPROM.begin(sizeof(data));
      EEPROM.put(0,data);
      EEPROM.commit();
      EEPROM.end();
    };
};
#endif
