/**

   reads signal from 2 mics, and switch on leds to indicate which mic is closer to the source of the noise.
   Works by identifying the timing of peak signal over 1000 samples

   Important: the arduino must run on batteries for the mics to work properly

*/


#define LOG(x) { Serial.print(x); Serial.print(" ");}
#define LOGN(x)  Serial.println(x);



// -- screen ----------------------------------------------------------


//#include "01-Sketch.cpp"


#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>




#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setupScreen() {
  LOGN(F("setupScreen"));
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    LOGN(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0, 0);            // Start at top-left corner
  display.display();


  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
}




// -- buttons --------------------------------------------------------------------------------

#define  BUTTON_OK (byte)4
#define  BUTTON_UP (byte)5
#define  BUTTON_DOWN (byte)6
#define NO_BUTTON (byte)-1

byte BUTTONS_ALL[] = {BUTTON_OK, BUTTON_UP, BUTTON_DOWN, NO_BUTTON};
byte BUTTONS_OK[] = {BUTTON_OK, NO_BUTTON};
byte BUTTONS_UP_DOWN[] = {BUTTON_UP, BUTTON_DOWN, NO_BUTTON};
#define INPUT_DELAY_MS 100


void setupButtons() {
  LOGN(F("setupButtons"));
  for (int i = 0; BUTTONS_ALL[i] != NO_BUTTON ; i++) {
    LOG(BUTTONS_ALL[i]);
    LOGN(i);

    pinMode(BUTTONS_ALL[i], INPUT);    // sets the digital pin 6 as input
  }
}


/**
   scan buttons status and return pin of first button pressed, or -1
*/
byte getInput(byte *pins) {
  for (int i = 0; pins[i] != NO_BUTTON; i++) {
    int v = digitalRead(pins[i]);
    if (v > 0)
      return pins[i];
  }
  return NO_BUTTON;
}


/**
   same as getInput(), but wait for release
*/
byte getInputReleased(byte * buttons) {
  byte pressed = getInput( buttons);
  if (pressed == NO_BUTTON)
    return NO_BUTTON;
  LOG(F("pressed:"));
  LOGN(pressed);

  waitForReleased(pressed);

  return pressed; // pressed and released
}
void waitForReleased(byte pin) {
  LOG(F("waitForReleased"));
  LOGN(pin);
  while (digitalRead(pin)) {
    delay(INPUT_DELAY_MS);
  }
  LOGN(F("released"));

}

/**
   wait for a button to be pressed and released
*/
byte expectInput(byte * buttons) {
  LOGN(F("expectInput"));
  while (true) {
    int button = getInputReleased(buttons);
    if (button != NO_BUTTON)
      return button;
    delay(INPUT_DELAY_MS);
  }
  // never returns
  return NO_BUTTON;
}


void displayReset() {
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0, 0);
}

/**
*/

void displayPrintf(const char *format, va_list ap) {

  char buffer[11 * 4];
  sprintf(buffer, format, ap);
  display.print(buffer);

  LOG(F("display:"))
  LOGN(buffer)

}


void displayPrintf(const char *format, ...) {
  va_list vl;
  va_start(vl, format);
  displayPrintf(format, vl);
}


void showMessage(const char *format, ...) {

  va_list vl;
  va_start(vl, format);
  displayReset();
  displayPrintf(format, vl);
  display.display();
}

void error(char * info) {
  showMessage(info);
  expectInput(BUTTONS_ALL);
}
/**
   display the list of choices
*/
void showChoices(int currentChoice, int choices, char ** choicesText) {
  Serial.println(currentChoice);
  int startDisplay = 0;

  if (currentChoice > 1) {
    startDisplay = currentChoice - 1;
    if (currentChoice + 2 > choices) {
      startDisplay = currentChoice - 2;
    }

  }




  displayReset();


  for (int i = startDisplay; i < choices; i++) {
    if (i == currentChoice)
      display.print("*");
    else
      display.print(" ");
    display.println(choicesText[i]);
  }
  display.display();
}



int getChoice(int currentChoice, int choices, char ** choicesText) {

  while (true) {

    showChoices(currentChoice, choices, choicesText);

    int button = getInputReleased(BUTTONS_ALL);
    switch (button) {
      case BUTTON_OK:

        LOG(F("getChoice:"));
        LOGN(currentChoice);
        return currentChoice;
      case BUTTON_UP:
        currentChoice--;
        if (currentChoice < 0)
          currentChoice = 0;
        break;
      case BUTTON_DOWN:
        currentChoice++;
        if (currentChoice >= choices)
          currentChoice = choices - 1;
    }

    delay(INPUT_DELAY_MS);
  }
  // never happens
  return -1;
}




