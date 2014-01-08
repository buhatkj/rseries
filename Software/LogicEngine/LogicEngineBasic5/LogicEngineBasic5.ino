//
//  RSeries Logic Engine AVR : Basic Sketch by Paul Murphy
//  ======================================================
//  This sketch provides basic functionality for the RSeries Logic Engine hardware.
//  Does not include microSD reading or I2C communication.
//
//  Uses the FastSPI_LED2 library : https://code.google.com/p/fastspi/downloads/detail?name=FastSPI_LED2.RC5.zip
//
//   revision history...
//   2014-01-08 : Removed debug options to free up some RAM. Fixed rldMap.
//   2014-01-07 : Added 'Testpattern Mode' to aid assembly of the Rear Logic
//
//

//  the TOGGLEPIN is used if there's no SD card. if this pin is jumped to +5V,
//  then we'll assume this is the RLD, otherwise we'll assume this is the FLD
#define TOGGLEPIN 4
//  the SPEEDPIN enables speed adjustment via the pots via the pause & delay
#define SPEEDPIN 3

//#define PROTOFLD //uncomment this line if your FLD boards have only 40 LEDs (final versions have 48 LEDs)

#define LOOPCHECK 50 //number of loops after which we'll check the pot values

#include "FastSPI_LED2.h"
#define NUM_LEDS 96 //absolute max number of LEDs possible
#define DATA_PIN 6
CRGB leds[NUM_LEDS];

//byte Keys=6;
byte Tweens=6;
byte tweenPause=7; // time to delay each tween color (aka fadeyness)
int keyPause=450; // time to delay each key color (aka pauseyness)
byte maxBrightness=128; // 0-255, usually no need to go over 128 (current and heat increases exponentially)

int keyPin = A0; //analog pin to read keyPause value
int tweenPin = A1; //analog pin to read tweenPause value 
int briPin = A2; //analog pin to read Brightness value
int huePin = A3; //analog pin to read Color/Hue shift value

byte totalColors,Keys; 
//default Front colors...
const byte fldColors[6][3] = { {170,0,0} , {170,255,54} , {170,255,120} , {166,255,200} , {154,84,150} , {174,0,200} };
//default Rear colors...
const byte rldColors[5][3] = { {87,0,0} , {87,206,105} , {79,255,184} , {18,255,250} , {0,255,214} };

byte AllColors[64][3]; // a big array to hold all original KeyColors and all Tween colors
byte oColors[64][3];   // will hold a copy of the original colors, useful when shifting hues
byte LEDstat[96][3];   // an array holding the current color number of each LED, its direction and pausetime
boolean speeds=0; //0 for preset, 1 for tweakable (depends on speedpin)
boolean logic=0;  //0 for fld, 1 for rld (depends on togglepin)

//our LEDs aren't in numeric order (FLD is wired serpentine and RLD is all over the place!)
//so we can address specific LEDs more easily, we'll map them out here...
byte rldMap[96]={
 0, 1, 2,12, 3,11, 4,10, 5, 9, 6, 7,48,49,62,50,61,51,60,52,59,53,54,55,
15,14,13,16,17,18,19,20,21,22,23, 8,63,64,65,66,67,68,69,70,71,58,57,56,
32,33,34,31,30,29,28,27,26,25,24,39,80,79,78,77,76,75,74,73,72,85,86,87,
47,46,45,35,44,36,43,37,42,38,41,40,95,94,81,93,82,92,83,91,84,90,89,88
};

#if defined PROTOFLD
// for prototype 40-LED boards...
byte fldMap[80]={
 0, 1, 2, 3, 4, 5, 6, 7,
15,14,13,12,11,10, 9, 8,
16,17,18,19,20,21,22,23,
31,30,29,28,27,26,25,24,
32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,
55,54,53,52,51,50,49,48,
56,57,58,59,60,61,62,63,
71,70,69,68,67,66,65,64,
72,73,74,75,76,77,78,79
};
#else
// for final 48-LED boards...
byte fldMap[80]={
15,14,13,12,11,10, 9, 8,
16,17,18,19,20,21,22,23,
31,30,29,28,27,26,25,24,
32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,

55,54,53,52,51,50,49,48,
56,57,58,59,60,61,62,63,
71,70,69,68,67,66,65,64,
72,73,74,75,76,77,78,79,
87,86,85,84,83,82,81,80
};
#endif

