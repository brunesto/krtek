/**
 * 
 * Dump the sound signal onto a 12864 lcd screen
 */




// -- screen ----------------------------------------------------------

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
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println("ready...");


  
  display.display();

   
  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
}




// -- mic configuration --------------------------------------------



//int DELAY











void setup() {
  Serial.begin(115200);  


  setupScreen();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  pinMode(6, INPUT);    // sets the digital pin 6 as input
      
  reset();
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


// number of samplings per bucket
#define SAMPLES 1000

#define MICS 2



#define SAMPLES_REC 128
// record the values from mic1, for debug only
byte d1[SAMPLES_REC*MICS];


int threshold=160;

// microphone pins
int micPin[MICS] = {A0,A1};    



// total number of buckets so far
int buckets;


// time inside bucket
int t=0;

// max sampling value
int vmax[MICS];

// timing of max sampling value
int tmax[MICS];


void reset(){
    // reset
    for(int i=0;i<MICS;i++){
      vmax[i]=0;
      tmax[i]=0;      
    }
}

void record(){

  

   //  read the mics values, and maybe update max
    for(int i=0;i<MICS;i++){
    
      int n=analogRead(micPin[i]); // n1 range is [0..1024]
     

      
      
          if (t<SAMPLES_REC){
            d1[t*MICS+i]=n/4; 
          }
      

      
      n=n-512;
      if (n<0)
        n=-n;

      
//   n1=n1/4;
//   n2=n2/4;
   
   //   int n=h-MIC_REF;
   // if (n<0) n=-n;

   if (n>vmax[i]){
        vmax[i]=n;
        tmax[i]=t;
   }
   //    d1[t]=n1;

    } 
  
   //   
   t++;
   
//   delayMicroseconds(5);


}


int leadingMicIdx=-1;


void analyze(){

  // reet analyze flag
  leadingMicIdx=-1;

    // 

   // check if the threshold was reach on both mics,
  int levelOk=0;
  for(int i=0;i<MICS;i++){
    if (vmax[i]<threshold)
      return;
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


void updateScreen(){
   if (buckets%4==-1)
   {
    display.clearDisplay();
    
   display.setCursor(0,0);             // Start at top-left corner
   float bps=(1000.0*buckets)/millis();
    display.print("bps:");
    display.println(bps);


    for(int i=0;i<MICS;i++){
      display.print(vmax[i]);
      display.print("@");
      display.println(tmax[i]);
      
    }
    
  
    
  
    // Show the display buffer on the screen. You MUST call display() after
    // drawing commands to make them visible on screen!
    display.display();
   }
    else
    //if (buckets%4==1)
    {

      display.clearDisplay();
      display.setTextSize(1);

      int h=64/MICS;

      for(int i=0;i<MICS;i++){

      int h0=i*h; 
    
      for(int j=0;j<SAMPLES_REC;j++){
        byte y=d1[j*MICS+i]/4;

        
        
        if (y<0)
          y=0;
        else if (y>63)
          y=63;

        y=y/MICS;
          
        display.drawLine(j,h0,j,h0+(int)y, SSD1306_WHITE);
      }
      }

// display.print(d1[10]);
  
      display.display();
    }

}


void dumpSerial(){

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

        for(int i=0;i<SAMPLES_REC;i++){
          Serial.print(" ");       
          int n=d1[i]-127;
          int an=n;
          if (an<0)
            an=-an;
  
          if (an>30)  
          Serial.print(d1[i]);
          else
          Serial.print("o");
          
          if (i%32==0)
          Serial.println();        
          
          
        }
   

}
void loop(){

   reset();
   for(t=0;t<SAMPLES;t++)
      record();  
   buckets++;
 //  analyze();    
 updateScreen();
// dumpSerial();
}
