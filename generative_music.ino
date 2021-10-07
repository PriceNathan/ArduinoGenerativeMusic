#include <MIDI.h>


/**
 *    MIDI generative music sequencer
 * 
 *    Authored by Nathan Price
 *    March 31 2021
 * 
 */

const uint8_t midiChannel = 1;

//arduino physical variables
int buttonPin = 2;
int buttonMode = 0;


//range initializing 
int rangeLow; //sets low range of melody
int rangeHigh; //sets high range of melody
int range; //is passed to selectRange function to set low and high range

//initializing arrays for melody:
int melodyArray[16]; //initialize empty array of 16 potential melody notes
int rhythmArray[16]; //initializes empty array for indexing on/off values to set rhythm for melody
int melodyNotes[16]; //melody transposed into MIDI note values

//modes are listed in arrays below - these figures are added to the scale place to set the increment in semitones or whole tones for the scale
int ionianMode[] = {0, 1, 2, 2, 3, 4, 5, 5};
int dorianMode[] = {0, 1, 1, 2, 3, 4, 4, 5};
int phrygianMode[] = {0, 0, 1, 2, 3, 3, 4, 5};
int lydianMode[] = {0, 1, 2, 3, 3, 4, 5, 5};
int mixolydianMode[] = {0, 1, 2, 2, 3, 4, 4, 5};
int aeolianMode[] = {0, 1, 1, 2, 3, 3, 4, 5};
int locrianMode[] = {0, 0, 1, 2, 2, 3, 4, 5};

//midi variables
int tempo = 120;//tempo has a starting value of 120
//float clockVal = (tempo / 96); //Clock events are sent at a rate of 24 pulses per quarter note - to be used for future functionality
float noteTime = (60000 / tempo) / 4;; //tempo divided by 16 for each note - it's a float since we may have time divisions that aren't whole numbers

// Created and binds the MIDI interface to the default hardware serial port.
MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {

  Serial.begin(9600);
  
  pinMode(buttonPin, INPUT);

  attachInterrupt(digitalPinToInterrupt(buttonPin), setButtonMode, FALLING);
  
  randomSeed(analogRead(13));//used for generating random numbers
  
  MIDI.begin(MIDI_CHANNEL_OMNI);  //initializes MIDI
  
}

void setButtonMode()
{
  buttonMode++;
}





/*
* we need to specify a range in octaves for the melody
* this determines if the melody is within 1 octave 0-7
* 2 octaves 0-15
* or 3 octaves (-8)-15
*/

/* THIS IS PLANNED FUNCTIONALITY FOR THE FUTURE VERSIONS OF THIS CODE
 * 
void selectRange(int range) {
  if (range == 1) {
   rangeLow = 0;
   rangeHigh = 8;
  } else if (range == 2) {
    rangeLow = 0;
    rangeHigh = 16;
  } else if (range == 3) {
    rangeLow = 0;
    rangeHigh = 24;
  }
 // Serial.print(rangeLow);
 // Serial.print(rangeHigh);
  
}//closes function selectRange
*/


/*THIS IS PLANNED FUNCTIONALITY FOR THE FUTURE VERSIONS OF THIS CODE
 * 
void chooseMode(int key)
{
  switch(key)
  {
    case 0:
    return ionianMode[];
    break;
  
    case 1:
    return dorianMode[];
    break;
  
    case 2:
    return phrygianMode[];
    break;
  
    case 3:
    return lydianMode[];
    break;
    
    case 4:
    return mixolydianMode[];
    break;
  
    case 5:
    return aeolianMode[];
    break;
  
    case 6:
    return locrianMode[];
    break;
  }
}
*/

//this generates a random selection of notes to represent values within a scale, later to be quantized
void melodyGenerator(int rangeLow, int rangeHigh)
{
  //for loop to randomly set each value of melody within range
  for (int i = 0; i < 16; i++) 
  {
    int note = random(rangeLow, rangeHigh);
    Serial.print(note);
    melodyArray[i] = note;
  }
}

void rhythmGenerator()
{
  //for loop to randomly set each value of melody within range
  for (int i = 0; i < 16; i++) 
  {
    int note = random(0, 2);
    Serial.print(note);
    rhythmArray[i] = note;
  }
}



/*
 * Currently the scale mapping just stays in Ionian mode, but future functionality is planned to incorporate various modes
 * 
 * This takes the generated pattern of scale notes and quantizes them to the values of the mode, to later be quantized to the key of the song
 */
void scaleMapper()
{
  for (int j = 0; j < 16; j++)
  {
      int scalePosition = melodyArray[j];
      int modePosition = ionianMode[scalePosition];
      int scaleNote = scalePosition + modePosition;
      melodyArray[j] = scaleNote;
  }
}

/*
 * NEXT STEP PLANNED FOR THIS IS TO INCORPORATE A SELECTION FOR DIFFERENT KEY SIGNATURES
 * 
 * This takes the melody newly quantized to a scale, and adds that value to a key value
 * 
 * For example, middle C is MIDI note 60, so a scale starting on C would have the '0' (root) note be added to 60, and work similarly for all other notes in the scale
 */
void keyMapper(int noteValue)
{
  for (int i = 0; i < 16; i++)
  {
    melodyNotes[i] = melodyArray[i] + noteValue;
  }
}

/*
 * This is the method that runs the loop through the melody using MIDI signals sent over the serial port
 * 
 */
void playMelody()
{
  MIDI.begin(1);
  for (int i = 0; i < 16; i++)
  {
    if (rhythmArray[i] > 0) //if the rhythmArray value is a 1, the note is played
    {
    
      MIDI.sendNoteOn(melodyNotes[i], 127, 1);    //sends note to be played to MIDI channel 1, with max velocity (127)
      delay(noteTime);                            //delays for 1/16th of the tempo duration (16th notes)
      MIDI.sendNoteOff(melodyNotes[i], 127, 1);  // Stop the note after the time duration has passed
    }
    else 
    {
      //no more note information is passed if the rhythmArray value is 0 - this indicates a rest in the melody
      delay(noteTime);                            //delays for 1/16th of the tempo duration (16th notes)
    }
  }
}

void loop() {

  Serial.println("*******************");
  Serial.println(buttonMode);

  //the initial state is to generate a new array for a melody
  if (buttonMode == 0)
  {
    //initial startup generates arrays for the melody pattern and rhythm array
    melodyGenerator(0, 9); //eventually a range will be selected to allow for a melody over multiple octaves but for now it sticks within 1 octave
    rhythmGenerator(); //generates a random pattern of 0 and 1 for rhythm on/off
    
    scaleMapper(); //maps scale values to those of the mode
    keyMapper(60); //maps values from mode to a key - for now this is 60, middle C's MIDI value
  }

  
  //when the button is pressed once, the system starts playing a melody
  while (buttonMode == 1)
  {
    playMelody();
  }

  //when the button is pressed again, the system reverts back to the beginning
  if (buttonMode > 1)
  {
    buttonMode = 0;
  }
  


}
