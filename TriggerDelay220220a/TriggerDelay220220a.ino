/*******************Digitaldelay for DRUM trigger***********************
     If Button is pressed, you got an 1ms trigger event in realtime
       and also you got a second 1ms trigger event with delay.
    The delayTime is set via the USB2serial interface of the ATmega32U4.
      The queue holds 512 trigger events with maximum delay of 60s.
                    The resolution is 1ms.
                         by 25mmHg
                         Have fun!
 ***********************************************************************/

#define MAXDELAYTIME 60000
#define MAXARRAYSIZE 512
#define PRELLTIME 200

#define INBUTTON MOSI
#define OUTDELAY MISO
#define OUTNOOOW LED_BUILTIN
#define DEBUGPIN PD6
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
  pinMode(INBUTTON, INPUT_PULLUP);
  pinMode(OUTDELAY, OUTPUT);
  pinMode(OUTNOOOW, OUTPUT);
#ifdef DEBUGMODE
  //pinMode(DEBUGPIN, OUTPUT);
  DDRD |= 1<<PD6;
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
  digitalWrite(OUTNOOOW, val);
}

void triggerDelay(boolean val) {
  digitalWrite(OUTDELAY, val);
}

boolean getButton() {
  static int counter;
  static boolean lastButton;
  if (!digitalRead(INBUTTON) && counter == 0) {
    counter = PRELLTIME;
    lastButton = true;
    return (true);
  }
  else {
    counter > 0 ? counter-- : counter = 0;
    lastButton = false;
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
    triggerDelay(readTimeStamps());
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
