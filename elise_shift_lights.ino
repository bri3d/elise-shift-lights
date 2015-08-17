#include <FastLED.h>
#include <SPI.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>

#define CAN_CS_PIN 10
#define NUM_LEDS 8
#define DATA_PIN 6
#define STATUS_LED_PIN 7
#define DATA_LED_PIN 8
#define WARM_REDLINE 8000
#define COLD_REDLINE 6000
#define NUM_COLORS 4
#define BASE_RPM 500

MCP_CAN CAN(CAN_CS_PIN);

CRGB leds[NUM_LEDS];
int breakpoints[NUM_LEDS];
CRGB colors[NUM_LEDS];

unsigned char warmed_up = 0;
unsigned char flag_recv = 0;
unsigned char len = 0;
unsigned char buf[8];
char str[20];

void resetAllLEDs(CRGB color)
{
   for (int i=0; i<NUM_LEDS; i++)
    {
      leds[i] = color;
    }
}

void setup_breakpoints(int redline)
{
  int step = redline / NUM_LEDS;
  breakpoints[0] = BASE_RPM;
  for(int i=1; i<NUM_LEDS; i++) {
    breakpoints[i] = breakpoints[i-1] + step;
    Serial.print("Breakpoint at ");
    Serial.println(breakpoints[i]);
  }
  Serial.print("Redline at ");
  Serial.println(redline);
}

void setup_colors()
{
  CRGB base_colors[NUM_COLORS] = {CRGB::Green, CRGB::Green, CRGB::Yellow, CRGB::Red};
  int divider = NUM_LEDS / NUM_COLORS;
  for(int c=0; c<NUM_LEDS; c++)
  {
      colors[c] = base_colors[c/divider];
  }
}

void displayRPM(int rpm)
{
  resetAllLEDs(CRGB::Black);
  char i = NUM_LEDS;
  while(i>0) {
    i--;
    if (rpm > breakpoints[i])
      leds[i] = colors[i]; 
  }
} 

void check_temperature(int temperatureDegC)
{
  if((!warmed_up) && temperatureDegC > 71) // Temperature: DegC * 1.6 + 64. So, 71C * 1.6 + 64 = 178 temp constant for "warm engine"
  {
    warmed_up = 1;
    setup_breakpoints(WARM_REDLINE);
  }
}

int extractRPM(unsigned char buffer[8])
{
  return (buffer[2] << 8) | buffer[3];
}

int extractShiftLight(unsigned char buffer[8])
{
  return buffer[6] & 0x1;
}

int extractTemperature(unsigned char buffer[8])
{ // returns temperature in Degrees C
  return (buffer[5] - 64 ) / 1.6;
}

void handle_message(unsigned char buffer[8])
{
  int temperature = extractTemperature(buffer);
  check_temperature(temperature);
  if (extractShiftLight(buffer)) {
    resetAllLEDs(CRGB::Red);
  } else {
    int rpm = extractRPM(buffer);
    displayRPM(rpm);
  }
}

void initialize_leds()
{
  Serial.println("Booting LEDs");
  resetAllLEDs(CRGB::Purple);
  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.show();
}

void initialize_can()
{
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
     flag_recv = 1;
}

void setup()
{
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(DATA_LED_PIN, OUTPUT);
    Serial.begin(115200);
    setup_breakpoints(COLD_REDLINE);
    setup_colors();
    initialize_leds();
    initialize_can();
}

void loop()
{
    if(flag_recv)
    {
        digitalWrite(DATA_LED_PIN, HIGH);
        flag_recv = 0;
        CAN.readMsgBuf(&len, buf);
        if (len > 6) {
          handle_message(buf);
        }
        FastLED.show();
        digitalWrite(DATA_LED_PIN, LOW);
    }
}

