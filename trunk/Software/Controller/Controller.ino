/*
 * Astromech RSeries Controller for the R2 Builders Club
 *  
 * Creative Copyright v3 SA BY - 2012 Michael Erwin 
 *                               michael(dot)erwin(at)mac(dot)com 
 * 
 * RSeries Open Controller Project  http://code.google.com/p/rseries-open-control/
 * Requires Arduino 1.0 IDE
 *
 * Liberal use of code from the following: 
 * Tim Hirzel   http://www.growdown.com
 * Fabio Biondi http://www.fabiobiondi.com
 * Tod Kurt     http://todbot.com
 * Chad Philips http://www.windmeadow.com/node/42
 * Limor Fried  http://adafruit.com/
 * Gabriel Bianconi  http://www.gabrielbianconi.com/projects/arduinonunchuk/
 */

#define astromechName "R4-I9"   // This will display on the screen, Keep length to 6 chars or less
#define VERSION "beta 0.4.5"    // You don't think we're done do you?
#define OWNER "Ted Koenig"    // Display on the Splash Screen

/* 
 Revision History
 v.0.4.5 - Fixing all the things broken by Adafruit GFX & Adafruit_TFTLCD
 v.0.4.4 - Adjusted for new adafruitGFX https://github.com/adafruit/Adafruit-GFX-Library 
 & adafruitTFTLCD https://github.com/adafruit/TFTLCD-Library
 v.0.4.3 - CVI Code Release - BETA
 v.0.4.2 - adjusted getVCC & parameter
 v.0.4.1 - Adjusted dome code to be much slower & reversed, adjusted VCC monitoring values
 v.0.4.0 - Started to update to handle Servo FX Modules
 v.0.3.9 - Updated configuration of AudioFX1 & AudioFX2, Added DroidRACEmode, included project page link, cleaned up splash screen
 v.0.3.8 - Added local Controller Shield battery monitor & display based on vinSTRONG & vinWEAK
 v.0.3.7 - Updated I2C Module Code to work with Arduino IDE 1.0
 v.0.3.6 - Need to have enough delay in between sending the packet and receiving a telemetry packet.
 v.0.3.5 - Code performance
 v.0.3.4 - Increased Xbee baud rate to 19200, cleaned up XBee TX Response code - Tested with Receiver v019
 v.0.3.3 - triggeritem is now sent as 2 bytes, reduced payload to 9 bytes
 v.0.3.2 - Telemetry data is now 4 bytes: 0 & 1= rxVCC, 2 & 3=rxVCA 
 v.0.3.1 - Removed Touchscreen POST code, built Controller_TouchScreenDebuger, Code Clean up continues
 v.0.3.0 - Arduino 1.0 IDE Support & updated to new ArduinoNunchuck library
 v.0.2.9 - Initial Wide Beta Test Release
 
 The goal of this sketch is to help develop a new wireless, touchscreen,
 R-Series Astromech Controller & Tranceiver system platform for the
 R2 Builders Club.
 
 Currently due to variable usage, we need more than 2048 bytes of SRAM.
 So we must use an Arduino MEGA 2560 or ATMEGA 2560 for the Transmitter
 
 Digi Xbee Series 2 & 2B PRO modules are FCC approved, and recommended.
 
 No FCC license is required. 
 Xbee S2 API Mode, ZigBee  & Radio Control communication understanding is helpful.
 
 It's going to take the output sent from the controller and interpret it into Servo & other data signals
 
 3 main servo outputs:
 servo1Pin = Joystick X, Forward/Reverse with Speed
 servo2Pin = Joystick Y, Left/Right
 servo3Pin = Accelerometer X, Dome Rotation
 
 However the controller sends all 5 motion channels from the handheld nunchuck controller to the receiver for completeness and future options.
 
 2 Default Nunchuck button activations:
 cbut = C Button Activate : make a ALERT or Scream noise. This is the TOP button on the Nunchuck
 zbut = Z Button Activate : make a CUTE random noise. This is the BOTTOM button on the Nunchuck
 
 32,767 virtual switches & configuration options via the LCD touchscreen
 We're going to limit our use 84 displayed events, 254 total events.
 If you need more than 84, feel free to mod the sketch in groups of 7.
 
 Receive & displaying telemetry from the Receiver.
 Charging Status = Future
 Battery Status = Implemented
 Current Draw = Implemented
 Calculates an Estimated run time = Future
 
 Nunchuck I2C Configuration Information
 UNO:         Green I2C Data= A4, Yellow I2C Clock A5
 MEGA 2560:   Green I2C Data = C21, Yellow I2C Clock Pin C22
 
 NOTE: Servo output code is in the Receiver sketch
 
 servoPin1 = Joystick X, Forward/Reverse with Speed
 servoPin2 = Joystick Y, Left/Right
 servoPin3 = Accelerometer X, Dome Rotation via A
 button1 = C Button Activate : Tell Astromech to make a ALERT or Scream noise.
 button2 = Z Button Activate : Tell Astromech to make a cute random noise.
 
 The I2C output from Nunchuck is made up of 6 bytes and stored in outbuf
 0 Joystick X
 1 Joystick Y
 2 Accel X
 3 Accel Y
 4 Accel Z
 5 C & Z Buttons
 
 */

