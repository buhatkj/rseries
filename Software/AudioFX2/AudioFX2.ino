/*
RSeries Receiver I2C Audio FX2 Module Slave for rMP3 
V009  6-Aug-2012
 Creative Copyright v3 SA BY - 2012 Michael Erwin 
                               michael(dot)erwin(at)mac(dot)com 

RSeries Open Controller Project  http://code.google.com/p/rseries-open-control/
Requires Arduino 1.0 IDE

To use this sketch, you need the following

rMP3 Rogue Robotics MP3 & SD Support
SD Card with SOUNDS 
Arduino IDE 1.0 Compatible with edited RogueMP3 & RogueSD libraries


*/

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  #else
   #include "WProgram.h"
#endif

#include <Wire.h>

#include <SoftwareSerial.h>   // this is to fix Arduino 0022 Core Stream
#include <RogueSD.h>         // Rogue Robotics SD File Support, which is on the rMP3
#include <RogueMP3.h>        // Rogue Robotics rMP3

#undef round
int x;

long loopCount = 0;
long twitchTime = 0;



//  rMP3 Serial
SoftwareSerial rmp3_serial(6, 7);
RogueMP3 rmp3(rmp3_serial);
RogueSD filecommands(rmp3_serial);

// variables for Sounds on SD
int numberOfSounds;
int lastSound;
char path[96];
char soundFile[96];
int soundNum;
char soundNum_str[3]; // supports up to 99 files in SOUND/SSSSS/ folders
int randomSoundPlay; // randomness of SoundPlay... not what you think

const char *directory = "/FX1";

//Setup Sound Folder Strings
const char ALARM[20] = "/FX1/ALARM/ALARM"; 
int ALARM_FN = 11; // Number of files in folder

const char HAPPY[25] = "/FX1/HAPPY/HAPPY";
int HAPPY_FN = 25; // Number of files in folder 

const char HUM[20] = "/FX1/HUM/HUM";
int HUM_FN = 25; // Number of files in folder 

const char MISC[20] = "/FX1/MISC/MISC";
int MISC_FN = 36; // Number of files in folder 

const char OOH[20] = "/FX1/OOH/OOH";
int OOH_FN = 7; // Number of files in folder 

String OTHER = "/FX1/OTHER/";

const char PROC[20] = "/FX1/PROC/PROC";
int PROC_FN = 15; // Number of files in folder 

const char RAZZ[20] = "/FX1/RAZZ/RAZZ";
int RAZZ_FN = 23; // Number of files in folder 

const char SCREAM[25] = "/FX1/SCREAM/SCREAM";
int SCREAM_FN = 4; // Number of files in folder 

const char SENT[25] = "/FX1/SENT/SENT";
int SENT_FN = 20; // Number of files in folder 


const char  WHIST[20] = "/FX1/WHIST/WHIST";
int WHIST_FN = 25; // Number of files in folder 


const char  AHSOKA[25] = "/PERSONS/AHSOKA/AHSOKA";
int AHSOKA_FN = 4; // Number of files in folder 


const char  ANAKIN[25] = "/PERSONS/ANAKIN/ANAKIN";
int ANAKIN_FN = 3; // Number of files in folder 

const char  C3PO[20] = "/PERSONS/C3PO/C3PO";
int C3PO_FN = 15; // Number of files in folder 


const char  CHEWIE[25] = "/PERSONS/CHEWIE/CHEWIE";
int CHEWIE_FN = 7; // Number of files in folder 

const char  HAN[20] = "/PERSONS/HAN/HAN";
int HAN_FN = 4; // Number of files in folder 

const char  JABBA[25] = "/PERSONS/JABBA/JABBA";
int JABBA_FN = 6; // Number of files in folder 

const char  JAWA[20] = "/PERSONS/JAWA/JAWA";
int JAWA_FN = 6; // Number of files in folder 

const char  LEIA[20] = "/PERSONS/LEIA/LEIA";
int LEIA_FN = 7; // Number of files in folder 

const char  LUKE[20] = "/PERSONS/LUKE/LUKE";
int LUKE_FN = 3; // Number of files in folder 