int chooseValue(char *name, int value, int min, int max, int steps) {
  LOG(F("chooseValue"));
  LOG(name);
  LOGN(value);


  while (true) {
    displayReset();
    display.println(name);
    display.print(value);
    display.display();


    int button = expectInput(BUTTONS_ALL);


    switch (button) {
      case BUTTON_OK:
        return value;

      case BUTTON_UP:
        value -= steps;
        if (value < min);
        value = min;
        break;
      case BUTTON_DOWN:
        value += steps;
        if (value > max)
          value = max;
    }

    LOGN(value);
  }
}


// -- mic configuration --------------------------------------------




//int DELAY



//
typedef struct Mic {
  byte pin;
  int vmax;
  int tmax;
  int vmin;
  int tmin;
  int tamax;
  int vamax;
  long acc;
} Mic;


#define BUTTON_PIN 6
#define MAX_MICS 4

int MICS = 2;


struct Mic mics[MAX_MICS];




void setupMics() {
  mics[0].pin = A0;
  mics[1].pin = A3;

}


void resetMic(int i) {
  mics[i].vmax = 0;
  mics[i].vmin = 1024;
  mics[i].vamax = 0;
  mics[i].tmax = -1;
  mics[i].tmin = -1;
  mics[i].tamax = 0;
  mics[i].acc = 0;
}




/*

  // small loop to detect what is the ref voltage of mic
  int hits=0;
  float acc=0;

  void loop() {
    int h=analogRead(micPin);
    acc+=h;
    hits++;
    float avg=acc/hits;

    Serial.print("hits:");
    Serial.print(hits);
    Serial.print("avg:");
    Serial.print(avg);

    Serial.println();
    delay(500);
  }
*/



// -- config -----------------------------------------

char * version="v0.01";

// number of samplings per bucket
#define SAMPLES 1000


#define SAMPLES_REC 64

// record the values from mic1, for debug only
byte d1[SAMPLES_REC];

// volume threshold
int threshold = 45;

int maxdt = 10;


int loopDelayMs = 20;

// -- stats ----------------------------------------
// total number of buckets so far
int buckets;
int startTimer;

// -- recording -------------------------------------
// time inside bucket
int t = 0;

// max sampling value
//int vmax[MICS];

// timing of max sampling value
//int tmax[MICS];



// -- analysis --------------------------------------
// index of first mic
#define UNSET -1
#define UNDER_THRESHOLD -2
#define DTT_TOO_HIGH -3

int firstMic = UNSET;


// dt to first
int dtf[MAX_MICS];


void reset() {
  // reset
  for (int i = 0; i < MICS; i++)
    resetMic(i);
}



#define MODE_DETECT 0
#define MODE_OSCILO 1
#define MODE_INFO 2

int mode = MODE_DETECT;

// -- recording -------------------------------------

void record() {



  //  read the mics values, and maybe update max
  for (int i = 0; i < MICS; i++) {

    int n = analogRead(mics[i].pin); // n1 range is [0..1024]


    if (t * MICS < SAMPLES_REC) {
      d1[t * MICS + i] = n / 4;
    }



    mics[i].acc += (long) n;

    //  n = n - 512;
    //if (n < 0)
    //n = -n;
    if (n > mics[i].vmax) {
      mics[i].vmax = n;
      mics[i].tmax = t;
    }
    if (n < mics[i].vmin) {
      mics[i].vmin = n;
      mics[i].tmin = t;
    }

  }

  delayMicroseconds(loopDelayMs);
}


