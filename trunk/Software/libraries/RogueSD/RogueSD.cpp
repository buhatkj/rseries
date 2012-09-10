/* $Id: RogueSD.cpp 126 2010-10-18 03:06:09Z bhagman@roguerobotics.com $

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

#include <stdint.h>
#include <ctype.h>
#include <util/delay.h>
#include "RogueSD.h"

/*************************************************
* Private Constants
*************************************************/

#define UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS  10201
#define UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS  11101

#define STATS_SIZE                            0
#define STATS_POSITION                        1
#define STATS_REMAINING                       2

/*************************************************
* Constructor
*************************************************/

/* Old constructor without abstract base class.
RogueSD::RogueSD(int8_t (*_af)(void), int (*_pf)(void), int (*_rf)(void), void (*_wf)(uint8_t))
: LastErrorCode(0),
  _promptchar(DEFAULT_PROMPT),
  _fwversion(0),
  _moduletype(uMMC)
{
  _availablef = _af;
  _peekf = _pf;
  _readf = _rf;
  _writef = _wf;
}
*/

RogueSD::RogueSD(Stream &comms)
: LastErrorCode(0),
  _promptchar(DEFAULT_PROMPT),
  _fwversion(0),
  _moduletype(uMMC)
{
  _comms = &comms;
}

/*************************************************
* Public Methods
*************************************************/

int8_t RogueSD::sync(void)
{
  // procedure:
  // 1. sync (send ESC, clear prompt)
  // 2. get version ("v"), and module type
  // 3. change settings as needed
  // 4. check status
  // 5. close files (if needed - E08, or other error, not needed)

  // 0. empty any data in the serial buffer
  _comm_flush();

  // 1. sync
  _comm_write(0x1b);                    // send ESC to clear buffer on uMMC
  _read_blocked();                      // consume prompt

  // 2. get version (ignore prompt - just drop it)
  _get_version();

  // 3. change settings as needed
  // OLD: write timeout setting = 10 ms timeout
  // NEW: listing style = old style (0)
  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // we need to set the write timeout to allow us to control when a line is finished
    // in writeln.
    changesetting('1', 1);              // 10 ms timeout
  }
  else
  {
    // we're using the new version
    // Let's make sure the Listing Style setting is set to the old style
    if (getsetting('L') != 0)
    {
      changesetting('L', 0);
    }

    // get the prompt char
    print('S');
    if (_moduletype != uMMC) { print('T'); };
    print('P'); print('\r');  // get our prompt (if possible)
    _promptchar = _getnumber(10);
    _read_blocked();                    // consume prompt
  }
  
  // 4. check status

  if (_moduletype != uMMC) { print("FC"); };
  print('Z'); print('\r');              // Get status
  
  if (_get_response())
    return -1;
  else
  {
    // good
    _read_blocked();                    // consume prompt

    // 5. close all files
    closeall();                         // ensure all handles are closed

    return 0;
  }
}


int8_t RogueSD::status(int8_t handle)
{
  int8_t resp = 0;

  if (_moduletype != uMMC) { print("FC"); };
  print('Z');
  if (handle > 0)
    write('0'+handle);
  print('\r');              // Get status
  
  resp = _get_response();

  if (resp == 0)
    _read_blocked();                    // consume prompt if OK

  return resp;
}


int8_t RogueSD::changesetting(char setting, uint8_t value)
{
  print('S');
  if (_moduletype != uMMC) { print('T'); };
  print(setting); print((long)value); print('\r');
  
  return _get_response();
}


int16_t RogueSD::getsetting(char setting)
{
  uint8_t value;
  
  print('S');
  if (_moduletype != uMMC) { print('T'); };
  print(setting); print('\r');
  
  while(!_comm_available());
  if(_comm_peek() != 'E')
  {
    value = _getnumber(10);
    _read_blocked();                    // consume prompt
  }
  else
  {
    value = _get_response();            // get the error
  }
  
  return value;
}


void RogueSD::gettime(int *rtc)
{
  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    return;
  }
  else
  {
    print('T'); print('\r');
    for (int i = 0; i < 7; i++)
    {
      rtc[i] = _getnumber(10);
      _read_blocked();                  // consume separators, and command prompt (at end)
    }
  }
}

