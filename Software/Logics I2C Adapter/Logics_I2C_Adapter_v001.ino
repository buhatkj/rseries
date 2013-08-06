// Logics I2C Adapter Firmware
// CCv3 SA BY - 2013 Michael Erwin
// Royal Engineers of Naboo
//
// Visit http://code.google.com/p/rseries-open-control/
//
// Contribution from John Vannoy (Teeces), Paul Murphy (JoyMonkey), 
// MilkB0ne, Michael Smith, Glyn Harper, Brad Oakley (BHD)
// and we're sure others that we've have missed!
//
// Firmware Release History
// v001 - Initial Firmware Release
//
//
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  #else
   #include "WProgram.h"
#endif
#include <Wire.h> //used to support I2C
#include <LedControl.h>
#undef round 
int LogicsI2CAdapter=2; // 1=RLD/RPSI, 2=FLD/PSI, 3=CBI/DP Personality
boolean PSISlide=true;  //   if FALSE will flash back and forth the old way for PSI prior to v3.1b
//long LeiaMessageTime = 31*1000; 
unsigned long mt=0;
unsigned long loopcount=0;

int DelayTime=30;
unsigned char OutPos;
unsigned char InPos;

// Define devices here. Change these numbers depending on
// what order you have everything connected in the chain.
int rld1dev=0; //first LED driver on RLD
int rld2dev=1; //second LED driver on RLD
int rld3dev=2; //third LED driver on RLD
int rpsidev=3; //LED driver on Rear (Green/Yellow) PSI
//
//int fpsidev=2; //LED driver on Front (Red/Blue) PSI
//int rpsidev=3; //LED driver on Rear (Green/Yellow) PSI
//
int fld1dev=0; //LED driver on FLD 1
int fld2dev=1; //LED driver on FLD 2
int fpsidev=2; //LED driver on Front (Red/Blue) PSI
//

int cbidev=0; //LED driver on CBI
int dpdev=1;  //LED drive on Data Port


boolean VRearLogic0[5][9];
boolean VRearLogic1[5][9];
boolean VRearLogic2[5][9];
boolean VFrontTopLogic[5][9];
boolean VFrontLowerLogic[5][9];

unsigned char RearLogic0[6];
unsigned char RearLogic1[6];
unsigned char RearLogic2[6];
unsigned char FrontTopLogic[6];
unsigned char FrontLowerLogic[6];



//
// CBI/DataPort
// you will need to adjust these variables for your CBI/DataPort configuration
boolean ESBmode = true;   // operates like charging scene on Dagobah otherwise operates like Chris James' line mode
boolean monitorVCC = false;// Set this to true to monitor voltage
boolean testCBILEDs = false; // Set to true to test every LED on initial startup
boolean fakeCharge = true; // Set to true to make it look like its charging even if your not
unsigned long timeNew = 0;

float vout = 0.0;       // for voltage out measured analog input
int value = 0;          // used to hold the analog value coming out of the voltage divider
float vin = 0.0;        // voltage calulcated... since the divider allows for 15 volts

float greenVCC = 12.0;    // Green LED l21 on if above this voltage
float yellowVCC = 11.3;    // Yellow LED l22 on if above this voltage & below greenVCC... below turn on only RED l23
float chargeVCC = 12.2;    // Voltage at which system is charging


//
// CBI/DP Display settings...
int cbibright=8; // CBI brightness (0 to 15) 8 default
int dpbright=6;  // DataPort brightness (0 to 15) 5 default
unsigned long delayCBItime=300;
unsigned long delayDPtime=200;
int DPdev=1;

//
// Logic Display settings...
int rldbright=7;  // RLD brightness (0 to 15)
int fldbright=7;   // FLD brightness (0 to 15)
int logicdelay=105; // adjust this number to change the speed of the logic patterns (lower is faster) OLD default=175, new=105
//
// PSI settings...
int psibright=15; // PSI brightness (0 to 15)
int red=2000;  // time (in milliseconds) to stay red (default is 2000)
int blue=1000; // time (in milliseconds) to stay blue (default is 1000)
int rbSlide=200; // mts - time to transition between red and blue in slide mode
int yellow=1000; // time (in milliseconds) to stay yellow (default is 1000)
int green=2000; // time (in milliseconds) to stay green (default is 2000)
int ygSlide=200; // mts - time to transition between yellow and green in slide mode
unsigned long RPSItimeNew=0;
unsigned long FPSItimeNew=0;
int BPM = 90;
unsigned long DanceBeatDelay=660;// Take the song you want 60/BPM

// this pattern is the secret sauce for the PSI...  I had to declare global here cause for some reason you can't assign it inside a class.
//static const int patternAtStage[] = { B01010000, B11010000, B10010000, B10110000, B10100000, B00100000, B01100000, B01000000, B01010000 };
static const int patternAtStage[] = { B01010101, B11010101, B10010101, B10110101, B10101010, B00101010, B01101010, B01010101, B01010101 };

//
// We're done with setting numbers.
// We shouldn't have to change anything below here.
//

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
    
    // Michael Smith - modified this to have a 3rd delay for the transition time between red and blue
    // the slide cycle is RED, LtoR transition to blue (3 steps), BLUE, LtoR transition to red (3 steps), RED.
    // then it reverses direction and does that cycle in reverse.
        
    // the LED pattern on a traditional PSI are
    // x R B x
    // R R B B
    // R R B B
    // x R B x
    
    // the LED pattern on my sliding PSI is checkerboarded:
    // pattern:      red on:       blue on:
    // x R B x       0101----      1010----
    // R B R B       1010----      0101----
    // B R B R       0101----      1010----
    // x B R x       1010----      0101----
    
    // even lines are the inverse of the odd lines.  So even = ~odd;  (bitwise NOT)
        
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
        if (PSISlide)
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
            control.setRow(device,4,patternAtStage[stage]);
            
        }
        else
        {
            // this is the original flip flop and assumes the original LED pattern
            if (state && (timeNow - timeLast)  < delay1) return;
            if (!state && (timeNow - timeLast) < delay2) return;
            
            timeLast = timeNow;
            state=!state;
            
            int cols=B11000000;
            if (state) cols=B00110000;
            for (int row=0; row<4; row++)
                control.setRow(device,row,cols);
        }
    }
};


//PSI psiFront=PSI(2500, 1700, 2); //2000 ms on red, 1000 ms on blue.  device is 2
//PSI psiRear =PSI(1700, 2500, 3); //1000 ms on yellow, 2000 ms on green.  device is 3

PSI psiFront=PSI(red, blue, rbSlide, fpsidev); //2000 ms on red, 1000 ms on blue.
PSI psiRear =PSI(yellow, green, ygSlide, rpsidev); //1000 ms on yellow, 2000 ms on green.

