#include <SD.h>

int sdCardPin = 10;
const int chipSelect = 10;

// Define the number of samples to keep track of.  The higher the number,
// the more the readings will be smoothed, but the slower the output will
// respond to the input.  Using a constant rather than a normal variable lets
// use this value to determine the size of the readings array.
const int numReadings = 10;

int readings[numReadings];      // the readings from the analog input
int index = 0;                  // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

int inputPin = A0;
File dataFile;
void setup()
{
  pinMode(sdCardPin, OUTPUT);
  // initialize serial communication with computer:
  Serial.begin(9600);  
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  // initialize all the readings to 0: 
  for (int thisReading = 0; thisReading < numReadings; thisReading++){
    readings[thisReading] = 0; 
  }
  SD.remove("smooth.txt");
  dataFile = SD.open("smooth.txt", FILE_WRITE);    
}
String sensorValues= "";
boolean isFileClosed = false;

void loop() {
  if (millis()<10000){
    // subtract the last reading:
    total= total - readings[index];         
    // read from the sensor:  
    readings[index] = analogRead(inputPin); 
    // add the reading to the total:
    total= total + readings[index];       
    // advance to the next position in the array:  
    index = index + 1;                    
  
    // if we're at the end of the array...
    if (index >= numReadings)       {       
      // ...wrap around to the beginning: 
      index = 0;                           
    }
    
    // calculate the average:
    average = total / numReadings;         
    // send it to the computer as ASCII digits
      
           // delay in between reads for stability 
    
     sensorValues = String(millis()) + ":" + String(average);
      
     dataFile.println(sensorValues); 
     dataFile.flush();
     Serial.println(sensorValues);
     //delay(1); 
  } else{
    if(!isFileClosed){
      dataFile.close();
      isFileClosed = true;
      Serial.println("Program done");
    }
  }
}