const char  OBIWAN[25] = "/PERSONS/OBIWAN/OBIWAN";
int OBIWAN_FN = 9; // Number of files in folder 

const char  VADER[25] = "/PERSONS/VADER/VADER";
int VADER_FN = 4; // Number of files in folder 

const char  YODA[20] = "/PERSONS/YODA/YODA";
int YODA_FN = 3; // Number of files in folder 

const char  SERVO[20] = "/FX2/SERVO/SERVO";
int SERVO_FN = 19; // Number of files in folder 

const char  SERVOL[20] = "/FX2/SERVOL/SERVOL";
int SERVOL_FN = 7; // Number of files in folder 

const char  RACE[20] = "/FX2/RACE/RACE";
int RACE_FN = 7; // Number of files in folder 


const char  SOUNDTRACK[25] = "/SOUNDTRACKS/SOUNDTRACK";
int SOUNDTRACK_FN = 7; // Number of files in folder 


const char  STARTOURS[30] = "/STARTOURS/STARTOURS";
int STARTOURS_FN = 4; // Number of files in folder 


void setup() {
// Setup rMP3 Serial
//  Serial.begin(9600); // DEBUG CODE
  rmp3_serial.begin(9600);
  rmp3.sync();
  rmp3.stop();
  
// Setup rSD access
  filecommands.sync();
  
  delay(1000);
  
// play a initializing start sound
  rmp3.playfile("/FX2/DOME2.mp3");

 delay(1000);


// mix up our random number generator
randomSeed(analogRead(0));

twitchTime = 1300;   // Initial time 

  
  Wire.begin(5);                // join i2c bus with address #5
  Wire.onReceive(receiveEvent); // register event


}

/*
                     "Alarm Sequenc",         // Item 1
                     "Leia HP Mesg",          // Item 2
                     "Sys Failure",           // Item 3
                     "Happy SFX",             // Item 4
                     "Rnd Whistle",           // Item 5
                     "Razzberry",             // Item 6
                     "Angry SFX",             // Item 7
                     "Fire Extngshr",         // Item 8 DisplayGroup = 2
                     "Dome Wave",             // Item 9 
                     "Wolf Whistle",          // Item 10
                     "Dance Cantina",         // Item 11
                     "VADER!",                // Don't use 3 exclamation symbols ! with a MEGA 2560
                     "C3PO",                  // Item 13   
                     "Yoda",                  // Item 14 
                     "Luke",                  // Item 15 DisplayGroup = 3 
                     "Leia",                  // Item 16 
                     "Han Solo",              // Item 17 -- Requires 104 workaround
                     "Obi Wan",               // Item 18
                     "Chewbacca",             // Item 19 -- requires 105 workaround
                     "Jawas",                 // Item 20
                     "Anakin",                // Item 21
                     "Jabba's Place",         // Item 22 DisplayGroup = 4
                     "Ahsoka",
                     "Launch Saber",
                     "Hello of RLD",
                     "Item 26",
                     "Item 27",
                     "Item 28",
                     "Item 29",
                     "Item 30", //Item 30
*/

