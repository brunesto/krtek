#include <Arduino.h>
#include "common.h"
#include "buttons.h"

// -- buttons --------------------------------------------------------------------------------


byte BUTTONS_ALL[] = { BUTTON_OK, BUTTON_UP, BUTTON_DOWN, NO_BUTTON };
byte BUTTONS_OK[] = { BUTTON_OK, NO_BUTTON };
byte BUTTONS_UP_DOWN[] = { BUTTON_UP, BUTTON_DOWN, NO_BUTTON };
byte BUTTONS_UP[] = { BUTTON_UP, NO_BUTTON };

void setupButtons() {
  LOGN(F("setupButtons"));
  for (int i = 0; BUTTONS_ALL[i] != NO_BUTTON ; i++) {
    LOG(BUTTONS_ALL[i]);
    LOGN(i);

    pinMode(BUTTONS_ALL[i], INPUT);    // sets the digital pin 6 as input
  }

  //  setupMenuScale();
}

/**
  scan buttons status and return pin of first button pressed, or -1
*/
byte getInput(byte *pins) {
  for (int i = 0; pins[i] != NO_BUTTON ; i++) {
    int v = digitalRead(pins[i]);
    if (v > 0)
      return pins[i];
  }
  return NO_BUTTON ;
}

/**
  same as getInput(), but wait for release
*/
byte getInputReleased(byte *buttons) {
  byte pressed = getInput(buttons);
  if (pressed == NO_BUTTON)
    return NO_BUTTON ;
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
byte expectInput(byte *buttons) {
  LOGN(F("expectInput"));
  while (true) {
    int button = getInputReleased(buttons);
    if (button != NO_BUTTON)
      return button;
    delay(INPUT_DELAY_MS);
  }
  // never returns
  return NO_BUTTON ;
}
