/* $Id: RogueSD.h 126 2010-10-18 03:06:09Z bhagman@roguerobotics.com $

  Rogue Robotics SD Library
  File System interface for:
   - uMMC
   - uMP3
   - rMP3

  A library to communicate with the Rogue Robotics
  SD Card modules. (uMMC, uMP3, rMP3)
  Rogue Robotics (http://www.roguerobotics.com/).
  Requires
  uMMC firmware > 102.01
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

#ifndef _RogueSD_h
#define _RogueSD_h

#include <avr/pgmspace.h>
#include <stdint.h>
#include <Stream.h>
// The Stream class is derived from the Print class

/*************************************************
* Public Constants
*************************************************/

#ifndef _RogueMP3_h

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

struct fileinfo {uint32_t position; uint32_t size;};
enum open_mode {OPEN_READ = 1, OPEN_WRITE = 2, OPEN_RW = 3, OPEN_APPEND = 4};

#ifndef _RogueMP3_h
enum moduletype {uMMC = 1, uMP3, rMP3};
#endif

/*************************************************
* Class
*************************************************/

class RogueSD : public Print
{
  public:
    // properties
    uint8_t LastErrorCode;

    // methods

    // constructor
//    RogueSD(int8_t (*_af)(void), int16_t (*_pf)(void), int16_t (*_rf)(void), void (*_wf)(uint8_t));
    RogueSD(Stream &comms);

    int8_t sync(void);

    moduletype getmoduletype(void) { return _moduletype; }

//    int8_t status(void);
    int8_t status(int8_t handle = 0);

    int8_t getfreehandle(void);
    int8_t open(const char *filename);
    int8_t open(const char *filename, open_mode mode);
    int8_t open(int8_t handle, const char *filename);
    int8_t open(int8_t handle, const char *filename, open_mode mode);
    int8_t open_P(const prog_char *filename);
    int8_t open_P(const prog_char *filename, open_mode mode);
    int8_t open_P(int8_t handle, const prog_char *filename);
    int8_t open_P(int8_t handle, const prog_char *filename, open_mode mode);

    int8_t opendir(const char *dirname);
    int32_t filecount(const char *filemask);
    int8_t readdir(char *filename, const char *filemask);

    int8_t entrytofilename(char *filename, uint8_t count, const char *filemask, uint16_t entrynum);

    // delete/remove a file/directory (directory must be empty)
    int8_t remove(const char *filename);

    // rename a file/directory
//    int8_t rename(const char *oldname, const char *newname);

    // read single byte (-1 if no data)
    int16_t readbyte(int8_t handle);

    // read exactly count bytes into buffer
    int16_t read(int8_t handle, uint16_t count, char *buffer);

    // read up to maxlength characters into tostr
    int16_t readln(int8_t handle, uint16_t maxlength, char *tostr);

//    int16_t readprep(int8_t handle, uint16_t bytestoread);

    // we will need to set up the write time-out to make this work properly (done in sync())
    // then you can use the Print functions to print to the file
    int8_t writeln(int8_t handle, const char *data);
    void writeln_prep(int8_t handle);
    int8_t writeln_finish(void);

    // write exactly count bytes to file
    int8_t write(int8_t handle, uint16_t count, const char *data);

    // write a single byte to the file
    int8_t writebyte(int8_t handle, char data);

    fileinfo getfileinfo(int8_t handle);
    int32_t getfilesize(const char *filename); // get using "L filename"
    
    int8_t seek(int8_t handle, uint32_t newposition);
    int8_t seektoend(int8_t handle);

    void gettime(int *rtc);

    void settime(int rtc[]);

//    void settime(uint32_t date, uint32_t time);
//    void settime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

    void close(int8_t handle);
    void closeall(void);
    int8_t changesetting(char setting, uint8_t value);
    int16_t getsetting(char setting);

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
    int8_t _open(int8_t handle, const char *filename, open_mode mode, int8_t pgmspc);
    uint32_t _get_filestats(int8_t handle, uint8_t valuetoget);
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
