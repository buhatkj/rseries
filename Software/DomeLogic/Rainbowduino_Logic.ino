/*

RAINBOWDUINO CODE FOR LOGIC DISPLAYS : 2013-05-09
by Paul Murphy (joymonkey@gmail.com)

The goal of this sketch is to get a Rainbowduino and 8x8 RGB LED Matrix to
emulate the logic display patterns seen in The Empire Strikes Back.
See the reference scenes here: http://youtu.be/JpyMmx0kMxQ

You'll need the rainbowduino library from here:
http://www.seeedstudio.com/wiki/images/4/43/Rainbowduino_for_Arduino1.0.zip

*/
// ** START OF SETTINGS **
/*
We have 5 Key Colors. For the FLD they're DARK BLUE, MEDIUM BLUE, BRIGHT BLUE, MEDIUM WHITE, BRIGHT WHITE
Each color is an array of 3 integers (Red,Green,Blue brightess 0-255)
*/
#define NUM_KEY_COLORS 5
unsigned int KeyColors[NUM_KEY_COLORS][3] = { {0,0,20} , {0,0,120} , {0,20,255} , {100,120,150} , {200,255,255} };
/*
each LED will loop back and forth between these key colors (not always reaching bright white),
pausing for a random time as each key color is hit.
*/
#define SPEED 2 //speed of color transition (smaller is faster)
#define STEPS 20 //number of 'tween' colors between the KeyColors. increase this for smoother (but slower) transitions
#define TWEENPAUSE 0 //default pause time to use for TWEEN colors
#define KEYPAUSE 100 //default pause time for KeyColors (eventually I'd like this to be an array so each key color can pause for a different time)
//
//#define DEBUG
//
// ** THE AVERAGE R2 BUILDER SHOULDN'T HAVE TO ADJUST SETTINGS UNDER HERE
//
#include <Rainbowduino.h>
#define TOTAL_COLORS ((NUM_KEY_COLORS-1)*STEPS)+NUM_KEY_COLORS //total number of colors including first, last and tweens
unsigned int AllColors[TOTAL_COLORS][3]; // a big array to hold all KeyColors and all Tween colors
unsigned int LEDcolor[64]; // an array holding the current color of each LED
unsigned int LEDdirection[64]; // an array holding the current diection of each LED (0 = going up, 1 = going down)
unsigned int LEDpauseTime[64]; // an array holding the current pause time of each LED (LED should remain current color until this hits 0)
//
void setup() {
  Serial.begin(9600); //used for debugging
  Serial.println();Serial.print(NUM_KEY_COLORS); Serial.print(" KeyColors with "); Serial.print(STEPS); Serial.println(" Tween colorws between each pair.");
  Serial.print("Total colors is ");Serial.print(TOTAL_COLORS);Serial.print(" (0 thru ");Serial.print(TOTAL_COLORS-1);Serial.println(")");
  Rb.init(); //initialize Rainbowduino driver
  randomSeed(analogRead(0)); //helps keep random numbers more random  
  
  //***********************  
  //create a big array with the KeyColors and every tween color in sequence
  int primary,value,kcolor,changePerStep;    
  for(kcolor=0;kcolor<(NUM_KEY_COLORS-1);kcolor++) { 
    int totalColorCount=0;   
    for(primary=0;primary<3;primary++) {
       changePerStep=int(KeyColors[kcolor+1][primary]-KeyColors[kcolor][primary])/(STEPS+1);
       int tranCount=0;
       unsigned int value=KeyColors[kcolor][primary];
       while (tranCount<=(STEPS)) {
         if (tranCount==0) {
           totalColorCount=kcolor*(STEPS+1); //this is the first transition, between these 2 colors, make sure the total count is right
         }
         AllColors[totalColorCount][primary]=value; //set the actual color
         tranCount++;
         totalColorCount++;
         value=int(value+changePerStep);
       }       
    }
  }
  //don't forget the very final colors...
  AllColors[TOTAL_COLORS-1][0]=KeyColors[NUM_KEY_COLORS-1][0];
  AllColors[TOTAL_COLORS-1][1]=KeyColors[NUM_KEY_COLORS-1][1];
  AllColors[TOTAL_COLORS-1][2]=KeyColors[NUM_KEY_COLORS-1][2];
  //done making our big array of colors to play with
  //*********************** 
  
  //lets print all those numbers out so we know things are going right... 
  //might as well display them on the matrix while we're at it...
  for(int color=0;color<TOTAL_COLORS;color++) {
     /* */
     Serial.print("Color ");
     Serial.print(color);
     Serial.print("  red: ");
     Serial.print(AllColors[color][0]);
     Serial.print("  grn: ");
     Serial.print(AllColors[color][1]);
     Serial.print("  blu: ");
     Serial.print(AllColors[color][2]);
     if (color%(STEPS+1)==0) Serial.print(" (KEY)");
     Serial.println();
     /* */
     for(int x=0;x<8;x++) {
       for(int y=0;y<8;y++) {
         Rb.setPixelXY(x,y,AllColors[color][0],AllColors[color][1],AllColors[color][2]);         
       }       
     }
     delay(5);
     //delay(100);
  }
  //ramp the colors back down again...
  for(int color=TOTAL_COLORS-1;color>=0;color--) {
     for(int x=7;x>=0;x--) {
       for(int y=7;y>=0;y--) {
         Rb.setPixelXY(x,y,AllColors[color][0],AllColors[color][1],AllColors[color][2]);         
       }
     }
     delay(5);
  }
  //set each LED to a random color from our big array
  unsigned int LEDnumber=0;
  for(int x=0;x<8;x++) {
     for(int y=0;y<8;y++) {
       LEDcolor[LEDnumber]=random(TOTAL_COLORS); //choose a random color number for this LED and store it in our LEDcolor array
       LEDdirection[LEDnumber]=random(2); //choose a random direction for this LED (0 up or 1 down)
	   /* Line above might need work - what happens if the random color is 0 or 5; then it's direction needs to be 0 or 1 respectively*/
       if (LEDcolor[LEDnumber]%(STEPS+1)==0) {
		LEDpauseTime[LEDnumber]=random(KEYPAUSE); //color is a key, set its pause time for longer than tweens
       }
       else LEDpauseTime[LEDnumber]=random(TWEENPAUSE);
       Rb.setPixelXY(x,y,AllColors[LEDcolor[LEDnumber]][0],AllColors[LEDcolor[LEDnumber]][1],AllColors[LEDcolor[LEDnumber]][2]);
       LEDnumber++;      
     }       
  }
  
}

