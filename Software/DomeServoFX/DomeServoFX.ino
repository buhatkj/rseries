/*
 * Astromech RSeries Dome Servo FX for the R2 Builders Club
 *  
 * Creative Copyright v3 SA BY - 2012 Michael Erwin 
 *                               michael(dot)erwin(at)mac(dot)com 
 *
 * RSeries Open Controller Project  http://code.google.com/p/rseries-open-control/
 * Requires Arduino 1.0 IDE
 *
 
  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/815

  These displays use I2C to communicate, 2 pins are required to  
  interface. For Arduino UNOs, thats SCL -> Analog 5, SDA -> Analog 4

 * Liberal use of code from the following: 
 * Tim Hirzel   http://www.growdown.com
 * Fabio Biondi http://www.fabiobiondi.com
 * Tod Kurt     http://todbot.com
 * Chad Philips http://www.windmeadow.com/node/42
 * Limor Fried  http://adafruit.com/
 * ArduPilot-Mega Development Team http://code.google.com/p/ardupilot-mega/
 *
*/

#define VERSION "beta 0.0.3"
// 0.2.5 - Updated configuration of AudioFX1 & AudioFX2, Added DroidRACEmode, included project page link
// 0.2.4 - Voice Recoginition Integration started via i2C
// 0.2.3 - Updated Servo Pins to match RSeries Receiver PCB

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  #else
   #include "WProgram.h"
#endif

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// you will need to adjust these variables for your configuration
boolean HP1installed = false; // Front
boolean HP2installed = true;  // Rear
boolean HP3installed = true;  // Top



// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
// you can also call it with a different address you want
//Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x41);

// Depending on your servo make, the pulse width min and max may vary, you 
// want these to be as small/large as possible without hitting the hard stop
// for max range. You'll have to tweak them as necessary to match the servos you
// have!
// Hitec Servo MIN & MAX Definitions
// HS55 140, 550
// HS56HB 140, 550
// HS65
// HS225MG 130, 550
// HS645MG 130, 550


#define SERVOMIN  130 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  550 // this is the 'maximum' pulse length count (out of 4096)

#define A40S0OPEN 240  // Dome Panel 1 // Fire Extengisher
#define A40S0CLOSE 450

#define A40S1OPEN 230 // Dome Panel 2
#define A40S1CLOSE 440
#define A40S2OPEN 220 // Dome Panel 3
#define A40S2CLOSE 444
#define A40S3OPEN 250 // Dome Panel 4
#define A40S3CLOSE 500
#define A40S4OPEN 290 // Pie Panel 1
#define A40S4CLOSE 474
#define A40S5OPEN 350 // Pie Panel 2
#define A40S5CLOSE 511
#define A40S6OPEN 250 // Pie Panel 3 - Typically Periscope & Not Used
#define A40S6CLOSE 500
#define A40S7OPEN 250 // Pie Panel 4 - Typically HP & Not Used
#define A40S7CLOSE 500
#define A40S8OPEN 250 // Pie Panel 5
#define A40S8CLOSE 500
#define A40S9OPEN 300 // Pie Panel 6
#define A40S9CLOSE 472


#define A40S10MIN 300 //HP1-A Front 
#define A40S10MAX 400
#define A40S11MIN 300 //HP1-B
#define A40S11MAX 400
#define A40S12MIN 300 //HP2-A Rear
#define A40S12MAX 400
#define A40S13MIN 300 //HP2-B
#define A40S13MAX 400
#define A40S14MIN 300 //HP3-A Top
#define A40S14MAX 400
#define A40S15MIN 300 //HP3-B
#define A40S15MAX 400


#undef round
int x;

long loopCount = 0;
unsigned long timeNew = 0;
unsigned long timeHP1New = 0;
unsigned long timeHP2New = 0;
unsigned long timeHP3New = 0;
long twitchTime = 0;
long twitchHP1Time = 0;
long twitchHP2Time = 0;
long twitchHP3Time = 0;


// our servo # counter
uint8_t servonum = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("16 channel Servo test!");

// mix up our random number generator
randomSeed(analogRead(0));



twitchTime = 1300;   // Initial time 
twitchHP1Time = 1300;   // Initial time 
twitchHP2Time = 2000;   // Initial time 
twitchHP3Time = 2800;   // Initial time 



  pwm.begin();                // join i2c bus with address #6
  delay(10);
  Wire.onReceive(receiveEvent); // register event

  
  pwm.setPWMFreq(50);  // Analog servos run at ~60 Hz updates
}

