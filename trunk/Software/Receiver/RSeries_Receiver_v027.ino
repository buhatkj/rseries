/*
 * Astromech RSeries Receiver for the R2 Builders Club
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
 * ArduPilot-Mega Development Team http://code.google.com/p/ardupilot-mega/
 *
*/

#define VERSION "beta 0.2.7"
// 0.2.7 - Adjust JoyX & JoyY
// 0.2.6 - Dome Code, enabled real getVCC telemetry
// 0.2.5 - Updated configuration of AudioFX1 & AudioFX2, Added DroidRACEmode, included project page link
// 0.2.4 - Voice Recoginition Integration started via i2C
// 0.2.3 - Updated Servo Pins to match RSeries Receiver PCB
// 0.2.2 - Updated getVCC(Actual) via voltage divider circuit using 47k & 24k precision resistors
//          Currently getVCA(simulated) till sensors are tested
// 0.2.1 - Updated I2C Module Code to work with Arduino IDE 1.0
// 0.2.0 - Added Failsafe Routine
// 0.1.9 - Increased Xbee baud rate to 19200
// 0.1.8 - Cleaned up loop, and telemetry. Renamed to AudioFX1Module & AudioFX2Module
// 0.1.7 - Moved the telemetry into multi-byte to support larger values than 254 & shortend the transmitted payload to 6 bytes
// 0.1.6 - Arduino 1.0 IDE Support
// 0.1.5 - Added support for AudioFX & ServoFX Modules for I2C bus
// 0.1.4 - typos
// 0.1.3 - Performance work
// 0.1.2 - xbee.h support TX
// 0.1.1 - xbee.h support RX
// 0.1.0 - xbee.h support phase 1
// 0.0.9 - preparing for xbee.h support
// 0.0.8 - adds support for I2C from Mega Master to UNO Slaves & eventTriggered
//         Defined Slaves : 4 = Audio/rMP3, 5 = Audion/rMP3 #2, 6 = Teeces Logics
// 0.0.7 - remarks out debug code... addresses the jittery servos
// 0.0.6 - first code released to beta testers


/* 
The goal of this sketch is to help develop a new wireless, touchscreen,
R-Series Astromech Controller & Tranceiver system platform for the
R2 Builders Club.

Currently due to variable usage, we need more than 2048 bytes of SRAM.
So we must use an Arduino MEGA 2560 or ATMEGA 2560 for the Transmitter

Digi Xbee Series 2 & 2B PRO modules are FCC approved. Pro Modules are optional, but highly recommended.

No license is required. 
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

*/


#include <XBee.h>
#include <Servo.h>
#include <Wire.h>

boolean AudioFX1module = true;     // Set to true if you have installed AudioFX1module on I2C Bus as 4
boolean AudioFX2module = true;     // Set to true if you have installed AudioFX2module on I2C Bus as 5
boolean ServoFX1module = true;     // Set to true if you have installed ServoFX1module on I2C Bus as 6

int xbeebps = 19200; // Bits Per Second (baud). Avoid 57600 Arduino UNO issue.

XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle 
ZBRxResponse rx = ZBRxResponse();
ModemStatusResponse msr = ModemStatusResponse();


// SH + SL Address of receiving XBee (Since this the receiver code, we need to use Controller address
XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, 0x405e0b99); // Destination (Controller) address

uint8_t payload[] = { '0', '0', '0', '0', '0', '0'}; // Our XBee Payload of 6 values (rxVCC=2, rxVCA=2, future=2)


ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();


byte xbAPIidResponse = 0x00;

Servo chan1servo;       // create servo object to control a servo 
Servo chan2servo;       // create servo object to control a servo 
Servo chan3servo;       // create servo object to control a servo 
Servo chan4servo;       // create servo object to control a servo 
Servo chan5servo;       // create servo object to control a servo 
Servo chan6servo;       // create servo object to control a servo 
Servo chan7servo;       // create servo object to control a servo -- Might Go Away
Servo chan8servo;       // create servo object to control a servo -- Due to the use of PCA9685 
Servo chan9servo;       // create servo object to control a servo -- via the i2C bus
Servo chan10servo;      // create servo object to control a servo -- to add unlimited servos