void loop() {
  //
  unsigned long timeNew=millis();
  RandomLogic(timeNew);
  
}

void updateLED(unsigned int LEDnumber) {
  //this will take an LED number (0-63) and adjust its values in the relevent arrays accordingly
  //check the current color this LED is set to...
  //unsigned int currentColor=LEDcolor[LEDnumber];
  #if defined(DEBUG)
  Serial.print("LED"); Serial.print(LEDnumber);
  #endif
  if (LEDpauseTime[LEDnumber]!=0) {
    #if defined(DEBUG)
    Serial.print("Paused(");Serial.print(LEDnumber);Serial.print(") ");
    #endif
    LEDpauseTime[LEDnumber]=LEDpauseTime[LEDnumber]-1; //reduce the LEDs pause number and check back next loop
  }
  else {
    //LED had 0 pause time, change things around...
    if (LEDdirection[LEDnumber]==0 && LEDcolor[LEDnumber]<(TOTAL_COLORS-1)) {
      #if defined(DEBUG)
      Serial.print(" Moving Up. ");
      #endif
      LEDcolor[LEDnumber]=LEDcolor[LEDnumber]+1; //change it to next color
      if (LEDcolor[LEDnumber]%(STEPS+1)==0) LEDpauseTime[LEDnumber]=random(KEYPAUSE); //color is a key, set its pause time for longer than tweens
      else LEDpauseTime[LEDnumber]=random(TWEENPAUSE);
    }
    else if (LEDdirection[LEDnumber]==0 && LEDcolor[LEDnumber]==(TOTAL_COLORS-1)) {
      #if defined(DEBUG)
      Serial.print(" Flipping Dn. ");
      #endif
      LEDdirection[LEDnumber]=1; //LED is at the final color, leave color but change direction to down
    }
    else if (LEDdirection[LEDnumber]==1 && LEDcolor[LEDnumber]>0) {
      #if defined(DEBUG)
      Serial.print(" Moving Dn. ");
      #endif
      LEDcolor[LEDnumber]=LEDcolor[LEDnumber]-1; //change it to previous color
      if (LEDcolor[LEDnumber]%(STEPS+1)==0) {
        LEDpauseTime[LEDnumber]=random(KEYPAUSE); //new color is a key, set LED's pause time for longer than tweens
      }
      else LEDpauseTime[LEDnumber]=TWEENPAUSE;
    }
    else if (LEDdirection[LEDnumber]==1 && LEDcolor[LEDnumber]==0) {
      #if defined(DEBUG)
      Serial.print(" Flipping Up. ");
      #endif
      LEDdirection[LEDnumber]=0; //LED is at the first color (0), leave color but change direction to up
    }
    else {
      //it's probably not possible to get here, but just incase let's print a debug message
      #if defined(DEBUG)
      Serial.print("No Action Taken!");
      #endif
    }
  }  
}

void RandomLogic(unsigned long elapsed) {
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < SPEED) return; //if not enough time as passed don't proceed any further 
  timeLast = elapsed; 
  //ENOUGH TIME HAS PASSED, MAKE STUFF HAPPEN HERE...  
  unsigned int LEDnumber=0;
  unsigned int prevColorNumber=0;
  for(int x=0;x<8;x++) {
     for(int y=0;y<8;y++) {
       prevColorNumber=LEDcolor[LEDnumber]; //hold the last color number of this LED
       updateLED(LEDnumber); //the updateLED function will change array values for this LED number
       if (LEDcolor[LEDnumber]!=prevColorNumber) {
         //set the LED color if it has changed since the last loop	   
         Rb.setPixelXY(x,y,AllColors[LEDcolor[LEDnumber]][0],AllColors[LEDcolor[LEDnumber]][1],AllColors[LEDcolor[LEDnumber]][2]);
       }
       LEDnumber++; 
     }
  }
  #if defined(DEBUG)
  Serial.println();
  delay(500);
  #endif  
}
