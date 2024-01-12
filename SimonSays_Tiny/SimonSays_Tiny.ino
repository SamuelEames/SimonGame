// 'Simon Game' assembled by S.Eames with help from the internet (thanks team :) )

// Include libraries
#include <Adafruit_NeoPixel.h>
#include "ENUMVars.h"

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

#define COL_RED_DIM     0x1F0000
#define COL_YELLOW_DIM  0x1F0B00
#define COL_GREEN_DIM   0x001F00
#define COL_BLUE_DIM    0x00001F

const uint32_t BTN_COLS[NUM_BTNS] = {COL_GREEN, COL_BLUE, COL_RED, COL_YELLOW};
const uint32_t BTN_DIM_COLS[NUM_BTNS] = {COL_GREEN_DIM, COL_BLUE_DIM, COL_RED_DIM, COL_YELLOW_DIM};

#define COL_WHITE   0x7F7F3F
#define COL_BLACK   0x000000


//////////////////// MISC VARIABLES ////////////////////

///// TIMING

#define DEBOUNCE           5     // (ms) Button debounce time
#define LED_REFRESH        100   // (ms) Refresh rate of LED patterns
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


//// GAME STATE

gameStates currentState = ST_Lobby;
gameStates lastState = ST_Lobby;


void setup() 
{

   // Initialise IO
   for (uint8_t i = 0; i < NUM_BTNS; ++i)
   {
      pinMode(Button[i], INPUT);
      btnState_last[i] = 1;            // Set initial assumed button state
   }

   // // Initialise LEDs
   leds.begin();
   leds.fill(COL_WHITE);      // All White
   leds.show();
}


void loop() 
{
   static long lasttime;

   switch (currentState) 
   {
      case ST_Lobby: // Waiting for someone to initiate a game by pressing any button
         updateLEDs();
         if(checkButtons() != NUM_BTNS)      // start game if any button is pressed
         {
            lastState = currentState;
            currentState = ST_Intro;
         }
         break;
      case ST_Intro:
         updateLEDs();
         // Refer to LED function for this part
         break;
      case ST_SeqPlay:
         
         // Reset variables
         if (lastState == ST_Intro)
         {
            generateSequence();
            seq_StepTimeStage = 0;

         }
         if (lastState != ST_SeqPlay)
         {
            lasttime = millis();          // Initiate timer
            OffAllButtons();
            seq_RecPlayStep = 0;
            lastState = currentState;
         }


         // Timing
         if (millis() < lasttime)         // Timer wrapped -- reset (should never be an issue here)
            lasttime = millis();

         if ((lasttime + seq_StepTime[seq_StepTimeStage]) > millis())      
            break; 
         
         lasttime = millis();

         // Show SeqStep
         if (seq_LightOn)              // Currently on --> turn off
         {
            OffAllButtons();
            seq_LightOn = !seq_LightOn;
         }
         else                       // Currently off --> turn on
         {
            HighLightButton(sequence[seq_RecPlayStep++]);
            seq_LightOn = !seq_LightOn;

            // Calculate StepTimeStage
            seq_StepTimeStage = floor(seq_RecPlayStep/SEQ_STEP_PER_BLOCK);
            if (seq_StepTimeStage >= sizeof(seq_StepTime) / sizeof(seq_StepTime[0]))      // NOTE: sizeof returns number of bytes, so divide by number of bytes per variable
               seq_StepTimeStage = (sizeof(seq_StepTime) / sizeof(seq_StepTime[0])) -1;   // -1 because index starts at 0
         }

         if (seq_RecPlayStep >= SEQ_MAX_LEN) // Winner winner chicken dinner!
         {
            seq_RecPlayStep = 0;
            // TODO: handle this case better - all levels achieved!
         }

         // GOTO Record state if we've finished playing sequence
         if (!seq_LightOn && (seq_RecPlayStep > seq_level))
         {
            currentState = ST_SeqRec;
            seq_level++;

            OffAllButtons();
         }
         break;

      case ST_SeqRec:

         // Timing                     // Note: timer still running from last stage & StepTime same as Play Stage
         if (millis() < lasttime)         // Timer wrapped -- reset (should never be an issue here)
            lasttime = millis();

         // Start indicator
         if (lastState == ST_SeqPlay)                 // Light all buttons white to indicate start of record sequence
         {
            // Note: need to measure button presses once we're out of this section so can't use break then
            if ((lasttime + seq_StepTime[seq_StepTimeStage]) > millis())      
               break; 
         
            lasttime = millis();

            if (seq_LightOn) 
            {
               LightDefault();                    // Start rec with all black again
               lastState = currentState;
               seq_LightOn = !seq_LightOn;
               seq_RecPlayStep = 0;                // Reset step variable when starting record sequence
            }
            else                                // NOTE: this is run first
            {
               leds.fill(COL_WHITE);      // All White
               leds.show();
               seq_LightOn = !seq_LightOn;      
            }
            break;
         }


         // Record player inputs!
         if ((lasttime + (seq_StepTime[seq_StepTimeStage] * SEQ_INPUTTIME_MULT)) > millis()) 
         {
            // Turn off previously lit buttons
            if ((lasttime + seq_StepTime[seq_StepTimeStage]) < millis())
               OffAllButtons();

            lastBtnPressed = checkButtons();    // check for input
            if(lastBtnPressed != NUM_BTNS)         // If any button pressed...
            {
               LightButton(lastBtnPressed);     // Light up button colour that was pressed
               lasttime = millis();          // reset timer on button press
            
               // Check correct button was pressed
               if (lastBtnPressed == sequence[seq_RecPlayStep])
               {
                  if (seq_RecPlayStep >= seq_level-1) // Go to next state if finished recording sequence
                     currentState = ST_Correct;
                  else
                     seq_RecPlayStep++;                     
               }
               else     // Incorrect
                  currentState = ST_Incorrect;
            }
         }
         else                             // Failed to press button in allowed time frame --> end game
         {
            // Timeout
            lasttime = millis();
            currentState = ST_Incorrect;
         }
         break;

      case ST_Correct:
         // Start indicator
         if (lastState != ST_Correct)                 // Keep buttons lit from last State, then play animation
         {
            // Note: need to measure button presses once we're out of this section so can't use break then
            if ((lasttime + LED_EFFECT_TIME) > millis())    
               break; 
         
            lasttime = millis();

            if (seq_LightOn) 
            {
               OffAllButtons();                    // Start with all black again
               lastState = currentState;
               seq_LightOn = !seq_LightOn;
            }
            else                                // NOTE: this is run first
               seq_LightOn = !seq_LightOn;      

            break;
         }
         updateLEDs();     // Play success animation
         break;

      case ST_Incorrect:
         updateLEDs();     // Play fail animation
         seq_level = 0;    // reset variables
         break;      

      case ST_HighScore:
         currentState = ST_Lobby;
         break;

      default:
      // statements
         break;
   }
}


