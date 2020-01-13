extern class Adafruit_SSD1306 display;

int chooseValue(class __FlashStringHelper *name, int value, int min, int max, int steps);
int chooseValueAndScale(class __FlashStringHelper *name, int value, int min, int max);
int getChoice(int currentChoice, int choices, class __FlashStringHelper **choicesText);
int chooseValue(class __FlashStringHelper *name, int value, int min, int max, int steps);
void displayMessage(class __FlashStringHelper *msg);
void displayReset();
void setupMenuScale();
void setupScreen();
    