void setup() {      
      randomSeed(analogRead(0)); //helps keep random numbers more random  
      //DEBUG
      //Serial.begin(9600);      
      //Serial.println(String(totalColors)+" colors");           
      pinMode(TOGGLEPIN, INPUT);
      //digitalWrite(TOGGLEPIN, HIGH); //used for dipswitch prototype
      logic=digitalRead(TOGGLEPIN);      
      if (logic==1) {
        //DEBUG
        //Serial.println("PIN TOGGLEPIN HIGH : RLD");
        tweenPause=40;
        keyPause=1200;
        Keys=sizeof(rldColors)/3;
      }  
      else {
        //DEBUG
        //Serial.println("PIN TOGGLEPIN LOW : FLD");
        Keys=sizeof(fldColors)/3;        
      } 
      
      //DEBUG
      // print all the colors
      //Serial.println(String(Keys)+" key colors");
      //delay(250);
  
      totalColors=Keys*Tweens;
      
      pinMode(SPEEDPIN, INPUT); 
      //digitalWrite(SPEEDPIN, HIGH); //used for dipswitch prototype
      if (digitalRead(SPEEDPIN)==HIGH) {
        speeds=1;  
        //DEBUG
        //Serial.println("SPEEDS TWEAKABLE");
      }  
      
      //make a giant array of all colors tweened...
      byte el,val,kcol,perStep,totalColorCount; //
      for(kcol=0;kcol<(Keys);kcol++) { //go through each Key color
        for(el=0;el<3;el++) { //loop through H, S and V 
          if (logic==0) perStep=int(fldColors[kcol+1][el]-fldColors[kcol][el])/(Tweens+1);
          else perStep=int(rldColors[kcol+1][el]-rldColors[kcol][el])/(Tweens+1);
          byte tweenCount=0;
          if (logic==0) val=fldColors[kcol][el];
          else val=rldColors[kcol][el];
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
      //DEBUG
      // print all the colors
      //for(byte x=0;x<totalColors;x++) Serial.println(String(x)+" : "+String(AllColors[x][0])+","+String(AllColors[x][1])+","+String(AllColors[x][2])+"  o  "+String(oColors[x][0])+","+String(oColors[x][1])+","+String(oColors[x][2]));
      
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
        if (logic==1)  leds[rldMap[x]].setHSV(AllColors[LEDstat[x][0]][0],AllColors[LEDstat[x][0]][1],AllColors[LEDstat[x][0]][2]); 
        else if (x<80) leds[fldMap[x]].setHSV(AllColors[LEDstat[x][0]][0],AllColors[LEDstat[x][0]][1],AllColors[LEDstat[x][0]][2]) ; 
        FastLED.show(); 
        delay(10);
      }
      delay(100);
  
      //do a startup animation of some sort
      for(byte x=0;x<NUM_LEDS;x++) {
        if (logic==1) {
          //RLD STARTUP
          leds[rldMap[x]] = CRGB::Green;
          if ((x+1)%24==0) { delay(250); FastLED.show(); }
        }  
        else {
          //FLD STARTUP
          if (fldMap[x]>7 && fldMap[x]<88) { //not all LEDs in the array are used for the FLD
            leds[fldMap[x]] = CRGB::Blue;
            if ((x+1)%8==0) { delay(100); FastLED.show(); } 
          }         
        } 
             
      }  
      
      delay(1000); 
}


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
         //DEBUG
         //Serial.println("checking key and tween pots\n"); 
         keyPause = analogRead(keyPin);  
         tweenPause = round(analogRead(tweenPin)/10);
       }
       

       // LET THE USER SHIFT THE HUES OF ALL COLORS...
       hueVal = round(analogRead(huePin)/4); 
       //DEBUG
       //Serial.println("hueVal is "+String(hueVal)+"\n"); 
       hueDiff=(max(hueVal,prevHue)-min(hueVal,prevHue)); 
       if (hueDiff>=2) {
           //DEBUG
           //Serial.println("hueDiff is "+String(hueDiff));  
           //Serial.println("Adjusting TweakedColors...\n");    
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
       //DEBUG
       //Serial.println("briVal is "+String(briVal)); 
       //delay(200);
       if (briVal!=prevBri) {
         briDiff=(max(briVal,prevBri)-min(briVal,prevBri)); 
         if (briDiff>=2) {
             //DEBUG
             //Serial.println("Setting brightness to "+String(briVal)+"...\n"); 
             FastLED.setBrightness(briVal); //sets the overall brightness
         }
         prevBri=briVal;
       } 
       loopCount=0;       
    }
    loopCount++;    
    
    //go through each LED and update it 
    for(byte LEDnum=0;LEDnum<NUM_LEDS;LEDnum++) {
      if (logic==1) updateLED(rldMap[LEDnum]);  
      else if (fldMap[LEDnum]>7 && fldMap[LEDnum]<88) updateLED(fldMap[LEDnum]); 
    }  
    FastLED.show();
    //delay(20);
}