#if defined(ARDUINO) && ARDUINO >= 100      // Check to see if your using Arduino 1.0 or higher
#include "Arduino.h"                        // if so, include Arduino.h
#else
#include "WProgram.h"                       // otherwise include WProgram.h which is the old name for Arduino.h
#endif

#include <Adafruit_GFX.h>       // Core graphics library
#include <Adafruit_TFTLCD.h>    // Hardware-specific library
#include <TouchScreen.h>        // Touch Screen functions - adafruit.com @ https://github.com/adafruit/Touch-Screen-Library
#include <XBee.h>               // Using to get RSSI values in displayRSSI
#include <Wire.h>               // Used to read the I2C data from Nunchuck - Arduino
//#include <ArduinoNunchuk.h>     // Arduino Nunchuk - Gabriel Bianconi @  http://www.gabrielbianconi.com/projects/arduinonunchuk/
#include <Encoder.h>            //Rotary encoder lib
#include <RSeriesGFX.h>         //R-Series GFX lib
#include <MemoryFree.h>

int xbeebps = 19200; // Bits Per Second (baud). Avoid 57600 Arduino UNO issue.
uint8_t payload[] = { 
  '0', '0', '0', '0', '0', '0', '0', '0', '0'};// Our XBee Payload of 9 potential values


Rx16Response rx16 = Rx16Response();
XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle 
ZBRxResponse rx = ZBRxResponse();
ModemStatusResponse msr = ModemStatusResponse();

// SH + SL Address of other XBee
//SH Address  SL Address   // You should only edit the SL
XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, 0x408d9cfb); // Destination (Receiver) address
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

// serial low
uint8_t slCmd[] = {
  'S','L'};

AtCommandRequest atRequest = AtCommandRequest(slCmd);
AtCommandResponse atResponse = AtCommandResponse();

long rx1DBm;
long lastrx1time;
long lasttx1time;
long nexttx1time;

int t =0;

boolean presstocontinue=false;   // used to handle waits for screen presses... you'll know it, when you see it.
boolean controllerstatus=false;  // nunchuk connected?
boolean transmitterstatus=false; // Are we transmitting?
boolean receiverstatus=false;    // Are we receiving?
boolean telemetrystatus=false;   // Are we receiving telemetry?
boolean rxpacketvalid=false;     // Is the packet we valid?
boolean rxpacketstart=false;     // RX Packet Start
boolean txbegin=false;           // Begin TX


int rx1ErrorCount=0;             // If >5 RX packets in a row are invalid, change status from OK to RX in YELLOW
int rx1ErrorCountMAX = 8;         // if >8 & receive errors, change status from OK to RX in RED: 8 packets ~1 Sec


boolean rxDEBUG=false;           // Set to monitor invalid TX packets via Serial Monitor baud 115200
boolean txDEBUG=false;           // Set to monitor sent TX packets via Serial Monitor baud 115200

char* radiostatus = "--";        // This is used to hold display Status... if "OK" = Green else it used to display error code "XX" from Transmitter, and displys in RED
char* previousradiostatus = "--";

