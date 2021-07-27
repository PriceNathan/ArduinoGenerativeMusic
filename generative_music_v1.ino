/*
 *    ARDUINO MUSIC GENERATOR - V1.0
 *    
 *    Author:     Nathan Price 100142522
 *    Date:       April 20 2021
 * 
 *    Brief:      This uses algorithmic generation to create a musical sequence based on user inputs, and outputs MIDI signal with MIDI clock functionality.
 *                This program is not separated into classes and functions yet, but I am working on a separate version to allow user-defined inputs and controls
 *                for adaptation into other embedded systems.
 *                As well, I am working on developing this deeper into further functionality with accompaniment, chord progressions, and countermelodies. 
 *                As of now, this program was written specifically for the hardware I've used but it is adaptable. Specifically the generator systems are functional for any use-case,
 *                the only thing that might need to be changed is the user interface. 
 *                
 *                Planned fixes include introducing non-blocking functions and making the interface more modular.
 *    
 *    Includes:   LiquidCrystal.h library, https://github.com/arduino-libraries/LiquidCrystal/blob/master/src/LiquidCrystal.h
 *                MIDI.h library, https://github.com/FortySevenEffects/arduino_midi_library/blob/master/src/MIDI.h
 * 
 */


#include <LiquidCrystal.h>    //LCD display header
#include <MIDI.h>             //MIDI connectivity header


//While we could just use two arrays, the third for melody notes is part of the next version, which will include options to keep a sequence and change modes, and to store a sequence using EEPROM
//initializing arrays for melody:
byte melodyArray[16]; //initialize empty array of 16 potential melody notes
byte rhythmArray[16]; //initializes empty array for indexing on/off values to set rhythm for melody
byte melodyNotes[16]; //melody transposed into MIDI note values

//modes are listed in arrays below - these figures are indexed by scale positons to return note values
byte modeSelect = 0; //initializes for selecting mode later - initialized to 0, ionian mode


//stored as bytes to conserve memory
byte ionianMode[] = {0, 2, 4, 5, 7, 9, 11, 12}; 
byte dorianMode[] = {0, 2, 3, 5, 7, 9, 10, 12};
byte phrygianMode[] = {0, 1, 3, 5, 7, 8, 10, 12};
byte lydianMode[] = {0, 2, 4, 6, 7, 9, 11, 12};
byte mixolydianMode[] = {0, 2, 4, 5, 7, 9, 10, 12};
byte aeolianMode[] = {0, 2, 3, 5, 7, 8, 10, 12};
byte locrianMode[] = {0, 1, 3, 5, 6, 8, 10, 12};

//String mode = ""; //initialize mode string to be returned as part of display()
String mode[] = {"IONIAN", "DORIAN", "PHRYGIAN", "LYDIAN", "MIXOLYDIAN", "AEOLIAN", "LOCRIAN"}; //mode names are stored in an array corresponding to the value of modeSelect - to be returned in display()

byte keySelect = 0;//initialized as C
String key[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};//display() will return a string based on the value of keyselect

//midi variables
byte tempo = 120;//tempo has a starting value of 120 - later functionality will include options to change this or use midi clock input to set time
float noteTime = (60000 / tempo) / 4;; //tempo divided by 4 for each 16th note - it's a float since we may have time divisions that aren't whole 
float clockVal = (noteTime / 6);//midi clock values are sent 

//arduino physical variables
byte buttonPin1 = 2;
volatile byte buttonMode1 = 0; //used for state changing with interrupts
byte buttonPin2 = 3;
volatile byte buttonMode2 = 0; //used for state changing with interrupts

//LCD wiring values
byte lcdV0 = 10;
byte lcdRS = 12;
byte lcdE = 11;
byte lcdD4 = 9;
byte lcdD5 = 8;
byte lcdD6 = 7;
byte lcdD7 = 6;
byte contrast = 90; //used to set contrast on LCD screen

