/**

 reads signal from 2 mics, and switch on leds to indicate which mic is closer to the source of the noise.
 Works by identifying the timing of peak signal over 1000 samples


 A0...A2 analog mics
 D2...D4 noise detectors

 Important:
 -the arduino must run on batteries for the mics to work properly
 -the ADC is quickly affected by low batteries

 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#include "common.h"
#include "buttons.h"
#include "display.h"


void detectCenter();
void detectSilence();
void dumpConfig2Serial();
boolean maybeExit();
void menuWizard();
void menuConfig();
void menuConfig2();
void updateScreenOsciloMode();
void updateScreenInfoMode();
void updateScreenDetectMode();
void rra();
void waitForReleased(byte p);

//--------------------------------------------------------

// -- mic configuration --------------------------------------------

/**
 maximum number of microphones
 */
#define MAX_MICS 3

/**
 pins of mic in analog mode
 */
#define MIC_ANALOG_PIN(x) (x+14)

/**
 pins of mic in digital mode
 */
#define MIC_DIGITAL_PIN(x) (x+2)

void setupMics() {
  for (int i = 0; i < MAX_MICS; i++)
    pinMode(MIC_DIGITAL_PIN(i), INPUT);

}

// -- recording --------------------------------------------------

//
// bucket recording information for a given mic
//
typedef struct Mic {
  byte pin;
  int vmax;
  int tmax;
  int vmin;
  int tmin;
  int tamax;
  int vamax;
//  int vat; // values above threshold
  long acc; // acc for values. used to compute avg
} Mic;

struct Mic mics[MAX_MICS];

void resetMic(int i) {
  mics[i].vmax = 0;
  mics[i].vmin = 1024;
  mics[i].vamax = 0;
  mics[i].tmax = -1;
  mics[i].tmin = -1;
  mics[i].tamax = 0;
//  mics[i].vat = 0;
  mics[i].acc = 0;
}

// -- analysis config -----------------------------------------

char *version = "v0.01";

typedef struct AnalysisConfig {

  /**
   are the mic analog or digital
   */
  boolean digital;

  /**
   number of mics beeing used
   */
  int MICS;

  /**
   number of samples per bucket, i.e. per period
   */
  int SAMPLES;

  // volume threshold
  int threshold;

  /**
   max difference in time (in number of recording loops)
   between the peaks. If one mic has a peak which is too far away it is discarded
   */
  int maxdt;

  /**
   recording loop delay in micro seconds
   */
  int loopDelayUs;

} AnalysisConfig;

AnalysisConfig config;

/**
 reset the analysis config
 */
void setupAnalysisConfig(boolean digital) {
  if (!digital) {
    config.digital = false;
    config.MICS = 2;
    config.SAMPLES = 1000;
    config.threshold = 65;
    config.maxdt = 10;
    config.loopDelayUs = 20;

  } else {
    config.digital = true;
    config.MICS = 2;
    config.SAMPLES = 10000;
    config.threshold = 100;
    config.maxdt = 500;
    config.loopDelayUs = 200;
  };
}

#define CONFIG_VERSION 'a'

/**
 save the
 */
void saveConfig() {
  EEPROM.write(0, 'K');
  EEPROM.write(1, CONFIG_VERSION);

  EEPROM.put(2, config);
}

void loadConfig() {
  byte k1 = EEPROM.read(0);
  byte k2 = EEPROM.read(1);
  if (k1 == 'K' && k2 == CONFIG_VERSION) {
    EEPROM.get(1, config);
    dumpConfig2Serial();
  }
}

void dumpConfig2Serial() {
  Serial.print(F("digital:"));
  Serial.println(config.digital);

  Serial.print(F("MICS:"));
  Serial.println(config.MICS);

  Serial.print(F("SAMPLES:"));
  Serial.println(config.SAMPLES);

  Serial.print(F("threshold:"));
  Serial.println(config.threshold);
  Serial.print(F("maxdt:"));
  Serial.println(config.maxdt);

  Serial.print(F("loopDelayUs:"));
  Serial.println(config.loopDelayUs);

}

/**
 * recording buffer.
 * used in oscilo mode to store reording values
 */

#define SAMPLES_REC 80

// record the values from mic1, for debug only
byte d1[SAMPLES_REC];

