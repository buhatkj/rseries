//
//  RSeries Logic Engine AVR : Basic Sketch by Paul Murphy
//  ======================================================
//  This sketch provides basic functionality for the RSeries Logic Engine hardware.
//  Does not include microSD reading or I2C communication.
//
//  Uses the FastSPI_LED2 library : https://code.google.com/p/fastspi/downloads/detail?name=FastSPI_LED2.RC5.zip
//
//   revision history...
//   2014-01-22 : Further ensmallening.
//   2014-01-15 : Optimizing memory to make way for the SD card's wretched 512 byte buffer.
//                Moved rldMap, fldMap, rldColors & fldColors out of SRAM and into flash memory.
//                Eliminated oColors array (adjusted hues are now calculated individually by updateLED instead of all at once).
//                Added debug option to report available SRAM.
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
#define DEBUG 1 //0=off, 1=only report available memory, 2=report lots of stuff, 3=slow the main loop down

#define PROTOFLD //uncomment this line if your FLD boards have only 40 LEDs (final versions have 48 LEDs)

#define LOOPCHECK 5 //number of loops after which we'll check the pot values and reduce pause countdowns

//                                                                                           Variable Sizes in SRAM:
#include "FastSPI_LED2.h"
#define DATA_PIN 6
CRGB leds[96];      // structure used by the FastSPI_LED library                                    288 bytes

byte Tweens=6;     // number of tween colors to generate                                              1 byte
byte tweenPause=7; // time to delay each tween color (aka fadeyness)                                  1 byte
int keyPause=350; // time to delay each key color (aka pauseyness)                                    2 bytes
byte maxBrightness=255; // 0-255, no need to ever go over 128                                         1 byte

byte totalColors,Keys; //                                                                             2 bytes

byte AllColors[45][3]; // a big array to hold all original KeyColors and all Tween colors           135 bytes
byte LEDstat[96]; // an array holding the current color number of each LED                           96 bytes
byte LEDpause[96]; // holds pausetime for each LED (how many loops before changing colors)           96 bytes
boolean speeds=0; //0 for preset, 1 for tweakable (depends on speedpin)                               1 byte
boolean logic=0; //0 for fld, 1 for rld (depends on togglepin)                                        1 byte

unsigned int loopCount; // used during main loop                                                      2 bytes
byte briVal,prevBri,briDiff,hueVal; // global variables used during main loop and functions           4 bytes
#if (DEBUG>1)
unsigned long time; //used to calculate loops-per-second in debug mode                                4 bytes (in debug only)
#endif

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

