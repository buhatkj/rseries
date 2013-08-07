/*

RAINBOWDUINO CODE FOR LOGIC DISPLAYS : 2013-07-28

The goal of this sketch is to get a Rainbowduino and 8x8 RGB LED Matrix to
emulate the logic display patterns seen in The Empire Strikes Back.
See the reference scenes here: http://youtu.be/JpyMmx0kMxQ

You'll need the rainbowduino library from here:
http://www.seeedstudio.com/wiki/images/4/43/Rainbowduino_for_Arduino1.0.zip

*/
// ** START OF SETTINGS **
#define LOGICMODE 0 //0 for FLD, 1 for Primary RLD, 2 for secondary RLD
/*
We typically have 5 Key Colors. For the FLD they're DARK BLUE, MEDIUM BLUE, BRIGHT BLUE, MEDIUM WHITE, BRIGHT WHITE
Each color is an array of 3 integers (Red,Green,Blue brightess 0-255)
*/
#define NUM_KEY_COLORS 6
#if (LOGICMODE==0)
 //unsigned int KeyColors[NUM_KEY_COLORS][3] = { {0,0,20} , {0,0,120} , {0,20,255} , {100,120,150} , {200,255,255} }; //FLD colors
 //unsigned int KeyColors[NUM_KEY_COLORS][3] = { {0,0,20} , {0,0,120} , {0,20,200} , {100,120,150} , {150,200,200} }; //darker FLD
 unsigned int KeyColors[NUM_KEY_COLORS][3] = { {0,0,0} , {0,0,20} , {0,0,120} , {0,20,255} , {100,120,150} , {200,255,255} }; //FLD colors
 //unsigned int KeyColors[NUM_KEY_COLORS][3] = { {0,0,0} , {50,50,150} , {255,100,55} , {205,55,150} , {150,55,255} };
#else
 unsigned int KeyColors[NUM_KEY_COLORS][3] = { {255,0,0} , {240,10,0} , {220,200,0} , {0,240,10} , {0,255,0} }; //RLD colors (the may be overridden by the Primary RLD)
#endif
//
//#define SPEED 1 //overall speed of color transition (smaller is faster)
#define STEPS 10 //number of 'tween' colors between the KeyColors
//
int tweenPause=50; //default pause time to use for TWEEN colors [50 for RLD, 15 for FLD]
int keyPause=2000; //default pause time for KeyColors [2000 for RLD, 500 for FLD] (eventually I'd like this to be an array so each key color can pause for a different time)
//
#define TWEAKMODE //in tweakmode we can adjust the timing using 10K pots attached to A0 and A1, comment this line out if you don't have pots connected
//#define TESTPATTERN 0 //0 for FLD, 1 for RLD. These testpatterns are helpful when aligning fibers
//
#define PSItype 0 //if using a Teeces PSI, tell us which kind it is here
// 0 = No PSI attached
// 1 = Teeces original (6 LEDs of each color, arranged side by side)
// 2 = Teeces original checkerboard (6 LEDs of each color arranged in a checkerboard pattern)
// 3 = Teeces V3.2 PSI by John V (13 LEDs of each color, arranged side by side)
// 4 = Teeces V3.2 PSI checkerboard by John V  (13 LEDs of each color, in a checkerboard pattern)
//set timing of the PSI's here (in milliseconds)...
int PSIbright=12; //rear PSI
int psiRed=2500;    //how long front PSI stays red (or green)
int psiBlue=1700;   //how long front PSI stays blue (or yellow)
int rbSlide=125; // mts - time to transition between red and blue in slide mode
//
//#define DEBUG //debug mode prints lots of info to Serial. this makes the display speed too slow and not recommended for full-time use.
//
// =======================================================================================
// ** THE AVERAGE R2 BUILDER SHOULDN'T HAVE TO ADJUST SETTINGS UNDER HERE
// =======================================================================================
//
#include <Wire.h> //the Wire library is used for the Arduino to talk to each other via I2C
#include <Rainbowduino.h>
#define TOTAL_COLORS ((NUM_KEY_COLORS-1)*STEPS)+NUM_KEY_COLORS //total number of colors including first, last and tweens
unsigned int AllColors[TOTAL_COLORS][3]; // a big array to hold all KeyColors and all Tween colors
unsigned int LEDcolor[64]; // an array holding the current color of each LED
unsigned int LEDdirection[64]; // an array holding the current diection of each LED (0 = going up, 1 = going down)
unsigned int LEDpauseTime[64]; // an array holding the current pause time of each LED (LED should remain current color until this hits 0)
//
#if (LOGICMODE==0)
 // this is the FLD. use all rows 
 #define STARTROW 0
 #define ENDROW 8
