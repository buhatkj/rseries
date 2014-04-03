/*
RSeries Receiver I2C Audio FX1 Module Slave for Adafruit WAV Shield
v001 - 4-NOV-2013
Creative Copyright v3 SA BY - 2012 Ted Koenig 
                               buhatkj@r2atl.org

RSeries Open Controller Project  http://code.google.com/p/rseries-open-control/
Requires Arduino 1.0 IDE

To use this sketch, you need the following

ADAFRUIT WAV SHIELD
SD Card with SOUNDS 
Arduino IDE 1.0 Compatible versions of Jukebox2, WaveHC(Edited version) and WaveUtil libraries 

*/

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  #else
   #include "WProgram.h"
#endif

#include <Wire.h>
#include "WaveHC.h"
#include "WaveUtil.h"
#include <Jukebox2.h>

uint8_t x; // WIRE command var
Jukebox2 snd = Jukebox2();
unsigned long nextTwitch = 0;
const unsigned int twitchMin = 20000; //max 65,535 ms
const unsigned int twitchMax = 60000; //max 65,535 ms
char path[16]; // This means, that including the ".wav" the filenames must be 16 chars or less

uint8_t randSoundSelection[18] = {1,3,4,5,6,7,8,9,32,33,34,35,36,37,38,39,40,42};

typedef struct 
{
  uint8_t commandCode; // What command code from the WIRE lib activates this sound
  char *fileName;  // the filename, minus ".wav" and any trailing numerals
  uint8_t fileCount;   // how many files of this type there are, if this is unique, set to 0
} SoundType;


// Define all the sounds now
const uint8_t SOUND_COUNT = 35;  //update this size to match how many you have
SoundType allSounds[SOUND_COUNT];  //update this size to match how many you have
void setupSounds()
{
  // can I get these out of RAM?
  uint8_t commandIndex = 0;
  allSounds[commandIndex].commandCode = 1;   allSounds[commandIndex].fileName = "ALARM";    allSounds[commandIndex].fileCount = 11; commandIndex+=1;
  allSounds[commandIndex].commandCode = 2;   allSounds[commandIndex].fileName = "LEIA";     allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 3;   allSounds[commandIndex].fileName = "FAILURE";  allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 4;   allSounds[commandIndex].fileName = "CHORTLE";  allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 5;   allSounds[commandIndex].fileName = "WHIST";    allSounds[commandIndex].fileCount = 25; commandIndex+=1;
  allSounds[commandIndex].commandCode = 6;   allSounds[commandIndex].fileName = "RAZZ";     allSounds[commandIndex].fileCount = 23; commandIndex+=1;
  allSounds[commandIndex].commandCode = 7;   allSounds[commandIndex].fileName = "ANNOYED";  allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 8;   allSounds[commandIndex].fileName = "ALARM";    allSounds[commandIndex].fileCount = 11; commandIndex+=1;
  allSounds[commandIndex].commandCode = 9;   allSounds[commandIndex].fileName = "MISC";     allSounds[commandIndex].fileCount = 36; commandIndex+=1;
  allSounds[commandIndex].commandCode = 10;  allSounds[commandIndex].fileName = "WOLFWSTL"; allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 11;  allSounds[commandIndex].fileName = "CANTINA";  allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 12;  allSounds[commandIndex].fileName = "VADER";    allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 13;  allSounds[commandIndex].fileName = "C3PO";     allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 14;  allSounds[commandIndex].fileName = "YODA";     allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 15;  allSounds[commandIndex].fileName = "LUKE";     allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 16;  allSounds[commandIndex].fileName = "LEIA";     allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 104; allSounds[commandIndex].fileName = "HAN";      allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 18;  allSounds[commandIndex].fileName = "OBIWAN";   allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 105; allSounds[commandIndex].fileName = "CHEWIE";   allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 20;  allSounds[commandIndex].fileName = "JAWA";     allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 21;  allSounds[commandIndex].fileName = "ANAKIN";   allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 22;  allSounds[commandIndex].fileName = "JABBA";    allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 23;  allSounds[commandIndex].fileName = "AHSOKA";   allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 102; allSounds[commandIndex].fileName = "WHIST";    allSounds[commandIndex].fileCount = 25; commandIndex+=1;
  // In between these command codes, on the original controller sketch there was a bunch of body commands.  leaving space here for compatibility...
  allSounds[commandIndex].commandCode = 32;  allSounds[commandIndex].fileName = "PROC";     allSounds[commandIndex].fileCount = 15; commandIndex+=1;
  allSounds[commandIndex].commandCode = 33;  allSounds[commandIndex].fileName = "SENT";     allSounds[commandIndex].fileCount = 20; commandIndex+=1;
  allSounds[commandIndex].commandCode = 34;  allSounds[commandIndex].fileName = "SCREAM";   allSounds[commandIndex].fileCount = 4; commandIndex+=1;
  allSounds[commandIndex].commandCode = 35;  allSounds[commandIndex].fileName = "OOH";      allSounds[commandIndex].fileCount = 7; commandIndex+=1;
  allSounds[commandIndex].commandCode = 36;  allSounds[commandIndex].fileName = "HUM";      allSounds[commandIndex].fileCount = 25; commandIndex+=1;
  allSounds[commandIndex].commandCode = 37;  allSounds[commandIndex].fileName = "GROAN";    allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 38;  allSounds[commandIndex].fileName = "DOODOO";   allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 39;  allSounds[commandIndex].fileName = "QUESTION"; allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 40;  allSounds[commandIndex].fileName = "SHORTCKT"; allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 41;  allSounds[commandIndex].fileName = "WOWIE";    allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  allSounds[commandIndex].commandCode = 42;  allSounds[commandIndex].fileName = "STARTSND"; allSounds[commandIndex].fileCount = 0; commandIndex+=1;
  Serial.println(commandIndex); 
}


