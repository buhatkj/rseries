// =======================================================================================
// ============================ Universal Dome Lighting Sketch ===========================
// =======================================================================================
//
// A sketch to run on an Arduino inside the dome. This runs Teeces lighting.
//
// Version : 2013-01-23
// Added a special 'random fadey' function for CuriousMarc's Holo Light (the Input+ of
//  each Holo Light should be connected to Arduino pins 3 or 6).
// Added support for Arduino/Adafruit Micro ( http://arduino.cc/en/Main/ArduinoBoardMicro )
//
//
// Thrown together by Paul Murphy (JoyMonkey) from various sources including...
// John V, Michael Erwin, Michael Smith, Roger Moolay, Chris Reiff and Brad Oakley
//
// Required : http://arduino.cc/playground/uploads/Main/LedControl.zip
// (that's an Arduino Library - it needs to be downloaded and extracted into your
// Libraries folder for this sketch to work)
//
// Logic Display and PSI Boards should be wired up in two chains (to prevent problems that
// some builders ran into when using a single chain setup for an extended period of time).
// In early 2012 a revised RLD board was released with two outputs to make wiring up in a
// two chain setup a little easier.
// V3.1 OUT2 uses Arduino Pro Micro or Pro Mini pins 9,8,7 for the FLDs and front PSI.
// If you're using the older V3 RLD and don't have the OUT2 pins, don't panic! You can
// still wire up a front chain by connecting directly to the Arduino pins.
//
// If using a V3 RLD...
//   RLD OUT -> Rear PSI
//   Arduino Pins 9,8,7 -> FLD IN D,C,L -> FLD -> Front PSI
//   (you will also need to supply +5V and GND to the front chain; it can go to any pins
//   labeled +5V and GND on any of the FLD or front PSI boards)
//
// If using a V3.1 RLD connections are a little simpler...
//   RLD OUT1 -> Rear PSI
//   RLD OUT2 -> FLD -> FLD -> Front PSI
//
// This sketch will work with an Arduino Pro Micro or Pro Mini mounted to the RLD.
// The Pro Micro uses slightly different pin numbers and has additional small LEDs on it
// that we can blink back and forth to confirm it's working (I call this microPSI).
// Because of these differences this sketch needs to be edited to suit a Pro Micro or a
// Pro Mini. If using a Pro Mini, simply delete the first line of code below...

// BOARDtype sets which Arduino we're using
// 1 = Arduino Pro Mini or Uno or Duemilanove ( http://arduino.cc/en/Main/ArduinoBoardProMini )
// 2 = Sparkfun Pro Micro ( https://www.sparkfun.com/products/11098 )
// 3 = Arduino Micro ( http://arduino.cc/en/Main/ArduinoBoardMicro )
#define BOARDtype 2

char text[] = "R2D2"; //PUT YOUR STARTUP TEXT HERE. (it'll scroll in aurebesh on all logics)

// set brightness levels here (a value of 0-15)...
int RLDbright=5;   //rear Logic
int RPSIbright=12; //rear PSI
int FLDbright=5;   //front Logics
int FPSIbright=12; //front PSI

//delay time of logic display blinkyness (lower = blink faster)
int LogicBlinkTime=175;

//set the type of our front and rear PSI's
#define PSItype 4
// 1 = Teeces original (6 LEDs of each color, arranged side by side)
// 2 = Teeces original checkerboard (6 LEDs of each color arranged in a checkerboard pattern)
// 3 = Teeces V3.2 PSI by John V (13 LEDs of each color, arranged side by side)
// 4 = Teeces V3.2 PSI checkerboard by John V  (13 LEDs of each color, in a checkerboard pattern)

//set timing of the PSI's here (in milliseconds)...
int psiRed=2500;    //how long front PSI stays red
int psiBlue=1700;   //how long front PSI stays blue
int psiYellow=1700; //how long rear PSI stays yellow
int psiGreen=2500;  //how long rear PSI stays green
int rbSlide=125; // mts - time to transition between red and blue in slide mode
int ygSlide=300; // mts - time to transition between yellow and green in slide mode

