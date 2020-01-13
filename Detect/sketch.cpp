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

#include "common.h"
#include "buttons.h"
#include "display.h"

#include "rra.h";
#include "menus.h";

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

}
