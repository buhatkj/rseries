#ifndef _RSERIES_GFX_H
#define _RSERIES_GFX_H

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Adafruit_GFX.h>
#include <Adafruit_TFTLCD.h>
#include <TouchScreen.h>
#include <stdint.h>


// Touch Screen Pin Configuration - Need to change A2 & A3, so as not to share
#define YM 9    // Y- (Minus) digital pin UNO = D9, MEGA = 9   // Orig 9
#define XM A8   // X- (Minus) must be an analog pin, use "An" notation! // Orig A2
#define YP A9   // Y+ (Plus)  must be an analog pin, use "An" notation! // Orig A3
#define XP 8    // X+ (Plus)  digital pin UNO = 8, MEGA = 8   // Orig 9

// These can be adjusted for greater precision 
#define TS_MINX 131 // Orig = 150
#define TS_MINY 120 // Orig = 120
#define TS_MAXX 920 // Orig = 920
#define TS_MAXY 950 // Orig = 940

#define rotation 3  // Which only changes the orientation of the LCD not the touch screen

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0 

#define LCD_RESET A4              // Use A4 to reset the Pin 7 of the TFT Display - Not Optional

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0 
#define WHITE           0xFFFF
#define WINE            0x8888
#define DRKBLUE         0x1111
#define TAN             0xCCCC
#define GRAY            0x038F

#define optionCOLOR RED

// button styles
#define BTN_STYLE_BOX 1
#define BTN_STYLE_ROUND_BOX 2
#define BTN_STYLE_TRIANGLE_UP 3
#define BTN_STYLE_TRIANGLE_DOWN 4
#define BTN_STYLE_CIRCLE 5

#define BTN_INDEX_MAX 20
#define MENU_INDEX_MAX 20


// Minimal class to replace std::vector
template<typename Data>
class Vector {
   size_t d_size; // Stores no. of actually stored objects
   size_t d_capacity; // Stores allocated capacity
   Data *d_data; // Stores data
   public:
     Vector() : d_size(0), d_capacity(0), d_data(0) {}; // Default constructor
     Vector(Vector const &other) : d_size(other.d_size), d_capacity(other.d_capacity), d_data(0) { d_data = (Data *)malloc(d_capacity*sizeof(Data)); memcpy(d_data, other.d_data, d_size*sizeof(Data)); }; // Copy constuctor
     ~Vector() { free(d_data); }; // Destructor
     Vector &operator=(Vector const &other) { free(d_data); d_size = other.d_size; d_capacity = other.d_capacity; d_data = (Data *)malloc(d_capacity*sizeof(Data)); memcpy(d_data, other.d_data, d_size*sizeof(Data)); return *this; }; // Needed for memory management
     void push_back(Data const &x) { if (d_capacity == d_size) resize(); d_data[d_size++] = x; }; // Adds new value. If needed, allocates more space
     size_t size() const { return d_size; }; // Size getter
     Data const &operator[](size_t idx) const { return d_data[idx]; }; // Const getter
     Data &operator[](size_t idx) { return d_data[idx]; }; // Changeable getter
   private:
     void resize() { d_capacity = d_capacity ? d_capacity*2 : 1; Data *newdata = (Data *)malloc(d_capacity*sizeof(Data)); memcpy(newdata, d_data, d_size * sizeof(Data)); free(d_data); d_data = newdata; };// Allocates double the old space
};


class rsButton{
  public:
  	int x, y, width, height, style, commandCode;
  	uint16_t color;
  	String label;
  	rsButton();
  	rsButton(int inX, int inY, int inWidth, int inHeight, int inBtnStyle, uint16_t inColor, String inLabel, int inCommandCode, void (*inCallback)(int cc));
  	void checkButton(Point testPoint);
  	void onButton();
  	void draw(Adafruit_TFTLCD tft);
  private:
  	void (*callback)(int cc);
};


class rsMenuSet{
  public:
  	rsMenuSet()
  	{
  		currentBtnIdx = 0;
  		currentMenuIdx = 0;
  	};

    void setMenuCount(int menuCount)
    {
      for(int i=0; i<menuCount; i++)
      {
        Vector<rsButton> tmp;   
        buttons.push_back(tmp); 
      }
    };

  	void addButton(rsButton inButton, int menuIndex){
  		buttons[menuIndex].push_back(inButton);
  	};

  	void drawButtons(Adafruit_TFTLCD tft)
  	{
      Serial.println("rsMenuSet::drawButtons");
  		for(int i=0;i<buttons[currentMenuIdx].size();i++)
  		{
        Serial.println("rsMenuSet::drawButtons - button");
  			buttons[currentMenuIdx][i].draw(tft);
  		}
  	};

  	void nextBtn()
  	{ 
	  	currentBtnIdx++; 
	  	if(currentBtnIdx >= buttons[currentMenuIdx].size())
	  		currentBtnIdx = 0;
	  };

  	void prevBtn()
  	{ 
  		currentBtnIdx--; 
  		if(currentBtnIdx < 0)
  	  		currentBtnIdx = (buttons[currentMenuIdx].size()-1);
	  };

  	void nextMenu()
  	{
  		currentMenuIdx++;
  		if(currentMenuIdx >= buttons.size())
	  		currentMenuIdx = 0;
  	};

  	void prevMenu()
  	{
	  	currentMenuIdx--;
	  	if(currentMenuIdx < 0)
	  		currentMenuIdx = (buttons.size()-1);
	  };

    void setMenuIndex(int inIdx)
    {
      if((inIdx >= 0)&&(inIdx < buttons.size()))
      {
        currentMenuIdx = inIdx;
      }
    };

    void checkButtons(Point testPoint)
    {
      for(int i=0;i<buttons[currentMenuIdx].size();i++)
      {
        buttons[currentMenuIdx][i].checkButton(testPoint);
      }
    };

  private:	
  	int currentBtnIdx;
  	int currentMenuIdx;
    Vector<Vector<rsButton> > buttons;
};


// class defaultMenu{
  
//   public:
//     virtual void setup(rsMenuSet &menus);
//     void setDefaultCallback(void (*inCallback)(int cc));

//   private:
//     void (*callback)(int cc);
// };


class RSeriesGFX{
  public:
  	RSeriesGFX();
    void initDisplay();
    void displayOPTIONS();
  	void displaySCROLL();
  	void displaySPLASH(String owner, String version);
  	void displayBATT(float vin, float vinSTRONG, float vinWEAK);
  	void displaySTATUS(
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
                     float vinWEAK);
    void displayRSSI(unsigned long RSSIduration);
    void clearScreen(){tft.fillScreen(BLACK);};
    void displaySendClear() { tft.fillRect(80, 21, 220, 17, BLACK); };// Clear Trigger Message
    void displaySendMessage(int triggerItem);

    void displayPOSTStage1();
    void displayPOSTStage2();
    void displayPOSTStage3();
    void displayPOSTStage4();
    void displayPOSTStage5();

    void processTouch();

    rsMenuSet menus; //public for debugging

  private:
  	TouchScreen ts;
  	Adafruit_TFTLCD tft;
  	//rsMenuSet menus;
  	Point getTouch();

};

#endif