void analyze() {
  LOGN(F("analyze"));
  // 0) reset analysis flags
  firstMic = UNSET;
  for (int i = 0; i < MICS; i++) {
    dtf[i] = 0;
  }


  // 1.0) normalize
  for (int i = 0; i < MICS; i++) {
    // compute avg
    int avg = (mics[i].acc / (long)SAMPLES);
    LOG(F("mic:"));
    LOGN(i);
    LOG(F("acc:"));
    LOG(mics[i].acc);
    LOG(F("avg:"));
    LOGN(avg);

    // remove avg from max and min
    // mics[i].vmax -= avg;
    // mics[i].vmin -= avg;
  }

  // if -vmin is greater than vmax, then use -vmin as vmax
  for (int i = 0; i < MICS; i++) {

    int amaxFromVmax = mics[i].vmax;
    int amaxFromVmin = -mics[i].vmin;
    if (amaxFromVmin > amaxFromVmax) {
      LOG(F("vamax s vmin"));

      mics[i].vamax = amaxFromVmin;
      mics[i].tamax = mics[i].tmin;
    } else {
      LOG(F("vamax s vmax"));
      mics[i].vamax = amaxFromVmax;
      mics[i].tamax = mics[i].tmax;

    }
    LOG(F("vamax:"));
    LOG( mics[i].vamax);
    LOG(F("tamax:"));
    LOGN(mics[i].tamax );


  }



  // 1.1) find loudest mic
  int maxi = -1;
  int maxv = -1;

  for (int i = 0; i < MICS; i++) {
    if (mics[i].vamax > maxv) {
      maxv = mics[i].vamax;
      maxi = i;
    }
  }

  // is loudest signal too weak?
  if (maxv < threshold) {
    firstMic = UNDER_THRESHOLD;
    return;
  }


  // 2) identify mic that picked up signal first
  int firstMicT = SAMPLES;
  for (int i = 0; i < MICS; i++) {
    if (mics[i].tamax < firstMicT) {
      firstMicT = mics[i].tamax;
      firstMic = i;
    }
  }

  // 3.a) fill up Difference in Time to First
  for (int i = 0; i < MICS; i++) {
    int dt = mics[i].tamax - firstMicT;
    dtf[i] = dt;
  }



  // 3.b) if any mic has its peak which is too far away in time, cancel the whole thing
  for (int i = 0; i < MICS; i++) {
    if (  dtf[i] > maxdt) {
      firstMic = DTT_TOO_HIGH;
      return;
    }
  }


  //

  //      if (max1>threshold &&  max2>threshold){
  //
  //
  //        // some noise occured
  //
  //
  //        // turn the default LED on
  //        digitalWrite(LED_BUILTIN, HIGH);
  //
  //
  //        // dtt is threshold for time difference
  //        int dtt=2;
  //
  //        // difference in peak times
  //        int dt=t1-t2;
  //
  //
  //        if (dt>dtt){
  //          // peak signal was recorded earlier on mic2
  //          digitalWrite(7, HIGH);
  //          digitalWrite(8, LOW);
  //        } else if (dt<-dtt){
  //          // peak signal was recorded earlier on mic1
  //          digitalWrite(8, HIGH);
  //          digitalWrite(7, LOW);
  //        } else {
  //          // unclear
  //          digitalWrite(8, HIGH);
  //          digitalWrite(7, HIGH);
  //        }
  //
  //         } else {
  //
  //        // not loud noise hear, so switch off all leds
  //
  //        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  //
  //        digitalWrite(8, LOW);
  //        digitalWrite(7, LOW);
  //
  //        Serial.print(".");
  //      }
  //

}


void resetBpsTimer() {
  LOG(F("resetBpsTimer"));
  startTimer = millis();
  buckets = 0;
}




void displayDetectModeMicLine(int i) {
  display.print(mics[i].vamax);
  if (i == firstMic)
    display.print("*");
  else
    display.print("@");

  display.print(mics[i].tamax);


}
void updateScreenDetectMode() {

  if (buckets % 4 == 0)
  {
    displayReset();
    float bps = (1000.0 * buckets) / (millis() - startTimer);
    display.print(bps);
    display.print(" ");
    if (firstMic >= 0)
      display.print(firstMic + 1);
    else
      display.print(firstMic);
    display.print(" ");


    display.println();


    for (int i = 0; i < MICS; i++) {

      displayDetectModeMicLine(i);

      if (i != firstMic) {
        display.print(" ");
        display.print(dtf[i]);
      }
      display.println();


    }




    // Show the display buffer on the screen. You MUST call display() after
    // drawing commands to make them visible on screen!
    display.display();

  }
}




int infoModeY = 0;


void updateScreenInfoMode() {
  LOG(F("updateScreenInfoMode "))
  //if (buckets % 4 == 0)
  int v = getInputReleased(BUTTONS_UP_DOWN);
  if (v == BUTTON_DOWN) {
    infoModeY++;
    if (infoModeY >= 2 * MICS)
      infoModeY = 2 * MICS - 1;

  }
  else if (v ==  BUTTON_UP) {
    infoModeY--;
    if (infoModeY < 0)
      infoModeY = 0;

  }

  int mic = getInputReleased(BUTTONS_UP_DOWN);


  displayReset();
  int infoModeMic = infoModeY / 2;
  int infoModePage = infoModeY % 2;



  LOG(F("infoModeY:"));
  LOG(infoModeY);
  LOG(F("infoModeMic:"));
  LOG(infoModeMic);
  LOG(F("infoModePage:"));
  LOG( infoModePage);



  display.print(F("mic "));
  display.print(infoModeMic + 1);
  display.print(F("/"));
  display.print(MICS );
  display.print(F("p"));
  display.print( (infoModePage) + 1);
  display.println();



  // CAPTCHA switch block not working ???
  if (infoModePage == 0) {
    //  switch( infoModePage) {  case 0:
    displayDetectModeMicLine(infoModeMic);
    display.println();
    display.print(F("r:"));
    display.print(mics[infoModeMic].vmin);
    display.print(F("-"));
    display.print(mics[infoModeMic].vmax);
    display.println();
    display.print(F("a"));
    int avg = mics[infoModeMic].acc / SAMPLES;
    display.print(avg);
    display.print(F("d:"));
    int d = mics[infoModeMic].vmax - mics[infoModeMic].vmin;

    display.print(d);
    display.println();
    //      break;
    //      case 1:
  } else if ( infoModePage == 1) {
    display.print(F("page 2..."));
    display.print( infoModeY );

    display.println();
    //      break;
  }


  display.display();

}