#else
 // this is the RLD. don't use the first and last rows
 #define STARTROW 1
 #define ENDROW 7
#endif
//
#if defined(TWEAKMODE)
 int keyPin = A0; //analog pin to read keyPause values  
 int tweenPin = A1; //analog pin to read tweenPause values  
#endif
//
//map the LEDs from the bezel to the matrix...
//the matrix LEDs begin with 0 (top left)
#if (LOGICMODE==0)
//we're dealing with the FLD
int ledNums[64] = { 
  //upper FLD...
   0, 1, 3, 4,46, 7,
   8, 9, 2,11, 5,13,47,
  17,10,19,12,21,14,
  16,18,26,20,29,22,23,
  24,25,27,28,30,31,
  //lower FLD...
  32,33,35,36,38,39,
  40,41,34,43,37,45,47,
  49,42,51,44,53,46,
  48,50,58,52,61,54,55,
  56,57,59,60,62,63
  };
#else
//we're dealing with the RLD
int ledNums[64] = { 
  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
   8, 9,18,10,19,11,20,12,21,13,14,15,
  16,24,17,25,26,27,28,29,30,22,31,23,
  40,32,41,33,34,35,36,37,38,46,39,47,
  48,49,42,50,43,51,44,52,45,53,54,55,
  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL
  };
#endif
//
#if (PSItype!=0)
 #include <LedControl.h>
 LedControl lc=LedControl(3,2,A7,1); //use Rainbowduino pins D3, D2 and A7 for the PSI D,C,L
#endif
// =======================================================================================
#if (PSItype==4)  // slide animation code for Teeces V2 PSI boards (code by John V)
#define HPROW 5
class PSI {
  int stage; //0 thru 6
  int inc;
  int stageDelay[7];
  int cols[7][5];
  int randNumber; //a random number to decide the fate of the last stage 
  unsigned long timeLast;
  int device; 
  public: 
  PSI(int _delay1, int _delay2, int _delay3, int _device)
  {
    device=_device;   
    stage=0;
    timeLast=0;
    inc=1;
    
    cols[0][0] = B10101000;
    cols[0][1] = B01010100;
    cols[0][2] = B10101000;
    cols[0][3] = B01010100;
    cols[0][4] = B10101000;
   
    cols[1][0] = B00101000; //R B R B R B
    cols[1][1] = B11010100; //B R B R B R
    cols[1][2] = B00101000; //R B R B R B
    cols[1][3] = B11010100; //B R B R B R
    cols[1][4] = B00101000; //R B R B R B
 
    cols[2][0] = B01101000;
    cols[2][1] = B10010100;
    cols[2][2] = B01101000;
    cols[2][3] = B10010100;
    cols[2][4] = B01101000;
   
    cols[3][0] = B01001000;
    cols[3][1] = B10110100;
    cols[3][2] = B01001000;
    cols[3][3] = B10110100;
    cols[3][4] = B01001000;
   
    cols[4][0] = B01011000;
    cols[4][1] = B10100100;
    cols[4][2] = B01011000;
    cols[4][3] = B10100100;
    cols[4][4] = B01011000;
   
    cols[5][0] = B01010000;
    cols[5][1] = B10101100;
    cols[5][2] = B01010000;
    cols[5][3] = B10101100;
    cols[5][4] = B01010000;
   
    cols[6][0] = B01010100;
    cols[6][1] = B10101000;
    cols[6][2] = B01010100;
    cols[6][3] = B10101000;
    cols[6][4] = B01010100;
   
    stageDelay[0] = _delay1 - _delay3;
    stageDelay[1] = _delay3/5;
    stageDelay[2] = _delay3/5;
    stageDelay[3] = _delay3/5;
    stageDelay[4] = _delay3/5;
    stageDelay[5] = _delay3/5;
    stageDelay[6] = _delay2 - _delay3;
  }
 
