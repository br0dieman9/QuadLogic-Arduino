/**
 * This program logs data from the Arduino ADC to a binary file.
 *
 * Data is logged from an analog pin in either 8-bit mode or 10-bit mode.
 *
 * Each 512 byte block of the file has a four byte header followed by
 * 508 bytes of data. (508 samples in 8-bit mode or 254 samples in 10-bit mode)
 *
 * The sample rate in 8-bit mode can be as high as 100000 samples per second
 * if you have a high quality SD card.  The card must have a very low write
 * latency.
 *
 * The sample rate in 10-bit mode should be 33,333 samples per second or less.
 * The ENOB (Effective Number of Bits) will be significantly reduced at higher
 * rates.
 *
 * The sample rate will be rounded so the interval between samples is a
 * multiple of the ADC clock period and timer1 period.  This reduces
 * sample time jitter.
 *
 * If your SD card has a longer latency, it may be necessary to use
 * slower sample rates.  Using a Mega Arduino helps overcome latency
 * problems since 13 512 byte buffers will be used.
 *
 * The program creates a contiguous file with FILE_BLOCK_COUNT 512 byte blocks.
 * This file is flash erased using special SD commands.
 *
 * Data is written to the file using SD multiple block write commands.
 */
#include <SdFat.h>
#include <SdFatUtil.h>
#include <BufferedWriter.h>

// set RECORD_EIGHT_BITS nonzero to only record high 8-bits
#define RECORD_EIGHT_BITS 1

// Sample rate in samples per second.
// This will be rounded so the sample interval
// is a multiple of the ADC clock period.
const uint32_t SAMPLE_RATE = 100000;

// Desired sample interval in CPU cycles (will be adjusted to ADC/timer1 period)
const uint32_t SAMPLE_INTERVAL = F_CPU/SAMPLE_RATE;

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
// Zero - use External Reference AREF
// (1 << REFS0) - Use Vcc
// (1 << REFS1) | (1 << REFS0) - use Internal 1.1V Reference
uint8_t const ADC_REF_AVCC = (1 << REFS0);

// Minimum ADC clock cycles per sample interval
const uint16_t MIN_ADC_CYCLES = 15;
//==============================================================================
SdFat sd;
#define error(msg) sd.errorHalt_P(PSTR(msg))
//------------------------------------------------------------------------------
// logger will use SdFat's buffer plus BUFFER_BLOCK_COUNT additional buffers
// QUEUE_DIM must be larger than (BUFFER_BLOCK_COUNT + 1)
#if defined(__AVR_ATmega1280__)\
|| defined(__AVR_ATmega2560__)
// Mega - use total of 13 512 byte buffers
const uint8_t BUFFER_BLOCK_COUNT = 12;
#else
// 328 cpu -  use total of two 512 byte buffers
const uint8_t BUFFER_BLOCK_COUNT = 1;
#endif
//------------------------------------------------------------------------------
// 512 byte SD block
#if RECORD_EIGHT_BITS
const uint16_t DATA_DIM = 508;
struct block_t {
  uint16_t count;    // count of data bytes
  uint16_t overrun;  // count of overruns since last block
  uint8_t  data[DATA_DIM];
};
#else  // RECORD_EIGHT_BITS
const uint16_t DATA_DIM = 254;  
struct block_t {
  uint16_t count;    // count of data bytes
  uint16_t overrun;  // count of overruns since last block
  uint16_t data[DATA_DIM];
};
#endif RECORD_EIGHT_BITS
// queues of 512 byte SD blocks
const uint8_t QUEUE_DIM = 16;  // Must be a power of two!

block_t* emptyQueue[QUEUE_DIM];
uint8_t emptyHead;
uint8_t emptyTail;

block_t* fullQueue[QUEUE_DIM];
uint8_t fullHead;
uint8_t fullTail;

// queueNext assumes QUEUE_DIM is a power of two
inline uint8_t queueNext(uint8_t ht) {return (ht + 1) & (QUEUE_DIM -1);}
//==============================================================================
// Interrupt Service Routines

//pointer to current buffer
static block_t* isrBuf;

// overrun count
static uint16_t isrOver = 0;
static volatile uint8_t adcPin = 0;

