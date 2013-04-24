#include "Arduino.h"
#include "SFSerLCD.h"


SFSerLCD::SFSerLCD(int serialPin):LCD(0, serialPin){
	serialPin = serialPin;
	pinMode(serialPin, OUTPUT);
	LCD.begin(9600);
}

//SerialLCD Functions
void SFSerLCD::selectLineOne(){  //puts the cursor at line 0 char 0.
   LCD.write(0xFE);   //command flag
   LCD.write(128);    //position
}
void SFSerLCD::selectLineTwo(){  //puts the cursor at line 2 char 0.
   LCD.write(0xFE);   //command flag
   LCD.write(192);    //position
}
void SFSerLCD::selectLineThree(){  //puts the cursor at line 3 char 0.
   LCD.write(0xFE);   //command flag
   LCD.write(148);    //position
}
void SFSerLCD::selectLineFour(){  //puts the cursor at line 4 char 0.
   LCD.write(0xFE);   //command flag
   LCD.write(212);    //position
}
void SFSerLCD::goTo(int position) { //position = line 1: 0-19, line 2: 20-39, etc, 79+ defaults back to 0
if (position<20){ LCD.write(0xFE);   //command flag
              LCD.write((position+128));    //position
}else if (position<40){LCD.write(0xFE);   //command flag
              LCD.write((position+128+64-20));    //position 
}else if (position<60){LCD.print(0xFE);   //command flag
              LCD.write((position+128+20-40));    //position
}else if (position<80){LCD.print(0xFE);   //command flag
              LCD.write((position+128+84-60));    //position              
} else { goTo(0); }
}
void SFSerLCD::clearLCD(){
   LCD.write(0xFE);   //command flag
   LCD.write(0x01);   //clear command.
}
void SFSerLCD::backlightOn(){  //turns on the backlight
    LCD.write(0x7C);   //command flag for backlight stuff
    LCD.write(157);    //light level.
}
void SFSerLCD::backlightOff(){  //turns off the backlight
    LCD.write(0x7C);   //command flag for backlight stuff
    LCD.write(128);     //light level for off.
}
void SFSerLCD::backlight50(){  //sets the backlight at 50% brightness
    LCD.write(0x7C);   //command flag for backlight stuff
    LCD.write(143);     //light level for off.
}
void SFSerLCD::serCommand(){   //a general function to call the command flag for issuing all other commands   
	LCD.write(0xFE);
}

void SFSerLCD::printStr(char* str){
	LCD.print(str);
}