// Define which digital pins on the arduino are for servo data signals 
int servo1Pin = 2;    // Channel 1 - Left Right                joyx
int servo2Pin = 3;    // Channel 2 - Forward & Reverse Speed   joyy
int servo3Pin = 4;    // Channel 3 - Dome Rotation             accx
int servo4Pin = 5;    // Channel 4
int servo5Pin = 7;    // Channel 5
int servo6Pin = 8;    // Channel 6
int servo7Pin = 9;    // Channel 7
int servo8Pin = 10;   // Channel 8
int servo9Pin = 11;   // Channel 9
int servo10Pin = 12;  // Channel 10




// Servo Center Adjustment 
int chan1Center = 90;   // Channel 1 - Left Right  
int chan2Center = 90;   // Channel 2 - Forward & Reverse Speed
int chan3Center = 85;   // Channel 3 - Dome Rotation 


// Channel Adjustment 
int chan1Adjust = 0;    // Channel 1 - Left Right  
int chan2Adjust = 0;    // Channel 2 - Forward & Reverse Speed
int chan3Adjust = 0;    // Channel 3 - Dome Rotation 

// Channel Center Adjustment
int chan1Trim = 10;     // Channel 1 - Left Right  
int chan2Trim = 10;     // Channel 2 - Forward & Reverse Speed
int chan3Trim = 0;      // Channel 3 - Dome Rotation 

// Neutral Adjustments 
int chan1Neutral = 8; // Channel 1 - Left Right  
int chan2Neutral = 8; // Channel 2 - Forward & Reverse Speed
int chan3Neutral = 92; // Channel 3 - Dome Rotation 

// Neutral Width Adjustment, which takes Neutral + & - these values. So 120 to 140 would be considered Neutral or 90
int chan1Width = 16;    // Channel 1 - Left Right  
int chan2Width = 16;    // Channel 2 - Forward & Reverse Speed
int chan3Width = 16;    // Channel 3 - Dome Rotation 


                        // End Points Channel Adjustment 
int chan1Min = 30;      // Channel 1 Min - Left Right  
int chan1Max = 220;     // Channel 1 Max - Left Right
int chan2Min = 30;      // Channel 2 Min - Forward & Reverse Speed 
int chan2Max = 220;     // Channel 2 Max - Forward & Reverse Speed
int chan3Min = 100;      // Channel 3 Min - Dome Rotation LEFT 
int chan3Max = 87;     // Channel 3 Max - Dome Rotation RIGHT



int loop_cnt=0;

byte joyx, joyy, accx, accy, accz, zbut, cbut;

int triggerEvent;

unsigned long timeEvent = 0;
unsigned long timeEventElapsed = 0;
unsigned long timeEventLast = 0;

//byte triggerEventMSB;
//byte triggerEventLSB;
byte triggerFX1item;  // Used for Sound FX on FX Module #1
byte triggerFX2item;  // Used for Sound FX on FX Module #2

boolean domerotation = false;
boolean rxpacketstart = false;     // RX Packet Start

byte telemetryVCCMSB;    // MSB of Voltage VCC
byte telemetryVCCLSB;    // LSB of Voltage VCC
 
byte telemetryVCAMSB;    // MSB of Current VCA
byte telemetryVCALSB;    // LSB of Current VCA

unsigned int rxVCC = 0;
unsigned int rxVCA = 0;

int rxErrorCount = 0;

int analogVCCinput = 5; // RSeries Receiver default VCC input is A5
float R1 = 47000.0;     // >> resistance of R1 in ohms << the more accurate these values are
float R2 = 24000.0;     // >> resistance of R2 in ohms << the more accurate the measurement will be
float vout = 0.0;       // for voltage out measured analog input
int VCCvalue = 0;          // used to hold the analog value coming out of the voltage divider
float vin = 0.0;        // voltage calulcated... since the divider allows for 15 volts


boolean DroidRACEmode = false;    // LEAVE set to false - triggered by triggerEvent=250, triggerEvent=254 sets it back to false
boolean DroidRACEstart = false;   // LEAVE set to false - triggered by forward JOYX & JOYY>100 from controller

// long timeEvent =0


