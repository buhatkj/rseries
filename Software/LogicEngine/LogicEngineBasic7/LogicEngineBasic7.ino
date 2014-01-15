//
//  RSeries Logic Engine AVR : Basic Sketch by Paul Murphy
//  ======================================================
//  This sketch provides basic functionality for the RSeries Logic Engine hardware.
//  Does not include microSD reading or I2C communication.
//
//  Uses the FastSPI_LED2 library : https://code.google.com/p/fastspi/downloads/detail?name=FastSPI_LED2.RC5.zip
//
//   revision history...
//   2014-01-15 : Optimizing memory to make way for the SD card's wretched 512 byte buffer.
//                Moved rldMap, fldMap, rldColors & fldColors out of SRAM and into flash memory.
//                Added debug option to report available SRAM.
//                Need to move oColors to flash too, or eliminate it completely (maybe try passing hueVal to upadeLED).
//                Need to write the makeColors function.    
//                Need to reduce LEDstat size (direction can be combined with color number)
//   2014-01-12 : Fixed prototype FLD bug
//   2014-01-08 : Removed debug options to free up some RAM. Fixed rldMap. Fixed fldMap.
//   2014-01-07 : Added 'Testpattern Mode' to aid assembly of the Rear Logic
//

/*

  Dr.Paul explains Colors, LED Direction and (if you're lucky) Midichlorians:
  
  We give the sketch a selection of 'Key' colors. For the Rear Logic we use 5 colors; black, dark green, green, yellow, red.
  The sketch takes those Key colors and generates several 'Tween' colors that fade between each Key color. These colors are
  all put in order in a big array called AllColors. The color elements themselves are HSV values (Hue, Saturation, Value).
  
  During the sketches main loop(), each pixel is cycled through all these colors, so thanks to the tween colors they appear to
  fade from color to color.  As each Key color is reached the pixel is paused for a slightly random number of loops; this
  randomness causes the color patterns to constantly evolve, so over time you will see different patterns emerge.
  
  Say we have 3 key colors and 1 tween color. This creates an AllColors array of 5 unique colors...
    0: Black
    1: Tween
    2: Red
    3: Tween
    4: Yellow
  After the 5th color (Yellow) is reached, we want to switch direction...
    5: Tween (3)
    6: Red   (2)
    7: Tween (1) 
  So even though we only use 5 unique colors, each pixel loops through 8 color changes.
  Sometimes we might refer to Color 6; there is no Color 6 in our AllColors array, but we can figure it out...
  if (colorNum>=numColors) actualColorNum=numColors-(colorNum-numColors)-2;
  using the above example numbers...
  if (6>=5) colorNum=5-(6-5)-2; // so color 6 is really 2 (Red) when looping through the array in reverse
  
  To put it simply, if colorNum >= numColors then the pixel is moving in reverse through the color array.
*/

//  the TOGGLEPIN is used if there's no SD card. if this pin is jumped to +5V,
//  then we'll assume this is the RLD, otherwise we'll assume this is the FLD
#define TOGGLEPIN 4
//  the SPEEDPIN enables speed adjustment via the pots via the pause & delay
#define SPEEDPIN 3

//debug will print a bunch of stuff to serial. useful, but takes up valuable memory and may slow down routines a little
#define DEBUG 1 //0=off, 1=only report available memoty, 2=report lots of stuff, 3=slow the main loop down

#define PROTOFLD //uncomment this line if your FLD boards have only 40 LEDs (final versions have 48 LEDs)

#define LOOPCHECK 50 //number of loops after which we'll check the pot values

#include "FastSPI_LED2.h"
#define NUM_LEDS 96 //absolute max number of LEDs possible (we use numLeds for actual number used later)
#define DATA_PIN 6
CRGB leds[NUM_LEDS];

byte Tweens=6;
byte tweenPause=7; // time to delay each tween color (aka fadeyness)
int keyPause=450; // time to delay each key color (aka pauseyness)
byte maxBrightness=128; // 0-255, no need to ever go over 100

int keyPin = A0; //analog pin to read keyPause value
int tweenPin = A1; //analog pin to read tweenPause value 
int briPin = A2; //analog pin to read Brightness value
int huePin = A3; //analog pin to read Color/Hue shift value