LedControl lc1=LedControl(2,4,8,4); //Rear Logic & Rear PSI chain
LedControl lc2=LedControl(2,4,8,3); //Front Logics & Front PSI chain
LedControl lc3=LedControl(2,4,8,2); //Charge Bay Indicator & Data Port chain

unsigned long t;
int DeviceCount = 0;

void setup()
{ 
  if (LogicsI2CAdapter==1) {Wire.begin(10);DeviceCount=4;// Start I2C Bus as Master I2C Address 10 (Assigned RLD/RPSI)
  for(int dev=0;dev<lc1.getDeviceCount();dev++)
  {
    lc1.shutdown(dev, false); //take the device out of shutdown (power save) mode
    lc1.clearDisplay(dev);
  }
  }

  if (LogicsI2CAdapter==2) {Wire.begin(11);DeviceCount=3;// Start I2C Bus as Master I2C Address 11 (Assigned FLD/FPSI)
  for(int dev=0;dev<lc2.getDeviceCount();dev++)
  {
    lc2.shutdown(dev, false); //take the device out of shutdown (power save) mode
    lc2.clearDisplay(dev);
  }
  }

  if (LogicsI2CAdapter==3) {Wire.begin(14);DeviceCount=2;// Start I2C Bus as Master I2C Address 14 (Assigned CBI/DP)
  for(int dev=0;dev<lc3.getDeviceCount();dev++)
  {
    lc3.shutdown(dev, false); //take the device out of shutdown (power save) mode
    lc3.clearDisplay(dev);
  }
  }
  
  Wire.onReceive(receiveEvent);            // register event so when we receive something we jump to receiveEvent();

if (LogicsI2CAdapter==1){
  lc1.setIntensity(0, rldbright); //RLD
  lc1.setIntensity(1, rldbright); //RLD
  lc1.setIntensity(2, rldbright); //RLD
  lc1.setIntensity(3, psibright); //Rear PSI
}

if (LogicsI2CAdapter==2){
  lc2.setIntensity(0, fldbright);  //FLD
  lc2.setIntensity(1, fldbright);  //FLD  
  lc2.setIntensity(2, psibright); //Front PSI
}

if (LogicsI2CAdapter==3){
  lc3.setIntensity(0, cbibright);  //CBI
  lc3.setIntensity(1, dpbright);  //DataPort  
}

//  pinMode(13, OUTPUT);
//  digitalWrite(13, HIGH);

  //Optional PSI Static HP lights on constant
if (LogicsI2CAdapter==1){
  lc1.setRow(3,4,255); //rear psi if v3.2 PSI, this will cause a little blink initially
  lc1.setRow(3,5,255); //only effects v3.2 PSI
  DisplayString (1, 1,1,1, 0,0, 0, "# LOGICS I2C v1.4    ");

}
if (LogicsI2CAdapter==2){
  lc2.setRow(2,4,255); //front psi if v3.2 PSI, this will cause a little blink initially
  lc2.setRow(2,5,255); //only effects v3.2 PSI
}


if (LogicsI2CAdapter==3){
// Nothing to set currently
}

//  pinMode(17, OUTPUT);  // Set RX LED as an output

//  DisplayString (1, 1,1,1, 0,0, 0, "#  LOGICS I2C v1.4");
//  DisplayString (4, 2,2,2, 0,1, 1, "D #");
//delay(500);
// RandomDelay        (2, 0,0,0, 50);
//FakeAudioEQ(2, 0,0,0, 50);

//DisplayString    (1, 0, 0, 0, 1,0, 0, "<<<<<<");
//DisplayString    (1, 0, 0, 0, 1,0, 0, "@@@@@@@");
//DisplayString    (1, 0, 0, 0, 1,0, 0, ">>>>>>>");
//DisplayString    (1, 0, 0, 0, 1,0, 0, "~~~~~~");


}

void loop(){
  loopcount++;
  unsigned long timeNew= millis();
  if (LogicsI2CAdapter==1){
    psiRear.Animate(timeNew, lc1);
    animateRearLogic(timeNew);
  }
  if (LogicsI2CAdapter==2){
    psiFront.Animate(timeNew, lc2);
    animateFrontLogic0(timeNew);
    animateFrontLogic1(timeNew);
  }

  if (LogicsI2CAdapter==3){
    timeNew= millis();
    animateDPLogic(timeNew);
    if (monitorVCC == true){
     getVCC();          // Get Battery Status via the voltage divider circuit & display status
     }

     if (ESBmode==false) {operatingSEQ();
     lc3.clearDisplay(0);
     }
     else if (vin >= chargeVCC or fakeCharge==true) {ESBoperatingSEQ();}
     
   }
//  if (loopcount > 120000){
//     LeiaMessage();
//     Disco(20,90);
//     alarm();
//     loopcount=0;
// }
  
}

void animateRearLogic(unsigned long elapsed)
{
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < logicdelay) return;
  timeLast = elapsed; 

  if (LogicsI2CAdapter==1){
  for (int dev=0; dev<3; dev++)
    for (int row=0; row<6; row++)
      lc1.setRow(dev,row,random(0,256)); 
  }    
}

void animateFrontLogic0(unsigned long elapsed)
{
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < logicdelay) return;
  timeLast = elapsed; 

  if (LogicsI2CAdapter==2){
  int dev=0;
    for (int row=0; row<6; row++)
      lc2.setRow(dev,row,random(0,256)); 
  }     
}
void animateFrontLogic1(unsigned long elapsed)
{
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < logicdelay) return;
  timeLast = elapsed; 

  if (LogicsI2CAdapter==2){
  int dev=1;
    for (int row=0; row<6; row++)
      lc2.setRow(dev,random(0,6),random(0,256)); 
  }     
}




// function that executes whenever data is received from an I2C master
// this function is registered as an event, see setup()
void receiveEvent(int eventCode) {
  int i2cEvent=Wire.read();
  sei();
     switch (i2cEvent) {
      case 2:
//       ledOFF();
       break;       
      case 3:
       alarm();
       break;
      case 9:
        MessageFX(31);
       break;
      case 10:
        Disco(10,120);
       break;
      case 11:
        SystemFailure1();
       break;
      case 12:
        SystemFailure2();
       break;
       case 13:
//        cantina();
       break;
       case 20:
//       resetLOGICS();
       break;
       case 40:
         
       break; 
       case 41:
         
       break; 
       case 42:
         
       break; 


      default: 
       // if nothing else matches, do the default
       // so we are going to do nothing... for that matter not even waste time
       break;
      }
}

