# elise-shift-lights
Simple Arduino sketch for turning Elise dashboard CAN message (0x400) into shift lights

# installation
* Add https://github.com/Seeed-Studio/CAN_BUS_Shield to your Arduino Libraries
* Add http://fastled.io/ to your Arduino Libraries
* Build
* Attach MCP2515 to SPI. I tested with the Sparkfun/SKPang MCP2515 shield as well as a MikroElektronika CAN-SPI.
* Attach a string of WS2811 LEDs to DATA_PIN (pin 6 by default).
* Attach CAN to the CAN pins on the Elise's diagnostic port.

# credits
* CANBus init routines from Seeed Studio Examples under the MIT license.
* http://www.lotustalk.com/forums/f129/canbus-re-analysis-10866/
* http://www.lotustalk.com/forums/1197327-post42.html
