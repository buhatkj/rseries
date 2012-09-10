/* $Id: RogueMP3.cpp 125 2010-10-18 03:04:22Z bhagman@roguerobotics.com $

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

#include <stdint.h>
#include <ctype.h>
#include <util/delay.h>
#include "RogueMP3.h"

/*************************************************
* Private Constants
*************************************************/

#define UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS 11101

#define FADE_STEPS                      40
#define FADE_AUDIBLE_DIFF               5
#define FADE_DEFAULT_TIME               1000


/*************************************************
* Constructor
*************************************************/

/*
RogueMP3::RogueMP3(int8_t (*_af)(void), int (*_pf)(void), int (*_rf)(void), void (*_wf)(uint8_t))
: LastErrorCode(0),
  _promptchar(DEFAULT_PROMPT),
  _fwversion(0),
  _moduletype(rMP3)
{
  _availablef = _af;
  _peekf = _pf;
  _readf = _rf;
  _writef = _wf;
}
*/

RogueMP3::RogueMP3(Stream &comms)
: LastErrorCode(0),
  _promptchar(DEFAULT_PROMPT),
  _fwversion(0),
  _moduletype(rMP3)
{
  _comms = &comms;
}


/*************************************************
* Public Methods
*************************************************/


int8_t RogueMP3::sync(void)
{
  // procedure:
  // 1. sync (send ESC, clear prompt)
  // 2. get version ("v"), and module type
  // 3. get prompt ("st p"), if newer firmware
  // 4. check status

  // 0. empty any data in the serial buffer
  _comm_flush();

  // 1. sync
  _comm_write(0x1b);                    // send ESC to clear buffer on uMMC
  _read_blocked();                      // consume prompt

  // 2. get version (ignore prompt - just drop it)
  _get_version();

  // 3. get prompt ("st p"), if newer firmware
  if (_moduletype == rMP3 || (_moduletype == uMP3 && _fwversion >= UMP3_MIN_FW_VERSION_FOR_NEW_COMMANDS))
  {
    // get the prompt char
    print('S');
    if (_moduletype != uMMC) { print('T'); };
    print('P'); print('\r');  // get our prompt (if possible)
    _promptchar = _getnumber(10);
    _read_blocked();                    // consume prompt
  }
  
  // 4. check status

  print('F'); print('C'); print('Z'); print('\r'); // Get status
  
  if (_get_response())
    return -1;
  else
  {
    // good
    _read_blocked();                    // consume prompt

    return 0;
  }
}


int8_t RogueMP3::changesetting(char setting, uint8_t value)
{
  print('S'); print('T'); print(setting); print(value, DEC); print('\r');
  
  return _get_response();
}


int8_t RogueMP3::changesetting(char setting, const char* value)
{
  print('S'); print('T'); print(setting); print(value); print('\r');
  
  return _get_response();
}