void textLOGIC(){
  
}

void ledOFF(){
  if (LogicsI2CAdapter==1){
   lc1.clearDisplay(0);
   lc1.clearDisplay(1);
   lc1.clearDisplay(2);
   lc1.clearDisplay(3);
  }
  
  if (LogicsI2CAdapter==2){
   lc2.clearDisplay(0);
   lc2.clearDisplay(1);
   lc2.clearDisplay(2);
  }

  if (LogicsI2CAdapter==3){
   lc3.clearDisplay(0);
   lc3.clearDisplay(1);
  }
}

void alarm(){
  int RPSIalarmdelay=1000;
  int FPSIalarmdelay=1000;
  unsigned long timeNew=millis();
  
 if (LogicsI2CAdapter==1){
   // bunch of code here for...
 }

 
  if (LogicsI2CAdapter==2){
   if (PSISlide==false){
    lc2.clearDisplay(0);
    lc2.clearDisplay(1);

    for (int cnt=0; cnt<6; cnt++){
      unsigned long flip = timeNew + FPSIalarmdelay;
      while (timeNew < flip) {
        for (int row=0; row<4; row++)
        lc2.setRow(fpsidev,row,B00110000);
        timeNew= millis();
        animateFrontLogic0(timeNew);
        animateFrontLogic1(timeNew);
    }
    unsigned long flop = (timeNew + (FPSIalarmdelay/2));
    while (timeNew < flop) {
        for (int row=0; row<4; row++)
        lc2.setRow(fpsidev,row,B00000000);
        timeNew= millis();
        animateFrontLogic0(timeNew);
        animateFrontLogic1(timeNew);
    }
   }
  }


   }
   if (PSISlide==true){
    //lc2.clearDisplay(0);
    //lc2.clearDisplay(1);

    for (int cnt=0; cnt<6; cnt++){
      unsigned long flip = timeNew + FPSIalarmdelay;
      while (timeNew < flip) {
        lc2.setRow(fpsidev,0,B10101010);
        lc2.setRow(fpsidev,1,B01010101);
        lc2.setRow(fpsidev,2,B10101010);
        lc2.setRow(fpsidev,3,B01010101);
        lc2.setRow(fpsidev,4,B10101010);
        timeNew= millis();
        animateFrontLogic0(timeNew);
        animateFrontLogic1(timeNew);
    }
    unsigned long flop = (timeNew + (FPSIalarmdelay/2));
    while (timeNew < flop) {
        for (int row=0; row<5; row++)
        lc2.setRow(fpsidev,row,B00000000);
        timeNew= millis();
        animateFrontLogic0(timeNew);
        animateFrontLogic1(timeNew);
    }
   }
  }
 

 if (LogicsI2CAdapter==3){
 }
  
  
}


void MessageFX(int MessageTime){
//   DisplayString(1, 1,1,1, 0,0, 0, "OBI");
  FPSItimeNew= millis();
  unsigned long timeNew= millis();
 if (LogicsI2CAdapter==2){ 
  unsigned long scrollt=millis();
   mt=scrollt + (MessageTime*1000);
   lc2.clearDisplay(0);
   lc2.clearDisplay(1);
   lc2.clearDisplay(2);
  lc2.setRow(0,0,B11111111);
  lc2.setRow(0,4,B11111111);
  while(scrollt < mt){ 
    for (int col=9; col>0; col--){
      lc2.setColumn(0,col,B11111000); 
      delay(50);
      lc2.setColumn(0,col,B10001000);
      delay(50);
      FPSItimeNew= millis();
      psiFront.Animate(FPSItimeNew, lc2);
      timeNew= millis();
//      animateFrontLogic0(timeNew);
      animateFrontLogic1(timeNew); //
    }
 //   lc2.setColumn(0,0,B11111000); 
 //   delay(50);
 //   lc2.setColumn(0,0,B10001000);
 //   delay(50);
    scrollt=millis();
    }
 }
}


void Disco(int dancetime, int bpm){ // so 20, 90 is 20 seconds @ 90 bpm
  DanceBeatDelay = 60000/bpm;
  FPSItimeNew= millis();
  unsigned long timeNew= millis();
  unsigned long gottadance = (dancetime*1000) + timeNew;
 
 if (LogicsI2CAdapter==1){ 
    RPSItimeNew= millis();
 }
 
 if (LogicsI2CAdapter==2){ 
    FPSItimeNew= millis();
   if (PSISlide==false){
    lc2.clearDisplay(0);
    lc2.clearDisplay(1);
    lc2.clearDisplay(2);

    while (timeNew < gottadance){  
      unsigned long flip = timeNew + DanceBeatDelay;
      while (timeNew < flip) {
        for (int row=0; row<4; row++)
        lc2.setRow(fpsidev,row,B00110000);
        for (int row=0; row<5; row++){
          lc2.setRow(fld1dev,row,B11111111);
          lc2.setRow(fld2dev,row,B00000000);
        }
        timeNew= millis();
      }
    unsigned long flop = timeNew + DanceBeatDelay;
    while (timeNew < flop) {
        for (int row=0; row<4; row++)
        lc2.setRow(fpsidev,row,B11000000);
        for (int row=0; row<5; row++){
          lc2.setRow(fld1dev,row,B00000000);
          lc2.setRow(fld2dev,row,B11111111);
        }
        timeNew= millis();
    }
    timeNew=millis();
  }
 }
   
   if (PSISlide==true){
    lc2.clearDisplay(0);
    lc2.clearDisplay(1);
    lc2.clearDisplay(2);

    while (timeNew < gottadance){
      unsigned long flip = timeNew + DanceBeatDelay;
      while (timeNew < flip) {
        lc2.setRow(fpsidev,0,B01010101);
        lc2.setRow(fpsidev,1,B10101010);
        lc2.setRow(fpsidev,2,B01010101);
        lc2.setRow(fpsidev,3,B10101010);
        lc2.setRow(fpsidev,4,B01010101);
        for (int row=0; row<5; row++){
          lc2.setRow(fld1dev,row,B00000000);
          lc2.setRow(fld2dev,row,B11111111);
        }
        timeNew= millis();
    }  
    unsigned long flop = timeNew + DanceBeatDelay;
    while (timeNew < flop) {
        lc2.setRow(fpsidev,0,B10101010);
        lc2.setRow(fpsidev,1,B01010101);
        lc2.setRow(fpsidev,2,B10101010);
        lc2.setRow(fpsidev,3,B01010100);
        lc2.setRow(fpsidev,4,B10101010);
        for (int row=0; row<5; row++){
          lc2.setRow(fld1dev,row,B11111111);
          lc2.setRow(fld2dev,row,B00000000);
        }
        timeNew= millis();
    }
   }
   }
  }
 if (LogicsI2CAdapter==3){ 
// do nothing 
 }
}