int buttonval;

unsigned int rxVCC;
unsigned int rxVCA;

float dmmVCC;  // Display variable for Voltage from Receiver
float dmmVCA;  // Display variable for Amperage from Receiver

float testdmmVCA;  // Display variable for Amperage from Receiver

float previousdmmVCC = 00.0;
float previousdmmVCA = 00.0;

unsigned char telemetryVCCMSB = 0;
unsigned char telemetryVCCLSB = 0;
unsigned char telemetryVCAMSB = 0;
unsigned char telemetryVCALSB = 0;

float yellowVCC = 12.0;  // If RX voltage drops BELOW these thresholds, text will turn yellow then red.
float redVCC = 11.5;

float yellowVCA = 50.0;// If current goes ABOVE these thresholds, text will turn yellow then red.
float redVCA = 65.0;

long lastStatusBarUpdate = 0;
long nextStatusBarUpdate = 0;

int updateSTATUSdelay = 3500; // Update Status Bar Display every 5000ms (5 Seconds), caution on reducing to low

int ProcessStateIndicator = 0;   // Alternates between 0 & 1 High

int RSSI = 0;                 // XBee Received packet Signal Strength Indicator, 0 means 0 Bars (No Signal)
int RSSIpin = 6;              // Arduino PWM Digital Pin used to read PWM signal from XBee Pin 6 RSSI 6 to 6 KISS
unsigned long RSSIduration;   // Used to store Pulse Width from RSSIpin
int xbRSSI = 0;               // XBee Received packet Signal Strength Indicator, 0 means 0 Bars (No Signal)

int xbATResponse = 0xFF;   // to verify Coordinator XBee is setup and ready, set to 0xFF to prevent false positives

long lastTriggerEventTime = millis(); 

// Define TFT LCD Screen Stuff
int menuScreens = 10;
int displaygroup = 1;
int displayitem = 1;
int scrollymin = 23;
int scrollyinc = 16;
int scrollymax = 167;
int scrollyloc = 23;

// Servo Center Adjustment 
int chan1Center = 90;   // Channel 1 - Left Right  
int chan2Center = 90;   // Channel 2 - Forward & Reverse Speed
int chan3Center = 90;   // Channel 3 - Dome Rotation 

// Channel Adjustment 
int chan1Adjust = 0;    // Channel 1 - Left Right  
int chan2Adjust = 0;    // Channel 2 - Forward & Reverse Speed
int chan3Adjust = 0;    // Channel 3 - Dome Rotation 

// Channel Center Adjustment
int chan1Trim = 0;     // Channel 1 - Left Right  
int chan2Trim = 0;     // Channel 2 - Forward & Reverse Speed
int chan3Trim = 0;      // Channel 3 - Dome Rotation 

// Neutral Adjustments 
int chan1Neutral = 130; // Channel 1 - Left Right  
int chan2Neutral = 130; // Channel 2 - Forward & Reverse Speed
int chan3Neutral = 130; // Channel 3 - Dome Rotation 

// Neutral Width Adjustment, which takes Neutral + & - these values. So 120 to 140 would be considered Neutral or 90
int chan1Width = 10;    // Channel 1 - Left Right  
int chan2Width = 10;    // Channel 2 - Forward & Reverse Speed
int chan3Width = 10;    // Channel 3 - Dome Rotation 


// Nunchuck End Points Channel Adjustment 
int chan1Min = 27;      // Channel 1 Min - Left Right  
int chan1Max = 229;     // Channel 1 Max - Left Right
int chan2Min = 30;      // Channel 2 Min - Forward & Reverse Speed 
int chan2Max = 232;     // Channel 2 Max - Forward & Reverse Speed
int chan3Min = 77;      // Channel 3 Min - Dome Rotation  
int chan3Max = 180;     // Channel 3 Max - Dome Rotation 

int loop_cnt=0;

//ArduinoNunchuk nunchuk = ArduinoNunchuk();
byte joyx, joyy, accx, accy, accz, zbut, cbut;
byte triggeritem;

boolean domerotation;

int itemsel;