//throw the RLD testpattern into flash RAM too...
const byte testColor[96]PROGMEM = {
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

//default colors also get stored in flash memory...
const byte fldColors[6][3]PROGMEM = { {170,0,0} , {170,255,54} , {170,255,120} , {166,255,200} , {154,84,150} , {174,0,200} };
const byte rldColors[5][3]PROGMEM = { {87,0,0} , {87,206,105} , {79,255,184} , {18,255,250} , {0,255,214} };

//pins used by the Delay/Fade/Bright/Color trimmers...
#define keyPin A0 //analog pin to read keyPause value
#define tweenPin A1 //analog pin to read tweenPause value
#define briPin A2 //analog pin to read Brightness value
#define huePin A3 //analog pin to read Color/Hue shift value

//experimental microSD stuff
/*#include <SD.h>
const int chipSelect = 10; //digital pin used for microSD
File myFile;*/

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
      /*#if (DEBUG>1)
      // print all the colors
      for(byte x=0;x<totalColors;x++) {
        Serial.println(String(x)+" : "+String(AllColors[x][0])+","+String(AllColors[x][1])+","+String(AllColors[x][2])+"  o  "+String(oColors[x][0])+","+String(oColors[x][1])+","+String(oColors[x][2]));
      }  
      #endif */ 
      
      FastLED.setBrightness(maxBrightness); //sets the overall brightness to the maximum
      FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, 96);
      
      for(byte x=0;x<96;x++) leds[x] = CRGB::Black; //sets all possible LEDs to black
      FastLED.show();  
      
      //to get the RLD test mode, S4 should have jumper, S3 no jumper, Delay trimmer should be turned completely counter-clockwise
      int delayVal = analogRead(keyPin);      
      if (logic==1 && speeds==0 && delayVal<10) {
        //testpattern for RLD (now moved to flash RAM)
        /*byte testColor[96] = {
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
        1,1,1,1,1,1,1,1};*/
        //go through all our LEDs, setting them to appropriate colors
        for(byte x=0;x<96;x++) {
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
      for(byte x=0;x<96;x++) {
        LEDstat[x]=byte(random((totalColors*2)-2)); //choose a random color number to start this LED at
        // ** CHECK THIS NEXT LINE
        if (LEDstat[x]%(Tweens+1)==0) LEDpause[x]=random(keyPause); //color is a key, set its pause time for longer than tweens
        else LEDpause[x]=random(tweenPause);
      }
      //now set the LEDs to their initial colors...
      for(byte x=0;x<96;x++) {          
        if (logic==1)  leds[pgm_read_byte(&rldMap[x])].setHSV(AllColors[LEDstat[x]][0],AllColors[LEDstat[x]][1],AllColors[LEDstat[x]][2]); 
        else if (x<80) leds[pgm_read_byte(&fldMap[x])].setHSV(AllColors[LEDstat[x]][0],AllColors[LEDstat[x]][1],AllColors[LEDstat[x]][2]) ; 
        FastLED.show(); 
        delay(10);
      }
  
      //do a startup animation of some sort
      for(byte x=0;x<96;x++) {
        if (logic==1) {
          //RLD STARTUP
          leds[pgm_read_byte(&rldMap[x])] = CRGB::Green;
          if ((x+1)%24==0) { delay(200); FastLED.show(); }
        }  
        else {
          //FLD STARTUP
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
// variables created by the build process when compiling the sketch (used for the memoryFree function)
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

//get a valid color number (allows colorNum to go over totalColors)
byte realColorNum(byte colorNum, byte totalColors) {
  if (colorNum<totalColors) return colorNum;
  else {
    //color number given is greater than totalColors, so we must be moving backwards through the colors, figure out where we're at...
    return(totalColors-(colorNum-totalColors)-2);
  }  
}  

//figure out the next color number given the current color and total colors...
byte nextColorNum(byte colorNum, byte totalColors) {
   if ((colorNum+1)<((totalColors*2)-2)) return(colorNum+1);
   else return(0);
}

void updateLED(byte LEDnum, byte hueVal) {
    //this will take an LED number and adjust its status in the LEDstat array
    //check the current color this LED is set to...
    //unsigned int currentColor=LEDstat[LEDnum];  
    /*if (LEDpause[LEDnum]!=0) {
      //LED is paused
      LEDpause[LEDnum]=LEDpause[LEDnum]-1; //reduce the LEDs pause number and check back next loop
    }*/
    //else {
        //LED had 0 pause time, let's change things around...
        //change it to next color...
        LEDstat[LEDnum]=nextColorNum(LEDstat[LEDnum],totalColors); 
        byte colorNum=realColorNum(LEDstat[LEDnum],totalColors);
        leds[LEDnum].setHSV(AllColors[colorNum][0]+hueVal,AllColors[colorNum][1],AllColors[colorNum][2]);
        //give it a new pause value...
        if (LEDstat[LEDnum]%(Keys+1)==0) LEDpause[LEDnum]=random(keyPause); //color is a key, set its pause time for longer than tweens  ** DOUBLE CHECK THIS MATH
        else LEDpause[LEDnum]=random(tweenPause);
    //}   
}


void loop() {
  
    /*#if (DEBUG>1)
    if (loopCount==1) time=millis();
    #endif*/
  
    if (loopCount==LOOPCHECK) {
       
       #if (DEBUG>0) 
       Serial.println(String(memoryFree())); // print the free memory  
       #endif
       #if (DEBUG>1)
       time=(micros()-time)/(LOOPCHECK-1);
       Serial.println("lps "+String(1000000/time)+"\n");
       time=micros();
       #endif
              
       if (speeds==1) {
         /*#if (DEBUG>1)
         Serial.println("checking key and tween pots\n"); 
         #endif*/
         keyPause = analogRead(keyPin);  
         tweenPause = round(analogRead(tweenPin)/10);
       }
       
       // LET THE USER ADJUST GLOBAL BRIGHTNESS... 
       briVal = (round(analogRead(briPin)/4)*maxBrightness)/255; //the Bright trimpot has a value between 0 and 1024, we divide this down to between 0 and our maxBrightness
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
      
       // update pause countdown of each LED...
       for(byte LEDnum=0;LEDnum<96;LEDnum++) {
         if (LEDpause[LEDnum]!=0) {
            //LED is paused
            LEDpause[LEDnum]=LEDpause[LEDnum]-1; //reduce the LEDs pause number and check back next loop
         } 
       }
       
       loopCount=0;       
    }
    
    loopCount++;    
    
    hueVal = round(analogRead(huePin)/4); //read the Color trimpot (gets passed to updateLED for each LED)
    //go through each LED and update it 
    for(byte LEDnum=0;LEDnum<96;LEDnum++) {
      if (LEDpause[LEDnum]==0) { //see if this LED's pause countdown has reached 0
        if (logic==1) updateLED(pgm_read_byte(&rldMap[LEDnum]),hueVal);
        #if defined(PROTOFLD)
        else if (LEDnum<80) updateLED(pgm_read_byte(&fldMap[LEDnum]),hueVal); 
        #else
        else if (pgm_read_byte(&fldMap[LEDnum])>7 && pgm_read_byte(&fldMap[LEDnum])<88) updateLED(pgm_read_byte(&fldMap[LEDnum]),hueVal); 
        #endif
      }
    }  
    FastLED.show();
    
    #if (DEBUG>2)
    delay(10); //slow things down to a crawl
    #endif
}