byte totalColors,Keys; //

byte AllColors[64][3]; // a big array to hold all original KeyColors and all Tween colors
byte oColors[64][3]; // will hold a copy of the original colors, useful when shifting hues
byte LEDstat[96][3]; // an array holding the current color number of each LED, its direction and pausetime
boolean speeds=0; //0 for preset, 1 for tweakable (depends on speedpin)
boolean logic=0; //0 for fld, 1 for rld (depends on togglepin)

//our LEDs aren't in numeric order (FLD is wired serpentine and RLD is all over the place!)
//so we can address specific LEDs more easily, we'll map them out here...
#include <avr/pgmspace.h> //to save SRAM, this stuff all gets stored in flash memory
const byte rldMap[]PROGMEM = {
 0, 1, 2, 3, 4, 5, 6, 7,48,49,50,51,52,53,54,55,
15,14,13,12,11,10, 9, 8,63,62,61,60,59,58,57,56,
16,17,18,19,20,21,22,23,64,65,66,67,68,69,70,71,
31,30,29,28,27,26,25,24,79,78,77,76,75,74,73,72,
32,33,34,35,36,37,38,39,80,81,82,83,84,85,86,87,
47,46,45,44,43,42,41,40,95,94,93,92,91,90,89,88};
#if defined PROTOFLD
//mapping for prototype fld boards (with only 40 LEDs)
const byte fldMap[]PROGMEM = { 
 0, 1, 2, 3, 4, 5, 6, 7,
15,14,13,12,11,10, 9, 8,
16,17,18,19,20,21,22,23,
31,30,29,28,27,26,25,24,
32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,
55,54,53,52,51,50,49,48,
56,57,58,59,60,61,62,63,
71,70,69,68,67,66,65,64,
72,73,74,75,76,77,78,79};
#else
//mapping for final fld boards (with 48 LEDs)
const byte fldMap[]PROGMEM = {
15,14,13,12,11,10, 9, 8,
16,17,18,19,20,21,22,23,
31,30,29,28,27,26,25,24,
32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,
55,54,53,52,51,50,49,48,
56,57,58,59,60,61,62,63,
71,70,69,68,67,66,65,64,
72,73,74,75,76,77,78,79,
87,86,85,84,83,82,81,80};
#endif

//default colors also get stored in flash memory...
const byte fldColors[6][3]PROGMEM = { {170,0,0} , {170,255,54} , {170,255,120} , {166,255,200} , {154,84,150} , {174,0,200} };
//const byte rldColors[6][3] = { {87,0,0}  , {87,206,105} , {79,255,214}  , {43,255,250}  , {25,255,214} , {0,255,214} }; 
const byte rldColors[5][3]PROGMEM = { {87,0,0} , {87,206,105} , {79,255,184} , {18,255,250} , {0,255,214} };