void setup() 
{                
  Serial.begin(9600); 
  // mix up our random number generator
  randomSeed(analogRead(0)); 
  Serial.println("init jukebox...");
  snd.begin();
  nextTwitch = (millis() + random(twitchMin, twitchMax));
  Serial.println("init WIRE...");
  Wire.begin(4);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Serial.println("init sound commands...");
  setupSounds(); 
  Serial.println("DONE setup...");
  putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(FreeRam());
  x = 42;// Play the start sound
}

void loop() 
{
  // Play a random sound if we have been sitting a while
  if(millis() >= nextTwitch)
  {
    Serial.println("Play random sound...");
    x = 32; // Play a PROC sound
    Serial.println("DONE Play random sound...");
    putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
    Serial.println(FreeRam());
  }  
  playSoundCommand();
  delay(100);
}

// receiveEvent function executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
   x = Wire.read();    // receive byte as an integer
   Serial.print("Received = "); 
   Serial.println(x);
}


void waitForSound()
{
  // Allow sounds to play to completion
  while (snd.wave.isplaying) 
  {
    x = 0;
    delay(100);
  }
}


uint8_t selectRandomSound()
{
  return randSoundSelection[random(0,17)];
}


void playSoundCommand()
{
  for(uint8_t i=0; i<SOUND_COUNT; i++)
  {
    if(allSounds[i].commandCode == x)
    {
      x = 0;
      strcpy (path, allSounds[i].fileName);               // copy filename char variable, to path
      if(allSounds[i].fileCount > 0)
      {
        char soundNum_str[3]; // supports up to 99 files in FX1/SSSSS/ folders
        uint8_t soundNum = random (1, allSounds[i].fileCount);    // generate a random # from 1 to the number of files declared in that folder
        itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
        strcat (path, soundNum_str);      // add sound_str to path
      }
      strcat (path, ".WAV");            // add .WAV to path
      Serial.println(path);           // used to debug
      snd.PlaySound(String(path));
      putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
      Serial.println(FreeRam());
      waitForSound();
      x = 0;
      nextTwitch = (millis() + random(twitchMin, twitchMax));
    }
  }
}