  void Animate(unsigned long elapsed, LedControl control)
  {
    if ((elapsed - timeLast) < stageDelay[stage]) return;   
    timeLast = elapsed;
    stage+=inc; 
    if (stage>6 || stage<0 )
    {
      inc *= -1;
      stage+=inc*2;
    }   
    if (stage==6) //randomly choose whether or not to go 'stuck'
      {
        randNumber = random(9);
        if (randNumber<5) { //set the last stage to 'stuck'
          cols[6][0] = B01010000;
          cols[6][1] = B10101100;
          cols[6][2] = B01010000;
          cols[6][3] = B10101100;
          cols[6][4] = B01010000;
        }
        else //reset the last stage to a solid color
        {
          cols[6][0] = B01010100;
          cols[6][1] = B10101000;
          cols[6][2] = B01010100;
          cols[6][3] = B10101000;
          cols[6][4] = B01010100;
        }
      }
     if (stage==0) //randomly choose whether or not to go 'stuck'
      {
        randNumber = random(9);
        if (randNumber<5) { //set the first stage to 'stuck'
          cols[0][0] = B00101000; //R B R B R B
          cols[0][1] = B11010100; //B R B R B R
          cols[0][2] = B00101000; //R B R B R B
          cols[0][3] = B11010100; //B R B R B R
          cols[0][4] = B00101000; //R B R B R B
        }
        else //reset the first stage to a solid color
        {
          cols[0][0] = B10101000;
          cols[0][1] = B01010100;
          cols[0][2] = B10101000;
          cols[0][3] = B01010100;
          cols[0][4] = B10101000;
        }
      }
 
    for (int row=0; row<5; row++)
      control.setRow(device,row,cols[stage][row]);
  }
};
 
#endif  
 
// =======================================================================================
#if (PSItype==3)  // slide animation code for Teeces V2 PSI boards (code by John V)
#define HPROW 5
  class PSI {    
    int stage; //0 thru 6
  int inc;
  int stageDelay[7];
  int cols[7]; 
  unsigned long timeLast;
  int device; 
  public: 
  PSI(int _delay1, int _delay2, int _delay3, int _device)
  {
    device=_device;   
    stage=0;
    timeLast=0;
    inc=1;
   
    cols[0] = B11100000;
    cols[1] = B11110000;
    cols[2] = B01110000;
    cols[3] = B01111000;
    cols[4] = B00111000;
    cols[5] = B00111100;
    cols[6] = B00011100;
   
    stageDelay[0] = _delay1 - _delay3;
    stageDelay[1] = _delay3/5;
    stageDelay[2] = _delay3/5;
    stageDelay[3] = _delay3/5;
    stageDelay[4] = _delay3/5;
    stageDelay[5] = _delay3/5;
    stageDelay[6] = _delay2 - _delay3;
    }  
    void Animate(unsigned long elapsed, LedControl control)
    {
    if ((elapsed - timeLast) < stageDelay[stage]) return;   
    timeLast = elapsed;
    stage+=inc; 
    if (stage>6 || stage<0 )
    {
      inc *= -1;
      stage+=inc*2;
    }   
    for (int row=0; row<5; row++)
      control.setRow(device,row,cols[stage]);
    }
  };
