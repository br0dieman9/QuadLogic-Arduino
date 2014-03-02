#include <SD.h>
 
int sdCardPin = 10;
const int chipSelect = 10;
int thisByte = 33;

void setup() {
  pinMode(sdCardPin, OUTPUT); 
  
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  
  // if the file is available, write to it:
  if (dataFile) {
    //dataFile.println(sensorValues);
    dataFile.print(thisByte);
    dataFile.close();
    // print to the serial port too:
    Serial.println(thisByte);
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  } 
} 

// first visible ASCIIcharacter '!' is number 33:
 
// you can also write ASCII characters in single quotes.
// for example. '!' is the same as 33, so you could also use this:
//int thisByte = '!';  

void loop() { 
//  // prints value unaltered, i.e. the raw binary version of the 
//  // byte. The serial monitor interprets all bytes as 
//  // ASCII, so 33, the first number,  will show up as '!' 
//  Serial.write(thisByte);    
//
//  Serial.print(", dec: "); 
//  // prints value as string as an ASCII-encoded decimal (base 10).
//  // Decimal is the  default format for Serial.print() and Serial.println(),
//  // so no modifier is needed:
//  Serial.print(thisByte);      
//  // But you can declare the modifier for decimal if you want to.
//  //this also works if you uncomment it:
//
//  // Serial.print(thisByte, DEC);  
//
//
//  Serial.print(", hex: "); 
//  // prints value as string in hexadecimal (base 16):
//  Serial.print(thisByte, HEX);     
//
//  Serial.print(", oct: "); 
//  // prints value as string in octal (base 8);
//  Serial.print(thisByte, OCT);     
//
//  Serial.print(", bin: "); 
//  // prints value as string in binary (base 2) 
//  // also prints ending line break:
//  Serial.println(thisByte, BIN);   
//
//  // if printed last visible character '~' or 126, stop: 
//  if(thisByte == 126) {     // you could also use if (thisByte == '~') {
//    // This loop loops forever and does nothing
//    while(true) { 
//      continue; 
//    } 
//  } 
//  // go on to the next character
//  thisByte++;  
} 