void setup() {      
      
      delay(50); // sanity check delay
      randomSeed(analogRead(0)); //helps keep random numbers more random  
      #if defined(DEBUG)
      Serial.begin(9600);         
      #endif      
      
      pinMode(TOGGLEPIN, INPUT);
      //digitalWrite(TOGGLEPIN, HIGH); //used for dipswitch prototype
      logic=digitalRead(TOGGLEPIN);      
      if (logic==1) {
        #if (DEBUG>1)
        Serial.println("RLD");
        #endif
        //logic=1;
        //numLeds=96;
        //slow default speeds down for the RLD... 
        tweenPause=40;
        keyPause=1200;
        Keys=sizeof(rldColors)/3;
        #if (DEBUG>1)
        Serial.println(String(Keys)+" Keys");
        #endif
      }  
      else {
        #if (DEBUG>1)
        Serial.println("FLD");
        #endif
        //numLeds=80; 
        Keys=sizeof(fldColors)/3;        
      }     
  
      totalColors=Keys*Tweens;
      #if (DEBUG>1)
      Serial.println(String(Keys)+" Keys\n"+String(Tweens)+" Tweens\n"+String(totalColors)+" Total");
      #endif
      
      pinMode(SPEEDPIN, INPUT); 
      //digitalWrite(SPEEDPIN, HIGH); //used for dipswitch prototype
      if (digitalRead(SPEEDPIN)==HIGH) {
        speeds=1;  
        #if (DEBUG>1)
        Serial.println("SPD");
        #endif
      }  
      
      //make a giant array of all colors tweened...
      byte el,val,kcol,perStep,totalColorCount; //
      for(kcol=0;kcol<(Keys);kcol++) { //go through each Key color
        for(el=0;el<3;el++) { //loop through H, S and V 
          if (logic==0) perStep=int(pgm_read_byte(&fldColors[kcol+1][el])-pgm_read_byte(&fldColors[kcol][el]))/(Tweens+1);
          else perStep=int(pgm_read_byte(&rldColors[kcol+1][el])-pgm_read_byte(&rldColors[kcol][el]))/(Tweens+1);
          byte tweenCount=0;
          if (logic==0) val=pgm_read_byte(&fldColors[kcol][el]);
          else val=pgm_read_byte(&rldColors[kcol][el]);
          while (tweenCount<=Tweens) {
            if (tweenCount==0) totalColorCount=kcol*(Tweens+1); //this is the first transition, between these 2 colors, make sure the total count is right
            AllColors[totalColorCount][el]=val; //set the actual color element (H, S or V)
            tweenCount++;
            totalColorCount++;
            val=byte(val+perStep);
          }  
        }  
      }
      memcpy(oColors,AllColors,totalColors*3); //copy AllColors to oColors (AllColors can shift, oColors never changes)
      /*#if (DEBUG>1)
      // print all the colors
      for(byte x=0;x<totalColors;x++) {
        Serial.println(String(x)+" : "+String(AllColors[x][0])+","+String(AllColors[x][1])+","+String(AllColors[x][2])+"  o  "+String(oColors[x][0])+","+String(oColors[x][1])+","+String(oColors[x][2]));
      }  
      #endif */ 
      
      FastLED.setBrightness(maxBrightness); //sets the overall brightness to the maximum
      FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
      
      for(byte x=0;x<NUM_LEDS;x++) leds[x] = CRGB::Black; //sets all possible LEDs to black
      FastLED.show();  
      
      //to get the RLD test mode, S4 should have jumper, S3 no jumper, Delay trimmer should be turned completely counter-clockwise
      int delayVal = analogRead(keyPin);      
      if (logic==1 && speeds==0 && delayVal<10) {
        //testpattern for RLD
        byte testColor[96] = {
        1,1,1,1,1,1,1,1,
        2,3,3,3,3,2,2,2,
        4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,
        2,2,2,3,3,3,3,2,
        1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,
        2,2,2,3,3,3,3,2,
        4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,
        2,3,3,3,3,2,2,2,
        1,1,1,1,1,1,1,1};
        //go through all our LEDs, setting them to appropriate colors
        //byte y;
        for(byte x=0;x<96;x++) {
           //if (x>47)  y=x-48;
           //else y=x;
           //y=x;
           if (testColor[x]==1) leds[x] = CRGB::DarkRed;
           if (testColor[x]==2) leds[x] = CRGB::DarkOrange;
           if (testColor[x]==3) leds[x] = CRGB::DarkGreen;
           if (testColor[x]==4) leds[x] = CRGB::DeepSkyBlue;
           if (testColor[x]==5) leds[x] = CRGB::Orchid;
           FastLED.setBrightness(10);  
           FastLED.show();  
           delay(10);        
        }
        delay(999999);
        FastLED.setBrightness(maxBrightness);
      }
      
      //configure each LED's status (color number, direction, pause time)
      for(byte x=0;x<NUM_LEDS;x++) {
        LEDstat[x][0]=byte(random(totalColors)); //choose a random color number to start this LED at
        LEDstat[x][1]=random(2); //choose a random direction for this LED (0 up or 1 down)
        if (LEDstat[x][0]%(Tweens+1)==0) LEDstat[x][2]=random(keyPause); //color is a key, set its pause time for longer than tweens
        else LEDstat[x][2]=random(tweenPause);
      }
      //now set the LEDs to their initial colors...
      for(byte x=0;x<NUM_LEDS;x++) {          
        if (logic==1)  leds[pgm_read_byte(&rldMap[x])].setHSV(AllColors[LEDstat[x][0]][0],AllColors[LEDstat[x][0]][1],AllColors[LEDstat[x][0]][2]); 
        else if (x<80) leds[pgm_read_byte(&fldMap[x])].setHSV(AllColors[LEDstat[x][0]][0],AllColors[LEDstat[x][0]][1],AllColors[LEDstat[x][0]][2]) ; 
        FastLED.show(); 
        delay(10);
        /*if (x>0) leds[x-1].setHSV(0,0,0);
        if (x==NUM_LEDS-1 && logic==1) leds[x].setHSV(0,0,0);*/
      }
  
      //do a startup animation of some sort
      for(byte x=0;x<NUM_LEDS;x++) {
        if (logic==1) {
          //RLD STARTUP
          //leds[pgm_read_byte(&rldMap[x])].setHSV(AllColors[LEDstat[pgm_read_byte(&rldMap[x])][0]][0],AllColors[LEDstat[pgm_read_byte(&rldMap[x])][0]][1],AllColors[LEDstat[pgm_read_byte(&rldMap[x])][0]][2]);
          leds[pgm_read_byte(&rldMap[x])] = CRGB::Green;
          if ((x+1)%24==0) { delay(200); FastLED.show(); }
        }  
        else {
          //FLD STARTUP
          //leds[pgm_read_byte(&fldMap[x])].setHSV(AllColors[LEDstat[pgm_read_byte(&fldMap[x])][0]][0],AllColors[LEDstat[pgm_read_byte(&fldMap[x])][0]][1],AllColors[LEDstat[pgm_read_byte(&fldMap[x])][0]][2]);
          #if defined(PROTOFLD)
          if (pgm_read_byte(&fldMap[x])<80) { //not all LEDs in the array are used for the FLD
          #else
          if (pgm_read_byte(&fldMap[x])>7 && pgm_read_byte(&fldMap[x])<88) { //not all LEDs in the array are used for the FLD
          #endif
            leds[pgm_read_byte(&fldMap[x])] = CRGB::Blue;
            if ((x+1)%8==0) { delay(100); FastLED.show(); } 
          }         
        } 
             
      }  
      
      #if defined(DEBUG)
      Serial.println(memoryFree()); // print the free memory 
      delay(1000); 
      #endif
}

#if defined(DEBUG)
// variables created by the build process when compiling the sketch (used for the memotyFree function)
extern int __bss_end;
extern void *__brkval;
// function to return the amount of free RAM
int memoryFree() {
  int freeValue;
  if((int)__brkval == 0) freeValue = ((int)&freeValue) - ((int)&__bss_end);
  else freeValue = ((int)&freeValue) - ((int)__brkval);
  return freeValue;
}
#endif


void updateLED(byte LEDnum) {
    //this will take an LED number and adjust its status in the LEDstat array
    //check the current color this LED is set to...
    //unsigned int currentColor=LEDstat[LEDnum];  
    if (LEDstat[LEDnum][2]!=0) {
      //LED is paused
      LEDstat[LEDnum][2]=LEDstat[LEDnum][2]-1; //reduce the LEDs pause number and check back next loop
    }
    else {
        //LED had 0 pause time, let's change things around...
        if (LEDstat[LEDnum][1]==0 && LEDstat[LEDnum][0]<(totalColors-1)) {
            LEDstat[LEDnum][0]=LEDstat[LEDnum][0]+1; //change it to next color
            leds[LEDnum].setHSV(AllColors[LEDstat[LEDnum][0]][0],AllColors[LEDstat[LEDnum][0]][1],AllColors[LEDstat[LEDnum][0]][2]);
            if (LEDstat[LEDnum][0]%(Keys+1)==0) LEDstat[LEDnum][2]=random(keyPause); //color is a key, set its pause time for longer than tweens
            else LEDstat[LEDnum][2]=random(tweenPause);
        }
        else if (LEDstat[LEDnum][1]==0 && LEDstat[LEDnum][0]==(totalColors-1)) {
            LEDstat[LEDnum][1]=1; //LED is at the final color, leave color but change direction to down
        }
        else if (LEDstat[LEDnum][1]==1 && LEDstat[LEDnum][0]>0) {
            LEDstat[LEDnum][0]=LEDstat[LEDnum][0]-1; //change it to previous color
            leds[LEDnum].setHSV(AllColors[LEDstat[LEDnum][0]][0],AllColors[LEDstat[LEDnum][0]][1],AllColors[LEDstat[LEDnum][0]][2]);
            if (LEDstat[LEDnum][0]%(Keys+1)==0) {
              LEDstat[LEDnum][2]=random(keyPause); //new color is a key, set LED's pause time for longer than tweens
            }
            else LEDstat[LEDnum][2]=tweenPause;
        }
        else if (LEDstat[LEDnum][1]==1 && LEDstat[LEDnum][0]==0) {
            LEDstat[LEDnum][1]=0; //LED is at the first color (0), leave color but change direction to up
        }
    }   
}


unsigned int loopCount;
byte briVal,prevBri,briDiff,hueVal,prevHue,hueDiff;
void loop() {
  
    if (loopCount==LOOPCHECK) { //only check this stuff every 100 or so loops
              
       if (speeds==1) {
         /*#if (DEBUG>1)
         Serial.println("checking key and tween pots\n"); 
         #endif*/
         keyPause = analogRead(keyPin);  
         tweenPause = round(analogRead(tweenPin)/10);
       }
       

       // LET THE USER SHIFT THE HUES OF ALL COLORS...
       hueVal = round(analogRead(huePin)/4); 
       /*#if (DEBUG>1)
       Serial.println("hueVal is "+String(hueVal)+"\n"); 
       #endif*/
       hueDiff=(max(hueVal,prevHue)-min(hueVal,prevHue)); 
       if (hueDiff>=2) {
          #if defined(DEBUG)
          Serial.println(memoryFree()); // print the free memory 
          delay(1000); 
          #endif
           /*#if (DEBUG>1)
           Serial.println("hueDiff "+String(hueDiff));  
           //Serial.println("Adjusting TweakedColors...\n");
           #endif*/      
           for(int color=0;color<totalColors;color++) {     
               //go through all the colors in the colors array and give them new Hue values (based on the original color + )  
               AllColors[color][0]=oColors[color][0]+hueVal;
               if (AllColors[color][0]>=255) AllColors[color][0]=AllColors[color][0]-255;    
           }       
       }
       prevHue=hueVal;
       prevBri=briVal;         

       
       // LET THE USER ADJUST GLOBAL BRIGHTNESS... 
       briVal = (round(analogRead(briPin)/4)*maxBrightness)/255; //the pot will give a value between 0 and 1024, we need to divide this down to something between 0 and our maxBrightness
       /*#if (DEBUG>1)
       Serial.println("briVal "+String(briVal)); 
       delay(200);
       #endif*/
       if (briVal!=prevBri) {
         briDiff=(max(briVal,prevBri)-min(briVal,prevBri)); 
         if (briDiff>=2) {
             /*#if (DEBUG>1)
             Serial.println("bri "+String(briVal)); 
             //delay(100);
             #endif*/
             FastLED.setBrightness(briVal); //sets the overall brightness
         }
         prevBri=briVal;
       } 
       loopCount=0;       
    }
    
    #if (DEBUG>1)
    if (loopCount%LOOPCHECK==0) {
      Serial.println(String(loopCount));
      Serial.println(memoryFree()); // print the free memory  
    }  
    #endif
    loopCount++;    
    
    //go through each LED and update it 
    for(byte LEDnum=0;LEDnum<NUM_LEDS;LEDnum++) {
      if (logic==1) updateLED(pgm_read_byte(&rldMap[LEDnum]));
      #if defined(PROTOFLD)
      else if (LEDnum<80) updateLED(pgm_read_byte(&fldMap[LEDnum])); 
      #else
      else if (pgm_read_byte(&fldMap[LEDnum])>7 && pgm_read_byte(&fldMap[LEDnum])<88) updateLED(pgm_read_byte(&fldMap[LEDnum])); 
      #endif
    }  
    FastLED.show();
    
    #if (DEBUG>2)
    delay(10); //slow things down to a crawl
    #endif
}


