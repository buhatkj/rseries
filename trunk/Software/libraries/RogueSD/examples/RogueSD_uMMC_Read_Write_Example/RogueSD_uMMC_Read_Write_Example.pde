#include <NewSoftSerial.h>
#include <RogueSD.h>

NewSoftSerial ummc_s(4, 5);

RogueSD ummc(ummc_s);

void setup()
{
  pinMode(13, OUTPUT);

  Serial.begin(9600);

  ummc_s.begin(9600);
  
  randomSeed(analogRead(0));
}

void loop()
{
  int8_t filehandle;
  char linebuf[60];

  Serial.println("Initializing uMMC");

  // prepares the communications with the uMMC and closes all open files (if any)
  ummc.sync();
  
  Serial.print("uMMC Version: ");
  Serial.println(ummc.version());

  // open a file
  Serial.print("Opening file for append: ");
  filehandle = ummc.open_P(PSTR("/filetest1.txt"), OPEN_APPEND);

  if(filehandle > 0)
  {
    Serial.print("handle ");
    Serial.println(filehandle);

    // append some data
    ummc.writeln_prep(filehandle);

    // you can use any of the standard print methods to send data to the file.
    ummc.print("You'll see this many times in this file. RAND: ");
    ummc.print(random(10000));
    ummc.writeln_finish();

    Serial.println("Append complete.");

    ummc.close(filehandle);
    Serial.println("File closed.");

    // now let's read the file
    filehandle = ummc.open_P(PSTR("/filetest1.txt"), OPEN_READ);
 
    Serial.print("Opening file for read: ");

    if(filehandle > 0)
    {
      Serial.print("opened handle ");
      Serial.println(filehandle);

      Serial.println("-------------");
 
      // this reads 60 characters at a time (if available, per line)
      while(ummc.readln(filehandle, 60, linebuf) > 0)
      {
        Serial.println(linebuf);
      
        delay(200);  // pause 200 milliseconds between lines
      }

      Serial.println("-------------");
      
      ummc.close(filehandle);
      Serial.println("File closed.");
    }
    else
    {
      Serial.print("ERROR: ");
      Serial.println(ummc.LastErrorCode, HEX);
    }
  }
  else
  {
    Serial.print("ERROR: ");
    Serial.println(ummc.LastErrorCode, HEX);
  }

  while(1);  // just loop infinitely here at the end, wait for reset

}