void setup() {  
  xbee.setSerial(Serial1);                     // Setup xbee to use Serial1
  xbee.begin(xbeebps);                         // Setup xbee to begin 9600
  Wire.begin();                                // Start I2C Bus as Master  
  Serial.begin(9600);                          // DEBUG CODE
  
  chan1servo.attach(servo1Pin);  // create servo object to control a servo 
  chan2servo.attach(servo2Pin);  // create servo object to control a servo 
  chan3servo.attach(servo3Pin);  // create servo object to control a servo 
//  chan4servo.attach(servo4Pin);  // create servo object to control a servo 
//  chan5servo.attach(servo5Pin);  // create servo object to control a servo 
//  chan6servo.attach(servo6Pin);  // create servo object to control a servo 
//  chan7servo.attach(servo7Pin);  // create servo object to control a servo 
//  chan8servo.attach(servo8Pin);  // create servo object to control a servo 
//  chan9servo.attach(servo9Pin);  // create servo object to control a servo 
//  chan10servo.attach(servo10Pin);// create servo object to control a servo 

  
  pinMode(analogVCCinput, INPUT);


//Serial.println("Starting up debug...");        // DEBUG CODE

}

// continuously reads packets, looking for ZB Receive or Modem Status
void loop() {
  loop_cnt++;
  timeEvent=millis();
  
  
  if (loop_cnt > 50) {                      // Every 50 loops send a telemetry packet to controller  
      sendTelemetry();                      // Send the telemetry to the controller
       loop_cnt = 0;                        // Reset loop_cnt to 0
  }
//  Serial.println("Sent zbTx & zerod loop_cnt");         // DEBUG CODE
 
  // after sending a tx request, we expect a status response
  // wait up to half second for the status response

  xbee.readPacket(100);        // Allow 100 msec to see if Xbee packet is available

//  Serial.print("1 XBee Response= >>>   ");Serial.println(xbee.getResponse().getApiId(), HEX);  // DEBUG CODE
 
  if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
    rxErrorCount=0;
    xbee.getResponse().getZBRxResponse(rx);
       
    int joyx = rx.getData()[0];    // Left Right
    int joyy = rx.getData()[1];    // Fwd Back
    int accx = rx.getData()[2];    // Dome
//    int accy = rx.getData()[3];  // Nunchuk Y Acceleramator 
//    int accz = rx.getData()[4];  // Nunchuk Z Acceleramator
    triggerEvent = rx.getData()[7];// TriggerEvent
//    int futureEvent = rx.getData()[8]; // Future Event Payload

/*                                                                                // DEBUG CODE
    Serial.print(">> joyx =");Serial.print(joyx);                                 // DEBUG CODE
    Serial.print("\tjoyy =");Serial.print(joyy);                                  // DEBUG CODE
    Serial.print("\taccx =");Serial.print(accx);                                  // DEBUG CODE
    Serial.print("\taccy =");Serial.print(accy);                                  // DEBUG CODE
    Serial.print("\taccz =");Serial.print(accz);                                  // DEBUG CODE
    Serial.print("\ttriggerEvent =");Serial.println(triggerEvent);                // DEBUG CODE
*/


// Dome Rotation FX
    if (AudioFX2module==true) {
      if (accx == 60 && domerotation==false) {triggerFX2item = 200; domerotation = true; sendAudioFX2();}    
        else if (accx == 120 && domerotation==false) {triggerFX2item = 201; domerotation = true; sendAudioFX2();}
        else if (accx == 90 && domerotation==true) {triggerFX2item = 202; domerotation = false; sendAudioFX2();}  // Dome Rotation Stop FX
    }

// Dome Rotation Reversal Code & Adjustment 
if (accx == 90) {accx=chan3Neutral;} // Neutral
if (accx == 120) {accx=chan3Max;}    // Right
if (accx == 60) {accx=chan3Min;}     // Left

//    Serial.print("\taccx =");Serial.print(accx);                                  // DEBUG CODE

 
    chan1servo.write(joyx);      // Update Channel 1 servo 
    chan2servo.write(joyy);      // Update Channel 2 servo
    chan3servo.write(accx);      // Update Channel 3 servo
//    delay(50);                   // Allow servos to move
    if (triggerEvent > 0 && AudioFX1module==true) {sendAudioFX1();} // if AudioFX1Module is installed, send every event
    if (ServoFX1module==true && triggerEvent > 0) {sendServoFX1();} // if ServoFX1Module is installed, send every event
 
    //---------------------- Install DroidRACEmode Code Below ---------------------- 
 
    if (triggerEvent == 27 && AudioFX2module==true) {triggerFX2item = 250; sendAudioFX2(); DroidRACEmode=true; DroidRACEstart=false;timeEventLast=millis();} // if AudioFX2Module is installed, send event   
    if (triggerEvent == 28 && AudioFX2module==true) {triggerFX2item = 254; sendAudioFX2(); DroidRACEmode=false;DroidRACEstart=false;} // if AudioFX2Module is installed, send event     

    timeEventElapsed=timeEvent-timeEventLast;
    
    if (DroidRACEmode == true && timeEventElapsed > 1000) {
      if (joyy > 130 && DroidRACEstart==false) {triggerFX2item = 251; sendAudioFX2(); DroidRACEstart=true;timeEventLast=millis();} // Race start
      else if (joyx > 120 && DroidRACEstart==true) {triggerEvent = 252; sendAudioFX1();timeEventLast=millis();} // RIGHT Tire Skid FX
      else if (joyx < 60 && DroidRACEstart==true) {triggerEvent = 253; sendAudioFX1();timeEventLast=millis();}  // LEFT Tire Skid FX
      else if (joyy < 10 && DroidRACEstart==true) {triggerFX2item = 254; sendAudioFX2(); DroidRACEstart=false; DroidRACEmode=false;} // Stop Race Mode FX
    }
    // Stop Race Mode FX
    //------------------------ End DroidRACEmode Code Above ------------------------ 
 
    //---------------------- Install Addtional Module Code Below ---------------------- 
 
 
   
   
    //------------------------ End Addtional Module Code Above ------------------------ 
    
  } else if (xbee.getResponse().getApiId() == MODEM_STATUS_RESPONSE) {
        xbee.getResponse().getModemStatusResponse(msr);
        // the local XBee sends this response on certain events, like association/dissociation
        if (msr.getStatus() == COORDINATOR_STARTED) {
          rxErrorCount = 0;
          //Serial.println("COORDINATOR_STARTED");                //DEBUG CODE
        } else if (msr.getStatus() == ASSOCIATED) {
            rxErrorCount = 0;
            //Serial.println("ASSOCIATED");                       //DEBUG CODE
        } else if (msr.getStatus() == DISASSOCIATED) {
            rxErrorCount++;
            //Serial.println("DISASSOCIATED");                    //DEBUG CODE
        } else if (msr.getStatus() == SYNCHRONIZATION_LOST) {
            rxErrorCount++;
            //Serial.println("SYNCHRONIZATION_LOST");             //DEBUG CODE
          }  
      } else if (xbee.getResponse().isError()) {
        rxErrorCount++;
//        Serial.print("Error reading packet.  Error code: ");    //DEBUG CODE
//        Serial.println(xbee.getResponse().getErrorCode());      //DEBUG CODE
  }
  if (rxErrorCount > 20) {failsafe();}  // If rxErrorCoun
}

