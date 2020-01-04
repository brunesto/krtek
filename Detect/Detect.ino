/**
 * 
 * reads signal from 2 mics, and switch on leds to indicate which mic is closer to the source of the noise.
 * Works by identifying the timing of peak signal over 1000 samples 
 * 
 * Important: the arduino must run on batteries for the mics to work properly
 *
 */

// microphone pins
int mic1 = A0;    
int mic2 = A3;    


void setup() {
  Serial.begin(115200);  

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
      
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
int SAMPLES=1000;


// record the values from mic1, for debug only
byte d1[1000];

// time inside bucket
int t=0;

// max sampling value and time for mic 1 and 2
int max1;
int t1;
int max2;
int t2;


void reset(){
    // reset
      t=0;
      max1=0;
      t1=-1;
      max2=0;
      t2=-1;
      
            
}

void loop(){

   if (t>SAMPLES){


      // End of the sample, so check if the threshold was reach on both mics,
      
      //
      int threshold=160;
      if (max1>threshold &&  max2>threshold){
      
        
        // some noise occured


        // turn the default LED on
        digitalWrite(LED_BUILTIN, HIGH);   


        // dtt is threshold for time difference
        int dtt=2;

        // difference in peak times
        int dt=t1-t2;

       
        if (dt>dtt){
          // peak signal was recorded earlier on mic2
          digitalWrite(7, HIGH); 
          digitalWrite(8, LOW);  
        } else if (dt<-dtt){
          // peak signal was recorded earlier on mic1
          digitalWrite(8, HIGH); 
          digitalWrite(7, LOW);  
        } else {
          // unclear
          digitalWrite(8, HIGH); 
          digitalWrite(7, HIGH);  
        }
 

        // dump some debug info on serial
        Serial.print("t1:");
        Serial.print(t1);
        Serial.print("max1:");
        Serial.println(max1);


        Serial.print("t2:");
        Serial.print(t2);
        Serial.print("max2:");
        Serial.println(max2);

        for(int i=0;i<SAMPLES;i++){
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
      } else {

        // not loud noise hear, so switch off all leds   
        
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

        digitalWrite(8, LOW); 
        digitalWrite(7, LOW);  
        
        Serial.print(".");        
      }

      reset();
    
   }



   //  read the mics values, and maybe update max
   
    
   int n1=analogRead(mic1)/4; // n1 range is [0..256]
   //   int n=h-MIC_REF;
   // if (n<0) n=-n;

   if (n1>max1){

       
        max1=n1;
        t1=t;
      
    
   }
   //    d1[t]=n1;

   delayMicroseconds(5);

   int n2=analogRead(mic2)/4;
   //   int n=h-MIC_REF;
   // if (n<0) n=-n;

   if (n2>max2){
        max2=n2;
        t2=t;
      
   }
   //   
   t++;
   delayMicroseconds(5);
}
