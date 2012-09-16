// R-Series Charging Bay Indicators & DataPort Demo
// with Optional Voltage Monitor & Display
// CCv3 SA BY - 2012 Michael Erwin
// An Extension to Teeces Logics
//
// Requires Arduino IDE 1.0 or higher
//
// Release History
//
// v014 - Adjusted chargeVCC to activate at 12.2v DC.
// v012 - Testing to improve start up reliability of 7219s
// v011 - Adjusted getVCC vin +1 to address voltage drop into VIN on Arduino, added chargeVCC threshold
// v010 - Fixed Issue that blanked the VCC Led status when running ESBmode on the CBI
// v009 - Added Support for DataPort - sketch is not pretty.. but it is functional & easy to edit!
// v008 - Added Single Test all LEDs, and if monitorVCC == false, it simulates battery condition OK
// v007 - Changed status indicator operation to reflect ESBmode
// v006 - Added ESBmode to charging SEQ
// v005 - Added Voltage Monitor
// v004 - Demo Sketch
//
//We always have to include the library
#include <LedControl.h>
#undef round 

/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 13 is connected to the DataIn 
 pin 12 is connected to the CLK 
 pin 11 is connected to LOAD 
 We have only 2 MAX72XX currently on the bus.
 */
//LedControl lc=LedControl(13,12,11,2);
LedControl lc=LedControl(2,4,8,2);   // RSeries FX i2c v5 Module Logics Connector
// That last ,2) means there are 2 7219s on this bus
// 0 is the CBI
// 1 is the A&A DP Logic


// you will need to adjust these variables for your configuration
boolean ESBmode = true;   // operates like charging scene on Dagobah otherwise operates line mode
boolean monitorVCC = true;// Set this to true to monitor voltage
boolean testLEDs = false; // Set to true to test every LED on initial startup

int analoginput = 0;       // Set this to which Analog Pin you use for the voltage in.

float greenVCC = 12.0;    // Green LED l21 on if above this voltage
float yellowVCC = 11.3;    // Yellow LED l22 on if above this voltage & below greenVCC... below turn on only RED l23
float chargeVCC = 12.2;    // Voltage at which system is charging


// For 15volts: R1=47k, R2=24k
// For 30volts: R1=47k, R2=9.4k

float R1 = 46800.0;     // >> resistance of R1 in ohms << the more accurate these values are
float R2 = 23980.0;     // >> resistance of R2 in ohms << the more accurate the measurement will be


float vout = 0.0;       // for voltage out measured analog input
int value = 0;          // used to hold the analog value coming out of the voltage divider
float vin = 0.0;        // voltage calulcated... since the divider allows for 15 volts



/* we always wait a bit between updates of the display */
unsigned long delaytime=300;

unsigned long delayDPtime=200;

unsigned long timeNew = 0;

void setup() {
  Serial.begin(9600);                    // DEBUG CODE
  
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  lc.shutdown(1,false);

  /* and clear the display */
  lc.clearDisplay(0);
  lc.clearDisplay(1);
  
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  lc.setIntensity(1,5);
  /* and clear the display */
//  lc.clearDisplay(0);
//  lc.clearDisplay(1);
  
   if (monitorVCC == true) { pinMode(analoginput, INPUT);}
 
  
  if (testLEDs == true) {
    singleTest(); 
    delay(2000);
  }
  if (monitorVCC == false){
    l23on();
  }
}

/* 
 This function will light up every Led on the CBI matrix.
 The led will blink along with the row-number.
 row number 4 (index==3) will blink 4 times etc.
 */
void singleTest() {
  // Test CBI First dev=0
  for(int row=0;row<4;row++) {
    for(int col=0;col<5;col++) {
      delay(delaytime);
      lc.setLed(0,row,col,true);
      delay(delaytime);
      for(int i=0;i<col;i++) {
        lc.setLed(0,row,col,false);
        delay(delaytime);
        lc.setLed(0,row,col,true);
        delay(delaytime);
      }
    }
  }
  l21on();  // RED
  delay(delaytime);
  l22on();  // YELLOW
  delay(delaytime);
  l23on();  // GREEN
  delay(delaytime);
 
  // Now for DP dev=1
  for(int row=0;row<6;row++) {
    for(int col=0;col<7;col++) {
      delay(delaytime);
      lc.setLed(1,row,col,true);
      delay(delaytime);
      for(int i=0;i<col;i++) {
        lc.setLed(1,row,col,false);
        delay(delaytime);
        lc.setLed(1,row,col,true);
        delay(delaytime);
      }
    }
  } 
  l21off();
  l22off();
  l23off();
}

