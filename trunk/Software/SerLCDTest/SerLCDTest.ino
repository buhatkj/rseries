#include <SoftwareSerial.h>
#include <SFSerLCD.h>

SFSerLCD LCD = SFSerLCD(2);

void setup()
{
LCD.clearLCD();
LCD.printStr("Aren't we going to dagobah?");
LCD.goTo(25);
LCD.printStr("NOW!");
LCD.backlight50();
}

void loop()
{
delay(100);
}