// ADC done interrupt
ISR(ADC_vect) {
  // read ADC
#if RECORD_EIGHT_BITS
  uint8_t d = ADCH;
#else  // RECORD_EIGHT_BITS
/* 
  uint8_t low = ADCL;
  uint8_t high = ADCH;
  uint16_t d = (high << 8) | low;
*/
  // this will access ADCL first. 
  uint16_t d = ADC;
#endif  // RECORD_EIGHT_BITS
  // check for buffer needed
  if (isrBuf == 0) {
    if (emptyHead != emptyTail) {
      // remove buffer from empty queue
      isrBuf = emptyQueue[emptyTail];
      emptyTail = queueNext(emptyTail);
      isrBuf->count = 0;
      isrBuf->overrun = isrOver;
    } else {
      // no buffers - count overrun
      if (isrOver < 0XFFFF) isrOver++;
      return;
    }
  }
  // store ADC data
  isrBuf->data[isrBuf->count++] = d;

  // check for buffer full
  if (isrBuf->count >= DATA_DIM) {
   
    // put buffer isrIn full queue
    fullQueue[fullHead] = isrBuf;
    fullHead = queueNext(fullHead);
    
    //set buffer needed and clear overruns
    isrBuf = 0;
    isrOver = 0;
  }
}
//------------------------------------------------------------------------------
// timer1 interrupt to clear OCF1B
ISR(TIMER1_COMPB_vect) {}
//==============================================================================
// initialize ADC and timer1
// ticks - sample interval in CPU cycles
// pin - analog pin number
// ref - ADC reference bits (See CPU datasheet)
void adcInit(uint32_t ticks, uint8_t pin, uint8_t ref) {
  PgmPrint("Using Analog pin: ");
  Serial.println(pin, DEC);
  
  if (ref & ~((1 << REFS0) | (1 << REFS1))) {
    error("Invalid ADC reference bits");
  }
  // Set ADC reference andlow three bits of analog pin number
  ADMUX = ref | (pin & 7);
#if RECORD_EIGHT_BITS
  // Left adjust ADC result to allow easy 8 bit reading
  ADMUX |= (1 << ADLAR);
#endif  // RECORD_EIGHT_BITS
  
 // trigger on timer/counter 1 compare match B
  ADCSRB = (1 << ADTS2) | (1 << ADTS0);
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  // the MUX5 bit of ADCSRB selects whether we're reading from channels
  // 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
  if (pin < 8) {
    ADCSRB &= ~(1 << MUX5);
    // disable Digital input buffer
    DIDR0 |= 1 << pin;
  } else {
    ADCSRB |= (1 << MUX5);
    // disable Digital input buffer
    DIDR2 |= 1 << (7 & pin);
  }
#else  // defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  // not a Mega disable Digital input buffer
  if (pin < 6) DIDR0 |= 1 << pin;
#endif  // defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

#if ADPS0 != 0 || ADPS1 != 1 || ADPS2 != 2
#error unexpected ADC prescaler bits
#endif

  uint8_t adps;  // prescaler bits for ADCSRA
  for (adps = 7; adps > 0; adps--) {
   if (ticks >= (MIN_ADC_CYCLES << adps)) break;
  }
  if (adps < 3) error("Sample Rate Too High");
  
  PgmPrint("ADC clock MHz: ");
  Serial.println((F_CPU >> adps)*1.0e-6, 3);

  // set ADC prescaler
  ADCSRA = adps;
  
  // round so interval is multiple of ADC clock
  ticks >>= adps;
  ticks <<= adps;
  
  // Setup timer1
  // no pwm
  TCCR1A = 0;
  
  uint8_t tshift;
  if (ticks < 0X10000) {
    // no prescale, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
    tshift = 0;
  } else if (ticks < 0X10000*8) {
    // prescale 8, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    tshift = 3;
  } else if (ticks < 0X10000*64) {
    // prescale 64, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11) | (1 << CS10);
    tshift = 6;
  } else if (ticks < 0X10000*256) {
    // prescale 256, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12);
    tshift = 8;
  } else if (ticks < 0X10000*1024) {
    // prescale 1024, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12) | (1 << CS10);
    tshift = 10;
  } else {
    error("Sample Rate Too Slow");
  }
  // divide by prescaler
  ticks >>= tshift;
  // set TOP for timer reset
  ICR1 = ticks - 1;
  // compare for ADC start
  OCR1B = 0;
  
  // multiply by prescaler
  ticks <<= tshift;
  PgmPrint("Sample interval usec: ");
  Serial.println(ticks*1000000.0/F_CPU);
  PgmPrint("Sample Rate: ");
  Serial.println((float)F_CPU/ticks);
}
//------------------------------------------------------------------------------
// enable ADC and timer1 interrupts
void adcStart() {
  // Enable ADC, Auto trigger mode, Enable ADC Interrupt, Start A2D Conversions
  ADCSRA |= (1 << ADATE)  |(1 << ADEN) | (1 << ADIE) | (1 << ADSC) ;
  // enable timer1 interrupts
  TIMSK1 = (1 <<OCIE1B);
  TCNT1 = 0;
}
//------------------------------------------------------------------------------
// convert binary file to text file
void binaryToText() {
  uint8_t lastPct;
  block_t buf;
  uint32_t t = 0;
  uint32_t syncCluster = 0;
  SdFile binFile;
  SdFile textFile;
  BufferedWriter bw;
  
  if (!binFile.open(FILE_NAME, O_READ)) {
    error("open binary");
  }
  // create a new binFile
  char name[13];
  strcpy_P(name, PSTR("DATA00.TXT"));
  for (uint8_t n = 0; n < 100; n++) {
    name[4] = '0' + n / 10;
    name[5] = '0' + n % 10;
    if (textFile.open(name, O_WRITE | O_CREAT | O_EXCL)) break;
  }
  if (!textFile.isOpen()) error("open textFile");
  PgmPrint("Writing: ");
  Serial.println(name);
  
  bw.init(&textFile);
  
  while (!Serial.available() && binFile.read(&buf, 512) == 512) {
    uint16_t i;
    if (buf.count == 0) break;
    if (buf.overrun) {
      bw.putStr("OVERRUN,");
      bw.putNum(buf.overrun);
      bw.putCRLF();
    }
    for (i = 0; i < buf.count; i++) {
      bw.putNum(buf.data[i]);
      bw.putCRLF();
    }
    
    if (textFile.curCluster() != syncCluster) {
      bw.writeBuf();
      textFile.sync();
      syncCluster = textFile.curCluster();
    }
    if ((millis() -t) > 1000) {
      uint8_t pct = binFile.curPosition() / (binFile.fileSize()/100);
      if (pct != lastPct) {
        t = millis();
        lastPct = pct;
        Serial.print(pct, DEC);
        Serial.println('%');
      }
    }
    if (Serial.available()) break;
  }
  bw.writeBuf();
  textFile.close();
  PgmPrintln("Done");
}
//------------------------------------------------------------------------------
// read data file and check for overruns
void checkOverrun() {
  SdFile file;
  bool headerPrinted = false;
  block_t buf;
  uint32_t bgnBlock, endBlock;
  uint32_t bn = 0;
  
  Serial.println();
  PgmPrintln("Checking for overrun errors");
  if (!file.open(FILE_NAME, O_READ)) {
    error("open");
  }
  if (!file.contiguousRange(&bgnBlock,&endBlock)) {
    error("contiguousRange");
  }
  while (file.read(&buf, 512) == 512) {
    if (buf.count == 0) break;
    if (buf.overrun) {
      if (!headerPrinted) {
        Serial.println();
        PgmPrintln("Overruns:");
        PgmPrintln("fileBlockNumber,sdBlockNumber,overrunCount");
        headerPrinted = true;
      }
      Serial.print(bn);
      Serial.print(',');
      Serial.print(bgnBlock + bn);
      Serial.print(',');
      Serial.println(buf.overrun);
    }
    bn++;
  }
  if (!headerPrinted) {
    PgmPrintln("No errors found");
  } else {
    PgmPrintln("Done");
  }
}
//------------------------------------------------------------------------------
// dump data file to Serial
void dumpData() {
  SdFile file;
  block_t buf;
  if (!file.open(FILE_NAME, O_READ)) {
    error("open");
  }
  while (!Serial.available() && file.read(&buf , 512) == 512) {
    uint16_t i;
    if (buf.count == 0) break;
    if (buf.overrun) {
      PgmPrint("OVERRUN,");
      Serial.println(buf.overrun);
    }
    for (i = 0; i < buf.count; i++) {
      Serial.println(buf.data[i], DEC);
    }
  }
  PgmPrintln("Done");
}
//------------------------------------------------------------------------------
// log data
// max number of blocks to erase per erase call
uint32_t const ERASE_SIZE = 262144L;
void logData() {
  SdFile file;
  uint32_t bgnBlock, endBlock;  
  // allocate extra buffer space
  uint8_t block[512 * BUFFER_BLOCK_COUNT];
  
  Serial.println();
  
  // initialize ADC and timer1
  adcInit(SAMPLE_INTERVAL, ANALOG_PIN, ADC_REF_AVCC);

  // delete old log file
  if (sd.exists(FILE_NAME)) {
    PgmPrintln("Deleting old file");
    if (!sd.remove(FILE_NAME)) error("remove");
  }
  // create new file
  PgmPrintln("Creating new file");
  if (!file.createContiguous(sd.vwd(), FILE_NAME, 512 * FILE_BLOCK_COUNT)) {
    error("create");
  }
  // get address of file on SD
  if (!file.contiguousRange(&bgnBlock, &endBlock)) {
    error("range");
  }
  file.close();
  
  // use SdFats internal buffer
  uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
  if (cache == 0) error("cacheClear"); 
  PgmPrintln("Erasing all data");
  
  // flash erase all data in file
  uint32_t bgnErase = bgnBlock;
  uint32_t endErase;
  while (bgnErase < endBlock) {
    endErase = bgnErase + ERASE_SIZE;
    if (endErase > endBlock) endErase = endBlock;
    if (!sd.card()->erase(bgnErase, endErase)) {
      error("erase");
    }
    bgnErase = endErase + 1;
  }
  // initialize queues
  emptyHead = emptyTail = 0;
  fullHead = fullTail = 0;
  
  // initialize ISR
  isrBuf = 0;
  isrOver = 0;
  
  // use SdFat buffer for one block
  emptyQueue[emptyHead] = (block_t*)cache;
  emptyHead = queueNext(emptyHead);
  
  // put rest of buffers in empty queue
  for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++) {
    emptyQueue[emptyHead] = (block_t*)(block + 512 * i);
    emptyHead = queueNext(emptyHead);
  }
  // start multiple block write
  if (!sd.card()->writeStart(bgnBlock, FILE_BLOCK_COUNT)) {
    error("writeBegin");
  }
  PgmPrintln("Logging - type any character to stop");
  // wait for Serial Idle
  Serial.flush();

  uint32_t bn = 0;
  uint32_t t0 = millis();
  uint32_t t1 = t0;
  uint32_t overruns = 0;
  uint32_t count = 0;
  // start logging interrupts
  adcStart();
  while (1) {
    if (fullHead != fullTail) {
      // block to write
      block_t* block = fullQueue[fullTail];
      
      // write block to SD
      if (!sd.card()->writeData((uint8_t*)block)) {
        error("writeData");
      }
      t1 = millis();
      count += block->count;
      // check for overrun
      if (block->overrun) {
        overruns += block->overrun;
        if (RED_LED_PIN >= 0) {
          digitalWrite(RED_LED_PIN, HIGH);
        }
      }
      // move block to empty queue
      emptyQueue[emptyHead] = block;
      emptyHead = queueNext(emptyHead);
      fullTail = queueNext(fullTail);
      bn++;
      if (bn == FILE_BLOCK_COUNT) {
        // stop ISR calls
        ADCSRA = 0;
        break;
      }
    }
    if (Serial.available()) {
      // stop ISR calls
      ADCSRA = 0;
      if (isrBuf != 0) {
        // put buffer isrIn full queue
        fullQueue[fullHead] = isrBuf;
        fullHead = queueNext(fullHead);
        isrBuf = 0;
      }
      if (fullHead == fullTail) break;
    }
  }

  if (!sd.card()->writeStop()) error("writeStop");
  // truncate file if recording stoped early
  if (bn != FILE_BLOCK_COUNT) {    
    if (!file.open(FILE_NAME, O_WRITE)) error("open");
    PgmPrint("Truncating file");
    if (!file.truncate(512L * bn)) error("truncate");
    file.close();
  }
  Serial.println();
  PgmPrint("Record time ms: ");
  Serial.println(t1 - t0);
  PgmPrint("Sample count: ");
  Serial.println(count);
  PgmPrint("Overruns: ");
  Serial.println(overruns);
  PgmPrintln("Done");
}
//------------------------------------------------------------------------------
void setup(void) {
  Serial.begin(9600);
  PgmPrint("FreeRam: ");
  Serial.println(FreeRam());
  
  // initialize file system.
  if (!sd.begin(chipSelect, SPI_FULL_SPEED)) {
    sd.initErrorHalt();
  }
}
//------------------------------------------------------------------------------
void loop(void) {
  // discard any input
  while (Serial.read() >= 0) {}
  Serial.println();
  PgmPrintln("type:");
  PgmPrintln("c to check for overruns");
  PgmPrintln("d to dump data to Serial");
  PgmPrintln("r to record ADC data");
  PgmPrintln("t to convert file to text");
  while(!Serial.available()) {}
  char c = tolower(Serial.read());
  if (RED_LED_PIN >= 0) {
    pinMode(RED_LED_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN, LOW);
  }
  if (c == 'c') {
    checkOverrun();
  } else if (c == 'd') {
    dumpData();
  } else if (c == 'r') {
    logData();
  } else if(c == 't') {
    binaryToText();
  } else {
    PgmPrintln("Invalid entry");
  }
}