#endif
// =======================================================================================
//michael smith's checkerboard PSI method for Teeces original PSI boards
// Michael's original sketch is here : http://pastebin.com/hXeZb7Gd
#if (PSItype==2)
#define HPROW 4
static const int patternAtStage[] = { B01010000, B11010000, B10010000, B10110000, B10100000, B00100000, B01100000, B01000000, B01010000 };
class PSI
{    
    bool state;
    int stage;
    unsigned long timeLast;
    int delay1, delay2, delay3;
    int device;    
    int delayAtStage[9];
    int slideDirection;  // is either 1 or -1
    int maxStage;  // for PSIslide it's either 5 or 9 stages, for traditional PSI it's just back and forth between 2    
public:        
    PSI(int _delay1, int _delay2, int _delay3, int _device)
    {
        delayAtStage[0] = _delay1;
        delayAtStage[1] = _delay3/3;        // delay3 is total transition time - divide it by the 3 stages of transition
        delayAtStage[2] = delayAtStage[1];
        delayAtStage[3] = delayAtStage[1];
        delayAtStage[4] = _delay2;
        delayAtStage[5] = delayAtStage[1];
        delayAtStage[6] = delayAtStage[1];
        delayAtStage[7] = delayAtStage[1];
        delayAtStage[8] = _delay1;          // repeated because it's not a loop it cycles back and forth across the pattern.        
        stage=0;
        slideDirection=1;
        maxStage=8;         // change to 5 would skip the LtoR from blue to red.
        timeLast=0;
        device=_device;        
        // legacy for traditional PSI animation
        delay1=_delay1;
        delay2=_delay2;
        delay3=_delay3;
        state=false;
    }    
    void Animate(unsigned long timeNow, LedControl control)
    {
            if ((timeNow - timeLast) < delayAtStage[stage]) return;
            //Serial.begin(9600);
            //Serial.println(stage);
            //Serial.println(patternAtStage[stage]);            
            timeLast = timeNow;
            stage+=slideDirection; //move to the next stage, which could be up or down in the array
            if (stage >= maxStage)
            {
                // limit the stage to the maxStage and reverse the direction of the slide
                stage=maxStage;
                slideDirection = -1;
            }
            else if (stage <= 0)
            {
                stage=0;
                slideDirection = 1;
            }
            // set the patterns for this stage
            control.setRow(device,0,patternAtStage[stage]);
            control.setRow(device,1,~patternAtStage[stage]);
            control.setRow(device,2,patternAtStage[stage]);
            control.setRow(device,3,~patternAtStage[stage]);
    }
};
#endif
// =======================================================================================
// slide animation code for Teeces original PSI boards
#if (PSItype==1)
#define HPROW 4
  class PSI {
    int stage; //0 thru 4
    int inc;
    int stageDelay[5];
    int cols[5];
    unsigned long timeLast;
    int device;
    public:  
    PSI(int _delay1, int _delay2, int _device)  {
      device=_device;    
      stage=0;
      timeLast=0;
      inc=1;    
      cols[0] = B11000000;
      cols[1] = B11100000;
      cols[2] = B01100000;
      cols[3] = B01110000;
      cols[4] = B00110000;    
      stageDelay[0] = _delay1 - 300;
      stageDelay[1] = 100;
      stageDelay[2] = 100;
      stageDelay[3] = 100;
      stageDelay[4] = _delay2 - 300;
    }  
    void Animate(unsigned long elapsed, LedControl control)  {
      if ((elapsed - timeLast) < stageDelay[stage]) return;
      timeLast = elapsed;
      stage+=inc;
      if (stage>4 || stage<0 ) {
        inc *= -1;
        stage+=inc*2;
      }    
      for (int row=0; row<4; row++) control.setRow(device,row,cols[stage]);
    }
  };
#endif  
// =======================================================================================
#if defined (PSItype)
#if (PSItype==1)
  PSI psi1=PSI(psiRed, psiBlue, 0); 
#elif (PSItype>1)
  PSI psi1=PSI(psiRed, psiBlue, rbSlide, 0);
