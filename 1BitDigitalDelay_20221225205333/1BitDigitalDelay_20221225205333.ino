/*******************Digitaldelay V1.10 for DRUM trigger******************
  If the INPUT got a LOW edge, the code generate an 1ms trigger event in
     realtime and also a second 1ms trigger event with delay.
      The delayTime is adjusted using the ATmega32U4-USB-interface.
               The ringbuffer holds 512 trigger events.
                 The maximum delay is limited by 60s.
                      The resolution is 100us.
                           by 25mmHg
                           Have fun!
                           Version 1.10
               100us resolution for with Timer3
                           Version 1.08
                  "!" is new Trigger, serial RX only mode
                           Version 1.07
                        new autotrigger mode
                          Version 1.06
                        serialtrigger mode
                          Version 1.05
                        heartbeat mode
                          Version 1.04
                 new pins (matched with PCB)
                          Version 1.03
          add LED-monitor output and change PORTS
                          Version 1.02
                    faster port operations
                samplingrate ca.100kHz possible
                          Version 1.01
            button debounce logic is much faster (5ms)
                          Version 1.00
                first working needs optimation
 ***********************************************************************/

#define MAXDELAYTIME 600000 //1=100us
#define MAXPERIODTIME 600000 //1=100us
#define MINPERIODTIME 200 //1=100us
#define MAXRHYTHMPATTERN 0xFFFF
#define MAXLOUDPATTERN 0xFFFF
#define MINPERIODTIME 200 //1=100us
#define MAXARRAYSIZE 512
#define BOUNCETIME 100 //1=100us
#define FLASHTIME 200 //1=100us
#define LOUDTIME 20 //1=100us

#define INBUTTON PF4 //A3
#define OUTDELAY PF7 //A0
#define OUTNOOOW PF6 //A1
#define OUTCOOPY PC7 //D13
#define DEBUGPIN PB6 //A10
#define DEBUGMODE

//globalFlags
boolean globalTrigFlag = false;

//global variables related to time
unsigned long val100Micros = 0;
unsigned long nextTrigTime = 0;
unsigned long delayTime = 0;
unsigned long periodTime = 0;
unsigned int rhythmPattern = 0;
unsigned int loudPattern = 0;
const unsigned long maxPeriodTime = MAXPERIODTIME;
const unsigned long minPeriodTime = MINPERIODTIME;
const unsigned long maxRhythmPattern = MAXRHYTHMPATTERN;
const unsigned long maxLoudPattern = MAXLOUDPATTERN;

//global variables related to ringbuffer
unsigned long timeStamps[MAXARRAYSIZE];
const unsigned int maxArrayCount = MAXARRAYSIZE - 1;
const unsigned long maxDelayTime = MAXDELAYTIME;
unsigned int writeCursor = 0;
unsigned int readCursor = 0;

//global variables related to ringbuffer
void initArray() {
  for (unsigned int i = 0; i < MAXARRAYSIZE; i++) {
    timeStamps[i] = 0xFFFFFFFF;
  }
}

void initUSB2serial() {
  Serial.begin(115200);
  while (!Serial);
  delay(1000);
}

void initPins() {
  DDRF &= ~(1 << INBUTTON);
  // pullups installed on PCB
  // PORTF |= (1 << INBUTTON);
  DDRF |=  (1 << OUTDELAY);
  DDRF |=  (1 << OUTNOOOW);
#ifdef DEBUGMODE
  DDRB |=  (1 << DEBUGPIN);
  DDRC |=  (1 << OUTCOOPY);
#endif
}

void getSerialData() {
  static unsigned long val;
  if (Serial.available()) {
    char ch = Serial.read();
    // Look for !
    if (ch == '!') {
      globalTrigFlag = true;
    }
    // Look for number
    else if (ch >= '0' && ch <= '9') {
      val = val * 10 + ch - '0';
    }
    // Look for d
    else if (ch == 'd') {
      if (val <= maxDelayTime) {
        delayTime = val;
      }
      val = 0;
    }
    // Look for p
    else if (ch == 'p') {
      if (!val || ((val <= maxPeriodTime) && (val >= minPeriodTime))) {
        periodTime = val;
      }
      val = 0;
    }
     // Look for r
    else if (ch == 'r') {
      if (val <= maxRhythmPattern) {
        rhythmPattern = val;
      }
      val = 0;
    }
     // Look for l
    else if (ch == 'l') {
      if (val <= maxLoudPattern) {
        loudPattern = val;
      }
      val = 0;
    }
    // Look for LF or CR
    else if (ch == 10 || ch == 13) val = 0;
    else;
  }
}

