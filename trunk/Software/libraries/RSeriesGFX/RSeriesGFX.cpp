#include <avr/pgmspace.h>
#include "RSeriesGFX.h"

          
rsButton::rsButton(int inX, int inY, int inWidth, int inHeight, int inBtnStyle, uint16_t inColor, String inLabel, int inCommandCode, void (*inCallback)(int cc))
{
	x = inX;
	y = inY;
	width = inWidth;
	height = inHeight;
	style = inBtnStyle;
	commandCode = inCommandCode;
	callback = inCallback;
	color = inColor;
	label = inLabel;
}


void rsButton::onButton()
{
	(*callback)(commandCode);
}


void rsButton::checkButton(Point testPoint)
{
	if( ((testPoint.x >= x) && (testPoint.x <= (x+width)))&&
	    ((testPoint.y >= y) && (testPoint.y <= (y+height)))
	  )	
	  {
	  	onButton();
	  }
}


void rsButton::draw(Adafruit_TFTLCD tft)
{
	if(style == BTN_STYLE_BOX)
	{
		tft.drawRect(x, y, width, height, color);
	}
	else if(style == BTN_STYLE_ROUND_BOX)
	{
		tft.drawRoundRect(x, y, width, height,2, color);
	}
	else if(style == BTN_STYLE_TRIANGLE_UP)
	{
		tft.drawTriangle((x+(width/2)), y, x, (y+height), (x+width), (y+height), color); 
	}
	else if(style == BTN_STYLE_TRIANGLE_DOWN)
	{
		tft.drawTriangle((x+(width/2)), (y+height), x, (y), (x+width), (y), color); 
	}
	else if(style == BTN_STYLE_CIRCLE)
	{
		int r = (width / 2);
		tft.drawCircle((x+r),(y+r), r, color);
	}
}


RSeriesGFX::RSeriesGFX():tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET), ts(XP, YP, XM, YM){}


void RSeriesGFX::initDisplay()
{
  tft.reset();
  uint16_t identifier = tft.readRegister(0x0);
  if (identifier == 0x9325) {
    Serial.println("Found ILI9325");
  } 
  else if (identifier == 0x9328) {
    Serial.println("Found ILI9328");
  } 
  else if (identifier == 0x7575) {
    Serial.println("Found HX8347G");
  } 
  else {
    Serial.print("Unknown driver chip ");
    Serial.println(identifier, HEX);
    return;
  }  

  tft.begin(identifier);
  tft.setRotation(rotation); 
  tft.fillScreen(BLACK);
}


void RSeriesGFX::processTouch()
{
  menus.checkButtons(getTouch());
}

void RSeriesGFX::displayOPTIONS()
{
  Serial.println("RSeriesGFX::displayOPTIONS");
  menus.drawButtons(tft);
}

void RSeriesGFX::displayPOSTStage1()
{
  tft.setCursor(0, 0);
  tft.setTextColor(GREEN);
  tft.setTextSize(2);
  tft.println("Testing...");
  tft.setCursor(20,20);
  tft.println("Hand Controller:");
  tft.setCursor(220,20);
  tft.setTextColor(RED);
  tft.println("waiting");
}


void RSeriesGFX::displayPOSTStage2()
{
  tft.fillRect(220, 20, 100, 17, BLACK);// Clear Waiting Message
  tft.setCursor(220,20);
  tft.setTextColor(GREEN);
  tft.println("OK");

  tft.setTextColor(GREEN);
  tft.setCursor(20,40);
  tft.println("Transmitter:");
  tft.setCursor(220,40);
  tft.setTextColor(RED);
  tft.println("waiting");
}


void RSeriesGFX::displayPOSTStage3()
{
  tft.fillRect(220, 40, 100, 17, BLACK);// Clear Waiting Message
  tft.setCursor(220,40);
  tft.setTextColor(GREEN);
  tft.println("OK");  

  tft.setTextColor(GREEN);
  tft.setCursor(20,60);
  tft.println("Receiver:");
  tft.setCursor(220,60);
  tft.setTextColor(RED);
  tft.println("waiting");
  tft.setCursor(220,60);
  tft.setTextColor(GREEN);

  tft.setCursor(220,60);
}


void RSeriesGFX::displayPOSTStage4()
{
  tft.fillRect(220, 60, 100, 17, BLACK);// Clear Waiting Message
  tft.setCursor(220,60);
  tft.setTextColor(GREEN);
  tft.println("OK");  

  // Looking for the Volts & Amperage from Receiver (Router)
  tft.setTextColor(GREEN);
  tft.setCursor(20,80);
  tft.println("Telemetry:");
  tft.setCursor(220,80);
  tft.setTextColor(RED);
  tft.println("waiting");
}


void RSeriesGFX::displayPOSTStage5()
{
  tft.fillRect(220, 80, 100, 17, BLACK);// Clear Waiting Message
  tft.setCursor(220,80);
  tft.setTextColor(GREEN);
  tft.println("OK");  
}


