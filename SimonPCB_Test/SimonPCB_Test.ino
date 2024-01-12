// 'Simon Game' assembled by S.Eames with help from the internet (thanks team :) )

// Include libraries
#include <Adafruit_NeoPixel.h>

///////////////////////// IO /////////////////////////
#define NUM_BTNS     4

#define LED_PIN      3
const uint8_t Button[NUM_BTNS] = {0, 1, 2, 4};  // Pins for buttons
#define RAND_ANALOG_PIN 5           // Analog pin -- Leave pin floating -- used to seed random number generator

// Button state arrays
uint8_t btnState_last[NUM_BTNS];    // State of buttons on previous iteration
uint8_t btnState_now[NUM_BTNS];        // Last read state of buttons

//////////////////// Pixel Setup ////////////////////
#define NUM_LEDS  4

Adafruit_NeoPixel leds = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800); 

#define LED_BRIGHTNESS 20

// Colours!
#define COL_RED     0xFF0000
#define COL_YELLOW  0xFF8F00
#define COL_GREEN   0x00FF00
#define COL_BLUE    0x0000FF

const uint32_t BTN_COLS[NUM_BTNS] = {COL_RED, COL_YELLOW, COL_GREEN, COL_BLUE};

#define COL_WHITE   0xFFFF7F
#define COL_BLACK   0x000000


//////////////////// MISC VARIABLES ////////////////////

///// TIMING

#define DEBOUNCE        5     // (ms) Button debounce time
#define LED_REFRESH     100   // (ms) Refresh rate of LED patterns
#define LED_EFFECT_TIME    150      // (ms) Interval of LED effects
#define LED_EFFECT_LOOP    10       // Multiplier of LED effect time

// StepTime (ms) is the on & off time intervals of steps played back in a sequence. 
// SEQ_STEP_PER_BLOCK indicates when speed increases - i.e. after level = SEQ_STEP_PER_BLOCK, seq_StepTime[1] is used for interval
// For levels higher than sizeof(seq_StepTime) * SEQ_STEP_PER_BLOCK, seq_StepTime[sizeof(seq_StepTime) -1] is used
// When in ST_SeqRec, 2*seq_StepTime is the allowed space for players to press button
uint16_t seq_StepTime[] = {600, 550, 500, 450, 400, 350, 300, 250, 200};
uint8_t seq_StepTimeStage = 0;
#define SEQ_STEP_PER_BLOCK 4

#define SEQ_INPUTTIME_MULT 3     // SEQ_INPUTTIME_MULT * seq_StepTime[x] = max allowed time to press button


///// PATTERN
#define SEQ_MAX_LEN  100            // Maximum sequence length (maximum value uint8_t can store)
uint8_t sequence[SEQ_MAX_LEN];      // Holds sequence used for game
uint8_t seq_level = 0;           // Current level achieved of sequence - increments with each success
uint8_t seq_RecPlayStep = 0;     // Current step being either played back or recorded
uint8_t highscore = 0;           // Holds highest achieved level (since power on);
uint8_t seq_LightOn = 0;         // (bool) holds whether in on or off state of playing sequence
uint8_t lastBtnPressed = NUM_BTNS;

#define TEST_LED_PIN 1

void setup() 
{

   // Test LED on PB1
   pinMode(TEST_LED_PIN, OUTPUT);
   for (uint8_t i = 0; i < 10; ++i)
   {
      digitalWrite(TEST_LED_PIN, HIGH);
      delay(100);
      digitalWrite(TEST_LED_PIN, LOW);
      delay(100);   
   }

   // Initialise IO
   for (uint8_t i = 0; i < NUM_BTNS; ++i)
   {
      pinMode(Button[i], INPUT);
      btnState_last[i] = 1;            // Set initial assumed button state
   }

   // // Initialise LEDs
   leds.begin();

   for (uint8_t i = 0; i < NUM_LEDS; ++i)
   {
      delay(1000);
      leds.setPixelColor(i, COL_WHITE);
      leds.show();
   }


}


void loop() 
{
   // Light LEDs according to whatever button was pressed
   uint8_t tempButton = checkButtons();
   if (tempButton != NUM_BTNS)
   {
      leds.fill(BTN_COLS[tempButton]);
      leds.show();
   }

}



uint8_t checkButtons()
{
   // Checks button state
   // If button just pressed returns that button number, else returns NUM_BTNS


   // Debounce buttons
   static long lasttime;

   if (millis() < lasttime)                  // Millis() wrapped around - restart timer
      lasttime = millis();

   if ((lasttime + DEBOUNCE) > millis())        // Debounce timer hasn't elapsed
      return NUM_BTNS; 
   
   lasttime = millis();                   // Debouncing complete; record new time & continue

   // Record button state
   for (uint8_t i = 0; i < NUM_BTNS; ++i)
   {
      btnState_now[i] = !digitalRead(Button[i]);   // Read input

      if (btnState_now[i] != btnState_last[i])  // If button state changed 
      {
         btnState_last[i] = btnState_now[i];    // Record button state

         if (!btnState_now[i])
            return i;                     // Return number of button that was just pressed
      }
   }

   return NUM_BTNS;                       // Return NUM_BTNS if no buttons pressed
}