void SystemFailure1(){
  if (LogicsI2CAdapter==1){ 
    for (int cnt=0; cnt<900; cnt++)
      lc1.setLed(random(0,4),random(0,6),random(0,8),false);delay(20);
    ledOFF();
    delay(5000);
  }
  if (LogicsI2CAdapter==2){ 
    for (int cnt=0; cnt<500; cnt++)
      lc2.setLed(random(0,2),random(0,6),random(0,8),false);delay(50);
    ledOFF();
    delay(5000);
    }
  if (LogicsI2CAdapter==3){ 
    for (int cnt=0; cnt<300; cnt++)
      lc3.setLed(random(0,1),random(0,6),random(0,8),false);delay(80);
    ledOFF();
    delay(5000);
  }
}

void SystemFailure2(){
  if (LogicsI2CAdapter==1){ 
    Failure(7, 5,5,5, 8);
//    delay(1000);
//    LogicOffDelay (6, 0,0,0, 1, 20);
//    FailureReverse(8, 4,4,4, 2);

  }
  if (LogicsI2CAdapter==2){ 
    Failure(7, 5,5,5, 8);
//    delay(1000);
 //   FailureReverse(8, 4,4,4, 2);
    }
  if (LogicsI2CAdapter==3){ 
    for (int cnt=0; cnt<300; cnt++)
      lc3.setLed(random(0,1),random(0,6),random(0,8),false);delay(80);
    ledOFF();
//    delay(5000);
  }
}


void CBIl21on() // Voltage GREEN
{
lc3.setLed(0,4,5,true);
}

void CBIl22on()// Voltage Yellow
{
lc3.setLed(0,5,5,true);
}

void CBIl23on()// Voltage RED
{
lc3.setLed(0,6,5,true);
}


void CBIl21off() // Voltage Green 
{
lc3.setLed(0,4,5,false);
}

void CBIl22off() // Voltage Yellow
{
lc3.setLed(0,5,5,false);
}

void CBIl23off() // Voltage Red
{
lc3.setLed(0,6,5,false);
}


void operatingSEQ() {          // used when monitorVCC == true
 // Step 0
  lc3.setRow(0,0,B11111000);
  lc3.setRow(0,1,B11111000);
  lc3.setRow(0,2,B11111000);
  lc3.setRow(0,3,B00000000);
  delay(delayCBItime);
 // Step 1 
  lc3.setRow(0,1,B00000000);
  delay(delayCBItime);
 // Step 2
  lc3.setRow(0,1,B11111000);
  lc3.setRow(0,2,B00000000);
  lc3.setRow(0,3,B11111000);
  delay(delayCBItime);
 // Step 3
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B11111000);
  lc3.setRow(0,2,B11111000); 
  lc3.setRow(0,3,B00000000);
  delay(delayCBItime);
// Step 4
  lc3.setRow(0,0,B11111000);
  lc3.setRow(0,1,B11111000); 
  lc3.setRow(0,2,B11111000);
  lc3.setRow(0,3,B11111000); 
  delay(300);
// Step 5
  lc3.setRow(0,0,B11111000);
  lc3.setRow(0,1,B00000000);
  lc3.setRow(0,1,B00000000);
  lc3.setRow(0,3,B00000000);
  delay(delayCBItime);
// Step 6
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B11111000); 
  lc3.setRow(0,2,B00000000);
  lc3.setRow(0,3,B00000000);
  delay(delayCBItime);
// Step 7
  lc3.setRow(0,0,B11111000);
  lc3.setRow(0,1,B11111000);
  lc3.setRow(0,2,B00000000);
  lc3.setRow(0,3,B00000000);
  delay(delayCBItime);
  // Step 8
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B11111000);
  lc3.setRow(0,2,B00000000);
  lc3.setRow(0,3,B11111000);
  delay(delayCBItime);
  // Step 9
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B11111000);
  lc3.setRow(0,2,B11111000);
  lc3.setRow(0,3,B11111000);
  delay(delayCBItime);
}

void blankCBI() {          // used when monitorVCC == true
 // Step 0
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00000000);
  lc3.setRow(0,2,B00000000);
  lc3.setRow(0,3,B00000000);
}



void ESBoperatingSEQ() {          // used when ESBmode == true
 // Step 0
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00000000);
  lc3.setRow(0,2,B00010000);
  lc3.setRow(0,3,B00010000);
  delay(delayCBItime);

 // Step 1
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00000000);
  lc3.setRow(0,2,B00010000);
  lc3.setRow(0,3,B00000000);
  if (monitorVCC == false){ 
    CBIl21on();
  }
  delay(delayCBItime);

 // Step 2 
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00010000);
  lc3.setRow(0,3,B00000000);
  delay(delayCBItime);
 
  // Step 3
  lc3.setRow(0,0,B10000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00010000); 
  lc3.setRow(0,3,B11000000);
  if (monitorVCC == false){ 
    CBIl21off();
  }
  delay(delayCBItime);
 
// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);


 // Step 4
  lc3.setRow(0,0,B00010000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00010000); 
  lc3.setRow(0,3,B11000000);
  delay(delayCBItime);

 // Step 5
  lc3.setRow(0,0,B00010000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00010000); 
  lc3.setRow(0,3,B01000000);
  if (monitorVCC == false){ 
    CBIl21on();
  }
  delay(delayCBItime);

  // Step 6
  lc3.setRow(0,0,B00010000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00010000);
  lc3.setRow(0,3,B00000000);
  delay(delayCBItime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);


 // Step 7
  lc3.setRow(0,0,B00010000);
  lc3.setRow(0,1,B00000000);
  lc3.setRow(0,2,B00010000);
  lc3.setRow(0,3,B00010000);
  if (monitorVCC == false){ 
    CBIl21off();
  }
  delay(delayCBItime);

 // Step 8
  lc3.setRow(0,0,B00010000);
  lc3.setRow(0,1,B00110000);
  lc3.setRow(0,2,B00000000);
  lc3.setRow(0,3,B00110000);
  delay(delayCBItime);

 // Step 9
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00000000); 
  lc3.setRow(0,3,B00000000);
  delay(delayCBItime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);


 // Step 10
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00000000);
  lc3.setRow(0,2,B00010000); 
  lc3.setRow(0,3,B00000000);
  if (monitorVCC == false){ 
    CBIl22on();
    CBIl21on();
  }
  delay(delayCBItime);


 // Step 11
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00010000); 
  lc3.setRow(0,3,B00000000);
  delay(delayCBItime);

 // Step 12
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00000000); 
  lc3.setRow(0,3,B00010000);
  if (monitorVCC == false){ 
    CBIl22off();
    CBIl21off();
  }
  delay(delayCBItime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);


 // Step 13
  lc3.setRow(0,0,B10000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00010000); 
  lc3.setRow(0,3,B10110000);
  delay(delayCBItime);

 // Step 14
  lc3.setRow(0,0,B10000000);
  lc3.setRow(0,1,B01010000);
  lc3.setRow(0,2,B00010000); 
  lc3.setRow(0,3,B10100000);
  if (monitorVCC == false){ 
    CBIl21on();
  }
  delay(delayCBItime);



 // Step 15
  lc3.setRow(0,0,B10000000);
  lc3.setRow(0,1,B01010000);
  lc3.setRow(0,2,B00010000); 
  lc3.setRow(0,3,B10000000);
  delay(delayCBItime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);

 // Step 16
  lc3.setRow(0,0,B10000000);
  lc3.setRow(0,1,B00100000);
  lc3.setRow(0,2,B00010000); 
  lc3.setRow(0,3,B11000000);
  if (monitorVCC == false){ 
    CBIl21off();
  }
  delay(delayCBItime);

 // Step 17
  lc3.setRow(0,0,B10000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00010000); 
  lc3.setRow(0,3,B11000000);
  delay(delayCBItime);

 // Step 18
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00000000); 
  lc3.setRow(0,3,B00000000);
  if (monitorVCC == false){ 
    CBIl21on();
  }
  delay(delayCBItime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);

 // Step 19
  lc3.setRow(0,0,B10000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B00010000); 
  lc3.setRow(0,3,B01100000);
  delay(delayCBItime);

 // Step 20
  lc3.setRow(0,0,B10000000);
  lc3.setRow(0,1,B10000000);
  lc3.setRow(0,2,B01010000); 
  lc3.setRow(0,3,B00000000);
  if (monitorVCC == false){ 
    CBIl21off();
  }
  delay(delayCBItime);

 // Step 21
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00100000);
  lc3.setRow(0,2,B01011000); 
  lc3.setRow(0,3,B01000000);
  delay(delayCBItime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);



 // Step 22
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00011000);
  lc3.setRow(0,2,B01011000); 
  lc3.setRow(0,3,B01000000);
  if (monitorVCC == false){ 
    CBIl21on();
  }
  delay(delayCBItime);

 // Step 25
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B01011000); 
  lc3.setRow(0,3,B01000000);
  delay(delayCBItime);

 // Step 26
  lc3.setRow(0,0,B10001000);
  lc3.setRow(0,1,B00100000);
  lc3.setRow(0,2,B01001000); 
  lc3.setRow(0,3,B01000000);
  if (monitorVCC == false){ 
    CBIl21off();
  }
  delay(delayCBItime);

 // Step 27
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00000000);
  lc3.setRow(0,2,B00001000); 
  lc3.setRow(0,3,B00000000);
  delay(delayCBItime);

 // Step 28
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B01001000); 
  lc3.setRow(0,3,B01000000);
  if (monitorVCC == false){ 
    CBIl21on();
  }
  delay(delayCBItime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);


 // Step 29
  lc3.setRow(0,0,B00000000);
  lc3.setRow(0,1,B00010000);
  lc3.setRow(0,2,B01001000); 
  lc3.setRow(0,3,B01000000);
  delay(delayCBItime);

}

  
void animateDPLogic(unsigned long elapsed)
{
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < 750) return;
  timeLast = elapsed; 
   for (int row=0; row<6; row++)
      lc3.setRow(DPdev,row,random(0,256));
}  
  


void getVCC(){
// value = analogRead(analoginput);// this must be between 0.0 and 5.0 - otherwise you'll let the blue smoke out of your ar
// vout= (value * 5.0)/1024.0;  //voltage coming out of the voltage divider
// vin = (vout / (R2/(R1+R2)))+1;  //voltage to display
 
 

// Serial.print("Volt Out = ");                                  // DEBUG CODE
// Serial.print(vout, 1);   //Print float "vin" with 1 decimal   // DEBUG CODE

   
// Serial.print("\tVIN Calc = ");                             // DEBUG CODE
// Serial.println(vin, 1);   //Print float "vin" with 1 decimal   // DEBUG CODE
 
 if (vin >= greenVCC) {
   CBIl21on();
   CBIl22on();
   CBIl23on();
//   Serial.println("\tTurn on Led l23 - GREEN");              // DEBUG CODE
 } else if (vin >= yellowVCC) {
     CBIl21on();
     CBIl22on();
     CBIl23off();
//     Serial.println("\tTurn on Led l22 - YELLOW");           // DEBUG CODE
 } else {              // voltage is below yellowVCC value, so turn on l23.
     CBIl21on();
     CBIl22off();
     CBIl23off();
//     Serial.println("\tTurn on Led l21 - RED");              // DEBUG CODE

 }
}


void IheartUSEQ() {
 // Step 0
  lc3.setRow(0,0,B01110000);
  lc3.setRow(0,1,B00100000);
  lc3.setRow(0,2,B00100000);
  lc3.setRow(0,3,B01110000);
  delay(1000);
  // Step 1
  lc3.setRow(0,0,B01010000);
  lc3.setRow(0,1,B11111000);
  lc3.setRow(0,2,B01110000);
  lc3.setRow(0,3,B00100000);
  delay(1000);
  // Step 1
  lc3.setRow(0,0,B01010000);
  lc3.setRow(0,1,B01010000);
  lc3.setRow(0,2,B01010000);
  lc3.setRow(0,3,B01110000);
  delay(1000);
}

void SetColumn(int LEDCol, unsigned char ColState){
  for(int LEDRow=0; LEDRow<5; LEDRow++){
    if(LEDCol<9){
      VRearLogic2[LEDRow][LEDCol]=((ColState >> LEDRow) & 1);
    } else if(LEDCol<18){
      VRearLogic1[LEDRow][LEDCol-9]=((ColState >> LEDRow) & 1);
      VFrontTopLogic[LEDRow][LEDCol-9]=((ColState >> LEDRow) & 1);
      VFrontLowerLogic[LEDRow][LEDCol-9]=((ColState >> LEDRow) & 1);
    } else{
      VRearLogic0[LEDRow][LEDCol-18]=((ColState >> LEDRow) & 1);
    }
  }
}