void RSeriesGFX::displaySendMessage(int triggerItem)
{
  tft.setCursor(80, 22);
  tft.setTextColor(GREEN);
  tft.setTextSize(2);
  tft.println("Sending:");
  tft.setCursor(180, 22);
  tft.println(int(triggerItem)); // using INT to make the byte readable on the display as a number vs a single byte
}


Point RSeriesGFX::getTouch()
{
	Point p = ts.getPoint();
	Point touch;
	if (p.z > ts.pressureThreshhold)
	{
		touch.y = map(p.x, TS_MINX, TS_MAXX, 240, 0);        // Notice mapping touchedY to p.x
    touch.x = map(p.y, TS_MAXY, TS_MINY, 320, 0);        // Notice mapping touchedX to p.y
	}
	return touch;
}


void RSeriesGFX::displaySCROLL() {                // Display Scroll Buttons
  tft.drawTriangle(30, 40,            // Up Scroll Button
  0, 100,
  60,100, BLUE); 
  tft.setCursor(20, 80);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.println("UP");

  tft.drawTriangle(0, 175,              // Down Scroll Button
  60, 175,
  30, 235, BLUE); 

  tft.setCursor(20, 180);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.println("DN");
}


void RSeriesGFX::displaySPLASH(String owner, String version) {                        // Display a Retro SPLASH Title Screen!
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(40, 40);
  tft.println(owner);
  tft.setCursor(40, 80);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.println("RSeries Open");
  tft.setCursor(60,110);
  tft.println("Controller");
  tft.setCursor(130,140);
  tft.setTextSize(2);
  tft.println(version);
  tft.setTextColor(BLUE);
  tft.setTextSize(2);
  tft.drawFastHLine(0,0, tft.width(), BLUE);
  tft.drawFastHLine(0,10, tft.width(), BLUE);

  tft.setCursor(20,170);
  tft.println("CC V3 SA BY 2012 M.Erwin");    // No vanity here... :-)
  tft.setCursor(20,190);
  tft.println("Royal Engineers of Naboo");    // Need to make it look like SW Canon
  tft.drawFastHLine(0, 229, tft.width(), BLUE); 
  tft.drawFastHLine(0, 239, tft.width(), BLUE);   
}


void RSeriesGFX::displayBATT(float vin, float vinSTRONG, float vinWEAK) {              
  // Display Local Battery Status
  tft.fillRect(84, 0, 9, 17, BLACK);       // Erase Battery Status area

  // Draw the battery outline in white
  tft.drawFastHLine(86, 0, 4, WHITE); // This is the little top of the battery
  tft.drawFastHLine(86, 1, 4, WHITE);
  tft.drawRect(84, 2, 8, 15, WHITE);       // Body of the battery

  if (vin >= vinSTRONG) {   
    tft.fillRect(85, 3, 6, 14, GREEN);    // If Battery is strong then GREEN  
  } 
  else if (vin >= vinWEAK) {
    tft.fillRect(85, 8, 6, 9, YELLOW);    // If Battery is medium then YELLOW
  } 
  else {
    tft.fillRect(85, 12, 6, 4, RED);    // If Battery is low then RED & Sound Warning
  }     
}


void RSeriesGFX::displaySTATUS(
                   uint16_t color, 
                   String mechName, 
                   String radiostatus, 
                   unsigned long RSSIduration,
                   int ProcessStateIndicator,
                   float dmmVCC,
                   float yellowVCC,
                   float redVCC,
                   float dmmVCA,
                   float yellowVCA,
                   float redVCA,
                   float vin, 
                   float vinSTRONG, 
                   float vinWEAK) 
{          // This builds the status line at top of display when in normal operation on the controller
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.println(mechName);
  displayRSSI(RSSIduration);
  displayBATT(vin, vinSTRONG, vinWEAK);
  tft.setCursor(128, 0);
  if (radiostatus == "OK") {
    tft.setTextColor(GREEN);
  } 
  else {
    tft.setTextColor(RED);
  }
  tft.println(radiostatus);

  if (ProcessStateIndicator == 0) {  // Display a ball heart beat in between Radio Status & dmmVCC
    tft.fillCircle(160,7, 5, BLUE);          
    ProcessStateIndicator = 1;
  } 
  else {
    tft.fillCircle(160,7, 5, RED);
    ProcessStateIndicator = 0;
  }

  tft.setCursor(170, 0);

  tft.setTextColor(GREEN);    // Default dmmVCC text color
  if (dmmVCC <= yellowVCC){    // If dmmVCC drops BELOW, change text to YELLOW or RED
    tft.setTextColor(YELLOW);
  }
  if (dmmVCC <= redVCC){
    tft.setTextColor(RED);
  }

  tft.setTextSize(2);
  tft.println(dmmVCC,2);      // Display dmmVCC FLOAT 2 Places
  tft.setCursor(230, 0);
  tft.println("V");

  tft.setTextColor(GREEN);    // Default dmmVCA text color
  if (dmmVCA >= yellowVCA){    // If dmmVCA goes ABOVE, change text to YELLOW or RED
    tft.setTextColor(YELLOW);
  }
  if (dmmVCA >= redVCA){
    tft.setTextColor(RED);
  }

  tft.setCursor(250, 0);
  tft.println(dmmVCC,2);      // Display dmmVCA FLOAT 2 Places
  tft.setCursor(310, 0);
  tft.println("A");

  tft.drawFastHLine(0, 19, tft.width(), BLUE);

  //lastStatusBarUpdate = millis();        // Could remove this one now..
  //nextStatusBarUpdate = millis() + updateSTATUSdelay; // Update Status Bar on Display every X + millis();
}