LiquidCrystal lcd(lcdRS, lcdE, lcdD4, lcdD5, lcdD6, lcdD7);//initialize LCD

MIDI_CREATE_DEFAULT_INSTANCE();// Creates and binds the MIDI interface to the default hardware serial port.

void setup() {
  
  analogWrite(lcdV0, contrast);//initialize lcd screen contrast value
  
  pinMode(buttonPin1, INPUT);//initialize button
  pinMode(buttonPin2, INPUT);//initialuze button

  attachInterrupt(digitalPinToInterrupt(buttonPin1), setButtonMode1, FALLING);//interrupt for button 1
  
  attachInterrupt(digitalPinToInterrupt(buttonPin2), setButtonMode2, FALLING);//interrupt for button 2

  randomSeed(analogRead(0));//used for generating random numbers

  MIDI.begin(MIDI_CHANNEL_OMNI);  //initializes MIDI

  lcd.begin(16, 2); //initialize lcd screen


}

void setButtonMode1()
{
  buttonMode1++;
  buttonMode2 = 0;//resets buttonMode2 with each press for cyclical state changes
}

void setButtonMode2()
{
  buttonMode2++;
}



/*
 *  refactoreed from 2 separate generator fucntions
 *  this takes an array as an argument with specified minimum and maximum ranges for generation
 *  a range of 0-8 will keep everything within a single octave
 *  a range of 0-2 is a binary sequence, used for generating a rhythmic pattern
 *  a larger range can be used for generating rhythms to include less rests and more notes - to be included in future functionality
 */
void generator(byte musicArray[], byte min, byte max)
{
  //for loop to randomly set each value of melody within range
  for (byte i = 0; i < 16; i++)
  {
    musicArray[i] = random(min, max);
  }
}

/*
 *  this takes the melodyArray (that uses scale positions) and maps the sequence to one of the modes declared earlier
 *  the pattern of numbers added indicates whole or half steps for each note
 *  this will be expanded with further logic next, to allow for a range beyond 8 notes (multiple octaves)
 */
void scaleMapper(byte mode[], byte length)
{
  for (byte j = 0; j < 16; j++)
  {
    byte scalePosition = melodyArray[j];
    melodyNotes[j] = mode[scalePosition];
  }
}


//used to select a mode to be passed as an argument to map scale values to mode values
void chooseMode()
{
  switch (modeSelect)
  {
    case 0:
      scaleMapper(ionianMode, 16);
      break;

    case 1:
      scaleMapper(dorianMode, 16);
      break;

    case 2:
      scaleMapper(phrygianMode, 16);
      break;

    case 3:
      scaleMapper(lydianMode, 16);
      break;

    case 4:
      scaleMapper(mixolydianMode, 16);
      break;

    case 5:
      scaleMapper(aeolianMode, 16);
      break;

    case 6:
      scaleMapper(locrianMode, 16);
      break;
  }
}

/*
   This takes the melody newly quantized to a scale, and adds that value to a key value and then to 60, corresponding to middle C on a keyboard

   For example, middle C is MIDI note 60, so a scale starting on C would have the '0' (root) note be added to 60, and work similarly for all other notes in the scale
*/
void keyMapper()
{
  for (byte i = 0; i < 16; i++)
  {
    byte tune = 60 + keySelect + melodyArray[i];
    melodyNotes[i] = tune; //adds modeSelector value (0 for C and increasing by 1 for each semitone increase in key)
  }
}

/* 
 * This method is manipulated with button2's interrupt to change the variable modeSelect, used for determining which mode is used to modify the melody sequence from a basic scale position pattern
 */
void modeSelector()
{

  if (buttonMode2 < 7)
  {
    modeSelect = buttonMode2;
  }
  if (buttonMode2 > 6)
  {
    buttonMode2 = 0;
    modeSelect = 0;
  }
}

/*
 * This method is manipulated with button2's interrupt to change the variable modeSelect, used for determining which mode is used to modify the melody sequence from a basic scale position pattern
 */