// -- stats ----------------------------------------
/**
 * total number of buckets recorded in the current mode loop (detect, info, oscilo,...)
 */
int buckets;

/**
 * total amount of time while recording, in the current mode loop
 */
long recordingAccMs;

// -- recording -------------------------------------
/**
 * sampling number inside the bucket
 */
int t = 0;

// -- analysis --------------------------------------

/**
 * firstMic is the main analysis result
 * if >=0 it indicates the mic which is closest to the noise source
 * if <0 it indicates reason for no detection
 */

// index of first mic
#define UNSET -1
#define UNDER_THRESHOLD -2
#define DTT_TOO_HIGH -3

int firstMic = UNSET;

// dt to first
int dtf[MAX_MICS];

void reset() {
  // reset
  for (int i = 0; i < config.MICS; i++)
    resetMic(i);
}

//
//
//#define MODE_DETECT 0
//#define MODE_OSCILO 1
//#define MODE_INFO 2
//
//int mode = MODE_DETECT;

// -- recording -------------------------------------

void recordMic(int i, int n) {

  if (t * config.MICS < SAMPLES_REC) {
    d1[t * config.MICS + i] = n / 4;
  }

  mics[i].acc += (long) n;

  // record min and max
  if (n > mics[i].vmax) {
    mics[i].vmax = n;
    mics[i].tmax = t;
  }
  if (n < mics[i].vmin) {
    mics[i].vmin = n;
    mics[i].tmin = t;
  }

//    // update values above threshold?
//    if (n>=config.threshold){
//      mics[i].vat++;
//    }
//    if (n>=config.threshold){
//      mics[i].vat++;
//    }

}
void recordMics() {

  //  read the mics values, and maybe update max
  for (int i = 0; i < config.MICS; i++) {

    int n;
    if (config.digital)
      n = digitalRead(MIC_DIGITAL_PIN(i)) == 0 ? 1024 : 0;
    else
      n = analogRead(MIC_ANALOG_PIN(i)); // 14=A0 n range is [0..1024]

    recordMic(i, n);
  }

}
void record() {
  long start = millis();
  delayMicroseconds(config.loopDelayUs);

  for (t = 0; t < config.SAMPLES; t++)
    recordMics();
  long end = millis();
  recordingAccMs += end - start;
  buckets++;

}

int findLoudestPeakMicI() {
  int maxi = -1;
  int maxv = -1;
  for (int i = 0; i < config.MICS; i++) {
    if (mics[i].vamax > maxv) {
      maxi = i;
      maxv = mics[i].vamax;
    }
  }
  return maxi;
}
/*
 find the loudest value recorded in last bucket
 */
int findLoudestPeakMicV() {
  int maxi = findLoudestPeakMicI();
  return mics[maxi].vamax;
}