// Controller shield battery monitor variables
int analogVCCinput = 5; // RSeries Controller default VCC input is A5
float R1 = 47000.0;     // >> resistance of R1 in ohms << the more accurate these values are
float R2 = 24000.0;     // >> resistance of R2 in ohms << the more accurate the measurement will be
float vout = 0.0;       // for voltage out measured analog input
int VCCvalue = 0;       // used to hold the analog value coming out of the voltage divider
float vin = 0.0;        // voltage calulcated... since the divider allows for 15 volts

float vinSTRONG=3.4;    // If vin is above vinSTRONG display GREEN battery
float vinWEAK=2.9;       // if vin is above vinWEAK display YELLOW otherwise display RED
float vinDANGER=2.7;    // If 3.7v LiPo falls below this your in real danger.

uint16_t touchedY;
uint16_t touchedX;

//Rotary encoder pins and vars
//const int ENC_A = 47;
//const int ENC_B = 45;
//const int ENC_BUTTON = 43;
//const int ENC_LED = 41;
//Encoder EncoderKnob(ENC_A,ENC_B);
//int encoderButtonState = 0;
//int lastEncoderButtonState = 0;
//long encoderPos = 0;
//long lastEncoderPos = 0;

RSeriesGFX gfx = RSeriesGFX();

// SETUP ROUTINE
void setup() {
  Serial.begin(9600); // DEBUG CODE
  //  Serial1.begin(xbeebps);

  xbee.setSerial(Serial1);                     // Setup xbee to use Serial1
  xbee.begin(xbeebps);                         // Setup xbee to begin 9600
  // Serial.begin(9600);                          // Setup Serial to begin 9600    ENABLE TO DEBUG
  pinMode(analogVCCinput, INPUT);
  //pinMode(ENC_BUTTON, INPUT);
  //digitalWrite(ENC_BUTTON, HIGH);       // turn on pullup resistors 
  //pinMode(ENC_LED, OUTPUT);

  gfx.initDisplay(); // Init the gfx object

  //nunchuk.init();          // Initialize Nunchuk - using I2C
  buttonval = 0;
  gfx.displaySPLASH(OWNER, VERSION);          // Display Splash Screen
  
  delay(1500);              // for 1500 ms
  gfx.clearScreen(); //clear screen
  doPOST();            // Perform POST Routine
  radiostatus = "OK";
  gfx.clearScreen(); //clear screen
  gfx.displaySTATUS(BLUE, 
                astromechName, 
                radiostatus, 
                rx16.getRssi(), 
                ProcessStateIndicator, 
                dmmVCC,
                yellowVCC,
                redVCC,
                dmmVCA,
                yellowVCA,
                redVCA,
                vin,
                vinSTRONG,
                vinWEAK );      // Displays status bar at TOP
             
  gfx.displaySCROLL();          // Displays scroll buttons
  setupDefaultMenu();
  gfx.displayOPTIONS();
}

void loop() {
  // Read data from rx
  RXdata();

  //getNunchuk();  //Get input from the nunchuk 
  gfx.processTouch(); // Check for TFT Touchscreen Input & process

  TXdata();                           // Transmit Data
  delay(100);                         // Give it time to transmit 
  gfx.displaySendClear();

  if (millis() >= nextStatusBarUpdate) {
    gfx.displaySTATUS(BLUE, 
                astromechName, 
                radiostatus, 
                rx16.getRssi(), 
                ProcessStateIndicator, 
                dmmVCC,
                yellowVCC,
                redVCC,
                dmmVCA,
                yellowVCA,
                redVCA,
                vin,
                vinSTRONG,
                vinWEAK );      // Displays status bar at TOP
    nextStatusBarUpdate = millis() + updateSTATUSdelay;
  }
} // Finished with loop, restart loop


