#include <SD.h>
 
int sdCardPin = 10;
const int chipSelect = 10;
int firstNumber = 33;
byte delimeter = 59;
int secondNumber = 66;

void setup() {  
  pinMode(sdCardPin, OUTPUT); 
  Serial.begin(9600);
  SD.begin(chipSelect);
  File file = SD.open("WR_TEST2.TXT", O_CREAT | O_WRITE);

  while(millis() < 1000);  // delay so mills() is four digits
 
  for (uint8_t i = 0; i < 100; i++) {
    file.println(millis());
  }
  // not needed in this example since close() will call flush.
  file.flush();  
 
  file.close();
//  //Serial.begin(9600);
//  //Serial.println("Program starting.");
//  pinMode(sdCardPin, OUTPUT); 
//  
//  if (!SD.begin(chipSelect)) {
//    //Serial.println("Card failed, or not present");
//    // don't do anything more:
//    return;
//  }
//  //Serial.println("card initialized.");
//  
//  if (SD.exists("binary.txt")) {
//    //Serial.println("binary.txt exists.");
//  }
//  else {
//    //Serial.println("binary.txt doesn't exist.");
//  }
//  
//  // if the file is available, write to it:
//  SD.remove("binary.txt");
//  File dataFile = SD.open("binary.txt", FILE_WRITE);
//  if (dataFile) {
//    //dataFile.println(sensorValues);
//    dataFile.write(firstNumber);
//    dataFile.write(delimeter);
//    dataFile.write(secondNumber);
//    dataFile.close();
//    // print to the serial port too:
//    //Serial.println(thisByte);
//  }  
//  // if the file isn't open, pop up an error:
//  else {
//    //Serial.println("error opening binary.txt");
//  } 
}

void loop() { 
 
}