//pulse the PSI Brightness (0=off, 1=Front, 2=Rear, 3=Both)
#define PSIPULSER 0

//Use CuriousMarc's Holo Light boards in PWM mode
//(this means they'll need to be connected to pins 3 and 6)
#define CURIOUS

//#define TESTLOGICS //turns on all logic LEDs at once, useful for troubleshooting

//#define FLDx4 //for an R7 dome with 4 FLDs (if you have 4 FLDs then delete the first // )

// Most builders shouldn't have to edit anything below here. Enjoy!
//




// =======================================================================================
// =======================================================================================

/*
Different Arduino's have different pin numbers that are used for the rear chains.
   Arduino Pro Mini uses pins 12,11,10 for rear D,C,L
 Sparkfun Pro Micro uses pins 14,16,10 for rear D,C,L
      Arduino Micro uses pins A2,A1,A0 for rear D,C,L
*/
#if (BOARDtype==1) 
 #define DVAL 12
 #define CVAL 11
 #define LVAL 10
#elif (BOARDtype==2) 
 #define DVAL 14
 #define CVAL 16
 #define LVAL 10
#elif (BOARDtype==3) 
 #define DVAL A2
 #define CVAL A1
 #define LVAL A0
#endif

#if defined(FLDx4)
#define FDEV 5 //5 devices for front chain
#define FPSIDEV 4 //front PSI is device #4 in the chain
#else
#define FDEV 3 //3 devices for front chain
#define FPSIDEV 2 //front PSI is device #2 in the chain
#endif

#define RPSIDEV 3 //rear PSI is device #3 in the chain

//for scrolling text on logics...
int pixelPos=27;
int scrollCount=0;
//virtual coords are 5x45; device coords are 3 panels 6x8 each
unsigned long v_grid[5]; //this will give 5x40 bits

#include <LedControl.h>
#undef round 

//START UP LEDCONTROL...
LedControl lcRear=LedControl(DVAL,CVAL,LVAL,4); //rear chain (Pro Mini/Pro Micro pins)
LedControl lcFront=LedControl(9,8,7,FDEV); //front chain

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
#if (PSItype==1)
  PSI psiFront=PSI(psiRed, psiBlue, FPSIDEV); // device is FPSIDEV (#2 or #4 for an R7 dome) 
  PSI psiRear =PSI(psiYellow, psiGreen, RPSIDEV); // device is #3
//#endif
//#if (PSItype==2) || (PSItype==3)
#else
  PSI psiFront=PSI(psiRed, psiBlue, rbSlide, FPSIDEV); //2000 ms on red, 1000 ms on blue.
  PSI psiRear =PSI(psiYellow, psiGreen, ygSlide, RPSIDEV); //1000 ms on yellow, 2000 ms on green.
