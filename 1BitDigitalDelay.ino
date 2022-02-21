/*******************Digitaldelay for DRUM trigger***********************
If the INPUT got a LOW edge, the code generate an 1ms trigger event in 
     realtime and also a second 1ms trigger event with delay.
      The delayTime is adjusted using the ATmega32U4-USB-interface.
               The ringbuffer holds 512 trigger events. 
                 The maximum delay is limited by 60s.
                      The resolution is 1ms.
                           by 25mmHg
                           Have fun!

                         Version 1.02
                    faster port operations
                samplingrate ca.100kHz possible                        
                         Version 1.01
            button debounce logic is much faster (5ms)
                         Version 1.00
                first working needs optimation
 ***********************************************************************/

#define MAXDELAYTIME 60000
#define MAXARRAYSIZE 512
#define BOUNCETIME 5

#define INBUTTON PB2 //MOSI
#define OUTDELAY PB3 //MISO
#define OUTNOOOW PC7 //D13
#define DEBUGPIN PD6 //D12
#define DEBUGMODE
#define SERIALDEBUG

//global variables related to time
unsigned long oldMillis = 0;
unsigned long valMillis = 1;
unsigned int delayTime = 0;

//global variables related to ringbuffer
unsigned long timeStamps[MAXARRAYSIZE];
const unsigned int maxArrayCount = MAXARRAYSIZE - 1;
const unsigned int maxDelayTime = MAXDELAYTIME;
unsigned int writeCursor = 0;
unsigned int readCursor = 0;

void initArray() {
  for (unsigned int i = 0; i < MAXARRAYSIZE; i++) {
    timeStamps[i] = 0xFFFFFFFF;
  }
}

void initUSB2serial() {
  Serial.begin(115200);
  while (!Serial);
  delay(2000);
  Serial.println(F("Hello. I generate Delays for you."));
  Serial.print(F("Please tell me a new delayTime fom 0 to "));
  Serial.print(maxDelayTime);
  Serial.println(F("ms"));
}

void initPins() {
  DDRB &= ~(1 << INBUTTON);
  PORTB |= (1 << INBUTTON);
  DDRB |=  (1 << OUTDELAY);
  DDRC |=  (1 << OUTNOOOW);
#ifdef DEBUGMODE
  DDRD |=  (1 << DEBUGPIN);
#endif
}

void bufferOverRun() {
  Serial.println(F("BUFFER OVERRUN, LOST TRIGGER"));
}

void setNewDelayTime() {
  unsigned long val = 0;
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch >= '0' && ch <= '9') {
      val = val * 10 + ch - '0';
    }
#ifdef SERIALDEBUG
    // Look 4 SPACE
    else if (ch == 32) {
      val = delayTime;
      Serial.println("Array:");
      for (unsigned int i = 0; i <= maxArrayCount; i++) {
        Serial.print(timeStamps[i]);
        if (i == writeCursor)Serial.print("<--w");
        if (i == readCursor) Serial.print("<--r");
        Serial.println();
      }
    }
#endif
    // Look 4 LF
    else if (ch == 10) {
      if (val <= maxDelayTime) {
        delayTime = val;
        Serial.print(val);
        Serial.println(F("ms"));
      }
      else
        Serial.println(F(" FALSE TIME VALUE"));
    }
  }
}

void triggerNow(boolean val) {
  val ? PORTC |= (1 << OUTNOOOW) : PORTC &= ~(1 << OUTNOOOW);
}

void triggerDel(boolean val) {
  val ? PORTB |= (1 << OUTDELAY) : PORTB &= ~(1 << OUTDELAY);
}

boolean getButton() {
  static int counter;
  static boolean lastButton;
  static boolean newButton;
  lastButton = newButton;
  newButton = !(PINB & (1 << INBUTTON));
  if (newButton  && !lastButton && counter == 0) {
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
    timeStamps[writeCursor] = valMillis + delayTime;
    writeCursor < maxArrayCount ? writeCursor++ : writeCursor = 0;
    return (true);
  }
  else
    return (false);
}

boolean readTimeStamps() {
  if (valMillis >= timeStamps[readCursor]) {
    timeStamps[readCursor] = 0xFFFFFFFF;
    readCursor < maxArrayCount ? readCursor++ : readCursor = 0;
    return (true);
  }
  else
    return (false);
}

void setup() {
  initArray();
  initUSB2serial();
  initPins();
}

void loop() {
  valMillis = millis();
  if (oldMillis < valMillis) {

#ifdef DEBUGMODE
    // Use Logic Analyzer
    PORTD |= (1 << DEBUGPIN);
#endif

    oldMillis = valMillis;
    /**********************************************/
    // put your code here, to run every millisecond
    // adjust delay
    setNewDelayTime();
    // Test 4 Button
    if (getButton()) {
      if (writeTimeStamps()) {
        triggerNow(true);
      }
      else
        bufferOverRun();
    }
    else triggerNow(false);
    // Test 4 Delay
    triggerDel(readTimeStamps());
    // end of your code, to run every millisecond
    /**********************************************/

#ifdef DEBUGMODE
    // Use Logic Analyzer
    PORTD &= ~(1 << DEBUGPIN);
#endif

  }
  else if (oldMillis > valMillis) {
    // put your code here, to deal with timer overflow, but this happens in 50days
    ;
  }
  else {
    // run over
    ;
  }
}