#endif
#endif
//
//
void setup() {
  Serial.begin(9600); //used for debugging
  Serial.println();Serial.print(NUM_KEY_COLORS); Serial.print(" KeyColors with "); Serial.print(STEPS); Serial.println(" Tween colorws between each pair.");
  Serial.print("Total colors is ");Serial.print(TOTAL_COLORS);Serial.print(" (0 thru ");Serial.print(TOTAL_COLORS-1);Serial.println(")");
  Rb.init(); //initialize Rainbowduino driver
  randomSeed(analogRead(0)); //helps keep random numbers more random  
  
  
  
  #if (LOGICMODE==1)
    Wire.begin(10);
    //this device is the Primary RLD Arduino, we need to send info to the Secondary RLD Arduino
    delay(100); //wait a moment to make sure secondary RLD is ready
    Wire.beginTransmission(40); //start talking to device 40
    Wire.write("number of key colors is ");
    Wire.write(NUM_KEY_COLORS); 
    Wire.endTransmission();
  #elif (LOGICMODE==2)
    //this device is the Secondary RLD Arduino, wait to receive info from the Primary RLD Arduino
    Wire.begin(40);
    //Wire.onReceive(receiveEvent);
    int ready=0;
    while (ready==0) {
      if (Wire.available()) {
        char c = Wire.read();
        Serial.print(c);
        delay(100);
        }
    }
  #endif
  
  #if !defined(TESTPATTERN)
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
     
     //starting at the top left of the logic bezel, starting lighting things up...
     //RLD bezel is 4 rows of 12 LEDs
     //FLD bezels have 5 rows which alternate between 6 and 7 LEDs
     /*int startLED=(STARTROW*8);
     int finalLED=(ENDROW*8)-1;
     int x,y;
     for(int z=startLED;z<finalLED;z++) {
       int matrixLED=ledNums[z];
       if (y==NULL) {
         int y=0;
         int x=STARTROW;
       }
       if (y==8) {
         int y=0;
         x++;
       }
       Rb.setPixelXY(x,y,AllColors[color][0],AllColors[color][1],AllColors[color][2]); 
       y++;
       delay(5);
     }
     delay(100);*/
     
     
     //light things up using the regular matrix order instead of bezel order...
     // * boring! *
     for(int x=STARTROW;x<ENDROW;x++) {
       for(int y=0; y<8; y++) {
         Rb.setPixelXY(x,y,AllColors[color][0],AllColors[color][1],AllColors[color][2]);         
       }
       delay(5);       
     }
        
     
     
  }
  //ramp the colors back down again...
  for(int color=TOTAL_COLORS-1;color>=0;color--) {
     for(int x=(ENDROW-1);x>=STARTROW;x--) {
       for(int y=7;y>=0;y--) {
         Rb.setPixelXY(x,y,AllColors[color][0],AllColors[color][1],AllColors[color][2]);         
       }
     }
     delay(5);
  }
  //set each LED to a random color from our big array
  unsigned int LEDnumber=0;
  for(int x=STARTROW;x<ENDROW;x++) {
     for(int y=0;y<8;y++) {
       LEDcolor[LEDnumber]=random(TOTAL_COLORS); //choose a random color for this LED
       LEDdirection[LEDnumber]=random(2); //choose a random direction for this LED (0 up or 1 down)
       if (LEDcolor[LEDnumber]%(STEPS+1)==0) LEDpauseTime[LEDnumber]=random(keyPause); //color is a key, set its pause time for longer than tweens
       else LEDpauseTime[LEDnumber]=random(tweenPause);
       Rb.setPixelXY(x,y,AllColors[LEDcolor[LEDnumber]][0],AllColors[LEDcolor[LEDnumber]][1],AllColors[LEDcolor[LEDnumber]][2]);
       LEDnumber++;      
     }       
  }
  #endif
  
  //now set up the PSI stuff if necessary
  #if (PSItype!=0)
   lc.shutdown(0, false); //take the device out of shutdown (power save) mode
   lc.clearDisplay(0);
   lc.setIntensity(0,PSIbright);
   lc.setRow(0,HPROW,255); //HP lights on constantly  
  #endif
  
  //display a test pattern
  #if defined(TESTPATTERN)
    Serial.println("TESTPATTERN MODE");
    //1=blue, 2=green, 3=yellow, 4=red, 5=pink
    #if (TESTPATTERN==1)
      Serial.println("TESTPATTERN 1 (typically used for setting up a 4 row RLD)");
      int TestColors[64] = {
      0,0,0,0,0,0,0,0,  
      1,1,1,1,1,1,1,1,
      5,3,4,4,4,4,3,5,
      2,3,3,3,3,3,3,2,
      2,3,3,3,3,3,3,2,
      5,3,4,4,4,4,3,5,
      1,1,1,1,1,1,1,1,
      0,0,0,0,0,0,0,0
      };
    #elif (TESTPATTERN==0) 
      Serial.println("TESTPATTERN 0 (typically used for setting up the FLD)");
      int TestColors[64] = {
      2,3,5,3,3,5,3,2,  
      2,4,1,4,1,4,1,2,
      2,1,4,1,4,1,4,2,
      2,3,5,3,3,5,3,2,
      2,3,5,3,3,5,3,2,
      2,4,1,4,1,4,1,2,
      2,1,4,1,4,1,4,2,
      2,3,5,3,3,5,3,2
      };
    #endif
    unsigned int LEDnumber=0;
    int tcolor[3];
    for(int x=0;x<8;x++) {
       for(int y=0;y<8;y++) {
         if (TestColors[LEDnumber]==0) Rb.setPixelXY(x,y,0,0,0); //unlit
         if (TestColors[LEDnumber]==1) Rb.setPixelXY(x,y,0,0,100); //blue         
         if (TestColors[LEDnumber]==2) Rb.setPixelXY(x,y,0,100,0); //green
         if (TestColors[LEDnumber]==3) Rb.setPixelXY(x,y,100,100,0); //yellow
         if (TestColors[LEDnumber]==4) Rb.setPixelXY(x,y,100,0,0); //red
         if (TestColors[LEDnumber]==5) Rb.setPixelXY(x,y,200,50,200); //pink
         Serial.println(String(TestColors[LEDnumber])+" : "+String(tcolor[0])+","+String(tcolor[1])+","+String(tcolor[2]));
         delay(100);
         LEDnumber++;      
       }       
    }
  #endif
  
  
}