// you can use this function if you'd like to set the pulse length in seconds
// e.g. setServoPulse(0, 0.001) is a ~1 millisecond pulse width. its not precise!
void setServoPulse(uint8_t n, double pulse) {
  double pulselength;
  
  pulselength = 1000000;   // 1,000,000 us per second
  pulselength /= 50;   // 60 Hz
  Serial.print(pulselength); Serial.println(" us per period"); 
  pulselength /= 4096;  // 12 bits of resolution
  Serial.print(pulselength); Serial.println(" us per bit"); 
  pulse *= 1000;
  pulse /= pulselength;
  Serial.println(pulse);
  pwm.setPWM(n, 0, pulse);
}

void loop() {
  loopCount++;
  // Setup HP Twitch
  timeNew= millis();
  timeHP1New= timeNew;
  timeHP2New= timeNew;
  timeHP3New= timeNew;
  if (HP1installed==true) {twitchHP1(timeHP1New);}
  if (HP2installed==true) {twitchHP2(timeHP2New);}
  if (HP3installed==true) {twitchHP3(timeHP3New);}
  
  // Look for these events from i2C
  if (x == 8) {openPanel0();delay(5000);closePanel0();delay(1000);}// Fire Exteng
  if (x == 29) {closePanelALL();}// All Panels Close 
  if (x == 30) {openPanelALL();}// DOME // DOME Rotation LEFT Start
  if (x == 31) {closePanelALL();}      // DOME Rotation End
  if (x == 32) {closePanelALL();}    // Activation CPU ARM
  if (x == 33) {closePanelALL();} // HOLO PROJECT Start & Continue
  if (x == 35) {closePanelALL();} // HOLO PROJECT Start & Continue
  if (x == 36) {closePanelALL();} // HOLO PROJECT Start & Continue

  // Drive each servo one at a time
//  Serial.println(servonum);
//           (Servo #, Address Offset (0=40), PulseLength Variable)
/*
  pwm.setPWM(0, 0, A40S0OPEN);
  pwm.setPWM(1, 0, A40S1OPEN);
  pwm.setPWM(2, 0, A40S2OPEN);
  pwm.setPWM(4, 0, A40S4OPEN);
  pwm.setPWM(8, 0, A40S8OPEN);
  
  delay(2000);
  pwm.setPWM(8, 0, A40S8CLOSE);
    delay(300);
  pwm.setPWM(4, 0, A40S4CLOSE);
  delay(300);
  pwm.setPWM(2, 0, A40S2CLOSE);
  delay(300);
   pwm.setPWM(1, 0, A40S1CLOSE);
  delay(300);
   pwm.setPWM(0, 0, A40S0CLOSE);
*/  
  delay(5);
}


void twitchHP1(unsigned long elapsed)              // Twitch HP1 Front
{
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < random(10,27)*1000) return;
  timeLast = elapsed; 
  uint16_t randomHP1A = random(A40S10MIN,A40S10MAX);
  uint16_t randomHP1B = random(A40S11MIN,A40S11MAX);
  pwm.setPWM(10, 0, randomHP1A);
  pwm.setPWM(11, 0, randomHP1B);
}

void twitchHP2(unsigned long elapsed){  // Twitch HP2 Rear
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < random(7,30)*1000) return;
  timeLast = elapsed; 
  uint16_t randomHP2A = random(A40S12MIN,A40S12MAX);
  uint16_t randomHP2B = random(A40S13MIN,A40S13MAX);
  pwm.setPWM(12, 0, randomHP2A);
  pwm.setPWM(13, 0, randomHP2B);
}

void twitchHP3(unsigned long elapsed){  // Twitch HP3 Top
  static unsigned long timeLast=0;
  if ((elapsed - timeLast) < random(7,30)*1000) return;
  timeLast = elapsed; 
  uint16_t randomHP3A = random(A40S14MIN,A40S14MAX);
  uint16_t randomHP3B = random(A40S15MIN,A40S15MAX);
  pwm.setPWM(14, 0, randomHP3A);
  pwm.setPWM(15, 0, randomHP3B);
}