void setupDefaultMenu()
{
  gfx.menus.setMenuCount(10);
  // Page 0
  rsButton *Alarm = new rsButton(70, 40, 200, 28, BTN_STYLE_ROUND_BOX, TAN, "Alarm", 1, sendtrigger);
  rsButton *LeiaMsg = new rsButton(70, 70, 200, 28, BTN_STYLE_ROUND_BOX, RED, "Leia Msg", 2, sendtrigger);
  rsButton *Failure = new rsButton(70, 100, 200, 28, BTN_STYLE_ROUND_BOX, BLUE, "Failure", 3, sendtrigger);
  rsButton *Happy = new rsButton(70, 130, 200, 28, BTN_STYLE_ROUND_BOX, GREEN, "Happy", 4, sendtrigger);
  rsButton *Whistle = new rsButton(70, 160, 200, 28, BTN_STYLE_ROUND_BOX, CYAN, "Whistle", 5, sendtrigger);
  rsButton *Razzberry = new rsButton(70, 190, 200, 28, BTN_STYLE_ROUND_BOX, MAGENTA, "Razzberry", 6, sendtrigger);
  gfx.menus.addButton(Alarm, 0);
  gfx.menus.addButton(LeiaMsg, 0);
  gfx.menus.addButton(Failure, 0);
  gfx.menus.addButton(Happy, 0);
  gfx.menus.addButton(Whistle, 0);
  gfx.menus.addButton(Razzberry, 0);
  
  //Page 1
  rsButton *Annoyed = new rsButton(70, 40, 200, 28, BTN_STYLE_ROUND_BOX, YELLOW, "Annoyed", 7, sendtrigger);
  rsButton *Fire = new rsButton(70, 70, 200, 28, BTN_STYLE_ROUND_BOX, RED, "Fire Extng", 8, sendtrigger);
  rsButton *DomeWave = new rsButton(70, 100, 200, 28, BTN_STYLE_ROUND_BOX, BLUE, "Dome Wave", 9, sendtrigger);
  rsButton *WolfWhistle = new rsButton(70, 130, 200, 28, BTN_STYLE_ROUND_BOX, GREEN, "Wolf Whistle", 10, sendtrigger);
  rsButton *Cantina = new rsButton(70, 160, 200, 28, BTN_STYLE_ROUND_BOX, CYAN, "Cantina", 11, sendtrigger);
  rsButton *Vader = new rsButton(70, 190, 200, 28, BTN_STYLE_ROUND_BOX, MAGENTA, "VADER!", 12, sendtrigger);
  gfx.menus.addButton(Annoyed, 1);
  gfx.menus.addButton(Fire, 1);
  gfx.menus.addButton(DomeWave, 1);
  gfx.menus.addButton(WolfWhistle, 1);
  gfx.menus.addButton(Cantina, 1);
  gfx.menus.addButton(Vader, 1);
  
  //Page 2
  rsButton *C3PO = new rsButton(70, 40, 200, 28, BTN_STYLE_ROUND_BOX, YELLOW, "C3PO", 13, sendtrigger);
  rsButton *Yoda = new rsButton(70, 70, 200, 28, BTN_STYLE_ROUND_BOX, RED, "Yoda", 14, sendtrigger);
  rsButton *Luke = new rsButton(70, 100, 200, 28, BTN_STYLE_ROUND_BOX, BLUE, "Luke", 15, sendtrigger);
  rsButton *Leia = new rsButton(70, 130, 200, 28, BTN_STYLE_ROUND_BOX, GREEN, "Leia", 16, sendtrigger);
  rsButton *HanSolo = new rsButton(70, 160, 200, 28, BTN_STYLE_ROUND_BOX, CYAN, "Han Solo", 104, sendtrigger);// to eliminate workaround, this is just 104 now. similarly, 19 can just be registered as command code 105.
  rsButton *ObiWan = new rsButton(70, 190, 200, 28, BTN_STYLE_ROUND_BOX, MAGENTA, "Obi Wan", 18, sendtrigger);
  gfx.menus.addButton(C3PO, 2);
  gfx.menus.addButton(Yoda, 2);
  gfx.menus.addButton(Luke, 2);
  gfx.menus.addButton(Leia, 2);
  gfx.menus.addButton(HanSolo, 2);
  gfx.menus.addButton(ObiWan, 2);
  
  //Page 3
  rsButton *Relay1ON = new rsButton(70, 40, 100, 28, BTN_STYLE_ROUND_BOX, GREEN, "RL1 ON", 85, sendtrigger);
  rsButton *Relay1OFF = new rsButton(200, 40, 100, 28, BTN_STYLE_ROUND_BOX, RED, "RL1 OFF", 86, sendtrigger);
  rsButton *Relay2ON = new rsButton(70, 80, 100, 28, BTN_STYLE_ROUND_BOX, GREEN, "RL2 ON", 87, sendtrigger);
  rsButton *Relay2OFF = new rsButton(200, 80, 100, 28, BTN_STYLE_ROUND_BOX, RED, "RL2 OFF", 88, sendtrigger);
  rsButton *Relay3ON = new rsButton(70, 120, 100, 28, BTN_STYLE_ROUND_BOX, GREEN, "RL3 ON", 89, sendtrigger);
  rsButton *Relay3OFF = new rsButton(200, 120, 100, 28, BTN_STYLE_ROUND_BOX, RED, "RL3 OFF", 90, sendtrigger);
  rsButton *Relay4ON = new rsButton(70, 160, 100, 28, BTN_STYLE_ROUND_BOX, GREEN, "RL4 ON", 91, sendtrigger);
  rsButton *Relay4OFF = new rsButton(200, 160, 100, 28, BTN_STYLE_ROUND_BOX, RED, "RL4 OFF", 92, sendtrigger);
  gfx.menus.addButton(Relay1ON, 3);
  gfx.menus.addButton(Relay1OFF, 3);
  gfx.menus.addButton(Relay2ON, 3);
  gfx.menus.addButton(Relay2OFF, 3);
  gfx.menus.addButton(Relay3ON, 3);
  gfx.menus.addButton(Relay3OFF, 3);
  gfx.menus.addButton(Relay4ON, 3);
  gfx.menus.addButton(Relay4OFF, 3);
}


