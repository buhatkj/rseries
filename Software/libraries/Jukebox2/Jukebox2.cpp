#if (ARDUINO >= 100)
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif

#include "Jukebox2.h"


Jukebox2::Jukebox2(){

}

void Jukebox2::begin(){
  if (!card.init()) {
    Serial.print("Card init. failed!");
  }
  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);
  if (!vol.init(card)) {
    Serial.print("No partition!");
  }
  if (!root.openRoot(vol)) {
    Serial.print("Couldn't open dir");
  }
  root.rewind();
  randomSeed(analogRead(0));
} // call this in setup

void Jukebox2::PlayRandomSound(FatReader &dir){
  //Serial.print("OMG!");
  int count = 0;
  FatReader tempFile;
  dir_t dirBuf;     // buffer for directory reads
  int r = random(1,fileCount(dir));
  dir.rewind();
  while (dir.readDir(dirBuf) > 0) {    // Read every file in the directory one at a time
    //Serial.print("LOL!");
    //Serial.println(r);
      // Skip it if not a subdirectory and not a .WAV file
    if (!DIR_IS_SUBDIR(dirBuf)
         && strncmp_P((char *)&dirBuf.name[8], PSTR("WAV"), 3)) {
      continue;
    }

    if (!tempFile.open(vol, dirBuf)) {        // open the file in the directory
      Serial.print("file.open failed");          // something went wrong
    }

    if (tempFile.isDir()) {                   // check if we opened a new directory

    }
    else
    {
      count++;
      if(count == r){
        Serial.println("found a sound!");
        PlaySound(dirBuf);
        dir.rewind();
        return;
      }
    }

  }
  Serial.println("Never found a sound!");
  Serial.print("Rand was:");
  Serial.println(r);
}

void Jukebox2::PlaySound(dir_t &filename){
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  if (!file.open(vol, filename)) {
    Serial.print("Couldn't open file ");
    //Serial.print(filename); 
    return;
  }
  if (!wave.create(file)) {
    Serial.print("Not a valid WAV");
    return;
  }
  // ok time to play!
  Serial.print("Playing a WAV!");
  wave.play();
}

void Jukebox2::PlaySound(String inFileName){
  char filename[16];
  //strcpy (filename, inFileName); 
  inFileName.toCharArray(filename, 16);

  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  if (!file.open(root, (char*)filename)) {
    Serial.print("Couldn't open file ");
    //Serial.print(filename); 
    return;
  }
  if (!wave.create(file)) {
    Serial.print("Not a valid WAV");
    return;
  }
  // ok time to play!
  Serial.print("Playing a WAV!");
  wave.play();
}

int Jukebox2::fileCount(FatReader &dir){
  //Serial.print("WTF!");
  int count = 0;
  FatReader tempFile;
  dir_t dirBuf;     // buffer for directory reads
  dir.rewind();
  while (dir.readDir(dirBuf) > 0) {    // Read every file in the directory one at a time
    //Serial.print("QQQ!");
      // Skip it if not a subdirectory and not a .WAV file
    if (!DIR_IS_SUBDIR(dirBuf)
         && strncmp_P((char *)&dirBuf.name[8], PSTR("WAV"), 3)) {
      continue;
    }

    if (!tempFile.open(vol, dirBuf)) {        // open the file in the directory
      Serial.print("file.open failed");          // something went wrong
    }

    if (tempFile.isDir()) {                   // check if we opened a new directory

    }
    else{
      count++;
    }

  }
  dir.rewind();
  Serial.print("Count was:");
  Serial.println(count);
  return count;
}


/*
 * print error message and halt
 */
void Jukebox2::error_P(const char *str)
{
  PgmPrint("Error: ");
  SerialPrint_P(str);
  sdErrorCheck();
  //while(1);
}

/*
 * print error message and halt if SD I/O error
 */
void Jukebox2::sdErrorCheck(void)
{
  if (!card.errorCode()) return;
  PgmPrint("\r\nSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  PgmPrint(", ");
  Serial.println(card.errorData(), HEX);
  //while(1);
}
