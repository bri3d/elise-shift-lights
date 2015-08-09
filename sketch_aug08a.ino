#include <FastLED.h>
#include <SPI.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>

MCP_CAN CAN(10);
unsigned char Flag_Recv = 0;
unsigned char len = 0;
unsigned char buf[8];
char str[20];
#define NUM_LEDS 6
#define DATA_PIN 6
#define STATUS_LED_PIN 7
#define DATA_LED_PIN 8
CRGB leds[NUM_LEDS];

void setup()
{
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(DATA_LED_PIN, OUTPUT);
    Serial.begin(115200);
    Serial.println("Booting LEDs");
    resetAllLEDs(CRGB::Black);
    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
    FastLED.show();
    Serial.println("Booting CANbus");
    
START_INIT:

    if(CAN_OK == CAN.begin(CAN_1000KBPS))
    {
        Serial.println("CAN BUS Shield init ok!");
        digitalWrite(STATUS_LED_PIN, HIGH);
    }
    else
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println("Init CAN BUS Shield again");
        delay(100);
        goto START_INIT;
    }
    attachInterrupt(0, MCP2515_ISR, FALLING); // start interrupt
}

void MCP2515_ISR()
{
     Flag_Recv = 1;
}

void resetAllLEDs(CRGB color)
{
   for (int i=0; i<NUM_LEDS; i++)
    {
      leds[i] = color;
    }
}

void displayRPM(unsigned char buffer[8])
{
  int rpm = (buffer[2] << 8) & buffer[3];
  Serial.println(rpm);
  resetAllLEDs(CRGB::Black);
  if (rpm > 7800)
    leds[5] = CRGB::Red;
  if (rpm > 6600)
    leds[4] = CRGB::Yellow;
  if (rpm > 5400)
    leds[3] = CRGB::Yellow;
  if (rpm > 4100)
    leds[2] = CRGB::Green;
  if (rpm > 2800)
    leds[1] = CRGB::Green;
  if (rpm > 500)
    leds[0] = CRGB::Green;
  FastLED.show();
} 

void loop()
{
    if(Flag_Recv)                   // check if get data
    {
        digitalWrite(DATA_LED_PIN, HIGH);
        Flag_Recv = 0;                // clear flag
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf
        if (len > 6)
          displayRPM(buf);
        for(int i = 0; i<len; i++)    // print the data
        {
            Serial.print(buf[i]);Serial.print("\t");
        }
        Serial.println();
        digitalWrite(DATA_LED_PIN, LOW);
    }
}

