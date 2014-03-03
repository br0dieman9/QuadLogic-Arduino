The AnalogIsrLogger.ino program demonstrates techniques for logging data to
an SD card at high rates by capturing data in a timer driven interrupt routine.

I have been able to log data at up to 100,000 samples per second using
AnalogIsrLogger.ino.  This requires an excellent SD card. See SD_CARDS.JPG
for photos of cards that have worked at 100,000 8-bit samples per second.

Example data is shown in DATA.PNG and it's FFT is in FFT.PNG.  See ExcelFFT.pdf
in the ADCdocs folder.

The accuracy of the ADC samples depends on the ADC clock rate.  See the
ADC_ENOB.PNG file for a plot of accuracy vs ADC clock frequency.

See files in ADCdocs folder for more information on ADC accuracy.

To modify this program you will need a good knowledge of the Arduino
ADC, timer1 and C++ programming.  This is not for the newbie.

You may need to increase the time between samples if your card has higher
latency.  Using a Mega Arduino can help since it has more buffering.

I have an LED and resistor connected to pin 3 to signal data overruns.
You can disable this feature by setting the pin number negative:

// led to indicate overrun occurred, set to -1 if no led.
const int8_t RED_LED_PIN = 3;

These programs must be used with a recent version of SdFat.  The program
was developed using sdfatlib20120719.zip from:

http://code.google.com/p/sdfatlib/downloads/list

You also need to install the included BufferedWriter library.  It provides
fast text formatting.

Place these three folders in your sketchbook libraries folder.

Place the AnalogIsrLogger folder in your sketchbook.

The program has four commands:

c to check for overruns
d to dump data to Serial
r to record ADC data
t to convert file to text

All commands can be terminated by entering a character from the serial monitor.

The r command will record ADC data to a binary file.  It will terminate
when a character is entered on the serial monitor or the the maximum file
block count has been reached.

The d command converts the binary file to text and displays it on the serial
monitor.  Entering a character on the serial monitor terminates the command.

The t command converts the binary file to a text file.  Entering a character
on the serial monitor terminates the command.

The c commands checks the binary file for overruns.  Data overruns happen when
data samples are lost due to long write latency of the SD.

A number of program options can be set by changing constants at the beginning
of the program.  Key settings include:

// set RECORD_EIGHT_BITS nonzero to only record high 8-bits
#define RECORD_EIGHT_BITS 1

// Sample rate in samples per second.
const uint32_t SAMPLE_RATE = 100000;

// Analog pin number
const uint8_t ANALOG_PIN = 0;

// digital pin to indicate overrun, set to -1 if not used
const int8_t RED_LED_PIN = 3;

// SD chip select pin
const uint8_t chipSelect = SS;

// max file size in blocks
const uint32_t FILE_BLOCK_COUNT = 256000;

// log file name
#define FILE_NAME "LOGFILE.BIN"

// Reference voltage
uint8_t const ADC_REF_AVCC = (1 << REFS0);