void RogueSD::settime(int rtc[])
{
  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    return;
  }
  else
  {
    print('T');
    for (int i = 0; i < 6; i++)
    {
      print(rtc[i], DEC); print(' ');
    }

    print('\r');
 
    // A bug in earlier versions returned the time after setting
    while(_read_blocked() != _promptchar);  // kludge: read everything including prompt

    // _read_blocked();                    // consume prompt
  }
}


int8_t RogueSD::getfreehandle(void)
{
  uint8_t r;

  if (_moduletype != uMMC) { print("FC"); };
  print('F'); print('\r');
  
  r = _read_blocked();

  if(r != 'E')
    r -= '0';                           // got our handle
  else
  {
    LastErrorCode = _getnumber(16);
    if(LastErrorCode == ERROR_NO_FREE_FILES)
      r = 0;
    else
      r = -1;
  }

  _read_blocked();                      // consume prompt
  
  return r;
}


int8_t RogueSD::open(const char *filename)
{
  // defaults to READ
  return open(filename, OPEN_READ);
}

int8_t RogueSD::open(const char *filename, open_mode mode)
{
  int8_t fh = getfreehandle();
  
  if(fh > 0)
    return _open(fh, filename, mode, 0);
  else
    return fh;
}

int8_t RogueSD::open(int8_t handle, const char *filename)
{
  // defaults to READ
  return _open(handle, filename, OPEN_READ, 0);
}

int8_t RogueSD::open(int8_t handle, const char *filename, open_mode mode)
{
  return _open(handle, filename, mode, 0);
}

int8_t RogueSD::open_P(const char *filename)
{
  return open_P(filename, OPEN_READ);
}

int8_t RogueSD::open_P(const char *filename, open_mode mode)
{
  int8_t fh = getfreehandle();
  
  if(fh > 0)
    return _open(fh, filename, mode, 1);
  else
    return fh;
}

int8_t RogueSD::open_P(int8_t handle, const prog_char *filename)
{
  return _open(handle, filename, OPEN_READ, 1);
}

int8_t RogueSD::open_P(int8_t handle, const prog_char *filename, open_mode mode)
{
  return _open(handle, filename, mode, 1);
}

// private
int8_t RogueSD::_open(int8_t handle, const char *filename, open_mode mode, int8_t pgmspc)
{
  int8_t resp;

  if (_moduletype != uMMC) { print("FC"); };
  print('O'); write('0'+handle); print(' ');

  switch (mode)
  {
    case OPEN_READ:
    case OPEN_RW:
      print('R');
      if (mode == OPEN_READ) break;
    case OPEN_WRITE:
      print('W');
      break;
    case OPEN_APPEND:
      print('A');
      break;      
  }

  print(' ');
  if (pgmspc)
    print_P(filename);
  else
    print(filename);

  print('\r');

  resp = _get_response();
  return resp < 0 ? resp : handle;
}



void RogueSD::close(int8_t handle)
{
  if (_moduletype != uMMC) { print("FC"); };
  print('C'); write('0'+handle); print('\r');
  _read_blocked();
}


void RogueSD::closeall(void)
{
  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    for(uint8_t i=1; i<=4; i++)
    {
      if (_moduletype != uMMC) { print("FC"); };
      print('C'); write('0'+i); print('\r');
      _get_response();
    }
  }
  else
  {
    // new
    if (_moduletype != uMMC) { print("FC"); };
    print('C'); print('\r');
    _read_blocked();                    // consume prompt
  }
}

int8_t RogueSD::opendir(const char *dirname)
{
  int8_t resp = 0;

  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    LastErrorCode = ERROR_NOT_SUPPORTED;
    return -1;
  }
  else
  {
    // new
    if (_moduletype != uMMC) { print("FC"); };
    print("LS "); print(dirname); print('\r');
    resp = _get_response();
    _read_blocked();                // consume prompt
    return resp;
  }
}

int32_t RogueSD::filecount(const char *filemask)
{
  int32_t fcount = 0;

  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    LastErrorCode = ERROR_NOT_SUPPORTED;
    return -1;
  }
  else
  {
    if (_moduletype != uMMC) { print("FC"); };
    print("LC "); print(filemask); print('\r');

    if(_get_response() == 0)
    {
      fcount = _getnumber(10);
      
      _read_blocked();                 // consume prompt
      
      return fcount;
    }
    else
    {
      // error occurred with "LC" command
      return -1;
    }
  }
}


