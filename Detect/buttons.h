
#define  BUTTON_OK (byte)8
#define  BUTTON_UP (byte)6
#define  BUTTON_DOWN (byte)5
#define NO_BUTTON (byte)-1

#define INPUT_DELAY_MS 100


extern byte BUTTONS_ALL[];
extern byte BUTTONS_OK[];
extern byte BUTTONS_UP_DOWN[];
extern byte BUTTONS_UP[]; 

byte getInputReleased(byte *buttons);
byte expectInput(byte *buttons) ;
void waitForReleased(byte pressed);
void setupButtons();
