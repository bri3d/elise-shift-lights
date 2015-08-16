#include <FastLED.h>
#include <SPI.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>

#define CAN_CS_PIN 10
#define NUM_LEDS 8
#define DATA_PIN 6
#define STATUS_LED_PIN 7
#define DATA_LED_PIN 8
#define REDLINE 8000
#define NUM_COLORS 4
#define BASE_RPM 500

MCP_CAN CAN(CAN_CS_PIN);

CRGB leds[NUM_LEDS];
int breakpoints[NUM_LEDS];
CRGB colors[NUM_LEDS];

unsigned char Flag_Recv = 0;
unsigned char len = 0;
unsigned char buf[8];
char str[20];

void setup()
{
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(DATA_LED_PIN, OUTPUT);
    Serial.begin(115200);
    setup_breakpoints();
    setup_colors();
    Serial.println("Booting LEDs");
    resetAllLEDs(CRGB::Purple);
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

void setup_breakpoints()
{
  int step = REDLINE / NUM_LEDS;
  breakpoints[0] = BASE_RPM;
  for(int i=1; i<NUM_LEDS; i++) {
    breakpoints[i] = breakpoints[i-1] + step;
    Serial.println("Breakpoint at ");
    Serial.print(breakpoints[i]);
  }
  Serial.print("Redline at ");
  Serial.println(REDLINE);
}

void setup_colors()
{
  CRGB base_colors[NUM_COLORS] = {CRGB::Green, CRGB::Green, CRGB::Yellow, CRGB::Red};
  int step = NUM_LEDS / NUM_COLORS;
  for(int c=0; c<NUM_COLORS; c++)
  {
    for (int i=0; i<step; i++) {
      colors[i] = base_colors[c];
    }
  }
}

void displayRPM(unsigned char buffer[8])
{
  int rpm = (buffer[2] << 8) | buffer[3];
  Serial.println(rpm);
  resetAllLEDs(CRGB::Black);
  char i = NUM_LEDS;
  while(i>0) {
    i--;
    if (rpm > breakpoints[i])
      leds[i] = colors[i]; 
  }
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