int8_t RogueSD::readdir(char *filename, const char *filemask)
{
  // retrieve the next file from the directory
  // returns:
  // 0 if entry is a file
  // 1 if entry is a folder/directory
  // -1 for no more files (EOF)
  // -2 on disaster

  // currently using the original file listing style, i.e. D/fs filename

  char c;
  int8_t entrytype = 0;

  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    LastErrorCode = ERROR_NOT_SUPPORTED;
    return -1;
  }
  else
  {
    // new
    if (_moduletype != uMMC) { print("FC"); };
    print("LI ");
    if (filemask)
      print(filemask);
    print('\r');

    if(_get_response())
    {
      // had an error
      if(LastErrorCode == ERROR_EOF)
        return -1;
      else
        return -2;
    }
    else
    {
      // we have the file info next
      while(!_comm_available());

      if(_comm_peek() == 'D')
      {
        // we have a directory
        _comm_read();                     // consume 'D'
        entrytype = 1;
      }
      else
      {
        // it's a file, with a file size
        _getnumber(10);                     // discard (for now)
        entrytype = 0;
      }

      _read_blocked();                      // consume separator ' '

      // now get filename
      while((c = _read_blocked()) != '\r')
      {
        *filename++ = c;
      }
      *filename = 0;                        // terminate string
      
      _read_blocked();                      // consume prompt
      
      return entrytype;
    }
  }
}

int8_t RogueSD::entrytofilename(char *filename, uint8_t count, const char *filemask, uint16_t entrynum)
{
  char c;
  int8_t entrytype = 0;

  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    LastErrorCode = ERROR_NOT_SUPPORTED;
    return -1;
  }
  else
  {
    // new
    if (_moduletype != uMMC) { print("FC"); };
    print("LE "); print(entrynum, DEC); print(' '); print(filemask); print('\r');

    if(_get_response())
    {
      // had an error
      return -1;
    }
    else
    {
      // we have the file info next
      while(!_comm_available());

      if(_comm_peek() == 'D')
      {
        // we have a directory
        _comm_read();                     // consume 'D'
        entrytype = 1;
      }
      else
      {
        // it's a file, with a file size
        _getnumber(10);                     // discard (for now)
        entrytype = 0;
      }

      _read_blocked();                      // consume separator ' '

      // now get filename
      while((c = _read_blocked()) != '\r')
      {
        if (count > 0)
        {
          count--;
          *filename++ = c;
        }
      }
      *filename = 0;                        // terminate string
      
      _read_blocked();                      // consume prompt
      
      return entrytype;
    }
  }
}



int8_t RogueSD::remove(const char *filename)
{
  if (_moduletype != uMMC) { print("FC"); };

  print('E'); print(filename); print('\r');

  return _get_response();
}


// Read a single byte from the given handle.
int16_t RogueSD::readbyte(int8_t handle)
{
  uint8_t ch = 0;

  if (_moduletype != uMMC) { print("FC"); };
  print('R'); write('0'+handle); print(' '); print((long)1); print('\r');

  // we will get either a space followed by 1 byte, or an error
  
  if(_get_response())
  {
    // had an error
    if(LastErrorCode == ERROR_EOF)
      return -1;
    else
      return -2;
  }
  else
  {
    // we have a single byte waiting
    ch = _read_blocked();
    _read_blocked();                   // consume prompt
    return ch;
  }
}


int16_t RogueSD::read(int8_t handle, uint16_t count, char *buffer)
{
  // read up to count bytes into buffer
  uint32_t bytesremaining;
  uint16_t i;
  fileinfo fi = getfileinfo(handle);

  // check first how many bytes are remaining
  bytesremaining = fi.size - fi.position;

  if(bytesremaining > 0)
  {
    if(count > bytesremaining)
      count = bytesremaining;
    if (_moduletype != uMMC) { print("FC"); };
    print('R'); write('0'+handle); print(' '); print(count); print('\r');
  }
  else
  {
    return 0;
  }

  // now read count bytes
  
  if(_get_response())
  {
    if(LastErrorCode == ERROR_EOF)
      return -1;
    else
      return -2;
  }

  for(i=0; i<count; i++)
    buffer[i] = _read_blocked();

  _read_blocked();                      // consume prompt
  
  return i;                             // return number of bytes read
}