//void getNunchuk(){
//  //update the nunchuk
//  nunchuk.update();                // ALL data from nunchuk is continually sent to Receiver
//  joyx = map(nunchuk.analogX, chan1Min, chan1Max, 60, 120); // Channel 1 joyx & Channel 2 joyy from NunChuck Joystick
//  joyy = map(nunchuk.analogY, chan2Min, chan2Max, 120, 60); // Map it to Min & Max of each channel
//  accx = nunchuk.accelX/2/2; // ranges from approx 70 - 182
//  accy = nunchuk.accelY/2/2; // ranges from approx 65 - 173
//  accz = nunchuk.accelZ/2/2; // ranges from approx 65 - 173
//  zbut = nunchuk.zButton; // either 0 or 1
//  cbut = nunchuk.cButton; // either 0 or 1
//  /* Serial.print("joyx: "); Serial.print((byte)joyx,DEC);          // DEBUG CODE
//   Serial.print("\tjoyy: "); Serial.print((byte)joyy,DEC);        // DEBUG CODE
//   Serial.print("\taccx: "); Serial.print((byte)accx,DEC);        // DEBUG CODE
//   Serial.print("\tzbut: "); Serial.print((byte)zbut,DEC);        // DEBUG CODE
//   Serial.print("\tcbut: "); Serial.println((byte)cbut,DEC);      // DEBUG CODE
//   */
//  // Left & Right w Proporational Speed
//  if (joyx < 82) {
//    joyx = joyx;
//  }                // Emulate Dead Stick Zone on joyx
//  else if (joyx > 98) {
//    joyx = joyx;
//  }
//  else joyx = 90;                                           
//  // Forward & Reverse w Proporational Speed
//  if (joyy < 82) {
//    joyy = joyy;
//  }                // Emulate Dead Stick Zone on joyx
//  else if (joyy > 98) {
//    joyy = joyy;
//  }
//  else joyy = 90;                                           
//
//  // Dome Rotation & Dome FX                                           
//  if (accx < 90) {
//    accx = 60;
//  }                   // Emulate Dead Stick Zone on accx
//  else if (accx > 160) {
//    accx = 120;
//  }          // If you need to reverse the dome rotation... see receiver code
//  else accx = 90;                                           
//  
//  // Nunchuck Buttons & Screen Update and TX
//  if (zbut == 1 && cbut == 0) {
//    triggeritem = 101; 
//    TXdata(); //TODO: sort of redundant, but probably here to ensure nunchuk data is sent immediately?
//  }
//  else if (zbut == 0 && cbut == 1) {
//    triggeritem = 102; 
//    TXdata(); //TODO: sort of redundant, but probably here to ensure nunchuk data is sent immediately?
//  }
//  else if (zbut == 1 && cbut == 1) {
//    triggeritem = 103; 
//    TXdata(); //TODO: sort of redundant, but probably here to ensure nunchuk data is sent immediately?
//  }
//}


