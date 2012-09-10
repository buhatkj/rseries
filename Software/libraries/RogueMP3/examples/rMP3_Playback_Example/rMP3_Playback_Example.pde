#include <RogueMP3.h>
#include <NewSoftSerial.h>

// make sure this file exists in the root folder on the SD card
#define SONG "/mysong.mp3"

NewSoftSerial rmp3_s(6, 7);

RogueMP3 rmp3(rmp3_s);

void setup()
{
  Serial.begin(9600);
  rmp3_s.begin(9600);
  
  rmp3.sync();
  rmp3.stop();
}

void loop()
{
  char c, status = 'S';
  uint8_t i, volume = 20;
  int16_t newtime;
  playbackinfo pinfo;

  volume = rmp3.getvolume();  // this only gets the right volume

  while (1)
  {
    while (!Serial.available())
    {
      // get information and display
      switch ((status = rmp3.getplaybackstatus()))
      {
        case 'P':
          Serial.print("Playing ");
          break;
        case 'D':
          Serial.print("Paused ");
          break;
        case 'S':
          Serial.print("Stopped ");
          break;
      }
      
      if (status != 'S')
      {
        pinfo = rmp3.getplaybackinfo();

        Serial.print(pinfo.position/60,DEC);Serial.print(':');
        if(pinfo.position%60 < 10) Serial.print('0');
        Serial.print(pinfo.position%60,DEC);
        Serial.print(' ');
        Serial.print(pinfo.bitrate, DEC); Serial.print(" kbps ");
        Serial.print(pinfo.samplerate, DEC); Serial.print(" KHz ");
      }
      
      Serial.print("vol: "); Serial.println(volume, DEC);

      // This is to clear the remainder of the line on a terminal
      /*
      Serial.print("vol: "); Serial.print(volume, DEC);
      for(i = 0; i < 28; i++) Serial.print(' ');
      Serial.print('\r');
      */
    }
    
    c = Serial.read();
    
    switch(c)
    {
      case 'p':
        // pause
        if(status == 'D')
        {
          // fade in
          rmp3.playpause();
          rmp3.fade(volume);
        }
        else if(status == 'P')
        {
          // fade out
          rmp3.fade(254);
          rmp3.playpause();
        }
        break;
      case 's':
        rmp3.stop();
        break;
      case 'f':
        rmp3.playfile(SONG);
        break;
      case 'k':
        if(volume < 254) volume++;
        if(status != 'D') rmp3.setvolume(volume);
        break;
      case 'i':
        if(volume > 0) volume--;
        if(status != 'D') rmp3.setvolume(volume);
        break;
      case 'a':
        // jump back 5 seconds
        newtime = pinfo.position - 5;
        if (newtime < 0) newtime = 0;
        rmp3.jump(newtime);
        break;
      case 'd':
        // jump forward 5 seconds
        rmp3.jump(pinfo.position + 5);
        break;
    }
  }
}