void RSeriesGFX::displayRSSI(unsigned long RSSIduration) 
{
 
  int RSSI;
  //unsigned long RSSIduration;
  long rx1DBm;
                         
  // Display Received packet Signal Strength Indicator
  //RSSIduration = rx16.getRssi();
  rx1DBm = RSSIduration;

  //RSSI (in dBm) is converted to PWM counts using the following equation: PWM counts = (41 * RSSI_Unsigned) - 5928
  // Signal Bars, Well its good enough for Apple.
  if (rx1DBm == 0) {                                     // No Signal Text
    tft.fillRect(100, 0, 22, 17, BLACK);                 // Erase Signal Bar area
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(100, 0);
    tft.println("No");
    tft.setCursor(100, 9);
    tft.println("Sgnl");
    tft.setTextSize(2);
  } 
  else if (rx1DBm >=1 && rx1DBm <=20) {
    RSSI=1;
  }                 // Signal Bars, Well its good enough for Apple.
  else if (rx1DBm >=21 && rx1DBm<=50) {
    RSSI=2;
  }
  else if (rx1DBm >=51 && rx1DBm<=100) {
    RSSI=3;
  }
  else if (rx1DBm >=101 && rx1DBm<=175) {
    RSSI=4;
  }
  else if (rx1DBm >=176 && rx1DBm<=255) {
    RSSI=5;
  }

  tft.fillRect(100, 0, 22, 17, BLACK);                 // Erase Signal Bar area

  tft.drawFastVLine(100, 12, 2, GRAY);  // Signal =1 
  tft.drawFastVLine(101, 12, 2, GRAY);

  tft.drawFastVLine(105, 10, 4, GRAY);  // Signal =2
  tft.drawFastVLine(106, 10, 4, GRAY);

  tft.drawFastVLine(110, 8, 6, GRAY);  // Signal =3 
  tft.drawFastVLine(111, 8, 6, GRAY);

  tft.drawFastVLine(115, 5, 9, GRAY);  // Signal =4 
  tft.drawFastVLine(116, 5, 9, GRAY);

  tft.drawFastVLine(120, 1, 13, GRAY);  // Signal =5 
  tft.drawFastVLine(121, 1, 13, GRAY);

  if (RSSI>=1) {
    tft.drawFastVLine(100, 12, 2, WHITE);  // Signal =1 
    tft.drawFastVLine(101, 12, 2, WHITE);
  }  
  if (RSSI>=2) {
    tft.drawFastVLine(105, 10, 4, WHITE);  // Signal =2
    tft.drawFastVLine(106, 10, 4, WHITE);
  }                                                                     
  if (RSSI>=3) {
    tft.drawFastVLine(110, 8, 6, WHITE);  // Signal =3 
    tft.drawFastVLine(111, 8, 6, WHITE);
  }
  if (RSSI>=4) {
    tft.drawFastVLine(115, 5, 9, WHITE);  // Signal =4 
    tft.drawFastVLine(116, 5, 9, WHITE);
  }
  if (RSSI>=5) {
    tft.drawFastVLine(120, 1, 13, WHITE);  // Signal =5 
    tft.drawFastVLine(121, 1, 13, WHITE);
  }
}




////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////                  DEFAULT menu                  ////////////////////////////////////

// void defaultMenu::setDefaultCallback(void (*inCallback)(int cc))
// {
//   callback = inCallback;
// }


// void defaultMenu::setup(rsMenuSet &menus)
// {
//   menus.setMenuCount(10);

//   rsButton Alarm = rsButton(70, 40, 100, 30, BTN_STYLE_BOX, TAN, "Alarm", 1, callback);
//   rsButton Leia = rsButton(70, 70, 100, 30, BTN_STYLE_BOX, TAN, "Leia", 2, callback);
//   rsButton Failure = rsButton(70, 100, 100, 30, BTN_STYLE_BOX, TAN, "Failure", 3, callback);
//   rsButton Happy = rsButton(70, 130, 100, 30, BTN_STYLE_BOX, TAN, "Happy", 4, callback);
//   rsButton Whistle = rsButton(70, 160, 100, 30, BTN_STYLE_BOX, TAN, "Whistle", 5, callback);
//   rsButton Razzberry = rsButton(70, 190, 100, 30, BTN_STYLE_BOX, TAN, "Razzberry", 6, callback);
//   rsButton Annoyed = rsButton(70, 220, 100, 30, BTN_STYLE_BOX, TAN, "Annoyed", 7, callback);
//   menus.addButton(Alarm, 0);
//   menus.addButton(Leia, 0);
//   menus.addButton(Failure, 0);
//   menus.addButton(Happy, 0);
//   menus.addButton(Whistle, 0);
//   menus.addButton(Razzberry, 0);
//   menus.addButton(Annoyed, 0);

// }