void sendtrigger(int inTrigger) {                          // We need to see how long since we last sent a trigger event
  triggeritem = inTrigger;  
  gfx.displaySendMessage(triggeritem);
}


void doPOST() {       // Power Up Self Test and Init
  presstocontinue=false;   // hey they gotta press something to continue
  controllerstatus=false;  // nunchuk connected?
  transmitterstatus=false; // Are we transmitting?
  receiverstatus=false;    // Are we receiving?
  telemetrystatus=false;   // Are we receiving telemetry?

  gfx.displayPOSTStage1();
  
//  while(controllerstatus == false) {
//    nunchuk.update();                // ALL data from nunchuk is continually sent to Receiver
//    joyx = nunchuk.analogX;    // ranges from approx 30 - 220
//    joyy = nunchuk.analogY;    // ranges from approx 29 - 230
//    accx = nunchuk.accelX; // ranges from approx 70 - 182
//    accy = nunchuk.accelY; // ranges from approx 65 - 173
//    accz = nunchuk.accelZ; // ranges from approx 65 - 173
//    zbut = nunchuk.zButton; // either 0 or 1
//    cbut = nunchuk.cButton; // either 0 or 1
//
//    if (joyx > 0 && joyy > 0) {
//      controllerstatus = true;
//    }
//  }
  

  gfx.displayPOSTStage2();

  while(transmitterstatus == false) {
    xbeeSL();                          // Send an AT command via API mode
    //    Serial.print("xbATResponse = ");Serial.println(xbATResponse, HEX);    // DEBUG CODE
    if (xbATResponse == 0x00) {         // to verify Coordinator XBee is setup and ready.
      transmitterstatus = true;
      //       Serial.println("Transmitter Status Good");   // DEBUG CODE
    }
  }
  
  gfx.displayPOSTStage3();
  
  while(receiverstatus == false) {
    loop_cnt++;
    if (loop_cnt > 1000){
      // --------  START OF TX
      payload[0] = byte(0x90); // joyx
      payload[1] = byte(0x90); // joyy
      payload[2] = byte(0x90); // accx
      payload[3] = byte(0x90); // accy
      payload[4] = byte(0x90); // accz
      payload[5] = byte(0x00); // zButton
      payload[6] = byte(0x00); // cButton
      payload[7] = byte(0x00); // TriggerEvent
      payload[8] = byte(0x00); // Future Use

      xbee.send(zbTx);         // Send a Test Payload to Receiver
    } 
    xbee.readPacket(500);  // Wait 500msec to see if we got a response back
    if (xbee.getResponse().isAvailable()){
      receiverstatus = true;
    }
    delay(100);
  }
  //     Serial.println("Receiver Responded, Status Good");   // DEBUG CODE
  gfx.displayPOSTStage4();

  while(telemetrystatus == false) {
    RXdata();
    //Serial.print("POST Received Telemetry -->> rxVCC =");Serial.print(rxVCC);  // DEBUG CODE
    //Serial.print("\trxVCA =");Serial.println(rxVCA);                                  // DEBUG CODE
    if (rxVCC > 0 && rxVCA > 0){
      telemetrystatus = true;
    }
  }
  
  gfx.displayPOSTStage5(); 
  delay(50); // Wait 50msec second before continueing
}