void updateScreenOsciloMode() {



  displayReset();
  if (SAMPLES_REC > 0 ) {


    int h = 64 / MICS;

    for (int i = 0; i < MICS; i++) {

      int h0 = i * h;

      for (int j = 0; j < SAMPLES_REC / MICS; j++) {
        byte y = d1[j * MICS + i] / 4;



        if (y < 0)
          y = 0;
        else if (y > 63)
          y = 63;

        y = y / MICS;

        display.drawLine(j, h0, j, h0 + (int)y, SSD1306_WHITE);
      }
    }

    // display.print(d1[10]);

    display.display();
  }


}
void updateScreen() {
  LOG(F("updateScreen mode:"));
  LOG(mode);
  LOG(F("buckets:"));
  LOGN(buckets);
  switch (mode) {
    case MODE_INFO:
      updateScreenInfoMode();
      break;
    case MODE_DETECT:
      updateScreenDetectMode();
      break;
    case MODE_OSCILO:
      updateScreenOsciloMode();
      break;
  }
}


void dumpSerial() {

  //        // dump some debug info on serial
  //        Serial.print("t1:");
  //        Serial.print(t1);
  //        Serial.print("max1:");
  //        Serial.println(max1);
  //
  //
  //        Serial.print("t2:");
  //        Serial.print(t2);
  //        Serial.print("max2:");
  //        Serial.println(max2);

  for (int i = 0; i < SAMPLES_REC; i++) {
    Serial.print(" ");
    int n = d1[i] - 127;
    int an = n;
    if (an < 0)
      an = -an;

    if (an > 30)
      Serial.print(d1[i]);
    else
      Serial.print("o");

    if (i % 32 == 0)
      Serial.println();


  }


}




// -- actual menus -----------------------------------------



int MENU_MAIN_S = 5;
char * MENU_MAIN[] = {"Detect", "Config", "Oscilo", "Info", "About"};

/**
   enter the main menu
   This method will only return after the user exits config

*/

int menuMainChoice = 0;
void menuMain() {
  int menuMainChoice = 0;
  while (true) {
    menuMainChoice = getChoice(menuMainChoice, MENU_MAIN_S, MENU_MAIN);
    switch (menuMainChoice) {
      case 0:
        mode = MODE_DETECT;
        return;
      case 1:
        menuConfig();
        break;
      case 2:
        mode = MODE_OSCILO;
        return;
      case 3:
        mode = MODE_INFO;
        return;
      case 4:
        showMessage(version);
        expectInput(BUTTONS_ALL);
        return;

    }
  }
}




int MENU_CONFIG_S = 5;
char * MENU_CONFIG[] = {"Back", "loop ms", "mics", "maxdt", "minv"};


void menuConfig() {
  int choice = 0;
  while (true) {
    choice = getChoice(choice, MENU_CONFIG_S, MENU_CONFIG);
    switch (choice) {
      case 0:
        return;
      case 1:
        loopDelayMs = chooseValue("loop ms", loopDelayMs, 0, 20, 1);
        break;
      case 2: MICS = chooseValue("mics", MICS, 1, MAX_MICS, 1);
        break;

      case 3: maxdt = chooseValue("maxdt", maxdt, 1, 50, 2);
        break;
      case 4: threshold = chooseValue("minv", threshold, 10, 100, 5);
        break;



    }
  }
}









void maybeEnterMenu() {

  if (buckets % 4 == 0) {

    int v = getInputReleased(BUTTONS_OK);

    //    int v = digitalRead(BUTTONS_PINS[BUTTON_OK]);
    if (v != NO_BUTTON) {
      LOG(F("input released:"));
      LOGN(v);
      //      waitForReleased(BUTTONS_PINS[BUTTON_OK]);

      menuMain();
      resetBpsTimer();
    }
    //           mode++;
    //           if (mode>MODE_CONFIG)
    //              mode=MODE_DETECT;
    //       }

  }
}

// --


void setup() {

  Serial.begin(115200);
  LOGN(F("setup"));

 
  setupScreen();
  showMessage(version);
  setupMics();
  setupButtons();
  resetBpsTimer();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);


  reset();
  LOGN(F("setup done"));
}


void loop() {

  reset();
  for (t = 0; t < SAMPLES; t++)
    record();
  buckets++;
  analyze();

  maybeEnterMenu();
  updateScreen();


  // dumpSerial();
}