#endif
// =======================================================================================
void setup() {
  Serial.begin(9600); //used for debugging
  for(int dev=0;dev<lcRear.getDeviceCount();dev++) {
    lcRear.shutdown(dev, false); //take the device out of shutdown (power save) mode
    lcRear.clearDisplay(dev);
  }
  for(int dev=0;dev<lcFront.getDeviceCount();dev++) {
    lcFront.shutdown(dev, false); //take the device out of shutdown (power save) mode
    lcFront.clearDisplay(dev);
  }
  //set intensity of devices in  rear chain...
  lcRear.setIntensity(0, RLDbright); //RLD
  lcRear.setIntensity(1, RLDbright); //RLD
  lcRear.setIntensity(2, RLDbright); //RLD
  lcRear.setIntensity(3, RPSIbright); //Rear PSI
  //set intensity of devices in front chain...
  for(int dev=0;dev<(lcFront.getDeviceCount()-1);dev++) {
    lcFront.setIntensity(dev, FLDbright);  //front logics (all but the last dev in chain)
  }
  lcFront.setIntensity(FPSIDEV, FPSIbright); //Front PSI
  //HP lights on constantly...
  lcRear.setRow(RPSIDEV,HPROW,255); //rear psi
  lcFront.setRow(FPSIDEV,HPROW,255); //front psi
  
  #if (BOARDtype==2)
  pinMode(17, OUTPUT);  // Set RX LED of Pro Micro as an output
  #endif
  
  #if defined(CURIOUS)
  pinMode(3, OUTPUT);
  pinMode(6, OUTPUT);
  #endif
  
}
// =======================================================================================
void loop() {
  //
  if (text!="") { //if startup text isn't empty, lets scroll!
    initGrid(); 
    if (scrollCount==0) {
      scrollingText();
      delay(60);
      return;
    }
  }
  //
  unsigned long timeNew= millis();
  psiFront.Animate(timeNew, lcFront);
  psiRear.Animate(timeNew, lcRear);
  animateLogic(timeNew);
#if (BOARDtype==2)
  microPSI(timeNew);
#endif
#if PSIPULSER>0
  PSIpulse(timeNew);
#endif
#if defined(CURIOUS)
  CuriousFade(timeNew);
#endif
}
// =======================================================================================
// this is the code to blink all the logic LEDs randomly...
void animateLogic(unsigned long elapsed) {
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < LogicBlinkTime) return;
  timeLast = elapsed; 
#if defined(TESTLOGICS)
//turn on all logic LEDs to make sure they're all working
  for (int dev=0; dev<3; dev++)
    for (int row=0; row<6; row++)
      lcRear.setRow(dev,row,255);
  for (int dev=0; dev<FPSIDEV; dev++)
    for (int row=0; row<6; row++)
      lcFront.setRow(dev,row,255); 
#else
//do the usual blinkyness
  for (int dev=0; dev<3; dev++)
    for (int row=0; row<6; row++)
      lcRear.setRow(dev,row,random(0,256));
  for (int dev=0; dev<FPSIDEV; dev++)
    for (int row=0; row<6; row++)
      lcFront.setRow(dev,row,random(0,256)); 
#endif      
}
// =======================================================================================
// PULSING PSI LED BRIGHTNESS/INTENSITY
#if PSIPULSER>0
int pulseState = LOW; //initial state of our PSI (high intensity, going down)
int pulseVal = 15; //initial value of our PSI (full intensity)
void PSIpulse(unsigned long elapsed) {
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < 100) return; //proceed if 100 milliseconds have passed
  timeLast = elapsed; 
  if (pulseState == HIGH) { //increase intensity
    pulseVal++; //increase value by 1
    if (pulseVal == 16) { //if we've gone beyond full intensity, start going down again
      pulseVal = 15;
      pulseState = LOW;
    }
  }
  else { //decrease intensity
    pulseVal--; //increase value by 1
    if (pulseVal == 0) { //if we've gone beyond full intensity, start going down again
      pulseVal = 1;
      pulseState = HIGH;
    }
  }
  #if PSIPULSER==1 || PSIPULSER==3
  lcFront.setIntensity(2,pulseVal); //set the front intensity
  #endif   
  #if PSIPULSER==2 || PSIPULSER==3
  lcRear.setIntensity(3,pulseVal); //set the rear intensity
  #endif  
}
#endif
////////////////////////////////////

// =======================================================================================
// PULSE THE BRIGHTNESS OF THE CURIOUS MARC HOLO LIGHTS
#if defined(CURIOUS)
int Cbrightness = 0;    // how bright the LED starts at
int CfadeAmount = 1;    // how many points to fade the LED by
int holdtime=1000; //default 'pause' is set to 5 seconds
int fadespeed=1; //milliseconds to wait between actions (lower number speeds things up)
void CuriousFade(unsigned long elapsed) {
  analogWrite(3, Cbrightness);  
  analogWrite(6, Cbrightness);  
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < (fadespeed + holdtime)) return; //proceed if some fadespeed+holdtime milliseconds have passed
  timeLast = elapsed; 
  holdtime = 0;
  Cbrightness = Cbrightness + CfadeAmount;
  if (Cbrightness == 0 || Cbrightness == 255) {
    CfadeAmount = -CfadeAmount ; //reverse fade direction
    holdtime = random(9)*1000; //generate a new 'pause' time of 0 to 9 seconds
    fadespeed = random(1,20); //generate a new fadespeed time of 1 to 20 milliseconds
  } 
}
#endif
////////////////////////////////////