void l21on() // Voltage GREEN
{
lc.setLed(0,4,5,true);
}

void l22on()// Voltage Yellow
{
lc.setLed(0,5,5,true);
}

void l23on()// Voltage RED
{
lc.setLed(0,6,5,true);
}


void l21off() // Voltage Green 
{
lc.setLed(0,4,5,false);
}

void l22off() // Voltage Yellow
{
lc.setLed(0,5,5,false);
}

void l23off() // Voltage Red
{
lc.setLed(0,6,5,false);
}


void heartSEQ() {
 // Step 0
  lc.setRow(0,0,B01110000);
  lc.setRow(0,1,B00100000);
  lc.setRow(0,2,B00100000);
  lc.setRow(0,3,B01110000);
  delay(1000);
  // Step 1
  lc.setRow(0,0,B01010000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B01110000);
  lc.setRow(0,3,B00100000);
  delay(1000);
  // Step 1
  lc.setRow(0,0,B01010000);
  lc.setRow(0,1,B01010000);
  lc.setRow(0,2,B01010000);
  lc.setRow(0,3,B01110000);
  delay(1000);


}


/* 
 This function in a fashon like ESB scene
i */
void chargingSEQ() {  // used when monitorVCC == false
 // Step 0
  lc.setRow(0,0,B11111000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B11111000);
  lc.setRow(0,3,B00000000);
  l21on();
  delay(delaytime);
 // Step 1 
  lc.setRow(0,1,B00000000);
  delay(delaytime);
 // Step 2
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B00000000);
  lc.setRow(0,3,B11111000);
  l22on();
  delay(delaytime);
 // Step 3
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B11111000); 
  lc.setRow(0,3,B00000000);
  l23on();
  delay(delaytime);
// Step 4
  lc.setRow(0,0,B11111000);
  lc.setRow(0,1,B11111000); 
  lc.setRow(0,2,B11111000);
  lc.setRow(0,3,B11111000); 
  delay(300);
// Step 5
  lc.setRow(0,0,B11111000);
  lc.setRow(0,1,B00000000);
  lc.setRow(0,1,B00000000);
  lc.setRow(0,3,B00000000);
  delay(delaytime);
// Step 6
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B11111000); 
  lc.setRow(0,2,B00000000);
  lc.setRow(0,3,B00000000);
  l23off();
  delay(delaytime);
// Step 7
  lc.setRow(0,0,B11111000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B00000000);
  lc.setRow(0,3,B00000000);
  l22off();
  delay(delaytime);
  // Step 8
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B00000000);
  lc.setRow(0,3,B11111000);
  l21off();
  delay(delaytime);
  // Step 9
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B11111000);
  lc.setRow(0,3,B11111000);
  delay(delaytime);
}


void operatingSEQ() {          // used when monitorVCC == true
 // Step 0
  lc.setRow(0,0,B11111000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B11111000);
  lc.setRow(0,3,B00000000);
  delay(delaytime);
 // Step 1 
  lc.setRow(0,1,B00000000);
  delay(delaytime);
 // Step 2
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B00000000);
  lc.setRow(0,3,B11111000);
  delay(delaytime);
 // Step 3
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B11111000); 
  lc.setRow(0,3,B00000000);
  delay(delaytime);
// Step 4
  lc.setRow(0,0,B11111000);
  lc.setRow(0,1,B11111000); 
  lc.setRow(0,2,B11111000);
  lc.setRow(0,3,B11111000); 
  delay(300);
// Step 5
  lc.setRow(0,0,B11111000);
  lc.setRow(0,1,B00000000);
  lc.setRow(0,1,B00000000);
  lc.setRow(0,3,B00000000);
  delay(delaytime);