void keySelector()
{
  if (buttonMode2 < 12) //if buttonmode is less than 12, it has a value corresponding to buttonMode2
  {
    keySelect = buttonMode2;
  }
  else //if buttonMode2 is 12, it resets to 0
  {
    buttonMode2 = 0;
    keySelect = 0;
  }
}


/*
 * Depending on the state of buttonMode1, different information is displayed for each state of the program
 * 0 - start mode, displays current mode choice and prompts for confirmation
 * 1 - select mode, after the button is pressed it will quantize the scale pattern to that of the mode and prompt user to press button to start sequence
 * 2 - play mode, seuqnce starts and the display will prompt user to press button to stop
 */
void display()
{
  switch (buttonMode1)
  {
    case 0:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CHOOSE KEY");
    lcd.setCursor(0, 1);
    lcd.print(key[keySelect]);
    break;

    case 1:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CHOOSE MODE");
    lcd.setCursor(0, 1);
    lcd.print(mode[modeSelect]);
    break;

    case 2:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PLAYING");
    lcd.setCursor(0, 1);
    lcd.print("PRESS TO STOP");
    break;
  }
}



/*
 * This is the method that runs the loop through the melody using MIDI signals sent over the serial port
 * Currently this uses the delay() method but this needs to be updated for better MIDI compatibility soon to use clock signals
 * EDIT: Sends clock signals now, but still using delay - will be changed to a non-blocking method soon
*/
void playMelody()
{
  MIDI.begin(1);
  for (byte i = 0; i < 16; i++)
  {
    //lcd.print(melodyArray[i]); for testing
    if (rhythmArray[i] > 0) //if the rhythmArray value is a 1, the note is played
    {

      MIDI.sendNoteOn(melodyNotes[i], 127, 1);    //sends note to be played to MIDI channel 1, with max velocity (127)
      MIDI.sendClock();
      delay(clockVal);
      MIDI.sendClock();
      delay(clockVal);
      MIDI.sendClock();
      delay(clockVal);
      MIDI.sendClock();
      delay(clockVal);
      MIDI.sendClock();
      delay(clockVal);
      MIDI.sendClock();
      delay(clockVal);
      //delay(noteTime);                            //delays for 1/16th of the tempo duration (16th notes)
      MIDI.sendNoteOff(melodyNotes[i], 127, 1);  // Stop the note after the time duration has passed
    }
    else
    {
      //no more note information is passed if the rhythmArray value is 0 - this indicates a rest in the melody
      MIDI.sendClock();
      delay(clockVal);
      MIDI.sendClock();
      delay(clockVal);
      MIDI.sendClock();
      delay(clockVal);
      MIDI.sendClock();
      delay(clockVal);
      MIDI.sendClock();
      delay(clockVal);
      MIDI.sendClock();
      delay(clockVal);//delays for 1/16th of the tempo duration (16th notes)
    }
  }
}






void loop() {
  //the initial state is to generate a new array for a melody
  
  if (buttonMode1 == 0)
  {
    //initial startup generates arrays for the melody pattern and rhythm array
    generator(melodyArray , 0, 8);//generates melody within one octave (C-C)
    generator(rhythmArray , 0, 2); //generates binary sequence for on/off of notes
    
    display();
    delay(150); //short delay for lcd screen flicker
    keySelector();

    
  }

  //when button1 is pressed it locks in values for the mode chosen
  if (buttonMode1 == 1)
  {
    display();
    delay(150); //short delay for lcd screen flicker
    modeSelector(); //selects mode using button2
    chooseMode();//send to scaleMapper
    keyMapper(); //maps values from mode to a key - passing key value as argument
  }


  //when the button is pressed again, the system starts playing a melody
  while (buttonMode1 == 2)
  {  
    display();
    playMelody();
  }

  //when the button is pressed again, the system reverts back to the beginning
  if (buttonMode1 > 2)
  {
    display();
    buttonMode1 = 0;
  }
  
}
