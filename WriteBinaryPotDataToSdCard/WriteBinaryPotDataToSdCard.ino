#include <SD.h>
 
//log vars
int sdCardPin = 10;
const int chipSelect = 10;
int baud = 9600;
File file;
String sensorValues = "";
  
//pot vars
int potPin1 = A0;
int potValue1 = 0;
int potPin2 = A1;
int potValue2 = 0;
int potPin3 = A2;
int potValue3 = 0;

void setup() {  
  pinMode(sdCardPin, OUTPUT); 
  pinMode(potPin1, INPUT);
  pinMode(potPin2, INPUT);
  pinMode(potPin3, INPUT);
  
  Serial.begin(baud);
  SD.begin(chipSelect);
  SD.remove("pot.TXT");
  file = SD.open("pot.TXT", O_CREAT | O_WRITE);
}

boolean isFileClosed = false;
void loop() { 
 if (millis() < 10000){
   potValue1 = analogRead(potPin1);
   potValue2 = analogRead(potPin2);
   potValue3 = analogRead(potPin3);  

   //sensorValues = String(millis()) + ":" + String(potValue1) + ";" + String(potValue2) + ";" + String(potValue3);
   file.println(millis());
//   for (uint8_t i = 0; i < 100; i++) {
//    file.println(millis());
//    }
  // not needed in this example since close() will call flush.
  file.flush();  
 } else{
   if(!isFileClosed){
     file.close();
     Serial.println("Program Complete");
     isFileClosed = true;
   }
 }
}