/*
void loop() {
  // Drive each servo one at a time
  Serial.println(servonum);
  for (uint16_t pulselen = SERVOMIN; pulselen < SERVOMAX; pulselen++) {
    pwm.setPWM(servonum, 0, pulselen);
  }
  delay(500);
  for (uint16_t pulselen = SERVOMAX; pulselen > SERVOMIN; pulselen--) {
    pwm.setPWM(servonum, 0, pulselen);
  }
  delay(500);

  servonum ++;
  if (servonum > 1) servonum = 0;
}
*/
// receiveEvent function executes whenever data is received from master
// this function is registered as an event, see setup()

void receiveEvent(int howMany) {
    x = Wire.read();    // receive byte as an integer
    Serial.println(x);         // print the integer - DEBUG CODE
}

void openPanel0() {
  pwm.setPWM(0, 0, A40S0OPEN);
}
void openPanel1() {
  pwm.setPWM(1, 0, A40S1OPEN);
}
void openPanel2() {
  pwm.setPWM(2, 0, A40S2OPEN);
}    
void openPanel3() {
  pwm.setPWM(3, 0, A40S3OPEN);
}      

void openPanel4() {
  pwm.setPWM(4, 0, A40S4OPEN);
}      

void openPanel5() {
  pwm.setPWM(5, 0, A40S5OPEN);
}      

void openPanel6() {
  pwm.setPWM(6, 0, A40S6OPEN);
}      

void openPanel7() {
  pwm.setPWM(7, 0, A40S7OPEN);
}      

void openPanel8() {
  pwm.setPWM(8, 0, A40S8OPEN);
}

void openPanel9() {
  pwm.setPWM(9, 0, A40S9OPEN);
}      


// CLOSE PANEL 
void closePanel0() {
  pwm.setPWM(0, 0, A40S0CLOSE);
  delay(300);
}  

void closePanel1() {
  pwm.setPWM(1, 0, A40S1CLOSE);
  delay(300);
}  

void closePanel2() {
  pwm.setPWM(2, 0, A40S2CLOSE);
  delay(300);
}  

void closePanel3() {
  pwm.setPWM(3, 0, A40S3CLOSE);
  delay(300);
}  

void closePanel4() {
  pwm.setPWM(4, 0, A40S4CLOSE);
  delay(300);
}  

void closePanel5() {
  pwm.setPWM(5, 0, A40S5CLOSE);
  delay(300);
}  

void closePanel6() {
  pwm.setPWM(6, 0, A40S6CLOSE);
  delay(300);
}  

void closePanel7() {
  pwm.setPWM(7, 0, A40S7CLOSE);
  delay(300);
}  

void closePanel8() {
  pwm.setPWM(8, 0, A40S8CLOSE);
  delay(300);
}  

void closePanel9() {
  pwm.setPWM(9, 0, A40S9CLOSE);
  delay(300);
}  


// Composite Panel Effects

void openPanelALL() {
  openPanel0();
  openPanel1();
  openPanel2();
  openPanel3();
  openPanel4();
  openPanel5();
  openPanel6();
  openPanel7();
  openPanel8();
  openPanel9();
  delay(300);
}  

void closePanelALL() {
  closePanel0();
  closePanel1();
  closePanel2();
  closePanel3();
  closePanel4();
  closePanel5();
  closePanel6();
  closePanel7();
  closePanel8();
  closePanel9();
  delay(300);
}  

void piePanelexplode() {
  openPanel4();
  openPanel5();
  openPanel6();
  openPanel7();
  openPanel8();
  openPanel9();
}

void piePanelclose() {
  closePanel4();
  closePanel5();
  closePanel6();
  closePanel7();
  closePanel8();
  closePanel9();
  delay(300);
}  

void piePanelopenwave() {
  openPanel4();
    delay(300);
  openPanel5();
    delay(300);
  openPanel6();
    delay(300);
  openPanel7();
    delay(300);
  openPanel8();
    delay(300);
  openPanel9();
    delay(300);
}

void piePanelclosewave() {
  closePanel4();
    delay(300);
  closePanel5();
    delay(300);
  closePanel6();
    delay(300);
  closePanel7();
    delay(300);
  closePanel8();
    delay(300);
  closePanel9();
    delay(300);
}  

void piePanelmaul() {
  openPanel4();
    delay(300);
  openPanel5();
    delay(300);
  openPanel6();
    delay(300);
  openPanel7();
    delay(300);
  openPanel8();
    delay(300);
  openPanel9();
    delay(300);
}