// =======================================================================================
// AUREBESH CHARACTERS TAKEN FROM MOOLAY'S SKETCH
// http://astromech.net/forums/showthread.php?p=99487
void showGrid() {
  //copy from virt coords to device coords
  unsigned char col8=0;
  unsigned char col17=0;
  unsigned char col26=0;
  for (int row=0; row<5; row++) {
    lcRear.setRow(0,row, rev( v_grid[row] & 255L ) ); //device 0
    lcRear.setRow(1,row, rev( (v_grid[row] & 255L<<9) >> 9 ) ); //device 1
    lcRear.setRow(2,row, rev( (v_grid[row] & 255L<<18) >> 18 ) ); //device 2
    lcFront.setRow(0,row, rev( v_grid[row] & 255L ) ); //device 0
    lcFront.setRow(1,row, rev( (v_grid[row] & 255L<<9) >> 9 ) ); //device 1    
    if ( (v_grid[row] & 1L<<8) == 1L<<8) col8 += 128>>row;
    if ( (v_grid[row] & 1L<<17) == 1L<<17) col17 += 128>>row;
    if ( (v_grid[row] & 1L<<26) == 1L<<26) col26 += 128>>row;
  }  
  lcRear.setRow(0, 5, col8);
  lcRear.setRow(1, 5, col17);
  lcRear.setRow(2, 5, col26);
  lcFront.setRow(0, 5, col8);
  lcFront.setRow(1, 5, col17); 
}
unsigned char rev(unsigned char b) {
  //reverse bits of a byte
  return (b * 0x0202020202ULL & 0x010884422010ULL) % 1023;
}
void initGrid() {
  for (int row=0; row<6; row++) v_grid[row]=0L;
}
int c2[] = {
	B00001111,
	B00001001,
	B00000100,
	B00001001,
	B00001111 };
int cA[] = {
	B00010001,
	B00001111,
	B00000000,
	B00001111,
	B00010001 };
int cB[] = {
	B00001110,
	B00010001,
	B00001110,
	B00010001,
	B00001110 };
int cC[] = {
	B00000001,
	B00000001,
	B00000100,
	B00010000,
	B00010000 };
int cD[] = {
	B00011111,
	B00001000,
	B00000111,
	B00000010,
	B00000001 };
int cE[] = {
	B00011001,
	B00011001,
	B00011001,
	B00010110,
	B00010100 };
int cF[] = {
	B00010000,
	B00001010,
	B00000111,
	B00000011,
	B00011111 };
int cG[] = {
	B00011101,
	B00010101,
	B00010001,
	B00001001,
	B00000111 };
int cH[] = {
	B00011111,
	B00000000,
	B00001110,
	B00000000,
	B00011111 };
int cI[] = {
	B00000100,
	B00000110,
	B00000100,
	B00000100,
	B00000100 };
int cJ[] = {
	B00010000,
	B00011000,
	B00001111,
	B00000100,
	B00000011 };
int cK[] = {
	B00011111,
	B00010000,
	B00010000,
	B00010000,
	B00011111 };
int cL[] = {
	B00010000,
	B00010001,
	B00010010,
	B00010100,
	B00011000 };
int cM[] = {
	B00011100,
	B00010010,
	B00000001,
	B00010001,
	B00011111 };
int cN[] = {
	B00001010,
	B00010101,
	B00010101,
	B00010011,
	B00010010 };
int cO[] = {
	B00000000,
	B00001110,
	B00010001,
	B00010001,
	B00011111 };
int cP[] = {
	B00010110,
	B00010101,
	B00010001,
	B00010001,
	B00011110 };
int cQ[] = {
	B00011111,
	B00010001,
	B00000001,
	B00000001,
	B00000111 };
