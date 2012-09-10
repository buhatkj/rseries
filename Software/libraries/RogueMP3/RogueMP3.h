/* $Id: RogueMP3.h 125 2010-10-18 03:04:22Z bhagman@roguerobotics.com $

  Rogue Robotics MP3 Library
  File System interface for:
   - uMP3
   - rMP3

  A library to communicate with the Rogue Robotics
  MP3 Playback modules. (uMP3, rMP3)
  Rogue Robotics (http://www.roguerobotics.com/).
  Requires
  uMP3 firmware > 111.01

  See http://www.roguerobotics.com/faq/update_firmware for updating firmware.

  Written by Brett Hagman
  http://www.roguerobotics.com/
  bhagman@roguerobotics.com

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*************************************************/

#ifndef _RogueMP3_h
#define _RogueMP3_h

#include <avr/pgmspace.h>
#include <stdint.h>
#include <Stream.h>
// The Stream class is derived from the Print class

/*************************************************
* Public Constants
*************************************************/

#ifndef _RogueSD_h

#define DEFAULT_PROMPT                        0x3E

#define ERROR_BUFFER_OVERRUN                  0x02
#define ERROR_NO_FREE_FILES                   0x03
#define ERROR_UNRECOGNIZED_COMMAND            0x04
#define ERROR_CARD_INITIALIZATION_ERROR       0x05
#define ERROR_FORMATTING_ERROR                0x06
#define ERROR_EOF                             0x07
#define ERROR_CARD_NOT_INSERTED               0x08
#define ERROR_MMC_RESET_FAIL                  0x09
#define ERROR_CARD_WRITE_PROTECTED            0x0a
#define ERROR_INVALID_HANDLE                  0xf6
#define ERROR_OPEN_PATH_INVALID               0xf5
#define ERROR_FILE_ALREADY_EXISTS             0xf4
#define ERROR_DE_CREATION_FAILURE             0xf3
#define ERROR_FILE_DOES_NOT_EXIST             0xf2
#define ERROR_OPEN_HANDLE_IN_USE              0xf1
#define ERROR_OPEN_NO_FREE_HANDLES            0xf0
#define ERROR_FAT_FAILURE                     0xef
#define ERROR_SEEK_NOT_OPEN                   0xee
#define ERROR_OPEN_MODE_INVALID               0xed
#define ERROR_READ_IMPROPER_MODE              0xec
#define ERROR_FILE_NOT_OPEN                   0xeb
#define ERROR_NO_FREE_SPACE                   0xea
#define ERROR_WRITE_IMPROPER_MODE             0xe9
#define ERROR_WRITE_FAILURE                   0xe8
#define ERROR_NOT_A_FILE                      0xe7
#define ERROR_OPEN_READONLY_FILE              0xe6
#define ERROR_NOT_A_DIR                       0xe5

#define ERROR_NOT_SUPPORTED                   0xff

#endif

/*************************************************
* Typedefs, structs, etc
*************************************************/

struct playbackinfo {
                      uint16_t position;
                      uint8_t samplerate;
                      uint16_t bitrate;
                      char channels;
                    };

#ifndef _RogueSD_h
enum moduletype {uMMC = 1, uMP3, rMP3};
#endif

/*************************************************
* Class
*************************************************/

class RogueMP3 : public Print
{
  public:
    // properties
    uint8_t LastErrorCode;
    
    // constructor
//    RogueMP3(int8_t (*_af)(void), int16_t (*_pf)(void), int16_t (*_rf)(void), void (*_wf)(uint8_t));
    RogueMP3(Stream &comms);


    // methods
    int8_t sync(void);

    moduletype getmoduletype(void) { return _moduletype; }

    // Play Command ("PC") methods
    int8_t playfile_P(const char *path);
    int8_t playfile(const char *path, const char *filename = NULL, uint8_t pgmspc = 0);
    void setloop(uint8_t loopcount);
    void jump(uint16_t newtime);
    void setboost(uint8_t bass_amp, uint8_t bass_freq, int8_t treble_amp, uint8_t treble_freq);
    void setboost(uint16_t newboost);

    uint16_t getvolume(void);

    void setvolume(uint8_t newvolume);
    void setvolume(uint8_t new_vleft, uint8_t new_vright);

    void fade(uint8_t newvolume);
    void fade(uint8_t newvolume, uint16_t fadems);
    void fade_lr(uint8_t new_vleft, uint8_t new_vright);
    void fade_lr(uint8_t new_vleft, uint8_t new_vright, uint16_t fadems);

    void playpause(void);
    void stop(void);
    
    playbackinfo getplaybackinfo(void);
    char getplaybackstatus(void);
    uint8_t getspectrumanalyzer(uint8_t values[], uint8_t peaks=0);
    void setspectrumanalyzer(uint16_t bands[], uint8_t count);

    // Information Commands ("IC" - MP3 information)
    int16_t gettracklength(const char *path, const char *filename = NULL, uint8_t pgmspc = 0);

    // Settings ("ST") methods
    int8_t changesetting(char setting, const char *value);
    int8_t changesetting(char setting, uint8_t value);
    int16_t getsetting(char setting);
    // ***************************

    inline int16_t version(void) { return _fwversion; }

    size_t write(uint8_t);  // needed for Print

    void print_P(const prog_char *str);

  private:

    // Polymorphism used to interact with serial class
    // SerialBase is an abstract base class which defines a base set
    // of functionality for serial classes.
    Stream *_comms;

    uint8_t _promptchar;
    int16_t _fwversion;
    moduletype _moduletype;
    
    // methods
    int16_t _get_version(void);
    int8_t _get_response(void);
    void _flush(void);

    int8_t _read_blocked(void);
    int32_t _getnumber(uint8_t base);

    uint8_t _comm_available(void);
    int _comm_peek(void);
    int _comm_read(void);
    void _comm_write(uint8_t);
    void _comm_flush(void);
};

#endif
