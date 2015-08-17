#include <FastLED.h>
#include <SPI.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>

//#define IS_MY08

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
uint_fast16_t breakpoints[NUM_LEDS];
CRGB colors[NUM_LEDS];

uint_fast8_t warmed_up = 0;
uint_fast8_t flag_recv = 0;

void resetAllLEDs(CRGB color)
{
   for (uint_fast8_t i=0; i<NUM_LEDS; i++)
    {
      leds[i] = color;
    }
}

void setup_breakpoints(int redline)
{
  uint_fast16_t step = redline / NUM_LEDS;
  breakpoints[0] = BASE_RPM;
  for(uint_fast8_t i=1; i<NUM_LEDS; i++) {
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
  uint_fast8_t divider = NUM_LEDS / NUM_COLORS;
  for(uint_fast8_t c=0; c<NUM_LEDS; c++)
  {
      colors[c] = base_colors[c/divider];
  }
}

void displayRPM(uint_fast16_t rpm)
{
  resetAllLEDs(CRGB::Black);
  uint_fast8_t i = NUM_LEDS;
  while(i>0) {
    i--;
    if (rpm > breakpoints[i])
      leds[i] = colors[i]; 
  }
} 

void check_temperature(int_fast8_t temperatureDegC)
{
  if((!warmed_up) && temperatureDegC > 71) // Temperature: DegC * 1.6 + 64. So, 71C * 1.6 + 64 = 178 temp constant for "warm engine"
  {
    warmed_up = 1;
    setup_breakpoints(WARM_REDLINE);
  }
}

uint_fast16_t extractRPM(uint8_t buffer[8])
{
  return (buffer[2] << 8) | buffer[3];
}

uint_fast8_t extractShiftLight(uint8_t buffer[8])
{
  return buffer[6] & 0x1;
}

int_fast8_t extractTemperature(uint8_t buffer[8])
{ // returns temperature in Degrees C
  return (buffer[5] - 64 ) / 1.6;
}

void handle_message(uint8_t buffer[8])
{
  int_fast8_t temperature = extractTemperature(buffer);
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
#ifdef IS_MY08
    if(CAN_OK == CAN.begin(CAN_500KBPS))
#else
    if(CAN_OK == CAN.begin(CAN_1000KBPS))
#endif
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
        uint8_t len = 0;
        uint8_t buf[8];
        uint32_t id = 0;
        digitalWrite(DATA_LED_PIN, HIGH);
        flag_recv = 0;
        CAN.readMsgBufID(&id, &len, buf);
        if ((0x400 == id) && (len > 6)) {
          handle_message(buf);
        }
        FastLED.show();
        digitalWrite(DATA_LED_PIN, LOW);
    }
}