int16_t RogueSD::readln(int8_t handle, uint16_t maxlength, char *tostr)
{
  int8_t r, i;
  
  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    return -1;
  }
  
  if (_moduletype != uMMC) { print("FC"); };
  print("RL");                       // Read a line, maxlength chars, maximum
  write('0'+handle); print(' ');
  print(maxlength, DEC);
  print('\r');
  
  if(_get_response())
  {
    if(LastErrorCode == ERROR_EOF)
      // EOF
      return -1;
    else
      return -2;
  }
  
  // otherwise, read the data

  i = 0;
  r = _read_blocked();

  while(r != _promptchar)               // we could have a blank line
  {
    tostr[i++] = r;
    r = _read_blocked();
  }

  tostr[i] = 0;                         // terminate our string

  return i;
}


int8_t RogueSD::writeln(int8_t handle, const char *data)
{
  writeln_prep(handle);

  while(*data)
  {
    write(*data++);
    if(*data == '\r') break;
  }

  return writeln_finish();
}


void RogueSD::writeln_prep(int8_t handle)
{
  // be warned: if using this command with firmware less than 102.xx/111.xx
  // you must IMMEDIATELY write data within 10ms or the write time-out will occur
  
  if (_moduletype != uMMC) { print("FC"); };
  print('W');
  
  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    write('0'+handle); print(" 512");
  }
  else
  {
    // new
    print('L'); write('0'+handle);
  }
  
   print('\r');
}


int8_t RogueSD::writeln_finish(void)
{
  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    // we wait for more than 10ms to terminate the Write command (write time-out)
    _delay_ms(11);
  }
  else
  {
    // new
    print('\r');
  }

  return _get_response();
}


int8_t RogueSD::write(int8_t handle, uint16_t count, const char *data)
{
  if (_moduletype != uMMC) { print("FC"); };
  print('W'); write('0'+handle); print(' '); print(count); print('\r');

  while(count--)
    write(*data++);

  // after we are done, check the response
  if(_get_response())
    return -1;
  else
    return 0;
}


int8_t RogueSD::writebyte(int8_t handle, char data)
{
  if (_moduletype != uMMC) { print("FC"); };
  print('W'); write('0'+handle); print(' '); print('1'); print('\r');

  write(data);

  // after we are done, check the response
  if(_get_response())
    return -1;
  else
    return 0;
}


fileinfo RogueSD::getfileinfo(int8_t handle)
{
  fileinfo fi;

  if (_moduletype != uMMC) { print("FC"); };
  print('I'); write('0'+handle); print('\r');

  while(!_comm_available());
  if(_comm_peek() != 'E')
  {
    fi.position = _getnumber(10);       // current file position

    _read_blocked();                    // consume '/' or ' '

    fi.size = _getnumber(10);           // file size

    _read_blocked();                    // consume prompt
  }
  else
  {
    _get_response();
    // if we have an error, just return 0's
    // error code is actually in .LastErrorCode
    fi.position = 0;
    fi.size = 0;
  }
  
  return fi;
}

int32_t RogueSD::getfilesize(const char *filename)
{
  char c;
  uint32_t filesize = 0;

  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    LastErrorCode = ERROR_NOT_SUPPORTED;
    return -1;
  }
  else
  {
    // new
    if (_moduletype != uMMC) { print("FC"); };
    print("L "); print(filename); print('\r');

    if(_get_response())
    {
      // had an error
      return -1;
    }
    else
    {
      // we have the file info next
      while(!_comm_available());

      if(_comm_peek() == 'D')
      {
        // we have a directory, file size = 0
        _comm_read();                       // consume 'D'
      }
      else
      {
        // it's a file, with a file size
        filesize = _getnumber(10);          // discard (for now)
      }

      // clear the rest
      while((c = _read_blocked()) != '\r');

      _read_blocked();                      // consume prompt

      return (int32_t) filesize;      
    }
  }
}



int8_t RogueSD::seek(int8_t handle, uint32_t newposition)
{
  if (_moduletype != uMMC) { print("FC"); };

  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    // we need to do an empty read to seek to our position
    // R fh 0 position
    print('R'); write('0'+handle); print(' '); print('0');
  }
  else
  {
    // new
    // J fh position
    print('J'); write('0'+handle);
  }
  
  // common portion
  print(' '); print(newposition); print('\r');  

  return _get_response();
}