void analyze() {
  LOGN(F("analyze"));
  // 0) reset analysis flags
  firstMic = UNSET;
  for (int i = 0; i < config.MICS; i++) {
    dtf[i] = 0;
  }

  for (int i = 0; i < config.MICS; i++) {

    // compute avg
    int avg = (mics[i].acc / (long) config.SAMPLES);
    LOG(F("mic:"));
    LOGN(i);
    LOG(F("acc:"));
    LOG(mics[i].acc);
    LOG(F("avg:"));
    LOGN(avg);

    // get absolute max
    int amaxFromVmax = mics[i].vmax - avg;
    int amaxFromVmin = avg - mics[i].vmin;
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
    LOG(mics[i].vamax);
    LOG(F("tamax:"));
    LOGN(mics[i].tamax);

  }

  // 1.1) find loudest mic
  //  int maxv = findLoudestPeakMicV();

  //  // is loudest signal too weak?
  //  if (maxv < config.threshold) {
  //    firstMic = UNDER_THRESHOLD;
  //    return;
  //  }

//TODO: threshold should vahe 2 values: min sound, min trigger noise
  for (int i = 0; i < config.MICS; i++) {
    if (mics[i].vamax < config.threshold) {
      firstMic = UNDER_THRESHOLD;
      return;
    }

  }

  // 2) identify mic that picked up signal first
  int firstMicT = config.SAMPLES;
  for (int i = 0; i < config.MICS; i++) {
    if (mics[i].tamax < firstMicT) {
      firstMicT = mics[i].tamax;
      firstMic = i;
    }
  }

  // 3.a) fill up Difference in Time to First
  for (int i = 0; i < config.MICS; i++) {
    int dt = mics[i].tamax - firstMicT;
    dtf[i] = dt;
  }

  // 3.b) if any mic has its peak which is too far away in time, cancel the whole thing
  for (int i = 0; i < config.MICS; i++) {
    if (dtf[i] > config.maxdt) {
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
  recordingAccMs = 0;
  buckets = 0;
}

void loopDetectMode() {
  resetBpsTimer();
  while (true) {
    rra();
    updateScreenDetectMode();
    if (maybeExit())
      return;
  }
}

void displayDetectModeMicLine(int i) {
  display.print(mics[i].vamax);
  if (i == firstMic)
    display.print(F("*"));
  else
    display.print(F("@"));

  display.print(mics[i].tamax);
}

float getBps() {
  return (buckets * 1000.0) / (recordingAccMs);
}

void updateScreenDetectMode() {

  // if (buckets % 4 == 0) nonsense
  {
    displayReset();
    display.print(config.digital ? F("D") : F("A"));
    display.print(getBps());
    display.print(" ");
    if (firstMic >= 0)
      display.print(firstMic + 1);
    else {
      switch (firstMic) {

      case UNDER_THRESHOLD:
        display.print(F("LOW"));
        break;
      case DTT_TOO_HIGH:
        display.print(F("DT!"));
        break;
      default:
        display.print(firstMic);

      }

    }
    display.print(" ");

    display.println();

    for (int i = 0; i < config.MICS; i++) {

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

/**
 InfoMode can display several pages of information
 */
#define INFO_MODE_MIN_PAGE -2

int infoModeY = INFO_MODE_MIN_PAGE;

void loopInfoMode() {
  resetBpsTimer();
  while (true) {
    rra();
    updateScreenInfoMode();
    if (maybeExit())
      return;
  }
}

void updateScreenInfoMode() {
  LOG(F("updateScreenInfoMode "))
  //if (buckets % 4 == 0)
  int v = getInputReleased(BUTTONS_UP_DOWN);
  if (v == BUTTON_DOWN) {
    infoModeY++;
    if (infoModeY >= 2 * config.MICS)
      infoModeY = 2 * config.MICS - 1;

  } else if (v == BUTTON_UP) {
    infoModeY--;
    if (infoModeY < INFO_MODE_MIN_PAGE)
      infoModeY = INFO_MODE_MIN_PAGE;

  }

  int mic = getInputReleased(BUTTONS_UP_DOWN);

  displayReset();

  display.print(F("p"));
  display.print((1 + infoModeY - INFO_MODE_MIN_PAGE));
  display.print(F("/"));
  display.print((-INFO_MODE_MIN_PAGE) + 2 * config.MICS);

  if (infoModeY < 0) {

    display.println();

    if (infoModeY == INFO_MODE_MIN_PAGE) {

      // recording time
      display.print(F("rt:"));
      display.print(recordingAccMs / 1000.0);
      display.println();

      // buckets
      display.print(F("b:"));
      display.print(buckets);
      display.println();

      // buckets per second
      display.print(F("b/s:"));
      display.print(getBps());

    } else if (infoModeY == INFO_MODE_MIN_PAGE + 1) {
      //      display.print(F("s:"));
      //      display.print(recordingAccMs/1000.0);
      //      display.println();

      // values sampled
      long valuesSampled = buckets * (long) config.SAMPLES;
      display.print(F("vs:"));
      display.print(valuesSampled);
      display.println();

      // values per second
      long vpms = (((long) 1000) * valuesSampled) / recordingAccMs;
      display.print(F("v/s:"));
      display.print(vpms);

    }
  } else {
    //  mic pages

    int infoModeMic = infoModeY / 2;
    int infoModePage = infoModeY % 2;

    LOG(F("infoModeY:"));
    LOG(infoModeY);
    LOG(F("infoModeMic:"));
    LOG(infoModeMic);
    LOG(F("infoModePage:"));
    LOG(infoModePage);

    // display the mic and finish header
    display.print(F(" m"));
    display.print(infoModeMic + 1);
    //    display.print(F("/"));
    //    display.print(config.MICS );
    //    display.print(F("p"));
    //    display.print( (infoModePage) + 1);
    display.println();

    // CAPTCHA switch block not working ???
    if (infoModePage == 0) {
      //  switch( infoModePage) {  case 0:
      displayDetectModeMicLine(infoModeMic);
      display.println();

      // min and max values in last bucket
      display.print(F("r:"));
      display.print(mics[infoModeMic].vmin);
      display.print(F("-"));
      display.print(mics[infoModeMic].vmax);
      display.println();

      // average
      display.print(F("a"));
      int avg = mics[infoModeMic].acc / config.SAMPLES;
      display.print(avg);

      // difference between min and max
      display.print(F("d:"));
      int d = mics[infoModeMic].vmax - mics[infoModeMic].vmin;
      display.print(d);
      display.println();

      //      break;
      //      case 1:
    } else if (infoModePage == 1) {

      // nothing yet
      display.print(F("Raw:"));
      int r;
      if (config.digital)
        r = digitalRead(MIC_DIGITAL_PIN(infoModeMic));
      else
        r = analogRead(MIC_ANALOG_PIN(infoModeMic));
      display.print(r);
      display.println();
      //      break;
    }
  }

  display.display();

}

void loopOsciloMode() {
  resetBpsTimer();
  while (true) {
    rra();
    updateScreenOsciloMode();
    if (maybeExit())
      return;
  }
}

/*
 in OSCILO_TRIGGER, if a firstMic is detected, the screen will freeze
 */

#define OSCILO_LOOP 0
#define OSCILO_TRIGGER 1
int osciloModeMode = OSCILO_LOOP;

void updateScreenOsciloMode() {

  int v = getInputReleased(BUTTONS_UP);
  if (v == BUTTON_UP) {
    osciloModeMode++;
    if (osciloModeMode > OSCILO_TRIGGER)
      osciloModeMode = OSCILO_LOOP;
  }

  if (SAMPLES_REC > 0) {
    int h = 64 / config.MICS;
    displayReset();
    display.setTextSize(1);
    display.setCursor(100, 0);
    display.print(osciloModeMode == OSCILO_LOOP ? F("") : F("T"));
    for (int i = 0; i < config.MICS; i++) {

      int h0 = i * h;

      for (int j = 0; j < SAMPLES_REC / config.MICS; j++) {
        byte y = d1[j * config.MICS + i] / 4;

        if (y < 0)
          y = 0;
        else if (y > 63)
          y = 63;

        y = y / config.MICS;

        display.drawLine(j, h0, j, h0 + (int) y, SSD1306_WHITE);
      }
    }

    // display.print(d1[10]);

    if (firstMic >= 0 && osciloModeMode == OSCILO_TRIGGER) {
      display.print("m:");
      display.print(firstMic + 1);
      display.display();
      expectInput(BUTTONS_ALL);
      osciloModeMode = OSCILO_LOOP;
    } else {
      display.display();
    }
  }

}
//void updateScreen() {
//  LOG(F("updateScreen mode:"));
//  LOG(mode);
//  LOG(F("buckets:"));
//  LOGN(buckets);
//  switch (mode) {
//    case MODE_INFO:
//      updateScreenInfoMode();
//      break;
//    case MODE_DETECT:
//      updateScreenDetectMode();
//      break;
//    case MODE_OSCILO:
//      updateScreenOsciloMode();
//      break;
//  }
//}
//
//
//void dumpSerial() {
//
//  //        // dump some debug info on serial
//  //        Serial.print("t1:");
//  //        Serial.print(t1);
//  //        Serial.print("max1:");
//  //        Serial.println(max1);
//  //
//  //
//  //        Serial.print("t2:");
//  //        Serial.print(t2);
//  //        Serial.print("max2:");
//  //        Serial.println(max2);
//
//  for (int i = 0; i < SAMPLES_REC; i++) {
//    Serial.print(" ");
//    int n = d1[i] - 127;
//    int an = n;
//    if (an < 0)
//      an = -an;
//
//    if (an > 30)
//      Serial.print(d1[i]);
//    else
//      Serial.print("o");
//
//    if (i % 32 == 0)
//      Serial.println();
//
//
//  }
//
//
//}

// -- actual menus -----------------------------------------

#define MENU_MAIN_S 7
class __FlashStringHelper *MENU_MAIN[MENU_MAIN_S];

void setupMenuMain() {
  MENU_MAIN[0] = F("Detect");
  MENU_MAIN[1] = F("Info");
  MENU_MAIN[2] = F("Auto");
  MENU_MAIN[3] = F("Config");
  MENU_MAIN[4] = F("Oscilo");
  MENU_MAIN[6] = F("About");
  MENU_MAIN[5] = F("Metacfg");

}
/**
 enter the main menu
 This method will only return after the user exits config

 */

// keep the main menu as a variable so that when we come back to the menu we are in the same spot
int menuMainChoice = 0;

void menuMain() {
  while (true) {
    menuMainChoice = getChoice(menuMainChoice, MENU_MAIN_S, MENU_MAIN);
    switch (menuMainChoice) {
    case 0:
      loopDetectMode();
      break;
    case 1:
      loopInfoMode();
      break;
    case 2:
      menuWizard();
      break;
    case 3:
      menuConfig();
      break;
    case 4:
      loopOsciloMode();
      break;
    case 5:
      menuConfig2();
      break;
    case 6:
      displayReset();
      display.println(version);
      display.display();
      expectInput(BUTTONS_ALL);
      break;
    }
  }
}

#define MENU_CONFIG_S 7
class __FlashStringHelper *MENU_CONFIG[MENU_CONFIG_S];

void setupMenuConfig() {
  MENU_CONFIG[0] = F("Back");

  MENU_CONFIG[1] = F("loop us");
  MENU_CONFIG[2] = F("mics");
  MENU_CONFIG[3] = F("maxdt");
  MENU_CONFIG[4] = F("minv");
  MENU_CONFIG[5] = F("samples");
  MENU_CONFIG[6] = F("Digital");

}

void menuConfig() {
  int choice = 0;
  while (true) {
    choice = getChoice(choice, MENU_CONFIG_S, MENU_CONFIG);
    switch (choice) {
    case 0:
      return;

    case 1:
      config.loopDelayUs = chooseValueAndScale(F("loop us"),
          config.loopDelayUs, 0, 2000);
      break;
    case 2:
      config.MICS = chooseValue(F("mics"), config.MICS, 1, MAX_MICS, 1);
      break;

    case 3:
      config.maxdt = chooseValueAndScale(F("maxdt"), config.maxdt, 1,
          5000);
      break;
    case 4:
      config.threshold = chooseValueAndScale(F("minv"), config.threshold,
          10, 505);
      break;
    case 5:
      config.SAMPLES = chooseValueAndScale(F("samples"), config.SAMPLES,
          10, 10000);
      break;
    case 6:
      config.digital = chooseValue(F("digital"), config.digital ? 1 : 0,
          0, 1, 1) != 0;
      break;
    case 7:

      if (chooseValue(F("reset 2 analogic"), 0, 0, 1, 1))
        setupAnalysisConfig(false);
      break;
    case 8:

      if (chooseValue(F("reset 2 digital"), 0, 0, 1, 1))
        setupAnalysisConfig(true);
      break;

    }
  }
}

#define MENU_CONFIG2_S 5
class __FlashStringHelper *MENU_CONFIG2[MENU_CONFIG2_S];

void setupMenuConfig2() {
  MENU_CONFIG2[0] = F("Back");
  MENU_CONFIG2[1] = F("save");
  MENU_CONFIG2[2] = F("load");
  MENU_CONFIG2[3] = F("RST Ana.");
  MENU_CONFIG2[4] = F("RST Dgt.");

}

void menuConfig2() {
  int choice = 0;
  while (true) {
    choice = getChoice(choice, MENU_CONFIG2_S, MENU_CONFIG2);
    switch (choice) {
    case 0:
      return;

    case 1:
      saveConfig();
      break;
    case 2:
      loadConfig();
      break;
    case 3:

      if (chooseValue(F("reset 2 analogic"), 0, 0, 1, 1))
        setupAnalysisConfig(false);
      break;
    case 4:

      if (chooseValue(F("reset 2 digital"), 0, 0, 1, 1))
        setupAnalysisConfig(true);
      break;

    }
  }
}

#define MENU_WIZARD_S 3
class __FlashStringHelper *MENU_WIZARD[MENU_WIZARD_S];

void setupMenuWizard() {
  MENU_WIZARD[0] = F("Back");
  MENU_WIZARD[1] = F("Silence");
  MENU_WIZARD[2] = F("Center");
}

void menuWizard() {
  int choice = 0;
  while (true) {
    choice = getChoice(choice, MENU_WIZARD_S, MENU_WIZARD);
    switch (choice) {
    case 0:
      return;
    case 1:
      detectSilence();
      choice = 0; // prevent double clicking on button to launch the wizard again
      break;
    case 2:
      detectCenter();
      choice = 0; // prevent double clicking on button to launch the wizard again
      break;

    }
  }
}

void rra() {
  reset();
  record();
  analyze();
}

int detectLoudest() {
  long startTime = millis();
  int maxv = 0;
  while (millis() - startTime < 1000 * 5) {
    rra();

    int v = findLoudestPeakMicV();
    if (v > maxv)
      maxv = v;

  }
  LOG(F("maxv:"));
  LOGN(maxv);
  return maxv;
}

/**
 detect the loudest signal for 5 seconds of silence,
 then detect the loudest signal for 5 seconds with user making minimal noise to be picked up
 the middle value is then set as new threshold
 */
void detectSilence() {
  displayMessage(F("wait..."));
  delay(1000);
  displayMessage(F("5s quiet"));
  int silence = detectLoudest();
  displayMessage(F("wait..."));
  delay(1000);
  displayMessage(F("5s min noise"));
  int noise = detectLoudest();

  int middle = (silence + noise) / 2;
  if (middle > 505)
    middle = 505;

  config.threshold = middle;

  LOG(F("threshold:"));
  LOGN(config.threshold);
  displayReset();
  display.print("s:");
  display.print(silence);
  display.print("n:");
  display.print(noise);
  display.println();
  display.print("minv:");
  display.print(middle);

  display.display();
  expectInput(BUTTONS_ALL);

}

/**
 help the user to locate the center by simply displaying firstMic counts
 UP: reset counters
 OK: exit
 */
void detectCenter() {
  long startTime = millis();
  int maxv = 0;

  int cnt = 0;
  int hits[MAX_MICS];

  for (int i = 0; i < config.MICS; i++)
    hits[i] = 0;

  while (true) {
    rra();
    if (firstMic >= 0) {
      cnt++;
      hits[firstMic]++;
    }
    displayReset();
    for (int i = 0; i < config.MICS; i++)
      display.println(hits[i]);

    display.display();

    byte button = getInputReleased(BUTTONS_ALL);
    if (button == BUTTON_OK) {
      return;
    } else if (button == BUTTON_UP) {
      cnt++;
      for (int i = 0; i < config.MICS; i++)
        hits[i] = 0;
    }

  }
}

#define CHECK_BUTTON_EVERY 4

/**
 returns true if the user pressed and release the Ok button
 */
boolean maybeExit() {

  if (buckets % CHECK_BUTTON_EVERY == 0) {

    int v = getInputReleased(BUTTONS_OK);

    //    int v = digitalRead(BUTTONS_PINS[BUTTON_OK]);
    if (v != NO_BUTTON) {
      LOG(F("input released:"));
      LOGN(v);
      //      waitForReleased(BUTTONS_PINS[BUTTON_OK]);

      return true;

    }
    //           mode++;
    //           if (mode>MODE_CONFIG)
    //              mode=MODE_DETECT;
    //       }

  }
  return false;
}

void setupMenus() {
  setupMenuMain();
  setupMenuWizard();
  setupMenuConfig();
  setupMenuConfig2();
}
// --

void setup() {

  Serial.begin(115200);
  LOGN(F("setup"));

  setupScreen();
  displayMessage(F("start..."));
  setupMenus();
  setupMics();
  setupButtons();
  setupMenuScale();
  setupAnalysisConfig(false);
  resetBpsTimer();

  loadConfig();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  reset();
  LOGN(F("setup done"));
}

void loop() {
  loopDetectMode();
  menuMain();

  displayMessage(F("error exit loop"));
  expectInput(BUTTONS_OK);
  //rra();
  //maybeEnterMenu();
  //updateScreen();

  // dumpSerial();
}
