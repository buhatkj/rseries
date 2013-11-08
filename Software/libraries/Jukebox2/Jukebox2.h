/*
  Jukebox2.h - Library for sound and music. altered for wave shield
  Ted Koenig, 7/17/2011
  GPL V3
*/
#ifndef Jukebox2_h
#define Jukebox2_h

#if (ARDUINO >= 100)
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif

#include "WaveHC.h"
#include "WaveUtil.h"

/*
 * Define macro to put error messages in flash memory
 */
#define error(msg) error_P(PSTR(msg))


class Jukebox2
{
  public:
    Jukebox2();
    void begin(); // call this in setup
    void update(unsigned long time){}; //call in each run of the loop

    void PlayRandomSound(FatReader &dir);
    void PlaySound(dir_t &filename);
    void PlaySound(String filename);

    FatReader root;   // This holds the information for the volumes root directory
    FatVolume vol;    // This holds the information for the partition on the card
    WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time
  //private:
    SdReader card;    // This object holds the information for the card
    FatReader file;   // This object represent the WAV file
    
    int fileCount(FatReader &dir);
    void error_P(const char *str);
    void sdErrorCheck(void);

};

#endif