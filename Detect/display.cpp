

#include <Arduino.h>
#include "common.h"
#include "buttons.h"
#include "display.h"


#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>



#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT); //, &Wire);//, OLED_RESET,65535);

void setupScreen() {
  LOGN(F("setupScreen"));
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    LOGN(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0, 0);            // Start at top-left corner
  display.display();
}

// screen helpers

void displayReset() {
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0, 0);
}

void displayMessage(class __FlashStringHelper *msg) {
  displayReset();
  display.print(msg);
  display.display();
}
//
//void displayPrintf(const char *format, va_list ap) {
//
//  char buffer[11 * 4];
//  sprintf(buffer, format, ap);
//  display.print(buffer);
//
//  LOG(F("display:"))
//  LOGN(buffer)
//
//}
//
//
//void displayPrintf(const char *format, ...) {
//  va_list vl;
//  va_start(vl, format);
//  displayPrintf(format, vl);
//}
//
//
//void showMessage(const char *format, ...) {
//
//  va_list vl;
//  va_start(vl, format);
//  displayReset();
//  displayPrintf(format, vl);
//  display.display();
//}
//
//void error(char * info) {
//  showMessage(info);
//  expectInput(BUTTONS_ALL);
////}
//
//
//int getStrings( char ** choicesText) {
//  int choice = 0;
//  while (*choicesText[choice])
//    choice++;
//  return choice;
//}

/**
  display the list of choices
*/
void showChoices(int currentChoice, int choices,
                 class __FlashStringHelper **choicesText) {
  LOG(F("currentChoice:"));
  LOGN(currentChoice);

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

int getChoice(int currentChoice, int choices, class __FlashStringHelper **choicesText) {

  while (true) {

    showChoices(currentChoice, choices, choicesText);

    int button = getInputReleased(BUTTONS_ALL);
    switch (button) {
      case BUTTON_OK :

        LOG(F("getChoice:"))
        ;
        LOGN(currentChoice)
        ;
        return currentChoice;
      case BUTTON_UP :
        currentChoice--;
        if (currentChoice < 0)
          currentChoice = 0;
        break;
      case BUTTON_DOWN :
        currentChoice++;
        if (currentChoice >= choices)
          currentChoice = choices - 1;
    }

    delay(INPUT_DELAY_MS);
  }
  // never happens
  return -1;
}

#define MENU_SCALE_S 5
class __FlashStringHelper *MENU_SCALE[MENU_SCALE_S];

void setupMenuScale() {
  MENU_SCALE[0] = F("Back");
  MENU_SCALE[1] = F("+- 1");
  MENU_SCALE[2] = F("+- 10");
  MENU_SCALE[3] = F("+- 100");
  MENU_SCALE[4] = F("+- 1000");

}

/**
  let the user specify a scale (+-1,+-10,+-100) a then specify a value
*/

int chooseValueAndScale(class __FlashStringHelper *name, int value, int min,
                        int max) {

  int choice = getChoice(0, MENU_SCALE_S, MENU_SCALE);
  switch (choice) {
    case 0:
      break;
    case 1:
      return chooseValue(name, value, min, max, 1);
    case 2:
      return chooseValue(name, value, min, max, 10);
    case 3:
      return chooseValue(name, value, min, max, 100);
    case 4:
      return chooseValue(name, value, min, max, 1000);
  }
  return -1;
}

/**
  let the user choose a value
*/
int chooseValue(class __FlashStringHelper *name, int value, int min, int max,
                int steps) {
  LOG(F("chooseValue"));
  LOG(name);
  LOGN(value);

  while (true) {
    displayReset();
    display.print(name);
    display.print(":");
    display.println();
    display.print(min);
    display.print("~");
    display.print(max);
    display.println();
    display.print(value);
    display.display();

    int button = expectInput(BUTTONS_ALL);

    switch (button) {
      case BUTTON_OK :
        return value;

      case BUTTON_DOWN :
        value -= steps;
        if (value < min)
          value = min;
        break;
      case BUTTON_UP :
        value += steps;
        if (value > max)
          value = max;
    }

    LOGN(value);
  }
}