// Step 6
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B11111000); 
  lc.setRow(0,2,B00000000);
  lc.setRow(0,3,B00000000);
  delay(delaytime);
// Step 7
  lc.setRow(0,0,B11111000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B00000000);
  lc.setRow(0,3,B00000000);
  delay(delaytime);
  // Step 8
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B00000000);
  lc.setRow(0,3,B11111000);
  delay(delaytime);
  // Step 9
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B11111000);
  lc.setRow(0,3,B11111000);
  delay(delaytime);
}

void blankCBI() {          // used when monitorVCC == true
 // Step 0
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00000000);
  lc.setRow(0,2,B00000000);
  lc.setRow(0,3,B00000000);
}



void ESBoperatingSEQ() {          // used when ESBmode == true
 // Step 0
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00000000);
  lc.setRow(0,2,B00010000);
  lc.setRow(0,3,B00010000);
  delay(delaytime);

 // Step 1
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00000000);
  lc.setRow(0,2,B00010000);
  lc.setRow(0,3,B00000000);
  if (monitorVCC == false){ 
    l21on();
  }
  delay(delaytime);

 // Step 2 
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00010000);
  lc.setRow(0,3,B00000000);
  delay(delaytime);
 
  // Step 3
  lc.setRow(0,0,B10000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00010000); 
  lc.setRow(0,3,B11000000);
  if (monitorVCC == false){ 
    l21off();
  }
  delay(delaytime);
 
// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);


 // Step 4
  lc.setRow(0,0,B00010000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00010000); 
  lc.setRow(0,3,B11000000);
  delay(delaytime);

 // Step 5
  lc.setRow(0,0,B00010000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00010000); 
  lc.setRow(0,3,B01000000);
  if (monitorVCC == false){ 
    l21on();
  }
  delay(delaytime);

  // Step 6
  lc.setRow(0,0,B00010000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00010000);
  lc.setRow(0,3,B00000000);
  delay(delaytime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);


 // Step 7
  lc.setRow(0,0,B00010000);
  lc.setRow(0,1,B00000000);
  lc.setRow(0,2,B00010000);
  lc.setRow(0,3,B00010000);
  if (monitorVCC == false){ 
    l21off();
  }
  delay(delaytime);

 // Step 8
  lc.setRow(0,0,B00010000);
  lc.setRow(0,1,B00110000);
  lc.setRow(0,2,B00000000);
  lc.setRow(0,3,B00110000);
  delay(delaytime);

 // Step 9
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00000000); 
  lc.setRow(0,3,B00000000);
  delay(delaytime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);


 // Step 10
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00000000);
  lc.setRow(0,2,B00010000); 
  lc.setRow(0,3,B00000000);
  if (monitorVCC == false){ 
    l22on();
    l21on();
  }
  delay(delaytime);


 // Step 11
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00010000); 
  lc.setRow(0,3,B00000000);
  delay(delaytime);

 // Step 12
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00000000); 
  lc.setRow(0,3,B00010000);
  if (monitorVCC == false){ 
    l22off();
    l21off();
  }
  delay(delaytime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);


 // Step 13
  lc.setRow(0,0,B10000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00010000); 
  lc.setRow(0,3,B10110000);
  delay(delaytime);

 // Step 14
  lc.setRow(0,0,B10000000);
  lc.setRow(0,1,B01010000);
  lc.setRow(0,2,B00010000); 
  lc.setRow(0,3,B10100000);
  if (monitorVCC == false){ 
    l21on();
  }
  delay(delaytime);



 // Step 15
  lc.setRow(0,0,B10000000);
  lc.setRow(0,1,B01010000);
  lc.setRow(0,2,B00010000); 
  lc.setRow(0,3,B10000000);
  delay(delaytime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);

 // Step 16
  lc.setRow(0,0,B10000000);
  lc.setRow(0,1,B00100000);
  lc.setRow(0,2,B00010000); 
  lc.setRow(0,3,B11000000);
  if (monitorVCC == false){ 
    l21off();
  }
  delay(delaytime);

 // Step 17
  lc.setRow(0,0,B10000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00010000); 
  lc.setRow(0,3,B11000000);
  delay(delaytime);

 // Step 18
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00000000); 
  lc.setRow(0,3,B00000000);
  if (monitorVCC == false){ 
    l21on();
  }
  delay(delaytime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);

 // Step 19
  lc.setRow(0,0,B10000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B00010000); 
  lc.setRow(0,3,B01100000);
  delay(delaytime);

 // Step 20
  lc.setRow(0,0,B10000000);
  lc.setRow(0,1,B10000000);
  lc.setRow(0,2,B01010000); 
  lc.setRow(0,3,B00000000);
  if (monitorVCC == false){ 
    l21off();
  }
  delay(delaytime);

 // Step 21
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00100000);
  lc.setRow(0,2,B01011000); 
  lc.setRow(0,3,B01000000);
  delay(delaytime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);



 // Step 22
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00011000);
  lc.setRow(0,2,B01011000); 
  lc.setRow(0,3,B01000000);
  if (monitorVCC == false){ 
    l21on();
  }
  delay(delaytime);

 // Step 25
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B01011000); 
  lc.setRow(0,3,B01000000);
  delay(delaytime);

 // Step 26
  lc.setRow(0,0,B10001000);
  lc.setRow(0,1,B00100000);
  lc.setRow(0,2,B01001000); 
  lc.setRow(0,3,B01000000);
  if (monitorVCC == false){ 
    l21off();
  }
  delay(delaytime);

 // Step 27
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00000000);
  lc.setRow(0,2,B00001000); 
  lc.setRow(0,3,B00000000);
  delay(delaytime);

 // Step 28
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B01001000); 
  lc.setRow(0,3,B01000000);
  if (monitorVCC == false){ 
    l21on();
  }
  delay(delaytime);

