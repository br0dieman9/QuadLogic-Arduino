#include <SD.h>

int sdCardPin = 10;
const int chipSelect = 10;
int potPin1 = A0;
int potValue1 = 0;
int potPin2 = A1;
int potValue2 = 0;
int potPin3 = A2;
int potValue3 = 0;

void setup()  { 
  // declare pin 9 to be an output:
  pinMode(sdCardPin, OUTPUT);
  pinMode(potPin1, INPUT);
  pinMode(potPin2, INPUT);
  pinMode(potPin3, INPUT);
  
  //begin serial communication  
  Serial.begin(9600);
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
} 

void loop()  { 
  String sensorValues = "";
  
  potValue1 = analogRead(potPin1);
  potValue2 = analogRead(potPin2);
  potValue3 = analogRead(potPin3);  
  
  sensorValues = String(millis()) + ":" + String(potValue1) + ";" + String(potValue2) + ";" + String(potValue3);
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(sensorValues);
    dataFile.close();
    // print to the serial port too:
    Serial.println(sensorValues);
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  } 
}