void generateSequence()
{
   // Seed random number generator
   randomSeed(analogRead(RAND_ANALOG_PIN));

   // fill sequence with random numbers according to number of buttons used in game
   for (uint8_t i = 0; i < SEQ_MAX_LEN; ++i)
      {
         sequence[i] = random(0, NUM_BTNS);

         #ifdef debugMSG
            Serial.print(F(" "));
            Serial.print(sequence[i]);
         #endif
      }

      #ifdef debugMSG
         Serial.println();
      #endif

   return;
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


void updateLEDs()
{
   // Lights up LEDs based off state
   static uint8_t effect_step = 0;
   // Debounce buttons
   static long lasttime;

   if (millis() < lasttime)                  // Millis() wrapped around - restart timer
      lasttime = millis();

   static uint8_t hue = 0;

   // Does patterns on LEDs according to state
   switch (currentState) 
   {
      case ST_Lobby: // Waiting for someone to initiate a game by pressing any button

         if ((lasttime + LED_REFRESH) > millis())
            return; 
         
         LightDefault();

         break;


      case ST_Intro:

         if ((lasttime + LED_EFFECT_TIME) > millis())
            return; 
         
         if (effect_step++ % 2)
            leds.fill(COL_WHITE);
         else
            leds.fill(COL_BLACK);


         if (effect_step > LED_EFFECT_LOOP)
         {
            effect_step = 0;
            lastState = currentState;
            currentState = ST_SeqPlay;
         }
      
         break;

      case ST_SeqPlay:
         // Done in main loop
         break;

      case ST_SeqRec:
         // Done in main loop
         break;

      case ST_Correct:
         if ((lasttime + LED_EFFECT_TIME) > millis())
            return; 

         leds.fill(COL_WHITE);

         for (uint8_t i = effect_step++ % 2; i < NUM_LEDS; i += 2)
            leds.setPixelColor(i, COL_GREEN);
            // leds[i] = COL_GREEN;

         if (effect_step > LED_EFFECT_LOOP)
         {
            effect_step = 0;
            lastState = currentState;
            currentState = ST_SeqPlay;
         }
         break;

      case ST_Incorrect:
         if ((lasttime + LED_EFFECT_TIME) > millis())
            return; 

         leds.fill(COL_BLACK);

         for (uint8_t i = effect_step++ % 2; i < NUM_LEDS; i += 2)
            leds.setPixelColor(i, COL_RED);
            // leds[i] = COL_RED;

         if (effect_step > LED_EFFECT_LOOP)
         {
            effect_step = 0;
            lastState = currentState;
            currentState = ST_Lobby;
         }
         break;

      default:
      // statements
         break;
   }

   lasttime = millis();

   leds.show(); 

   return;
}

void LightDefault()
{
   // Sets buttons to default colours

   for (uint8_t i = 0; i < NUM_LEDS; ++i)
      leds.setPixelColor(i, BTN_DIM_COLS[i]);
   leds.show();
   
   return;
}

void LightButton(uint8_t button)
{
   // Lights up according to given button number

   for (uint8_t i = 0; i < NUM_LEDS; ++i)
   {
      if (i == button)
         leds.setPixelColor(i, BTN_COLS[i]);
      else
         leds.setPixelColor(i, BTN_DIM_COLS[i]);
   }
   leds.show();

   return;
}


void HighLightButton(uint8_t button)
{
   // Lights up according to given button number

   for (uint8_t i = 0; i < NUM_LEDS; ++i)
   {
      if (i == button)
         leds.setPixelColor(i, BTN_COLS[i]);
      else
         leds.setPixelColor(i, COL_BLACK);
   }
   leds.show();

   return;
}


void OffAllButtons()
{
   // Turns off lights on buttons

   leds.fill(COL_BLACK);
   leds.show();

   return;
}