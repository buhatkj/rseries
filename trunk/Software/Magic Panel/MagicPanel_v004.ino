// Magic Panel FX by IA-PARTS.com 
// with Optional I2C RSeries 
// CCv3 SA BY - 2013 Michael Erwin
// Royal Engineers of Naboo
//
//
// Release History
// v003 - Decode events... next add selectable jumper support.
// v002 - Added default operation & I2C support
// v001 - Initial Demo Sketch
//
//We always have to include the library
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  #else
   #include "WProgram.h"
#endif
#include "LedControl.h"
#include "Wire.h"
#include "binary.h"
#include "SoftwareSerial.h"      //Load Software Serial to support additional serial pins on the Arduino Micro


boolean testLEDs=true;

/*
To load a sketch onto Magic Panel as Arduino Duemilanove w/ ATmega328

 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****

7221
Pin 1 - Data IN
Pin 12 - Load
Pin 13 - CLK
Pin 24 - Data Out

Top 7221 = 0
Bottom 7221 =1

Now we need to assign the pins from the 328p to LedControl
 
 pin D8 is connected to the DataIn 
 pin D7 is connected to the CLK 
 pin D6 is connected to LOAD 
 We have two MAX7221 on the Magic Panel, so we use 2.
 */
LedControl lc=LedControl(8,7,6,2);

/* we always wait a bit between updates of the display */
unsigned long delaytime=10;

unsigned long scrolldelaytime=5;

int brow;

boolean flipflop=false;
int triggerEvent;

long loopCount = 0;
long twitchMPtime = 0; // Long??

unsigned long t;
int randomDisplayFX;


// now setup for JEDI

#define rxPin 10
#define txPin 11

char* JEDIcodes[14] = {"10T0","10T1","10T2","10T3","10T4","10T5","10T6","10T7","10T8","10T9","10T10","10T11","10T12","0D"};
int JEDIMapEvent[] = { 2, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 2};
int MPeventCode = 0;
boolean JEDIpacketvalid=false;     // Is the packet valid?
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

// set up a new serial port for xv
SoftwareSerial jediSerial =  SoftwareSerial(rxPin, txPin);// RX, TX