int8_t RogueSD::seektoend(int8_t handle)
{
  if ((_moduletype == uMMC && _fwversion < UMMC_MIN_FW_VERSION_FOR_NEW_COMMANDS) ||
      (_moduletype == uMP3 && _fwversion < UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // old
    // two step process - get filesize, seek to that position
    return seek(handle, getfileinfo(handle).size);
  }
  else
  {
    // new
    if (_moduletype != uMMC) { print("FC"); };
    // J fh E
    print('J'); write('0'+handle); print('E'); print('\r');
  }

  return _get_response();
}

// Added for sending prog_char strings
void RogueSD::print_P(const prog_char *str)
{
  while (pgm_read_byte(str) != 0)
  {
    print(pgm_read_byte(str++));
  }
}

/*************************************************
* Public (virtual)
*************************************************/

size_t RogueSD::write(uint8_t c)
{
  _comms->write(c);
} 


/*************************************************
* Private Methods
*************************************************/

int8_t RogueSD::_read_blocked(void)
{
  // int8_t r;
  
  while(!_comm_available());
  // while((r = this->_readf()) < 0);   // this would be faster if we could guarantee that the _readf() function
                                        // would return -1 if there was no byte read
  return _comm_read();
}


int8_t RogueSD::_get_response(void)
{
  // looking for a response
  // If we get a space " ", we return as good and the remaining data can be retrieved
  // " ", ">", "Exx>" types only
  uint8_t r;
  uint8_t resp = 0;

  // we will return 0 if all is good, error code otherwise

  r = _read_blocked();

  if(r == ' ' || r == _promptchar)
    resp = 0;

  else if(r == 'E')
  {
    LastErrorCode = _getnumber(16);     // get our error code
    _read_blocked();                    // consume prompt
    
    resp = -1;
  }
  
  else
  {
    LastErrorCode = 0xFF;               // something got messed up, a resync would be nice
    resp = -1;
  }
  
  return resp;
}


int16_t RogueSD::_get_version(void)
{
  // get the version, and module type
  print('V'); print('\r');
  
  // Version format: mmm.nn[-bxxx] SN:TTTT-ssss...
  
  // get first portion mmm.nn
  _fwversion = _getnumber(10);
  _read_blocked();                      // consume '.'
  _fwversion *= 100;
  _fwversion += _getnumber(10);
  // ignore beta version (-bxxx), if it's there
  if (_read_blocked() == '-')
  {
    for (char i = 0; i < 5; i++) // drop bxxx plus space
      _read_blocked();
  }
  // otherwise, it was a space

  // now drop the SN:
  _read_blocked();
  _read_blocked();
  _read_blocked();

  if (_read_blocked() == 'R')
    _moduletype = rMP3;
  else
  {
    // either UMM1 or UMP1
    // so drop the M following the U
    _read_blocked();
    if (_read_blocked() == 'M')
      _moduletype = uMMC;
    else
      _moduletype = uMP3;
  }

  // ignore the rest
  while (_read_blocked() != '-');

  // consume up to and including prompt
  while (isalnum(_read_blocked()));
  
  return _fwversion;
}


int32_t RogueSD::_getnumber(uint8_t base)
{
	uint8_t c, neg = 0;
	uint32_t val;

	val = 0;
	while(!_comm_available());
  c = _comm_peek();
  
  if(c == '-')
  {
    neg = 1;
    _comm_read();  // remove
    while(!_comm_available());
    c = _comm_peek();
  }
  
	while(((c >= 'A') && (c <= 'Z'))
	    || ((c >= 'a') && (c <= 'z'))
	    || ((c >= '0') && (c <= '9')))
	{
		if(c >= 'a') c -= 0x57;             // c = c - 'a' + 0x0a, c = c - ('a' - 0x0a)
		else if(c >= 'A') c -= 0x37;        // c = c - 'A' + 0x0A
		else c -= '0';
		if(c >= base) break;

		val *= base;
		val += c;
		_comm_read();                       // take the byte from the queue
		while(!_comm_available());          // wait for the next byte
		c = _comm_peek();
	}
	return neg ? -val : val;
}

uint8_t RogueSD::_comm_available(void)
{
  return _comms->available();
}

int RogueSD::_comm_peek(void)
{
  return _comms->peek();
}

int RogueSD::_comm_read(void)
{
  return _comms->read();
}

void RogueSD::_comm_write(uint8_t c)
{
  _comms->write(c);
}

void RogueSD::_comm_flush(void)
{
  _comms->flush();
}