void MapBoolGrid(){
  for(int PrintRow=0; PrintRow<5; PrintRow++){
    for(int LED=0; LED<8; LED++){
      if(LED<5){
        RearLogic0[5]=2*RearLogic0[5]+VRearLogic0[LED][0]; //9
        RearLogic1[5]=2*RearLogic1[5]+VRearLogic1[LED][0];
        RearLogic2[5]=2*RearLogic2[5]+VRearLogic2[LED][0];
        FrontTopLogic[5]=2*FrontTopLogic[5]+VFrontTopLogic[LED][0];
        FrontLowerLogic[5]=2*FrontLowerLogic[5]+VFrontLowerLogic[4-LED][8];        
      }
      RearLogic0[PrintRow]=2*RearLogic0[PrintRow]+VRearLogic0[PrintRow][8-LED];
      RearLogic1[PrintRow]=2*RearLogic1[PrintRow]+VRearLogic1[PrintRow][8-LED];
      RearLogic2[PrintRow]=2*RearLogic2[PrintRow]+VRearLogic2[PrintRow][8-LED];
      FrontTopLogic[PrintRow]=2*FrontTopLogic[PrintRow]+VFrontTopLogic[PrintRow][8-LED];
      FrontLowerLogic[4-PrintRow]=2*FrontLowerLogic[4-PrintRow]+VFrontLowerLogic[PrintRow][LED];
    }
  }
  RearLogic0[5]=8*RearLogic0[5];
  RearLogic1[5]=8*RearLogic1[5];
  RearLogic2[5]=8*RearLogic2[5];
  FrontTopLogic[5]=8*FrontTopLogic[5];
  FrontLowerLogic[5]=8*FrontLowerLogic[5];
}


void RandomFeedOut(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedOutRandom){
  InPos=0;
//  PSIReset();
  for(int Column=0; Column<27; Column++){
    boolean Temp8[5], Temp17[5];
    for(int Row=0; Row<5; Row++){
      Temp8[Row]=VRearLogic2[Row][8];
      Temp17[Row]=VRearLogic1[Row][8];
      for(int LED=0; LED<8; LED++){
          VRearLogic2[Row][8-LED]=VRearLogic2[Row][7-LED];
          VRearLogic1[Row][8-LED]=VRearLogic1[Row][7-LED];
          VFrontTopLogic[Row][8-LED]=VFrontTopLogic[Row][7-LED];
          VFrontLowerLogic[Row][8-LED]=VFrontLowerLogic[Row][7-LED];
          VRearLogic0[Row][8-LED]=VRearLogic0[Row][7-LED];
      }
      VRearLogic0[Row][0]=Temp17[Row];
      VRearLogic1[Row][0]=Temp8[Row];
      VFrontTopLogic[Row][0]=Temp8[Row];
      VFrontLowerLogic[Row][0]=Temp8[Row];
      VRearLogic2[Row][0]=((B00000 >> Row) & 1);
    }
    if(FeedOutRandom==1){
      for(int RandomApply=0; RandomApply<27; RandomApply++){
        if(RandomApply<=InPos){
          SetColumn(RandomApply, random(32)|random(32)|random(32));   
        }
      }
    } 
    InPos=InPos+1;
    MapBoolGrid();
//    SetPSIsHPs(PSI, FrontHP, RearHP, TopHP);
    displayLOGIC();
  }
}

void displayLOGIC(){
  if (LogicsI2CAdapter==1) {
  for(int i=0; i<6; i++){
    lc1.setRow(0, i, RearLogic0[i]);
    lc1.setRow(1, i, RearLogic1[i]);
    lc1.setRow(2, i, RearLogic2[i]);
    delay(DelayTime);
  }
  }
  if (LogicsI2CAdapter==2) {
    for(int i=0; i<6; i++){
    lc2.setRow(0, i, FrontTopLogic[i]);
    lc2.setRow(1, i, FrontLowerLogic[i]); 
    delay(DelayTime);
  }
  }
}


void DisplayString(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom, boolean FeedOutRandom, boolean FeedOut, char textString[]){
  OutPos=0;
//  PSIReset();
  for (unsigned char i=0; i<strlen(textString); i++) {
    drawLetter(PSI, FrontHP, RearHP, TopHP, FeedInRandom, textString[i]);
  }  
  if(FeedOut==true){
    RandomFeedOut(PSI, FrontHP, RearHP, TopHP, FeedOutRandom);
  }
}

