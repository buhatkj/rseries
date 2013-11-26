// R2ATL Lifter controller sketch V 0.1
// Ted Koenig - 11/25/2013

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"

// Input mode: 0 = button/relay, 1 = I2C
// Edit to match your choice of input mode
const int inputMode = 0;
/**********************************************************************/
//               probably can leave below here alone
/**********************************************************************/

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4) - This matches the pololu #1200 stepper
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2);

// Button/relay input pin
const int btnInputPin = 7;

// Limit switch pins
const int upperLimitPin = 6;
const int lowerLimitPin = 5;

// Lifter state
boolean lifterShouldBeUp = false;

// Somehow "x" here has become the standard variable name for input I2C event variables
// I dunno man, I just work here....
int x;

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("Stepper test!");
  
  // pin modes
  pinMode(lowerLimitPin, INPUT); 
  pinMode(upperLimitPin, INPUT); 
  pinMode(btnInputPin, INPUT); 
  
  AFMS.begin();  // create with the default frequency 1.6KHz
  //AFMS.begin(1000);  // OR with a different frequency, say 1KHz
  myMotor->setSpeed(100);  // 10 rpm  
  myMotor->release();  // let the motor chill out till somebody tells it what to do.
  
  //THe motor shield is actually on I2c too locally, hopefully the default address doesnt conflict with anything :-p
  Wire.begin(22);                // join i2c bus with address #22 Periscope & LFS Lifter Module
  Wire.onReceive(receiveEvent); // register event
}

void loop() {
  readInput();
  if(lifterShouldBeUp == true)
  {
    motorUp();
  }
  else
  {
    motorDown();
  }
  delay(50);
}

// Sets the state of the lifter
void readInput()
{
  if(inputMode == 1)
  {
    // Just picking the next two integer values after the ones I used in the relay I2C controller here...
    if (x == 93) {lifterShouldBeUp = true;}
    if (x == 94) {lifterShouldBeUp = false;}
  }
  else //assume 0
  {
    if(digitalRead(btnInputPin) == HIGH)
    {
      Serial.println("go up btn"); 
      lifterShouldBeUp = true;
    }
    else
    {
      Serial.println("go down btn"); 
      lifterShouldBeUp = false; 
    }
  }
}


void motorUp()
{
  if(digitalRead(upperLimitPin) != HIGH)
  {
     myMotor->step(10, FORWARD, DOUBLE);
  }
  else
  {
     // Do nothing, just stay there and keep the stepper powered. 
     Serial.println("go up limit"); 
  }
}


void motorDown()
{
  // If we haven't hit the lower limit, keep goin down
  if(digitalRead(lowerLimitPin) != HIGH)
  {
     myMotor->step(10, BACKWARD, DOUBLE);
  }
  else
  {
     // We've hit the lower limit, so no need to keep motor power on, just chill...
     Serial.println("go down limit"); 
     myMotor->release();
  }
}


// receiveEvent function executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
   x = Wire.read();    // receive byte as an integer
//    Serial.println(x);         // print the integer - DEBUG CODE
   Serial.print("Received = "); 
   Serial.println(x);
}