// Update DP Logic  
   timeNew= millis();
   animateDPLogic(timeNew);


 // Step 29
  lc.setRow(0,0,B00000000);
  lc.setRow(0,1,B00010000);
  lc.setRow(0,2,B01001000); 
  lc.setRow(0,3,B01000000);
  delay(delaytime);

}

void allON(){
// Step 4
  lc.setRow(0,0,B11111000);
  lc.setRow(0,1,B11111000); 
  lc.setRow(0,2,B11111000);
  lc.setRow(0,3,B11111000); 
  delay(300);

}



void getVCC(){
 value = analogRead(analoginput);// this must be between 0.0 and 5.0 - otherwise you'll let the blue smoke out of your ar
 vout= (value * 5.0)/1024.0;  //voltage coming out of the voltage divider
 vin = (vout / (R2/(R1+R2)))+1;  //voltage to display
 
 
/*
 Serial.print("Volt Out = ");                                  // DEBUG CODE
 Serial.print(vout, 1);   //Print float "vin" with 1 decimal   // DEBUG CODE

   
 Serial.print("\tVolts Calc = ");                             // DEBUG CODE
 Serial.print(vin, 1);   //Print float "vin" with 1 decimal   // DEBUG CODE
*/
 
 if (vin >= greenVCC) {
   l21on();
   l22on();
   l23on();
//   Serial.println("\tTurn on Led l23 - GREEN");              // DEBUG CODE
 } else if (vin >= yellowVCC) {
     l21on();
     l22on();
     l23off();
//     Serial.println("\tTurn on Led l22 - YELLOW");           // DEBUG CODE
 } else {              // voltage is below yellowVCC value, so turn on l23.
     l21on();
     l22off();
     l23off();
//     Serial.println("\tTurn on Led l21 - RED");              // DEBUG CODE

 }
}



void loop() { 
 timeNew= millis();
 animateDPLogic(timeNew);
 if (monitorVCC == true){
   getVCC();          // Get Battery Status via the voltage divider circuit & display status
 }
 if (ESBmode==false) {operatingSEQ(); lc.clearDisplay(0);}
   else if (vin >= chargeVCC) {ESBoperatingSEQ();}
   // blankCBI();
   
}
  
void animateDPLogic(unsigned long elapsed)
{
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < 750) return;
  timeLast = elapsed; 

  int DPdev=1;
    for (int row=0; row<6; row++)
      lc.setRow(DPdev,row,random(0,256));
}  
  