void setup() {
//  Serial.begin(19200);                    // DEBUG CODE
   Wire.begin(20);                          // Start I2C Bus as Master I2C Address 20 (Assigned)
   Wire.onReceive(receiveEvent);            // register event so when we receive something we jump to receiveEvent();
 
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  jediSerial.begin(19200); 
  inputString.reserve(200);

  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  lc.shutdown(1,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  lc.setIntensity(1,8);
  
  /* and clear the display */
  lc.clearDisplay(0);
  lc.clearDisplay(1);
  
  if (testLEDs == true) {
    singleTest(); 
    delay(2000);
    lc.clearDisplay(0);
    lc.clearDisplay(1);
  }
 
  randomSeed(analogRead(0));           // Randomizer, well at least an attempt at it.
  blankPANEL();
  twitchMPtime = 800;   // Initial time 
}




void loop() { 
blankPANEL();
getJEDIevent(); 

// wait around
// t = millis();
 loopCount++; 

   if (loopCount > twitchMPtime)
    {
     //Serial.println("Time to twitch FHP");   // used to debug
//     twitchFHP();                            // Call the twitchFHP routine
     randomDisplay();                           // call playRandom routine
     loopCount = 0;                           // reset loopCount
     twitchMPtime = random (20,90) * 1000;   // set the next twitchMPtime
//     delay (twitchMPtime);
     
  }
 blankPANEL(); 
}


void demoPANEL(){
  blankPANEL();
    delay(1000);

  rowTest();
    delay(1000);

  blankPANEL();
    delay(1000);
    
  for(int ff=0;ff<10;ff++) {
    flipFlop();
    delay(800);
  }

 alarm();
  
  for(int sD=0;sD<20;sD++) {  
    scrollDOWN();
  }
  
  
  for(int sU=0;sU<20;sU++) {  
    scrollUP();
  }
  
  for(int cI=0;cI<10;cI++) {  
    compressIN();
  delay(200);
  allOFF();  
  }  

  delay(1000);
  
  for(int eO=0;eO<10;eO++) {  
    explodeOUT();
  delay(200);
  allOFF();  
  }
  
  for(int h=0;h<6;h++) {  
    heartPANEL();
    delay(1500);
    allOFF();
    delay(1500);
  }
  
}  






/* 
 This function will light up every Led on the matrix.
 The led will blink along with the row-number.
 row number 4 (index==3) will blink 4 times etc.
 */
void singleTest() {
  for(int ic=0;ic<2;ic++) {
    for(int row=0;row<8;row++) {
      for(int col=0;col<8;col++) {
        lc.setLed(ic,row,col,true);
        delay(delaytime);
 //         for(int i=0;i<col;i++) {
 //           lc.setLed(ic,row,col,false);
 //           delay(delaytime);
 //           lc.setLed(ic,row,col,true);
 //         delay(delaytime);
 //         }
        }
      }
    }
}

void rowTest() {
   for(int ic=0;ic<2;ic++) {
    for(int row=0;row<8;row++) {
      lc.setRow(ic,row,B11111111);
      delay(10);
    }
   }
}

void flipFlop() {  // Each time this is called it will alternate the top with bottom half

  if (flipflop == false){
    flipflop = true;
    for(int row=0;row<8;row++) {
      lc.setRow(0,row,B11111111);
    }
    for(int row=0;row<8;row++) {
      lc.setRow(1,row,B00000000);
    }
  }
  else {
    for(int row=0;row<8;row++) {
      lc.setRow(0,row,B00000000);
      }
    for(int row=0;row<8;row++) {
        lc.setRow(1,row,B11111111);
      }
    flipflop = false;
    }
  delay(100);
}

void fuelGage() {  //lits up from the bottom x number of rows
  
}

void allON() {  //all LEDs ON simple style - Huh how does this work?
lc.setRow(0,0,B11110000);
lc.setRow(0,1,B00001111);
lc.setRow(0,2,B11110000);
lc.setRow(0,3,B00001111);
lc.setRow(0,4,B11110000);
lc.setRow(0,5,B00001111);
lc.setRow(0,6,B11110000);
lc.setRow(0,7,B00001111);

lc.setRow(1,0,B11110000);
lc.setRow(1,1,B00001111);
lc.setRow(1,2,B11110000);
lc.setRow(1,3,B00001111);
lc.setRow(1,4,B11110000);
lc.setRow(1,5,B00001111);
lc.setRow(1,6,B11110000);
lc.setRow(1,7,B00001111);
}

void allOFF() {
  lc.clearDisplay(0);
  lc.clearDisplay(1);
}



void heartPANEL() {   // something is wrong with the matrix
 // Step 0
 
lc.setRow(0,0,B11110000);
lc.setRow(0,1,B00001111);
lc.setRow(0,2,B11110000);
lc.setRow(0,3,B00001111);
lc.setRow(0,4,B11110000);
lc.setRow(0,5,B00001111);
lc.setRow(0,6,B11110000);
lc.setRow(0,7,B00001111);

lc.setRow(1,0,B11110000);
lc.setRow(1,1,B00001111);
lc.setRow(1,2,B11110000);
lc.setRow(1,3,B00001111);
lc.setRow(1,4,B11110000);
lc.setRow(1,5,B00001111);
lc.setRow(1,6,B11110000);
lc.setRow(1,7,B00001111);

 
// lc.setColumn(0,1,B00001111);
// lc.setColumn(0,2,B11110000);
// lc.setColumn(0,3,B00001111);
// lc.setRow(0,4,B11110000);
// lc.setRow(0,5,B00001111);
// lc.setRow(0,6,B11110000);
// lc.setRow(0,7,B00001111);
 

/*  lc.setRow(0,0,B10000001);
  lc.setRow(0,1,B10000001);
  lc.setRow(0,2,B10000001);
  lc.setRow(0,3,B10000001);

  lc.setRow(0,4,B10000001);
  lc.setRow(0,5,B10000001);
  lc.setRow(0,6,B10000001);
  lc.setRow(0,7,B10000001);


// Now the bottom half 

  lc.setRow(1,0,B10000001);
  lc.setRow(1,1,B10000001);
  lc.setRow(1,2,B10000001);
  lc.setRow(1,3,B10000001);

  lc.setRow(1,4,B10000001);
//  lc.setRow(1,5,B01000001);
//  lc.setRow(1,6,B00000000);
//  lc.setRow(1,7,B00000000);
*/
}

void blankPANEL() {
  lc.clearDisplay(0);
  lc.clearDisplay(1);
}

void scrollDOWN() {
  for(int row=0;row<8;row++) {
      lc.setRow(0,row,B11111111);
      delay(scrolldelaytime);
      lc.setRow(0,row,B00000000);
      delay(scrolldelaytime);
  }
  for(int row=0;row<8;row++) {
      lc.setRow(1,row,B11111111);
      delay(scrolldelaytime);
      lc.setRow(1,row,B00000000);
      delay(scrolldelaytime);
  }
  
}

void scrollUP() {
  for(int row=7;row>0;row--) {
      lc.setRow(1,row,B11111111);
      delay(scrolldelaytime);
      lc.setRow(1,row,B00000000);
      delay(scrolldelaytime);
  }
  for(int row=7;row>0;row--) {
      lc.setRow(0,row,B11111111);
      delay(scrolldelaytime);
      lc.setRow(0,row,B00000000);
      delay(scrolldelaytime);
  }
  
}

void compressIN() {
  brow=8;
  for(int trow=0;trow<8;trow++) {
    brow=brow-1;
      lc.setRow(0,trow,B11111111);
      lc.setRow(1,brow,B11111111);
      delay(20);
  } 
}

void explodeOUT() {
  brow=0;
  for(int trow=8;trow>0;trow--) {
      lc.setRow(0,trow,B11111111);
      lc.setRow(1,brow,B11111111);
    brow=brow+1;
    delay(30);
  } 
}

void alarm() {
  
  allON();
    delay(2000);
  allOFF();  
    delay(2000);
  allON();
    delay(2000);
  allOFF();  
    delay(2000);
  allON();
    delay(2000);
  allOFF();  
    delay(2000);

}


// function that executes whenever data is received from an I2C master
// this function is registered as an event, see setup()
void receiveEvent(int eventCode) {
     switch (Wire.read()) {
      case 1:
       allON();
       break;
      case 2:
       allOFF();
       break;
      case 3:
       alarm(); 
       break;
      case 4: // FlipFlop 10 times
      for(int ff=0;ff<40;ff++) {
        flipFlop();
        delay(800);
      }
       break;
      case 5:  
        for(int sD=0;sD<100;sD++) {  
        scrollDOWN();
        }
       break;
      case 6:
        for(int sU=0;sU<100;sU++) {  
        scrollUP();
        }
       break;
      case 7:
        for(int cI=0;cI<40;cI++) {  
        compressIN();
        delay(200);
        allOFF();  
        }  
       break;
      case 8:
        for(int eO=0;eO<40;eO++) {  
        explodeOUT();
        delay(200);
        allOFF();
        }
       break;
      case 9:
       heartPANEL();
       delay(5000);
       allOFF();
       break;
      case 10:
        demoPANEL();
       break;
      default: 
       // if nothing else matches, do the default
       // so we are going to do nothing... for that matter not even waste time  
       break;
      }
}

void randomDisplay() {
     randomDisplayFX = random (1,9);    // Pick a number 1 - 7
     switch (randomDisplayFX) {
      case 1:
       allON();
       delay(3200);
        allOFF();
       break;
      case 2:
       allOFF();
       break;
      case 3:
       alarm(); 
       delay(5000);
        allOFF();  
       break;
      case 4: // FlipFlop 10 times
      for(int ff=0;ff<40;ff++) {
        flipFlop();
        allOFF();  
      }
       break;
      case 5:  
        for(int sD=0;sD<100;sD++) {  
        scrollDOWN();  
        }
        delay(5000);
        allOFF();
       break;
      case 6:
        for(int sU=0;sU<100;sU++) {  
        scrollUP();
        }
        delay(5000);
        allOFF();
       break;
      case 7:
        for(int cI=0;cI<40;cI++) {  
        compressIN();
        }  
        delay(5000);
        allOFF();
       break;
      case 8:
        for(int eO=0;eO<40;eO++) {  
        explodeOUT();
        }
        delay(5000);
        allOFF();
       break;
      case 9:
       heartPANEL();
       delay(5000);
       allOFF();
       break;
      default: 
       // if nothing else matches, do the default
       // so we are going to do nothing... for that matter not even waste time
       break;
      }
}



void getJEDIevent() {                             // Reads Controller Sent Data
  JEDIpacketvalid=false;                         // Is the packet we received valid? 
//  inputString ="";
  MPeventCode = 0;
 
   while (jediSerial.available()) {
    // get the new byte:
    char inChar = (char)jediSerial.read(); 
    // add it to the inputString:
    if (inChar == 0x13) {
      stringComplete = true;
    } else {   
    inputString += inChar;
    }
   }
   if (stringComplete == true){
    jediSerial.println(inputString);                         // echo received code back out onto Serial
    Serial.print("Received ="); Serial.print(inputString); // debug
      for (int x = 0; x< sizeof(JEDIcodes)-1; x++) {
        if (inputString == JEDIcodes[x]) {
          Serial.print(" and is Valid");                    // debug
          MPeventCode = JEDIMapEvent[x];
          JEDIpacketvalid=true;
          
        }
       }      
       Serial.println("");    //debug
    }
    
    if (stringComplete == true || JEDIpacketvalid == true){
      stringComplete = false;
      JEDIpacketvalid == false;
      inputString="";
     switch (MPeventCode) {
      case 1:
      Serial.println("Case 1");    //debug
//       allON();
       break;
      case 2:
      Serial.println("Case 2");    //debug
//       allOFF();
       break;
      case 3:
      Serial.println("Case 3");    //debug
//       alarm(); 
       break;
      case 4: // FlipFlop 10 times
      Serial.println("Case 4");    //debug
      for(int ff=0;ff<40;ff++) {
//        flipFlop();
//        delay(800);
      }
       break;
      case 5:  
      Serial.println("Case 5");    //debug
        for(int sD=0;sD<100;sD++) {  
//        scrollDOWN();
        }
       break;
      case 6:
      Serial.println("Case 6");    //debug
        for(int sU=0;sU<100;sU++) {  
//        scrollUP();
        }
       break;
      case 7:
      Serial.println("Case 7");    //debug
        for(int cI=0;cI<40;cI++) {  
//        compressIN();
        delay(200);
//        allOFF();  
        }  
       break;
      case 8:
      Serial.println("Case 8");    //debug
        for(int eO=0;eO<40;eO++) {  
//        explodeOUT();
        delay(200);
//        allOFF();
        }
       break;
      case 9:
      Serial.println("Case 9");    //debug
//       heartPANEL();
       delay(5000);
//       allOFF();
       break;
      case 10:
      Serial.println("Case 10");    //debug
//       fuelGage();
       delay(5000);
//       allOFF();
       break;
      case 11:
      Serial.println("Case 11");    //debug
//       message();
       delay(5000);
//       allOFF();
       break;      
      case 12:
      Serial.println("Case 12");    //debug
//        demoPANEL();
       break;
      default: 
       // if nothing else matches, do the default
       // so we are going to do nothing... for that matter not even waste time  
       break;
      }
      
    }
}