int16_t RogueMP3::getsetting(char setting)
{
  uint8_t value;
  
  print('S'); print('T'); print(setting); print('\r');
  
  while (!_comm_available());
  if (_comm_peek() != 'E')
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

int8_t RogueMP3::playfile_P(const char *path)
{
  return playfile(path, NULL, 1);
}

/*
int8_t RogueMP3::playfile(const char *filename, uint8_t pgmspc)
{
  print("PCF");

  if (pgmspc == 1)
    print_P(filename);
  else
    print(filename);
  print('\r');
  
  return _get_response();
}
*/

int8_t RogueMP3::playfile(const char *path, const char *filename, uint8_t pgmspc)
{
  print("PCF");

  if (pgmspc == 1)
    print_P(path);
  else
    print(path);

  if (filename)
  {
    print('/');
    print(filename);
  }
  print('\r');
  
  return _get_response();
}


uint16_t RogueMP3::getvolume(void)
{
  uint16_t l, r;

  print("PCV\r");

  l = _getnumber(10);
  _read_blocked();                      // consume separator
  r = _getnumber(10);

  _read_blocked();                      // consume prompt
  
  return l << 8 | r;
}


void RogueMP3::setvolume(uint8_t newvolume)
{
  print("PCV");
  print(newvolume, DEC);
  print('\r');
  
  _read_blocked();                      // consume prompt
}


void RogueMP3::setvolume(uint8_t new_vleft, uint8_t new_vright)
{
  print("PCV");
  print(new_vleft, DEC);
  print(' ');
  print(new_vright, DEC);
  print('\r');

  _read_blocked();                      // consume prompt
}



void RogueMP3::fade(uint8_t newvolume)
{
  fade_lr(newvolume, newvolume, FADE_DEFAULT_TIME);
}



void RogueMP3::fade(uint8_t newvolume, uint16_t fadems)
{
  fade_lr(newvolume, newvolume, fadems);
}



void RogueMP3::fade_lr(uint8_t new_vleft, uint8_t new_vright)
{
  fade_lr(new_vleft, new_vright, FADE_DEFAULT_TIME);
}



void RogueMP3::fade_lr(uint8_t new_vleft, uint8_t new_vright, uint16_t fadems)
{
  // fades either/both channels to new volume in fadems milliseconds
  // always 20 steps
  uint16_t vleft, vright;
  uint16_t currentvolume;
  uint16_t fadetimestep = 0;
  int16_t il, ir;
  int8_t i;

  fadetimestep = fadems/FADE_STEPS;

  if (fadetimestep<FADE_AUDIBLE_DIFF)
  {
    // too fast to hear - just set the volume
    setvolume(new_vleft, new_vright);
  }
  else
  {
    currentvolume = getvolume();
    // for precision, we move the volume over by 4 bits
    vleft = ((currentvolume >> 8) & 0xff) * 16;
    vright = (currentvolume & 0xff) * 16;

    il = (((uint16_t)new_vleft)*16 - vleft);
    ir = (((uint16_t)new_vright)*16 - vright);

    il /= FADE_STEPS;
    ir /= FADE_STEPS;

    for (i = 0; i < FADE_STEPS; i++)
    {
      vleft += il;
      vright += ir;
      setvolume(vleft/16, vright/16);
      _delay_ms(fadetimestep);
    }
  }
}



void RogueMP3::playpause(void)
{
  print("PCP\r");

  _read_blocked();                      // consume prompt
}



void RogueMP3::stop(void)
{
  print("PCS\r");

  _read_blocked();                      // consume prompt
}
    


playbackinfo RogueMP3::getplaybackinfo(void)
{
  playbackinfo pi;

  print("PCI\r");
  // now get the info we need
  
  // first, time
  pi.position = _getnumber(10);
  _read_blocked();                      // consume separator

  // second, samplerate
  pi.samplerate = _getnumber(10);
  _read_blocked();                      // consume separator

  // third, bitrate
  pi.bitrate = _getnumber(10);
  _read_blocked();                      // consume separator

  // fourth, channels
  pi.channels = _read_blocked();

  _read_blocked();                      // consume prompt
  
  return pi;
}


char RogueMP3::getplaybackstatus(void)
{
  char value;

  print("PCZ\r");

  value = _read_blocked();

  while (_read_blocked() != _promptchar);
  
  return value;
}


void RogueMP3::jump(uint16_t newtime)
{
  print("PCJ");
  print(newtime, DEC);
  print('\r');

  _read_blocked();                      // consume prompt
}


void RogueMP3::setboost(uint8_t bass_amp, uint8_t bass_freq, int8_t treble_amp, uint8_t treble_freq)
{
  uint16_t newBoostRegister = 0;

  if (treble_freq > 15) treble_freq = 15;
  if (treble_amp < -8) treble_amp = -8;
  else if (treble_amp > 7) treble_amp = 7;
  if (bass_freq == 1) bass_freq = 0;
  else if (bass_freq > 15) bass_freq = 15;
  if (bass_amp > 15) bass_amp = 15;

  newBoostRegister = (uint8_t)treble_amp << 12;
  newBoostRegister |= treble_freq << 8;
  newBoostRegister |= bass_amp << 4;
  newBoostRegister |= bass_freq;

  setboost(newBoostRegister);
}

void RogueMP3::setboost(uint16_t newboost)
{
  print("PCB");
  print(newboost, DEC);
  print('\r');

  _read_blocked();                      // consume prompt
}


void RogueMP3::setloop(uint8_t loopcount)
{
  print("PCO");
  print(loopcount, DEC);
  print('\r');

  _read_blocked();                      // consume prompt
}


// Added for sending prog_char strings
void RogueMP3::print_P(const char *str)
{
  while (pgm_read_byte(str) != 0)
  {
    print(pgm_read_byte(str++));
  }
}



uint8_t RogueMP3::getspectrumanalyzer(uint8_t values[], uint8_t peaks)
{
  uint8_t count = 0;
  uint8_t value = 0;
  uint8_t ch;
  
  print_P(PSTR("PCY"));
  if (peaks)
    print('P');
  print('\r');

  // now get the info we need

  ch = _read_blocked();  // start it off (ch should be a space)
  
  while (ch == ' ')
  {
    value = _getnumber(10);
    values[count++] = value;
    ch = _read_blocked();
  }
  
  return count;
}


void RogueMP3::setspectrumanalyzer(uint16_t bands[], uint8_t count)
{
  uint8_t i;
  
  if (count == 0)
    return;

  if (count > 23)
    count = 23;
  
  print_P(PSTR("PCYS"));

  // now send the band frequencies

  for (i = 0; i < count; i++)
  {
    print(' ');
    print(bands[i], DEC);
  }
  
  print('\r');
  
  _read_blocked();                      // consume prompt
}


int16_t RogueMP3::gettracklength(const char *path, const char *filename, uint8_t pgmspc)
{
  int16_t tracklength = 0;

  print("ICT");

  if (pgmspc == 1)
    print_P(path);
  else
    print(path);
  
  if (filename)
  {
    print('/');
    print(filename);
  }

  print('\r');
  
  if(_get_response() == 0)
  {
    tracklength = _getnumber(10);
    
    _read_blocked();                 // consume prompt
    
    return tracklength;
  }
  else
  {
    // error occurred with "IC" command
    return -1;
  }
}



/*************************************************
* Public (virtual)
*************************************************/

size_t RogueMP3::write(uint8_t c)
{
  _comms->write(c);
} 


/*************************************************
* Private Methods
*************************************************/

int8_t RogueMP3::_read_blocked(void)
{
  // int8_t r;
  
  while (!_comm_available());
  // while((r = this->_readf()) < 0);   // this would be faster if we could guarantee that the _readf() function
                                        // would return -1 if there was no byte read
  return _comm_read();
}


int8_t RogueMP3::_get_response(void)
{
  // looking for a response
  // If we get a space " ", we return as good and the remaining data can be retrieved
  // " ", ">", "Exx>" types only
  uint8_t r;
  uint8_t resp = 0;

  // we will return 0 if all is good, error code otherwise

  r = _read_blocked();

  if (r == ' ' || r == _promptchar)
    resp = 0;

  else if (r == 'E')
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


int16_t RogueMP3::_get_version(void)
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
    for (char i = 0; i < 4; i++)
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


int32_t RogueMP3::_getnumber(uint8_t base)
{
	uint8_t c, neg = 0;
	uint32_t val;

	val = 0;
	while (!_comm_available());
  c = _comm_peek();
  
  if(c == '-')
  {
    neg = 1;
    _comm_read();  // remove
    while (!_comm_available());
    c = _comm_peek();
  }
  
	while (((c >= 'A') && (c <= 'Z'))
	    || ((c >= 'a') && (c <= 'z'))
	    || ((c >= '0') && (c <= '9')))
	{
		if (c >= 'a') c -= 0x57;             // c = c - 'a' + 0x0a, c = c - ('a' - 0x0a)
		else if (c >= 'A') c -= 0x37;        // c = c - 'A' + 0x0A
		else c -= '0';
		if (c >= base) break;

		val *= base;
		val += c;
		_comm_read();                     // take the byte from the queue
		while (!_comm_available());        // wait for the next byte
		c = _comm_peek();
	}
	return neg ? -val : val;
}


uint8_t RogueMP3::_comm_available(void)
{
  return _comms->available();
}

int RogueMP3::_comm_peek(void)
{
  return _comms->peek();
}

int RogueMP3::_comm_read(void)
{
  return _comms->read();
}

void RogueMP3::_comm_write(uint8_t c)
{
  _comms->write(c);
}

void RogueMP3::_comm_flush(void)
{
  _comms->flush();
}
