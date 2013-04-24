/*
  SFSerLCD.h - Library for Sparkfun Serial LCDs.
  Ted Koenig, 3/27/2011
  GPLv3
*/
#ifndef SFSerLCD_h
#define SFSerLCD_h

#include "Arduino.h"
#include <SoftwareSerial.h>

class SFSerLCD
{
  public:
    SFSerLCD(int serialPin);
    void selectLineOne();
    void selectLineTwo();
	void selectLineThree();
	void selectLineFour();
	void goTo(int position);
	void clearLCD();
	void backlightOn();
	void backlightOff();
	void backlight50();
	void serCommand();
	void printStr(char* str);

  private:
    int serialPin;
	SoftwareSerial LCD;
};

#endif