#include <SD.h>

int sdCardPin = 10;
const int chipSelect = 10;
int potPin1 = A0;
int potValue1 = 0;
int potPin2 = A1;
int potValue2 = 0;
int potPin3 = A2;
int potValue3 = 0;
File dataFile;
const int numReadings = ;

int readings1[numReadings]; 
int readings2[numReadings]; 
int readings3[numReadings]; 
int index1 = 0;                  // the index of the current reading
int total1 = 0;                  // the running total
int average1 = 0; 

int index2 = 0;                  // the index of the current reading
int total2= 0;                  // the running total
int average2 = 0; 

int index3 = 0;                  // the index of the current reading
int total3 = 0;                  // the running total
int average3 = 0; 

String sensorValues = "";

boolean programComplete = false;
boolean showAverageLog = false;

int runForInSeconds = 30;

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
  
  SD.remove("smooth.csv");
  dataFile = SD.open("smooth.csv", FILE_WRITE);
  
  for (int thisReading = 0; thisReading < numReadings; thisReading++){
    readings1[thisReading] = 0; 
    readings2[thisReading] = 0; 
    readings3[thisReading] = 0; 
  }
} 

void loop()  { 
  if (millis()< (runForInSeconds * 1000)){ 
    CalculateAverage(total1, readings1, index1, potPin1, average1);
    CalculateAverage(total2, readings2, index2, potPin2, average2);
    CalculateAverage(total3, readings3, index3, potPin3, average3);
    
    sensorValues = String(millis()) + "," + String(average1) + "," + String(average2) + "," + String(average3);  
    
    // if we're at the end of the array...
    if (index1 >= numReadings)       {       
      // ...wrap around to the beginning: 
      index1 = 0;                           
      index2 = 0;                           
      index3 = 0;                           
    }
    
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(sensorValues);
      dataFile.flush();
      // print to the serial port too:
      Serial.println(sensorValues);
    }  
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening smooth.csv");
    } 
  } else{
    if (!programComplete){  
      dataFile.close();
      Serial.println("Program Complete...");
      programComplete = true;
    } 
  }
}

String avgLog = "";
void CalculateAverage(int &total, int readings[], int &index, int &inputPin, int &avg){
    avgLog += "NEW CALCULATION------------\n";
    // subtract the last reading:
    total = total - readings[index]; 
    avgLog += "Total before: " + String(total) + "\n";        
    avgLog += "Reading before: " + String(readings[index] + "\n");
    
    // read from the sensor:  
    readings[index] = analogRead(inputPin); 
    avgLog += "Reading after: " + String(readings[index]) + "\n";
    
    // add the reading to the total:
    total= total + readings[index];   
    avgLog += "Total after: " + String(total) + "\n";
    
    // advance to the next position in the array:  
    index = index + 1; 
    avgLog += "Index: " + String(index) + "\n";
    
    avg= total/numReadings;
    avgLog += "Average: " + String(avg) + "\n";
    
    if (showAverageLog){
      Serial.println(avgLog);
    }
} 