void drawLetter(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom, char let) {
  switch (let) {
    case '0': Feed0(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break; 
    case '1': Feed1(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
    case '2': Feed2(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
    case '3': Feed3(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case '4': Feed4(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
    case '5': Feed5(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
    case '6': Feed6(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case '7': Feed7(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
    case '8': Feed8(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
    case '9': Feed9(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'A': FeedA(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'B': FeedB(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'C': FeedC(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'D': FeedD(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'E': FeedE(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'F': FeedF(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'G': FeedG(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'H': FeedH(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'I': FeedI(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'J': FeedJ(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'K': FeedK(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'L': FeedL(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'M': FeedM(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'N': FeedN(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'O': FeedO(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'P': FeedP(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'Q': FeedQ(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'R': FeedR(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
    case 'S': FeedS(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'T': FeedT(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
    case 'U': FeedU(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'V': FeedV(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'W': FeedW(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'X': FeedX(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'Y': FeedY(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case 'Z': FeedZ(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break; 
//    case 'a': FeedA(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'b': FeedB(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'c': FeedC(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'd': FeedD(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'e': FeedE(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'f': FeedF(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'g': FeedG(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'h': FeedH(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'i': FeedI(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'j': FeedJ(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'k': FeedK(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'l': FeedL(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'm': FeedM(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'n': FeedN(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'o': FeedO(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'p': FeedP(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'q': FeedQ(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'r': FeedR(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
//    case 's': FeedS(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 't': FeedT(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
//    case 'u': FeedU(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'v': FeedV(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'w': FeedW(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'x': FeedX(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'y': FeedY(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
//    case 'z': FeedZ(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case ' ': FeedSpace(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case '-': FeedDash(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case '!': FeedExc(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
    case '.': FeedStop(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
    case '+': FeedApp(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;  
    case '?': FeedQues(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case '#': FeedR2(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case '~': SineWave(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case '>': HeartBeat(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case '<': Graph(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case '@': Scope(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case '^': Message(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;
    case '_': FlatHeartBeat(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;
    case '£': Packman(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;   
    case '$': Ghost(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;
    case '&': Dotted(PSI, FrontHP, RearHP, TopHP, FeedInRandom); break;
    default:return;
  }
}


void Feed0(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);  // left colunm  from bottom to top e.g 10000 is bottom led on
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Feed1(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Feed2(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Feed3(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Feed4(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Feed5(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Feed6(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Feed7(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00011);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Feed8(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Feed9(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedA(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedB(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedC(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedD(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedE(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedF(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedG(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedH(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedI(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedJ(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedK(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11011);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedL(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedM(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedN(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedO(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedP(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedQ(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedR(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedS(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedT(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedU(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedV(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedW(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedX(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11011);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11011);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedY(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00011);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00011);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedZ(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedDash(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedSpace(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedExc(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedStop(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedApp(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00011);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedQues(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void FeedR2(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10011);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void SineWave(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10100);
}

void HeartBeat(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);  
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
}

void FlatHeartBeat(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100); 
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100); 
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100); 
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100); 
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100); 
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100); 
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100); 
}

void Graph(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11100);  
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11110);
}

void Scope(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);  
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00010);
}

void Message(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10011);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10011);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Packman(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B10001);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11011);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Ghost(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11111);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B01101);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B11110);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}

void Dotted(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom){
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00100);
  FeedGridLeft(PSI, FrontHP, RearHP, TopHP, FeedInRandom, B00000);
}


void FeedGridLeft(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, boolean FeedInRandom, unsigned char NewCol){
  boolean Temp8[5], Temp17[5];
  for(int Row=0; Row<5; Row++){
    Temp8[Row]=VRearLogic2[Row][8];
    Temp17[Row]=VRearLogic1[Row][8];
    for(int LED=0; LED<8; LED++){
        VRearLogic2[Row][8-LED]=VRearLogic2[Row][7-LED];
        VRearLogic1[Row][8-LED]=VRearLogic1[Row][7-LED];
        VFrontTopLogic[Row][8-LED]=VFrontTopLogic[Row][7-LED];
        VFrontLowerLogic[Row][8-LED]=VFrontLowerLogic[Row][7-LED];
        VRearLogic0[Row][8-LED]=VRearLogic0[Row][7-LED];
    }
    VRearLogic0[Row][0]=Temp17[Row];
    VRearLogic1[Row][0]=Temp8[Row];
    VFrontTopLogic[Row][0]=Temp8[Row];
    VFrontLowerLogic[Row][0]=Temp8[Row];
    VRearLogic2[Row][0]=((NewCol >> Row) & 1);
  }
  if(FeedInRandom==1){
    if(OutPos<27){
      OutPos=OutPos+1;
      for(int RandomApply=0; RandomApply<27; RandomApply++){
        if(RandomApply>=OutPos){
          SetColumn(RandomApply, random(32)|random(32)|random(32));     
        }
      }
    }
  }
  MapBoolGrid();
//  SetPSIsHPs(PSI, FrontHP, RearHP, TopHP);
  displayLOGIC();
}

void RandomDelay(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, int DelayFactor){
//  PSIReset();
  for(int i=0; i<DelayFactor; i++){
    RandomLOGIC();
 //   SetPSIsHPs(PSI, FrontHP, RearHP, TopHP);
    displayLOGIC();
  }
}

void RandomLOGIC(){
  if (LogicsI2CAdapter==1) {
  for(unsigned char i=0; i<6; i++){
    RearLogic0[i]=(random(256)|random(256)|random(256));
    RearLogic1[i]=(random(256)|random(256)|random(256));
    RearLogic2[i]=(random(256)|random(256)|random(256));
  }
  }
  if (LogicsI2CAdapter==2) {
  for(unsigned char i=0; i<6; i++){
    FrontTopLogic[i]=(random(256)|random(256)|random(256));
    FrontLowerLogic[i]=(random(256)|random(256)|random(256));
  }
  }

}


void PrintFlatlineAll(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, int DelayFactor){
  PrintSymbolAll(PSI, FrontHP, RearHP, TopHP, DelayFactor,
    B00100,  // Bits from bottom left of display unit, PrintSymbol(2, .... is central column, so both front and centre rear
    B00100,
    B00100,
    B00100,
    B00100,
    B00100,
    B00100,
    B00100,
    B00100);
}

void PrintGraphAll(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, int DelayFactor){
  PrintSymbolAll(PSI, FrontHP, RearHP, TopHP, DelayFactor,
    B00100,  // Bits from bottom left of display unit, PrintSymbol(2, .... is central column, so both front and centre rear
    B00010,
    B00100,
    B01000,
    B00100,
    B00100,
    B01000,
    B10000,
    B01000);
}

void FakeAudioEQ(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, int DelayFactor){
  PrintSymbolAll(PSI, FrontHP, RearHP, TopHP, DelayFactor,
    B10000,  // Bits from bottom left of display unit, PrintSymbol(2, .... is central column, so both front and centre rear
    B11000,
    B11100,
    B10000,
    B11000,
    B11100,
    B11111,
    B11000,
    B11100);
}


void PrintSymbolAll(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, int delayFactor, 
                       unsigned char One, unsigned char Two, unsigned char Three,    
                       unsigned char Four, unsigned char Five, unsigned char Six,     
                       unsigned char Seven, unsigned char Eight, unsigned char Nine){ 
//  PSIReset();
  for(int LoopDelay=0; LoopDelay<delayFactor; LoopDelay++){
    SetColumn(0, One);   // left colunm  from bottom to top e.g 10000 is bottom led on
    SetColumn(9, One);   // left colunm  from bottom to top e.g 10000 is bottom led on
    SetColumn(18, One);   // left colunm  from bottom to top e.g 10000 is bottom led on
    SetColumn(1, Two);
    SetColumn(10, Two);
    SetColumn(19, Two);
    SetColumn(2, Three); 
    SetColumn(11, Three);
    SetColumn(20, Three);  
    SetColumn(3, Four);
    SetColumn(12, Four);
    SetColumn(21, Four);
    SetColumn(4, Five);
    SetColumn(13, Five);
    SetColumn(22, Five);
    SetColumn(5, Six);
    SetColumn(14, Six);
    SetColumn(23, Six);
    SetColumn(6, Seven);
    SetColumn(15, Seven);
    SetColumn(24, Seven); 
    SetColumn(7, Eight);
    SetColumn(16, Eight);
    SetColumn(25, Eight);
    SetColumn(8, Nine);
    SetColumn(17, Nine);
    SetColumn(26, Nine);
    MapBoolGrid();
    displayLOGIC();
  }
}


void Failure(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, unsigned char Count){
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256)|random(256));
      RearLogic1[i]=(random(256)|random(256));
      RearLogic2[i]=(random(256)|random(256));
      FrontTopLogic[i]=(random(256)|random(256));
      FrontLowerLogic[i]=(random(256)|random(256));
    }
  displayLOGIC();
  }
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256));
      RearLogic1[i]=(random(256));
      RearLogic2[i]=(random(256));
      FrontTopLogic[i]=(random(256));
      FrontLowerLogic[i]=(random(256));
    }
    displayLOGIC();
  }
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256)&random(256));
      RearLogic1[i]=(random(256)&random(256));
      RearLogic2[i]=(random(256)&random(256));
      FrontTopLogic[i]=(random(256)&random(256));
      FrontLowerLogic[i]=(random(256)&random(256));
    }
    displayLOGIC();
  }
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256)&random(256)&random(256));
      RearLogic1[i]=(random(256)&random(256)&random(256));
      RearLogic2[i]=(random(256)&random(256)&random(256));
      FrontTopLogic[i]=(random(256)&random(256)&random(256));
      FrontLowerLogic[i]=(random(256)&random(256)&random(256));
    }
    displayLOGIC();
  }
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256)&random(256)&random(256)&random(256));
      RearLogic1[i]=(random(256)&random(256)&random(256)&random(256));
      RearLogic2[i]=(random(256)&random(256)&random(256)&random(256));
      FrontTopLogic[i]=(random(256)&random(256)&random(256)&random(256));
      FrontLowerLogic[i]=(random(256)&random(256)&random(256)&random(256));
    }
    displayLOGIC();
  }
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256)&random(256)&random(256)&random(256)&random(256));
      RearLogic1[i]=(random(256)&random(256)&random(256)&random(256)&random(256));
      RearLogic2[i]=(random(256)&random(256)&random(256)&random(256)&random(256));
      FrontTopLogic[i]=(random(256)&random(256)&random(256)&random(256)&random(256));
      FrontLowerLogic[i]=(random(256)&random(256)&random(256)&random(256)&random(256));
    }
    displayLOGIC();
  }
  ClearGrids();
  MapBoolGrid();
  displayLOGIC();
}

void FailureReverse(unsigned char PSI, unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, unsigned char Count){
  ClearGrids();
  MapBoolGrid();
  displayLOGIC();
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256)&random(256)&random(256)&random(256)&random(256));
      RearLogic1[i]=(random(256)&random(256)&random(256)&random(256)&random(256));
      RearLogic2[i]=(random(256)&random(256)&random(256)&random(256)&random(256));
      FrontTopLogic[i]=(random(256)&random(256)&random(256)&random(256)&random(256));
      FrontLowerLogic[i]=(random(256)&random(256)&random(256)&random(256)&random(256));
    }
    displayLOGIC();
  }  
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256)&random(256)&random(256)&random(256));
      RearLogic1[i]=(random(256)&random(256)&random(256)&random(256));
      RearLogic2[i]=(random(256)&random(256)&random(256)&random(256));
      FrontTopLogic[i]=(random(256)&random(256)&random(256)&random(256));
      FrontLowerLogic[i]=(random(256)&random(256)&random(256)&random(256));
    }
    displayLOGIC();
  }  
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256)&random(256)&random(256));
      RearLogic1[i]=(random(256)&random(256)&random(256));
      RearLogic2[i]=(random(256)&random(256)&random(256));
      FrontTopLogic[i]=(random(256)&random(256)&random(256));
      FrontLowerLogic[i]=(random(256)&random(256)&random(256));
    }
    displayLOGIC();
  }  
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256)&random(256));
      RearLogic1[i]=(random(256)&random(256));
      RearLogic2[i]=(random(256)&random(256));
      FrontTopLogic[i]=(random(256)&random(256));
      FrontLowerLogic[i]=(random(256)&random(256));
    }
//    SetPSIsHPs(PSI, FrontHP, RearHP, TopHP);
    displayLOGIC();
  }
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256));
      RearLogic1[i]=(random(256));
      RearLogic2[i]=(random(256));
      FrontTopLogic[i]=(random(256));
      FrontLowerLogic[i]=(random(256));
    }
//    SetPSIsHPs(PSI, FrontHP, RearHP, TopHP);
    displayLOGIC();
  }  
  for(unsigned char c=0; c<Count; c++){
    for(unsigned char i=0; i<6; i++){
      RearLogic0[i]=(random(256)|random(256));
      RearLogic1[i]=(random(256)|random(256));
      RearLogic2[i]=(random(256)|random(256));
      FrontTopLogic[i]=(random(256)|random(256));
      FrontLowerLogic[i]=(random(256)|random(256));
    }
//    SetPSIsHPs(PSI, FrontHP, RearHP, TopHP);
    displayLOGIC();
  }
}




void BarMeter(unsigned char PSI,  unsigned char FrontHP, unsigned char RearHP, unsigned char TopHP, int DelayFactor, int Percentage){
  ClearGrids();
  for(int c=0; c<DelayFactor; c++){
    int NoOfColumns=map(Percentage, 0, 100, 0, 27);
    for(int i=0; i<NoOfColumns; i++){
      SetColumnRear(26-i, B01110);
    }
    NoOfColumns=map(Percentage, 0, 100, 0, 9);
    for(int i=0; i<NoOfColumns; i++){
      SetColumnFront(8-i, B01110);
    }
    MapBoolGrid();
//    SetPSIsHPs(PSI, FrontHP, RearHP, TopHP);
    displayLOGIC();
  }
}

void SetColumnRear(int LEDCol, unsigned char ColState){
  for(int LEDRow=0; LEDRow<5; LEDRow++){
    if(LEDCol<9){
      VRearLogic2[LEDRow][LEDCol]=((ColState >> LEDRow) & 1);
    } else if(LEDCol<18){
      VRearLogic1[LEDRow][LEDCol-9]=((ColState >> LEDRow) & 1);
    } else{
      VRearLogic0[LEDRow][LEDCol-18]=((ColState >> LEDRow) & 1);
    }
  }
}

void SetColumnFront(int LEDCol, unsigned char ColState){
  for(int LEDRow=0; LEDRow<5; LEDRow++){
    VFrontTopLogic[LEDRow][LEDCol]=((ColState >> LEDRow) & 1);
    VFrontLowerLogic[LEDRow][LEDCol]=((ColState >> LEDRow) & 1);
  } 
}

void ClearGrids(){
  for(int ColBool=0; ColBool<9; ColBool++){
    SetColumn(ColBool, B00000);
    SetColumn(ColBool+9, B00000);
    SetColumn(ColBool+18, B00000);
  }
}
void HeartBeat(){
 
}



void fakeAudioEQ() {

}

void cantina(){
  if (LogicsI2CAdapter==1){
  DisplayString(1, 0, 0, 0, 1,0, 0, "<<<<<<<<");
  }
  if (LogicsI2CAdapter==2){
  DisplayString(1, 0, 0, 0, 1,0, 0, "<<<<<<<<");
  }
  
  
}