void loop()
{
  loopCount++;
  if (x == 200) {rmp3.playfile("/FX2/DOME5.mp3"); x=0; loopCount = 0;}     // DOME Rotation LEFT Start
  if (x == 201) {rmp3.playfile("/FX2/DOME2.mp3"); delay(500); rmp3.playfile("/FX2/DOME5.mp3"); x=0; loopCount = 0;}// DOME // DOME Rotation LEFT Start
  if (x == 202) {rmp3.playfile("/FX2/DOME6.mp3"); x=0;loopCount = 0;}      // DOME Rotation End
  if (x == 203) {rmp3.playfile("/FX2/CPUARM1.mp3"); x=0;loopCount = 0;}    // Activation CPU ARM
  if (x == 204) {rmp3.playfile("/FX2/HOLOPROJ1.mp3"); x=0; loopCount = 0;} // HOLO PROJECT Start & Continue
  if (x == 205) {rmp3.playfile("/FX2/HOLOPROJ2.mp3"); x=0; loopCount = 0;} // HOLO PROJECT end

  if (x == 239) {playSERVO(); x=0; loopCount = 0;} // Random Servo Sound

  if (x == 240) {rmp3.playfile("/SOUNDTRACKS/SOUNDTRACK1.mp3"); x=0; loopCount = 0;} // 20th Century Fox Fanfare
  if (x == 241) {playSOUNDTRACK(); x=0; loopCount = 0;} // HOLO PROJECT Start & Continue
  if (x == 242) {rmp3.playfile("/STARTOURS/STARTOURS1.mp3"); x=0; loopCount = 0;} // HOLO PROJECT Start & Continue
  if (x == 243) {rmp3.playfile("/FX2/HOLOPROJ2.mp3"); x=0; loopCount = 0;} // HOLO PROJECT Start & Continue
  if (x == 244) {rmp3.playfile("/FX2/HOLOPROJ2.mp3"); x=0; loopCount = 0;} // HOLO PROJECT Start & Continue



//DroidRACEmode=true from controller then...
  if (x == 250) {rmp3.playfile("/FX2/RACE/RACE1.mp3"); x=0; loopCount = 0;} // RACE Start & Idle
  if (x == 251) {rmp3.playfile("/FX2/RACE/RACE2.mp3"); x=0; loopCount = 0;} // RACE Run & Shifting
  if (x == 252) {rmp3.playfile("/FX2/RACE/RACE3.mp3"); x=0; loopCount = 0;} // RACE Skid #1
  if (x == 253) {rmp3.playfile("/FX2/RACE/RACE4.mp3"); x=0; loopCount = 0;} // RACE Skid #2
  if (x == 254) {rmp3.playfile("/FX2/RACE/RACE5.mp3"); x=0; loopCount = 0;} // RACE Skid #2
 

//  
//  Serial.println(x);         // print the integer DEBUG CODE
//

  if (loopCount > twitchTime)
  {
     playSERVO();                            // call playPROC routine
     loopCount = 0;                         // reset loopCount
     twitchTime = random (25,70) * 1000;    // set the next twitchtime
  }
  delay(5);
}

// receiveEvent function executes whenever data is received from master
// this function is registered as an event, see setup()

void receiveEvent(int howMany) {
    x = Wire.read();    // receive byte as an integer
//    Serial.println(x);         // print the integer - DEBUG CODE
}


void playRandom()                        // Randomly play random sound from sound folders... create a personality & attitude
{
     randomSoundPlay = random (1,7);    // Pick a number 1 - 7
     switch (randomSoundPlay) {
      case 1:
       //do something when randomSoundPlay equals 1
       playHUM();
       break;
      case 2:
       playMISC();
       break;
      case 3:
       playOOH();
       break;
      case 4:
       playPROC();
       break;
      case 5:
       playRAZZ();
       break;
      case 6:
       playSENT();
       break;
      case 7:
       playWHIST();
       break;
      case 8:
      
       break;
      case 9:
      
       break;
      case 10:
      
       break;
      case 11:
      
       break;
      case 12:
      
       break;
      
      default: 
       // if nothing else matches, do the default
       // so we are going to do nothing... for that matter not even waste time
       break;
      }
}

void playALARM()                        // Randomly play random ALARM sound from sound folders... R2 is not happy
{
       soundNum = random (1, ALARM_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, ALARM);              // copy ALARM char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playHUM()
{
  soundNum = random (1, HUM_FN);    // generate a random # from 1 to the number of files declared in that folder
  strcpy (path, HUM);               // copy HUM char variable, to path
  itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
  strcat (path, soundNum_str);      // add sound_str to path
  strcat (path, ".mp3");            // add .mp3 to path
  //Serial.println(path);           // used to debug
  rmp3.playfile(path);
}      

void playMISC()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, MISC_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, MISC);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playOOH()
{
  soundNum = random (1, OOH_FN);    // generate a random # from 1 to the number of files declared in that folder
  strcpy (path, OOH);               // copy OOH char variable, to path
  itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
  strcat (path, soundNum_str);      // add sound_str to path
  strcat (path, ".mp3");            // add .mp3 to path
  //Serial.println(path);           // used to debug
  rmp3.playfile(path);
}

