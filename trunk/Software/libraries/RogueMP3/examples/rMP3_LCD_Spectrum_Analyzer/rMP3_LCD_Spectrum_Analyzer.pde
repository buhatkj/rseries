
// rMP3 Spectrum Analyzer Demo
// on a serial LCD (Sparkfun SerLCD on a 20x4 LCD)

#include <RogueMP3.h>
#include <RogueSD.h>
#include <NewSoftSerial.h>

NewSoftSerial LCD(255, 4);
NewSoftSerial rmp3_s(6, 7);

RogueMP3 rmp3(rmp3_s);
RogueSD filecommands(rmp3_s);

#define NUMBER_OF_BANDS 10
uint16_t bandfreqs[] = { 50, 120, 200, 500, 1000, 2000, 5000, 8000, 10000, 20000 };

uint16_t numfiles = 0;

#define MP3PATH "/mp3/"
#define MP3FILTER "/mp3/""*.mp3"

#define lcdHeight 4
#define lcdWidth 20
#define maxSpecValue 20

void lcdClear()
{
   LCD.print(0xFE, BYTE);      //command flag
   LCD.print(0x01, BYTE);      //clear command.
}

void lcdBacklightOn(uint8_t level)
{  //turns on the backlight
    LCD.print(0x7C, BYTE);     //command flag for backlight stuff
    LCD.print(level, BYTE);    //light level.
}

void lcdBacklightOff()
{  //turns off the backlight
    LCD.print(0x7C, BYTE);     //command flag for backlight stuff
    LCD.print(0x80, BYTE);     //light level for off.
}

void lcdGotoXY(int x, int y)
{
  int pos;

  if (x >= lcdWidth)
    x = lcdWidth-1;
  if (y >= lcdHeight)
    y = lcdHeight-1;

  pos = x + ((y % 2) * 0x40);

  if (y >= 2)
    pos += lcdWidth;

  LCD.print(0xFE, BYTE);
  LCD.print(0x80 + pos, BYTE);
}


void lcdSetCustomChar(byte pos, byte values[])
{
  LCD.print(0xFE, BYTE);
  LCD.print(0x40 + pos * 8, BYTE);
  for (int i = 0; i < 8; i++)
    LCD.print(values[i], BYTE);
}


void lcdCustomChars(void)
{
  // sets up the custom chars
  byte chars[] = {
    0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111,
    0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111,
    0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111,
    0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111,
    0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111,
    0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111,
    0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111,
    0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111
  };

  for (int i = 0; i < 8; i++)
    lcdSetCustomChar(i, chars + i*8);

  delay(100);
}

void lcdSpecMap(int x, int value, int numBars)
{
  int8_t i, val;

  // map value of 0 - maxSpecValue -> 0->7, 0->15, 0->23, 0->31
  if (value > maxSpecValue)
    value = maxSpecValue;

  value = map(value, 0, maxSpecValue, 0, 8*numBars);

  for (i = 0; i < numBars; i++)
  {
    lcdGotoXY(x, i);
    val = value - (8 * (numBars - 1 - i));

    if (val <= 0)
      val = ' ';
    else if (val > 8)
      val = 7;
    else
      val -= 1;
    LCD.print(val, BYTE);
  }
}

void doLCDSpec(void)
{
  uint8_t v[NUMBER_OF_BANDS];

  rmp3.getspectrumanalyzer(v);


  for (uint8_t i=0; i<NUMBER_OF_BANDS; i++)
  {
    // Double up each bar
    lcdSpecMap(2*i, v[i], 4);
    lcdSpecMap(2*i+1, v[i], 4);
  }
}


void setup(void)
{
  Serial.begin(9600);
  Serial.println("Started");

  LCD.begin(9600);
  
  for(int i = 0; i < 10; i++)
  {
    LCD.print(18, BYTE);
    delay(20);
  }
  
  LCD.print(124, BYTE);
  LCD.print(16, BYTE);
  delay(20);

  // If you find that the LCD updates a bit slow, you may want to boost this to 38400
  // (you will need to change the baud rate on the SerLCD - see its documentation)
  LCD.begin(38400);
  rmp3_s.begin(9600);
  
  pinMode(2, INPUT);
  pinMode(3, INPUT);

  lcdCustomChars();
  lcdClear();

  Serial.println("Starting sync");
  rmp3.sync();
  filecommands.sync();
  Serial.println("Done sync");
  
  // Math below centers the 'Ready' on the LCD
  lcdGotoXY((lcdWidth - 5)/2, 0);
  LCD.print("Ready");
}


void playTrack(void)
{
  char mp3path[128];

  numfiles = filecommands.filecount(MP3FILTER);

  Serial.print("MP3 count: ");
  Serial.println(numfiles, DEC);
 
  // play a file (random)
  strcpy(mp3path, MP3PATH);
  filecommands.entrytofilename(mp3path + strlen(mp3path), 127, MP3FILTER, random(0, numfiles));

  Serial.println(mp3path);
  rmp3.playfile(mp3path);

  rmp3.setspectrumanalyzer(bandfreqs, NUMBER_OF_BANDS);

  Serial.println("Playing");
}



void loop(void)
{
  int i;
  // uint8_t volume, lastvolume = 20;
  static char status = 'S';

  status = rmp3.getplaybackstatus();

  while (status == 'P')
  {
    // for volume control on analog 6 (rEDI board)
    // int adc6 = analogRead(6);
    // volume = (byte)fscale(0, 1023, 0, 254, adc6, -3);

    // if (volume != lastvolume)
    // {
    //   lastvolume = volume;
    //   rmp3.setvolume(volume);
    // }

    doLCDSpec();

    status = rmp3.getplaybackstatus();
  }

  if (status == 'S')
  {
    // Math below centers the 'Stopped'
    lcdGotoXY((lcdWidth - 7) / 2, 0);
    LCD.print("Stopped");
    playTrack();
    status = rmp3.getplaybackstatus();
  }

  while (status != 'P')
  {
    status = rmp3.getplaybackstatus();
  }
}