void flashLED() {
  static int flashTimer;
  if (PORTF & (1 << OUTNOOOW | 1 << OUTDELAY) || (0 < flashTimer  && flashTimer < FLASHTIME)) {
    PORTC  |= (1 << OUTCOOPY);
    flashTimer > 0 ? flashTimer-- : flashTimer = 0;
  }
  else {
    PORTC &= ~(1 << OUTCOOPY);
    flashTimer = FLASHTIME;
  }
}

void loudness() {
  static int loudTimer;
  if (globalTrigFlag && globalLoudFlag || (0 < loudTimer  && loudTimer < LOUDTIME)) {
    loudTimer > 0 ? loudTimer-- : loudTimer = 0;
  }
  else {
    loudTimer = LOUDTIME;
  }
}



boolean getTrigger() {
  static int counter;
  static boolean lastTrigger;
  static boolean newTrigger;
  lastButton = newTrigger;
  newTrigger = !(PINF & (1 << INBUTTON)) || globalTrigFlag;
  if (newTrigger  && !lastButton && counter == 0) {
    counter = BOUNCETIME;
    return (true);
  }
  else {
    counter > 0 ? counter-- : counter = 0;
    return (false);
  }
}

boolean writeTimeStamps() {
  if (timeStamps[writeCursor] == 0xFFFFFFFF) {
    timeStamps[writeCursor] = val100Micros + delayTime;
    writeCursor < maxArrayCount ? writeCursor++ : writeCursor =  0;
    return (true);
  }
  else
    return (false);
}

boolean readTimeStamps() {
  if (val100Micros >= timeStamps[readCursor]) {
    timeStamps[readCursor] = 0xFFFFFFFF;
    readCursor < maxArrayCount ? readCursor++ :  readCursor = 0;
    return (true);
  }
  else
    return (false);
}

void initTimer3()
{
    TCCR3A = 0; // no pins
    TCCR3B = 0; // Reset
    TCCR3B |= (1 << WGM32); // CTC mode (clear@compareMatch)
    TCCR3B |= (1 <<  CS31); // prescaler is 8
    OCR3A = 200; // set up output compare value for 100us
}

void setup() {
  initTimer3();
  initArray();
  initUSB2serial();
  initPins();
}

void loop() {
  //Timer3 compare match
  if (TIFR3 & (1 << OCF3A)) {
    TIFR3 = (1 << OCF3A);
    val100Micros++;      
#ifdef DEBUGMODE
    // Use Logic Analyzer
    PORTB |= (1 << DEBUGPIN);
#endif

    /*************************************************/
    // put your code here, to run every 100microseconds
    
    getSerialData();
    // test 4 next auto trigger
    if ((val100Micros >= nextTrigTime) && periodTime) {
      nextTrigTime += periodTime;
      globalTrigFlag = true;
    }
    // Test 4 Button TRIGGER AND CLEAR NOW
    if (getTrigger()) {
      if (writeTimeStamps()) PORTF |= (1 << OUTNOOOW);
      else;
    }
    else PORTF &= ~(1 << OUTNOOOW);
    globalTrigFlag = false;
    
    // Test 4 Delay TRIGGER AND CLEAR DELAY
    if (readTimeStamps())PORTF |= (1 << OUTDELAY);
    else PORTF &= ~(1 << OUTDELAY);
    
    // end of your code, to run every 100microseconds
    /***********************************************/

#ifdef DEBUGMODE
    // Use Logic Analyzer
    PORTB &= ~(1 << DEBUGPIN);
    flashLED();
#endif
  } 
  else;
}
