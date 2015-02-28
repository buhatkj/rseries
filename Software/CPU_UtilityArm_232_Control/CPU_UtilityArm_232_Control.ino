// CPU arm/utility arm/2-3-2 servo controller sketch v0.01
// Ted Koenig - 2/27/2015

#include <Wire.h>
#include <Servo.h> 

// Lifter state
boolean centerLegUp = false;
boolean outLegsIn = false;
boolean cpuArmUp = false;
boolean topUtilityArmOut = false;
boolean bottomUtilityArmOut = false;

const int CPUArmPin = 3;
const int TopUtilityArmPin = 5;
const int BottomUtilityArmPin = 9;
const int CenterLegPin = 10;
const int OuterLegPin = 11;

Servo cpuArmServo;
Servo topUtilityServo;
Servo bottomUtilityServo;
Servo centerLegServo;
Servo outerLegServo;

// Somehow "x" here has become the standard variable name for input I2C event variables
// I dunno man, I just work here....
int x;

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("CPU arm/utility arm/2-3-2 servo controller!");
  
  cpuArmServo.attach(CPUArmPin);
  topUtilityServo.attach(TopUtilityArmPin);
  bottomUtilityServo.attach(BottomUtilityArmPin);
  centerLegServo.attach(CenterLegPin);
  outerLegServo.attach(OuterLegPin);

  Wire.begin(38);                // join i2c bus with address #22 Periscope & LFS Lifter Module
  Wire.onReceive(receiveEvent); // register event
}

void loop() {
  readInput();
  if(cpuArmUp == true) { cpuArmServo.attach(CPUArmPin); cpuArmServo.write(45); } else {  cpuArmServo.write(180); delay(200); cpuArmServo.detach(); }
  if(topUtilityArmOut == true) {  topUtilityServo.attach(TopUtilityArmPin); topUtilityServo.write(0); } else { topUtilityServo.write(90); delay(200); topUtilityServo.detach(); }
  if(bottomUtilityArmOut == true) { bottomUtilityServo.attach(BottomUtilityArmPin); bottomUtilityServo.write(0); } else { bottomUtilityServo.write(90); delay(200); bottomUtilityServo.detach(); }
  if(centerLegUp == true) { centerLegServo.write(180); } else { centerLegServo.write(0); }
  if(outLegsIn == true) { outerLegServo.write(180); } else { outerLegServo.write(0); }
  delay(50);
}

// Sets the state of the lifter
void readInput()
{
  // Just picking the next two integer values after the ones I used in the relay I2C controller here...
  if (x == 95) {cpuArmUp = true;}
  if (x == 96) {cpuArmUp = false;}
  if (x == 97) {topUtilityArmOut = true;}
  if (x == 98) {topUtilityArmOut = false;}
  if (x == 99) {bottomUtilityArmOut = true;}
  if (x == 100) {bottomUtilityArmOut = false;}
  if (x == 101) {centerLegUp = true;}
  if (x == 102) {centerLegUp = false;}
  if (x == 103) {outLegsIn = true;}
  if (x == 105) {outLegsIn = false;}
}

// receiveEvent function executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
   x = Wire.read();    // receive byte as an integer
//    Serial.println(x);         // print the integer - DEBUG CODE
   Serial.print("Received = "); 
   Serial.println(x);
}