void RXdata() {
  xbee.readPacket(50); // wait 50msec to see if we received a packet
  if (xbee.getResponse().isAvailable()) {
    if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
      xbee.getResponse().getZBRxResponse(rx);
      telemetryVCCMSB = rx.getData()[0];
      telemetryVCCLSB = rx.getData()[1];
      telemetryVCAMSB = rx.getData()[2];
      telemetryVCALSB = rx.getData()[3];
      rxVCC = (unsigned int)word(telemetryVCCMSB,telemetryVCCLSB);
      rxVCA = (unsigned int)word(telemetryVCAMSB,telemetryVCALSB);
      dmmVCC = (float)rxVCC / 10.0;                   // Float & move the point for Volts
      dmmVCA = (float)rxVCA / 10.0;                   // float & move the point for Amps
    }
  }
}


void TXdata() {      // Build & TX Payload Packet
  // Frame  Data Payload START
  // Bytes 17 - 23 NunChuck Data
  payload[0]=joyx;  // 17 JoyX ranges from approx 30 - 220
  payload[1]=joyy;  // 18 JoyY ranges from approx 29 - 230
  payload[2]=accx;  // 19 AccX ranges from approx 70 - 182
  payload[3]=accy;  // 20 AccY ranges from approx 65 - 173
  payload[4]=accz;  // 21 AccZ ranges from approx 65 - 173
  payload[5]=zbut;  // 22 ZButton Status
  payload[6]=cbut;  // 23 CButton Status
  // Take the value of stored in triggeritem, and convert them to MSB & LSB 2 bytes via a bitshift operation 
  //    triggeritemLSB = triggeritem &0xFF;
  //    triggeritemMSB = (triggeritem >> 8) &0xFF;  
  //  NOTE:  To make the 2 Byte (MSB & LSB) values back into an int use the following code:    
  //         int triggeritem = (int)(word(triggeritemMSB,triggeritemLSB));    
  // Bytes 24 & 25 Trigger Event
  payload[7]=triggeritem;    // 24 0 to 254  If you have more than 254 events... need to rework event code
  payload[8]=0x00;           // 25 - Future USE
  // Frame Data Payload END
  xbee.send(zbTx);
  triggeritem = 0;
}  


void xbeeSL() {                   // Get Serial number Low (SL) of the Transmitter
  // See Chapter 9, API Operation (~Page 99) of Digi 
  // XBee/Xbee-PRO ZB RF Modules Manual for details
  atRequest.setCommand(slCmd);  
  // send the command
  xbee.send(atRequest);
  // wait up to 5 seconds for the status response
  if (xbee.readPacket(5000)) {
    // got a response!
    // should be an AT command response
    if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
      xbee.getResponse().getAtCommandResponse(atResponse);
      if (atResponse.isOk()) {
        Serial.print("Command [");
        Serial.print(atResponse.getCommand()[0]);
        Serial.print(atResponse.getCommand()[1]);
        Serial.println("] was successful!");
        xbATResponse = 0x00;
        if (atResponse.getValueLength() > 0) {
          Serial.print("Command value length is ");
          Serial.println(atResponse.getValueLength(), DEC);
          Serial.print("Command value: ");
          for (int i = 0; i < atResponse.getValueLength(); i++) {
            Serial.print(atResponse.getValue()[i], HEX);
            Serial.print(" ");
          }
          Serial.println("");
        }
      } 
      else {
        Serial.print("Command return error code: ");
        Serial.println(atResponse.getStatus(), HEX);
      }
    } 
    else {
      Serial.print("Expected AT response but got ");
      Serial.print(xbee.getResponse().getApiId(), HEX);
    }   
  } 
  else {
    // at command failed
    if (xbee.getResponse().isError()) {
      Serial.print("Error reading packet.  Error code: ");  
      Serial.println(xbee.getResponse().getErrorCode());
    } 
    else {
      Serial.print("No response from radio");  
    }
  }
}


void getVCC(){
  //    vin = random(80,100)/10.0;             // DEBUG CODE
  VCCvalue = analogRead(analogVCCinput); // this must be between 0.0 and 5.0 - otherwise you'll let the blue smoke out of your ar
  vout= (VCCvalue * 5.0)/1024.0;         //voltage coming out of the voltage divider
  vin = vout / (R2/(R1+R2));             //voltage based on vout to display battery status
  Serial.print("Battery Voltage =");
  Serial.println(vin,1);  // DEBUG CODE
}