void playPROC()
{
  soundNum = random (1, PROC_FN);   // generate a random # from 1 to the number of files declared in that folder
  strcpy (path, PROC);              // copy PROC char variable, to path
  itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
  strcat (path, soundNum_str);      // add sound_str to path
  strcat (path, ".mp3");            // add .mp3 to path
  //Serial.println(path);           // used to debug
  rmp3.playfile(path);
}

void playRAZZ()
{
  soundNum = random (1, RAZZ_FN);   // generate a random # from 1 to the number of files declared in that folder
  strcpy (path, RAZZ);              // copy RAZZ char variable, to path
  itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
  strcat (path, soundNum_str);      // add sound_str to path
  strcat (path, ".mp3");            // add .mp3 to path
  //Serial.println(path);           // used to debug
  rmp3.playfile(path);
}  

void playSENT()
{
  soundNum = random (1, SENT_FN);   // generate a random # from 1 to the number of files declared in that folder
  strcpy (path, SENT);              // copy SENT char variable, to path
  itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
  strcat (path, soundNum_str);      // add sound_str to path
  strcat (path, ".mp3");            // add .mp3 to path
  //Serial.println(path);           // used to debug
  rmp3.playfile(path);
}

void playSCREAM()
{
  soundNum = random (1, SCREAM_FN);   // generate a random # from 1 to the number of files declared in that folder
  strcpy (path, SCREAM);              // copy SENT char variable, to path
  itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
  strcat (path, soundNum_str);      // add sound_str to path
  strcat (path, ".mp3");            // add .mp3 to path
  //Serial.println(path);           // used to debug
  rmp3.playfile(path);
}

void playWHIST()
{
  soundNum = random (1, WHIST_FN);  // generate a random # from 1 to the number of files declared in that folder
  strcpy (path, WHIST);             // copy WHIST char variable, to path
  itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
  strcat (path, soundNum_str);      // add sound_str to path
  strcat (path, ".mp3");            // add .mp3 to path
  //Serial.println(path);            // used to debug
  rmp3.playfile(path);
}


void playAHSOKA()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, AHSOKA_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, AHSOKA);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playANAKIN()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, ANAKIN_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, ANAKIN);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playC3PO()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, C3PO_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, C3PO);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playCHEWIE()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, CHEWIE_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, CHEWIE);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playHAN()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, HAN_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, HAN);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playJABBA()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, JABBA_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, JABBA);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playJAWA()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, JAWA_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, JAWA);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playLEIA()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, LEIA_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, LEIA);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playLUKE()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, LUKE_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, LUKE);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playOBIWAN()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, OBIWAN_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, OBIWAN);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playVADER()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, VADER_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, VADER);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playYODA()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, YODA_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, YODA);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playSERVO()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, SERVO_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, SERVO);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playSERVOL()                        // Randomly play random MISC sound from sound folders... create a little chatter.
{
       soundNum = random (1, SERVOL_FN);   // generate a random # from 1 to the number of files declared in that folder
       strcpy (path, SERVOL);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playSOUNDTRACK()                        // Randomly play random SOUNDTRACK sound from sound folders... create a little chatter.
{
       soundNum = random (2, SOUNDTRACK_FN);   // generate a random # from 2 to the number of files declared in that folder
       strcpy (path, SOUNDTRACK);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}

void playSTARTOURS()                        // Randomly play random SOUNDTRACK sound from sound folders... create a little chatter.
{
       soundNum = random (2, STARTOURS_FN);   // generate a random # from 2 to the number of files declared in that folder
       strcpy (path, STARTOURS);              // copy MISC char variable, to path
       itoa(soundNum, soundNum_str, 10); // convert the random number integer to string and store it in sound_str
       strcat (path, soundNum_str);      // add sound_str to path
       strcat (path, ".mp3");            // add .mp3 to path
       //Serial.println(path);           // used to debug
       rmp3.playfile(path);
}
