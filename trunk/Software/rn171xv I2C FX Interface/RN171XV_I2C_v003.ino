// RSeries RN171XV I2C Interface
// CCv3 SA BY - 2013 Royal Engineers of Naboo
// Captures Serial Events from Roving Networks 171XV
// Maps it to RSeries I2C Event
//
// Release History
// v003 - IA-Parts.com Magic Panel Support
// v002 - Code Clean Up
// v001 - Functional Demo Sketch
// When new serial data arrives, tries to see if it matches the Code
// When a newline is received, the loop prints the string and 
// clears it.
// 
//
/*
Please upgrade your RN171XV to version 4.00 or higher of the firmware.

R2Touch                   
Code      Description           Goto: I2C Address, Event
:OP00     Open All Dome Panels     

*/
char* xvCodes[10] = {"*MO99","*MO00","*MO05","*MF99","*OPEN","*CLOS",":CL00",":OP00",":SE01"":SE15","*ON00"};
int i2cAddress[] = {20, 20, 20, 20, 0, 0, 12, 12, 3, 3, 1};
int i2cEvent[] =   { 1,  2,  3,  4, 0, 0,  1,  3, 1, 1, 1};


int rxCode = 0;

#include <SoftwareSerial.h>      //Load Software Serial to support additional serial pins on the Arduino Micro
#include <Wire.h>                //used to support I2C

#define rxPin 10
#define txPin 11

int led = 13;
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

char rxArray1[20]; // Allows for 60 Bytes to be read at once, XBee S2B only has room for about 100 packets of 54 bytes 

boolean xvpacketvalid=false;     // Is the packet valid?
boolean xvpacketstart=false;     // xv Packet Start

// set up a new serial port for xv
SoftwareSerial xvSerial =  SoftwareSerial(rxPin, txPin);// RX, TX

void setup() {
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  pinMode(led, OUTPUT); 
// Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo & Micro
  }

// set the data rate for the SoftwareSerial port for xv
  xvSerial.begin(9600);
//  Serial.println("Started xvSerial @ 9600 baud for RN171XV");

// reserve 200 bytes for the inputString:
  inputString.reserve(200);
  ledOFF();
  Wire.begin(125); // RN171XV FX I2C Interface is assigned address 125.
}


void loop() {
  getxvdata();
  sendEvent();
}


void sendEvent() {
  if (xvpacketvalid == true) {
    int triggerFXaddress=i2cAddress[rxCode];
    int triggerFXevent=i2cEvent[rxCode];
    
    Serial.print("Code Received= \t");
    Serial.print(rxCode);
    Serial.print(" Sending to I2C: \t");
    Serial.print(triggerFXaddress);
    Serial.print(" Event: \t");
    Serial.println(triggerFXevent);

    Wire.beginTransmission(triggerFXaddress);  // transmit to device  which is the Audio FX 2 Module rMP3
    Wire.write(triggerFXevent); // sends Trigger Event LSB 
    Wire.endTransmission(); 
    delay(100);
   
  }
}  


void ledON() {
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
}

void ledOFF() {
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
}



void getxvdata() {                             // Reads Controller Sent Data
  xvpacketstart=false;                         // Have we started RXing a RX Packet?
  xvpacketvalid=false;                         // Is the packet we received valid? 
  inputString ="";
 
    while(xvpacketstart == false) {           // Loop until start byte 1 == 3A
        rxArray1[0] = xvSerial.read();         // read Byte 0 from XV

        if (rxArray1[0] == 0x3A || rxArray1[0] == 0x2A) {  // Byte 0 should be 0x3A ":"  or 0x2A "*" if packet start
            xvpacketstart = true;}      
    }        
    if (xvpacketstart ==true);{ 
      delay(100);      
      rxArray1[1] = xvSerial.read();           // 
      delay(100);
      rxArray1[2] = xvSerial.read();           // 
      delay(100);
      rxArray1[3] = xvSerial.read();           // 
      delay(100);
      rxArray1[4] = xvSerial.read();           //
      for (int i = 0; i<5; i++) { // Remaining Bytes, for i them
        inputString += rxArray1[i];    
      }
      Serial.print("String Received = ");  // DEBUG CODE
      Serial.print(inputString);
      for (int x = 0; x< sizeof(xvCodes)-1; x++) {
        if (inputString == xvCodes[x]) {
          Serial.print(" and is Valid");
          rxCode = x;
          xvpacketvalid=true;
        }
       }      
       Serial.println("");
    }
}