int cR[] = {
	B00011111,
	B00001000,
	B00000100,
	B00000010,
	B00000001 };
int cS[] = {
	B00010000,
	B00010010,
	B00010101,
	B00011010,
	B00010100 };
int cT[] = {
	B00001111,
	B00000010,
	B00000010,
	B00000010,
	B00000010 };
int cU[] = {
	B00000100,
	B00000100,
	B00010101,
	B00001110,
	B00000100 };
int cV[] = {
	B00010001,
	B00001010,
	B00000100,
	B00000100,
	B00000100 };
int cW[] = {
	B00011111,
	B00010001,
	B00010001,
	B00010001,
	B00011111 };
int cX[] = {
	B00000100,
	B00001010,
	B00010001,
	B00010001,
	B00011111 };
int cY[] = {
	B00010011,
	B00010101,
	B00001010,
	B00001010,
	B00000100 };
int cZ[] = {
	B00010110,
	B00010101,
	B00010000,
	B00010001,
	B00011111 };
int cZZ[] = {
	B00000000,
	B00000000,
	B00000000,
	B00000000,
	B00000000 };
int co[] = {
  B01010101,
  B01010101,
  B01010101,
  B01010101,
  B01010101 };
void drawLetter(char let, int shift) {
  int *pLetter;
  switch (let) {
    case '2': pLetter=c2; break;
    case 'A': pLetter=cA; break;
    case 'B': pLetter=cB; break;
    case 'C': pLetter=cC; break;
    case 'D': pLetter=cD; break;
    case 'E': pLetter=cE; break;
    case 'F': pLetter=cF; break;
    case 'G': pLetter=cG; break;
    case 'H': pLetter=cH; break;
    case 'I': pLetter=cI; break;
    case 'J': pLetter=cJ; break;
    case 'K': pLetter=cK; break;
    case 'L': pLetter=cL; break;
    case 'M': pLetter=cM; break;
    case 'N': pLetter=cN; break;
    case 'O': pLetter=cO; break;
    case 'P': pLetter=cP; break;
    case 'Q': pLetter=cQ; break;
    case 'R': pLetter=cR; break;
    case 'S': pLetter=cS; break;
    case 'T': pLetter=cT; break;
    case 'U': pLetter=cU; break;
    case 'V': pLetter=cV; break;
    case 'W': pLetter=cW; break;
    case 'X': pLetter=cX; break;
    case 'Y': pLetter=cY; break;
    case 'Z': pLetter=cZ; break;
    case '_': pLetter=cZZ; break;  
    default:return;
  }
  //loop thru rows of the letter
  for (int i=0; i<5; i++) {
    if (shift>0) //positive shift means letter is slid to the right on the display
    v_grid[i] += (long)pLetter[i] << shift;
    else //negative shift means letter is slid to the left so that only part of it is visible
    v_grid[i] += (long)pLetter[i] >> -shift;
  }
}
void scrollingText() {
  for (int i=0; i<strlen(text); i++) {
    drawLetter(text[i], i*5 + pixelPos);
  }  
  pixelPos--;
  if (pixelPos < -5*(int)strlen(text)) {
    pixelPos=27;
    scrollCount++;
  }
  showGrid();
}

// =======================================================================================
// this is the code to flash the Pro Micro's green and yellow LEDs back and fourth...
#if (BOARDtype==2)
int ledState = LOW; //initial state of our Pro Micro microPSI function
void microPSI(unsigned long elapsed) {
  //blink the ProMicro's TX and RX LEDs back and forth
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < 2000) return;
  timeLast = elapsed; 
  // if the LED is off turn it on and vice-versa:
  if (ledState == LOW) {
    ledState = HIGH;
    digitalWrite(17, HIGH);   // set the RX LED on
    TXLED1; //TX LED is not tied to a normally controlled pin
  }
  else {
    ledState = LOW;
    digitalWrite(17, LOW);    // set the RX LED off
    TXLED0;
  }
}
#endif
// =======================================================================================