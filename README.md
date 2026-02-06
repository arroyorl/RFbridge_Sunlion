# RFbridge for Sunlion
RFbridge allows to switch on/off light of fans, not neededing to switch on/off the complete fan.

Leave the fan conected always to power, and use RF433 buttons to transmit a code to the bridge. Once the bridge receives the code checks if is in the list and transmits the fan light toggle code as per the stored table. This version runs on an ESP8266 controller, plus an CC1101 Rx/Tx board, using ELECHOUSE_CC1101_SRC_DRV library.

In this version for Sunlion fans, the bridge keeps internal track of light status and fan speed, sending a MQTT message with the new status or speed. Commands may be also received by MQTT command message.

## MQTT messages

**topic/set_light/code:** 	sets the light corresponding to the code to the status defined by payload (ON/OFF)
**topic/ligth_code:**	ligth status message, payload is the current status (ON/OFF)
**topic/set_fanspeed/code:**	sets the fan corresponding to code to the payload defined speed (0-6), zero stops the fan
**topic/speed_code:**	fan speed status message, payload is the current speed (0-6), zero is fan is stopped
**topic/cmd:**		json command. json is: {"device": "code", "light": "ON/OFF", "speed": speed }
**topic/sendtxdata:**	toggle the light corresponding to device number "payload", as per Tx table (first in list is 1)
**topic/send:**		send RF message {"protocol": pp, "length": ll, "code": nnnnnn, "pulselength": pl}



