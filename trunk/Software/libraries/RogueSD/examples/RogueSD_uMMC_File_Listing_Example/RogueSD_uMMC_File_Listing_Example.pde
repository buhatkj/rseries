
// This example lists all the files on the card.

// You will need to have uMMC firmware version 102.01 or higher to have this
// example work on a uMMC.
// To update your firmware:  http://www.roguerobotics.com/faq/update_firmware

#include <NewSoftSerial.h>
#include <RogueSD.h>

NewSoftSerial ummc_s(4, 5);

RogueSD ummc(ummc_s);

void setup()
{
  pinMode(13, OUTPUT);

  Serial.begin(9600);

  ummc_s.begin(9600);
}

void loop()
{
  // this buffer is used to store the filenames from readdir() below
  char filename[64];

  Serial.println("Initializing uMMC");

  // prepares the communications with the uMMC and closes all open files (if any)
  ummc.sync();

  Serial.print("uMMC Version: ");
  Serial.println(ummc.version());

  if (ummc.status() == 0)
  {
    // list all files

    Serial.print("File count: ");

    // filecount() takes 2 arguments:
    // 1. source path
    // 2. file mask (e.g. list only MP3 files: "*.mp3")
    // (note: the "/""*" is there because of a problem with the Arduino IDE
    // thinking that /* is the start of a comment
    Serial.println(ummc.filecount("/""*"), DEC);

    Serial.println("--- Files ---");

    // opendir() opens the directory for reading.
    // argument is the path
    ummc.opendir("/");

    // readdir() gets the next directory entry that matches the mask. 2 arguments:
    // 1. char buffer to store the name - make sure that it's big enough to store the
    //    largest name in the directory.
    // 2. file mask (same as in filecount())
    
    int type;
    
    while((type = ummc.readdir(filename, "*")) >= 0)
    {
      if (type == 1)  // if it's a folder/directory
        Serial.print("*DIR* ");
      Serial.println(filename);
    }

    Serial.println("-------------");
  }
  else
  {
    Serial.print("uMMC sync failed. Error code: ");
    Serial.println(ummc.LastErrorCode, HEX);
  }

  while(1);  // just loop infinitely here at the end, wait for reset
}
