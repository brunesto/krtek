
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#include "common.h"
#include "buttons.h"
#include "display.h"

#include "rra.h";
#include "menus.h";

// -- helpers ---------------------------------------------------

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


// -- persisting the config ----------------------------------------------------------

/**
  save the
*/
void saveConfig() {
  EEPROM.write(0, 'K');
  EEPROM.write(1, CONFIG_VERSION);

  EEPROM.put(2, config);
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

void loadConfig() {
  byte k1 = EEPROM.read(0);
  byte k2 = EEPROM.read(1);
  if (k1 == 'K' && k2 == CONFIG_VERSION) {
    EEPROM.get(2, config);
    dumpConfig2Serial();
    displayMessage(F("loaded cfg"));
    delay(500);
  }
}
// -- detect mode -----------------------------------------------------------------



void displayDetectModeMicLine(int i) {
  display.print(mics[i].vamax);
  if (i == firstMic)
    display.print(F("*"));
  else
    display.print(F("@"));

  display.print(mics[i].tamax);
}

void displayFirstMic() {
  if (firstMic >= 0){
    display.print(F("m"));
    display.print(firstMic + 1);
  } else {
    switch (firstMic) {

      case ALL_UNDER_THRESHOLD:
        display.print(F(" - "));
        break;
     case SOME_UNDER_THRESHOLD:
        display.print(F("!-"));
        break;
      case DTT_TOO_HIGH:
        display.print(F("DT!"));
        break;
      default:
        display.print(firstMic);

    }

  }
}
void updateScreenDetectMode() {


  display.print(config.digital ? F("D") : F("A"));
  displayFirstMic();
  display.println();

  for (int i = 0; i < config.MICS; i++) {

    displayDetectModeMicLine(i);

    if (i != firstMic) {
      display.print(" ");
      display.print(dtf[i]);
    }
    display.println();

  }




}


// -- info mode -----------------------------------------------------------------


#define OSCILO_LOOP 0
#define OSCILO_TRIGGER 1
#define  OSCILO_BLOCKED 2
int osciloModeMode = OSCILO_LOOP;


/**
  InfoMode can display several pages of information
*/
#define INFO_MODE_MIN_PAGE -3

int infoModeY = INFO_MODE_MIN_PAGE;


boolean updateScreenInfoMode() {
  while (true) {
    LOG(F("updateScreenInfoMode "))
    //if (buckets % 4 == 0)
    int v = getInputReleased(BUTTONS_ALL);
    if (v == BUTTON_DOWN) {
      infoModeY++;
      if (infoModeY >= 2 * config.MICS)
        infoModeY = INFO_MODE_MIN_PAGE;

    } else if (v == BUTTON_UP && osciloModeMode != OSCILO_BLOCKED) {
      osciloModeMode++;
      if (osciloModeMode > OSCILO_TRIGGER)
        osciloModeMode = OSCILO_LOOP;
    } else if (v == BUTTON_OK) {

      if (osciloModeMode == OSCILO_BLOCKED) {
        osciloModeMode = OSCILO_LOOP;

      } else {
        return true; // exit
      }

    }


    int mic = getInputReleased(BUTTONS_UP_DOWN);

    displayReset();

    display.setTextSize(1);
    display.setCursor(100, 0);
    display.print(osciloModeMode == OSCILO_LOOP ? F("") : (osciloModeMode == OSCILO_TRIGGER ? F("T") : F("B")));


    display.setTextSize(2);
    display.setCursor(0, 0);

    display.print(F("p"));
    display.print((1 + infoModeY - INFO_MODE_MIN_PAGE));
    display.print(F("/"));
    display.print((-INFO_MODE_MIN_PAGE) + 2 * config.MICS);

    if (infoModeY < 0) {

      if (infoModeY == INFO_MODE_MIN_PAGE) {
        updateScreenDetectMode();


      } else if (infoModeY == INFO_MODE_MIN_PAGE + 1) {
        display.println();
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

      } else if (infoModeY == INFO_MODE_MIN_PAGE + 2) {
        display.println();
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


    if (firstMic != ALL_UNDER_THRESHOLD && osciloModeMode == OSCILO_TRIGGER) {
      osciloModeMode = OSCILO_BLOCKED;
    } else {
      display.display();
    }

    if (osciloModeMode != OSCILO_BLOCKED)
      return false;
  }

}

void loopInfoMode() {
  resetBpsTimer();
  while (true) {
    rra();
    if (updateScreenInfoMode())
      return;
  }
}
// -- oscilomode mode -----------------------------------------------------------------

/*
  in OSCILO_TRIGGER, if a firstMic is detected, the screen will freeze
*/


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

void loopOsciloMode() {
  resetBpsTimer();
  while (true) {
    rra();
    updateScreenOsciloMode();
    if (maybeExit())
      return;
  }
}

// -- actual menus -----------------------------------------

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
int charsDec(int dec) {
  int shift = 0;
  if (dec >= 10000)
    shift += 4;
  if (dec >= 1000)
    shift += 3;
  else if (dec >= 100)
    shift += 2;
  else if (dec >= 10)
    shift += 1;
  return shift;
}

/**
  help the user to locate the center by simply displaying firstMic counts
  UP: reset counters
  OK: exit
*/
void displayHits() {
  long startTime = millis();
  int maxv = 0;

  int cnt = 0;
  int hits[MAX_MICS];

  for (int i = 0; i < config.MICS; i++)
    hits[i] = 1;

  while (true) {
    rra();
    if (firstMic >= 0) {
      cnt++;
      hits[firstMic]++;
    }
    displayReset();


    display.setCursor(0, 32);
    display.print((char)0x11);
    display.setCursor(0, 48);
    display.println(hits[0]);
    if (2 <= config.MICS) {

      display.setCursor(110, 32);
      display.print((char)0x10);

      int x = 9 - charsDec(hits[1]);
      display.setCursor(x * 12, 48);
      display.println(hits[1]);
    } if (3 <= config.MICS) {

      display.setCursor(40, 0);
      display.print((char)0x1e);

      display.setCursor(30, 16);
      display.println(hits[2]);
    }
    display.setCursor(12*4, 32);
    displayFirstMic();

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

//
///**
//  help the user to locate the center by simply displaying firstMic counts
//  UP: reset counters
//  OK: exit
//*/
//void detectCenter() {
//  long startTime = millis();
//  int maxv = 0;
//
//  int cnt = 0;
//  int hits[MAX_MICS];
//
//  for (int i = 0; i < config.MICS; i++)
//    hits[i] = 0;
//
//  while (true) {
//    rra();
//    if (firstMic >= 0) {
//      cnt++;
//      hits[firstMic]++;
//    }
//    displayReset();
//    for (int i = 0; i < config.MICS; i++)
//      display.println(hits[i]);
//
//    display.display();
//
//    byte button = getInputReleased(BUTTONS_ALL);
//    if (button == BUTTON_OK) {
//      return;
//    } else if (button == BUTTON_UP) {
//      cnt++;
//      for (int i = 0; i < config.MICS; i++)
//        hits[i] = 0;
//    }
//
//  }
//}
//
//#define MENU_WIZARD_S 3
//class __FlashStringHelper *MENU_WIZARD[MENU_WIZARD_S];
//
//void setupMenuWizard() {
//  MENU_WIZARD[0] = F("Back");
//  MENU_WIZARD[1] = F("Silence");
//  MENU_WIZARD[2] = F("Center");
//}
//
//void menuWizard() {
//  int choice = 0;
//  while (true) {
//    choice = getChoice(choice, MENU_WIZARD_S, MENU_WIZARD);
//    switch (choice) {
//      case 0:
//        return;
//      case 1:
//        detectSilence();
//        choice = 0; // prevent double clicking on button to launch the wizard again
//        break;
//      case 2:
//        detectCenter();
//        choice = 0; // prevent double clicking on button to launch the wizard again
//        break;
//
//    }
//  }
//}

#define MENU_MAIN_S 7
class __FlashStringHelper *MENU_MAIN[MENU_MAIN_S];

void setupMenuMain() {
  MENU_MAIN[0] = F("Detect");
  MENU_MAIN[1] = F("Info");
  MENU_MAIN[2] = F("Silence");
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
        displayHits();
        break;
      case 1:
        loopInfoMode();
        break;
      case 2:
        detectSilence();
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


void setupMenus() {
  setupMenuMain();
  //  setupMenuWizard();
  setupMenuConfig();
  setupMenuConfig2();
}