void updateLED(unsigned int LEDnumber) {
  //this will take an LED number (0-63) and adjust its values in the relevent arrays accordingly
  //check the current color this LED is set to...
  //unsigned int currentColor=LEDcolor[LEDnumber];
  if (LEDpauseTime[LEDnumber]!=0) {
    LEDpauseTime[LEDnumber]=LEDpauseTime[LEDnumber]-1; //reduce the LEDs pause number and check back next loop
  }
  else {
    //LED had 0 pause time, change things around...
    if (LEDdirection[LEDnumber]==0 && LEDcolor[LEDnumber]<(TOTAL_COLORS-1)) {
      LEDcolor[LEDnumber]=LEDcolor[LEDnumber]+1; //change it to next color
      if (LEDcolor[LEDnumber]%(STEPS+1)==0) LEDpauseTime[LEDnumber]=random(keyPause); //color is a key, set its pause time for longer than tweens
      else LEDpauseTime[LEDnumber]=random(tweenPause);
    }
    else if (LEDdirection[LEDnumber]==0 && LEDcolor[LEDnumber]==(TOTAL_COLORS-1)) {
      LEDdirection[LEDnumber]=1; //LED is at the final color, leave color but change direction to down
    }
    else if (LEDdirection[LEDnumber]==1 && LEDcolor[LEDnumber]>0) {
      LEDcolor[LEDnumber]=LEDcolor[LEDnumber]-1; //change it to previous color
      if (LEDcolor[LEDnumber]%(STEPS+1)==0) {
        LEDpauseTime[LEDnumber]=random(keyPause); //new color is a key, set LED's pause time for longer than tweens
      }
      else LEDpauseTime[LEDnumber]=tweenPause;
    }
    else if (LEDdirection[LEDnumber]==1 && LEDcolor[LEDnumber]==0) {
      LEDdirection[LEDnumber]=0; //LED is at the first color (0), leave color but change direction to up
    }
  }  
}

void RandomLogic(unsigned long elapsed) {
  static unsigned long timeLast=0;
  //if ((elapsed - timeLast) < SPEED) return;
  timeLast = elapsed; 
  //MAKE STUFF HAPPEN HERE  
  unsigned int LEDnumber=0;
  for(int x=STARTROW;x<ENDROW;x++) {
     for(int y=0;y<8;y++) {
       updateLED(LEDnumber); //updates the data for this LED in the relevant arrays (color, pause, direction)
       Rb.setPixelXY(x,y,AllColors[LEDcolor[LEDnumber]][0],AllColors[LEDcolor[LEDnumber]][1],AllColors[LEDcolor[LEDnumber]][2]);
       LEDnumber++; 
     }
  }  
}

void loop() {
  #if !defined(TESTPATTERN)
    unsigned long timeNew=millis(); //changed from micros()
    #if defined(TWEAKMODE)
      keyPause = analogRead(keyPin);  
      tweenPause = round(analogRead(tweenPin)/10);
    #endif
    #if defined(DEBUG)
      //print the pause values to serial if they've changed
      if (prevkeyPause!=NULL && (prevkeyPause!=keyPause || prevtweenPause!=tweenPause) {
        Serial.println("Tween:"+String(tweenPause)+"  Key:"+String(keyPause));
      }  
    #endif
    RandomLogic(timeNew);
    #if (PSItype!=0)
      psi1.Animate(timeNew,lc);  
    #endif
  #endif
  int prevkeyPause=keyPause;
  int prevtweenPause=tweenPause;
}