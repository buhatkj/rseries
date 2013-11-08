#include <Wire.h>
#include <Servo.h> 

const int leftSensorPin = A2;
const int rightSensorPin = A3;
const int servoInputPin = 7;

int pwm_a = 10;  //PWM control for motor outputs 1 and 2 is on digital pin 10
int pwm_b = 11;  //PWM control for motor outputs 3 and 4 is on digital pin 11
int dir_a = 12;  //direction control for motor outputs 1 and 2 is on digital pin 12
int dir_b = 13;  //direction control for motor outputs 3 and 4 is on digital pin 13

const int lowStopValue = 215;
const int highStopValue = 730;

const int pulseMin = 1090;
const int pulseMax = 1900;

const int hysterisis = 10;

int goalValue = 0;
int leftValue = 0;
int rightValue = 0;
int x = 0;

void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600);
    Serial.println("R-Series Shoulder motor synchronizer and Controller.");
    pinMode(servoInputPin, INPUT);
    
    pinMode(pwm_a, OUTPUT);  //Set control pins to be outputs
    pinMode(pwm_b, OUTPUT);
    pinMode(dir_a, OUTPUT);
    pinMode(dir_b, OUTPUT);
    
    analogWrite(pwm_a, 0);
    analogWrite(pwm_b, 0);
    
    Wire.begin(7);              // join i2c bus with address #7
    Wire.onReceive(receiveEvent); // register event  
    Serial.println("done startup!");
}

void loop() {
  // put your main code here, to run repeatedly: 
    readPots();
    readGoalValue();
    motorTick(dir_a, pwm_a, leftValue);
    motorTick(dir_b, pwm_b, rightValue);
    delay(10);
}

void motorTick(int motorpindir, int motorpinspd, int potVal){
    if(closeEnough(potVal, goalValue)){
       digitalWrite(motorpindir, LOW);  //Set motor direction, 1 low, 2 high
       analogWrite(motorpinspd, 0);
    }else if(potVal < goalValue){
       digitalWrite(motorpindir, HIGH);
       analogWrite(motorpinspd, 255); 
    }else{
       digitalWrite(motorpindir, LOW); 
       analogWrite(motorpinspd, 255);
    }
}

boolean closeEnough(int testVal, int targetVal){
    if( abs(testVal - targetVal) <= hysterisis ){
        return true;
    }
    return false;
}

void receiveEvent(int howMany) {
    x = Wire.read();    // receive byte as an integer
    Serial.println(x);         // print the integer - DEBUG CODE
}

void readPots(){
    leftValue = analogRead(leftSensorPin);
    rightValue = analogRead(rightSensorPin);
}

void readGoalValue(){
    int pw = pulseIn(servoInputPin, HIGH); // read pulse
    pw = constrain(pw, pulseMin, pulseMax);
    goalValue = map(pw, pulseMin, pulseMax, lowStopValue, highStopValue); // scale to match pot range
}