void failsafe() { //  go into failsafe mode and require a restart.

        chan1servo.write(chan1Neutral);       // Update Chanel 1 servo 
        chan2servo.write(chan2Neutral);       // Update Chanel 2 servo
        chan3servo.write(chan3Neutral);       // Update Chanel 3 servo

//        Serial.println("Receiver went into failsafe mode.");  //DEBUG CODE

        while(1){                   // Loop here until restart.
        }
}

void sendAudioFX1() {
        Wire.beginTransmission(4);  // transmit to device #4 which is the Audio FX#1 rMP3
        Wire.write(triggerEvent);   // sends Trigger Event LSB 
        Wire.endTransmission();     // stop transmitting
}

void sendAudioFX2() {
        Wire.beginTransmission(5);  // transmit to device #5 which is the Audio FX#2 Module rMP3
        Wire.write(triggerFX2item); // sends Trigger Event LSB 
        Wire.endTransmission();     // stop transmitting
}

void sendServoFX1() {
        Wire.beginTransmission(6);  // transmit to device #6 which is the Servo FX#1 Module rMP3
        Wire.write(triggerEvent);   // sends Trigger Event LSB 
        Wire.endTransmission();     // stop transmitting
}

  

void sendTelemetry() {  
  getVCC();
  getVCA();
  
 //Serial.print("rxVCC=");  Serial.print(rxVCC);     // DEBUG CODE
 //Serial.print("\trxVCA=");  Serial.println(rxVCA); // DEBUG CODE

 // Take the value of stored in rxVCC & rxVCA, and convert them to MSB & LSB 2 bytes via a bitshift operation 


//    telemetryVCAMSB = highByte(rxVCA);
//    telemetryVCALSB = lowByte(rxVCA);


//    telemetryVCCMSB = highByte(rxVCC);
//    telemetryVCCLSB = lowByte(rxVCC);



    telemetryVCAMSB = (rxVCA >> 8) & 0xFF;  
    telemetryVCALSB = rxVCA & 0xFF;

    telemetryVCCMSB = (rxVCC >> 8) & 0xFF;  
    telemetryVCCLSB = rxVCC & 0xFF;


/*
    Serial.print("rxVCC =");Serial.print(rxVCC);                        // DEBUG CODE                                          // DEBUG CODE
    Serial.print("\trxVCA =");Serial.print(rxVCA);                      // DEBUG CODE
*/
/*
    Serial.print("\ttelemetryVCCMSB =");Serial.print(telemetryVCCMSB);  // DEBUG CODE
    Serial.print("\ttelemetryVCCLSB =");Serial.print(telemetryVCCLSB);                      // DEBUG CODE
    Serial.print("\ttelemetryVCAMSB =");Serial.print(telemetryVCAMSB);                        // DEBUG CODE                                          // DEBUG CODE
    Serial.print("\ttelemetryVCALSB =");Serial.println(telemetryVCALSB);                      // DEBUG CODE
*/

 
//  NOTE:  To make the 2 Byte (MSB & LSB) values back into an int use the following code:    
//            int telemetryVCC = (int)(word(telemetryVCCMSB,telemetryVCCLSB));    
//            int telemetryVCA = (int)(word(telemetryVCAMSB,telemetryVCALSB));
    
// --------  START OF TX
  payload[0] = telemetryVCCMSB;  // MSB of rxVCC Voltage from Receiver. Test Voltage sent is 136 as a 2 byte byte 0x88h, this will get divided by 10
  payload[1] = telemetryVCCLSB;  // LSB of rxVCC
  payload[2] = telemetryVCAMSB;  // MSB of rxVCA Amperage from Receiver. 
  payload[3] = telemetryVCALSB;  // LSB of rxVCA
  payload[4] = '0';              // Future Use - User usable
  payload[5] = '0';              // Future Use - transmitting RX Error codes back to the controller

  xbee.send(zbTx); 
 
 //  Serial.println("Sent zbTx");         // DEBUG CODE
// --------  END OF TX  
}  

void getVCC(){
//    rxVCC = random(80,260);            // DEBUG CODE
   VCCvalue = analogRead(analogVCCinput); // this must be between 0.0 and 5.0 - otherwise you'll let the blue smoke out of your Ardiuno
   vout= (VCCvalue * 5.0)/1024.0;      //voltage coming out of the voltage divider
   vin = vout / (R2/(R1+R2));          //voltage based on vout to send to Controller
   rxVCC = (vin+1)*10;                   // The +1 is for the voltage drop on the VIN pin is for us to break it down into MSB & LSB
                                         // before we send it to the XBee, otherwise, well be off
/*
    Serial.print("getVCC >> VCCvalue =");Serial.print(VCCvalue);                        // DEBUG CODE                                          // DEBUG CODE
    Serial.print("\tvout =");Serial.print(vout);                      // DEBUG CODE

    Serial.print("\tvin =");Serial.print(vin);  // DEBUG CODE
    Serial.print("\trxVCC =");Serial.print(rxVCC);                      // DEBUG CODE  
    Serial.println();
    */
}
  
void getVCA(){
  rxVCA = random(1,999);  // DEBUG CODE
}  
