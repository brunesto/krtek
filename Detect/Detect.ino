
int mic1 = A0;    // select the input pin for the potentiometer
int mic2 = A3;    // select the input pin for the potentiometer




void setup() {
  Serial.begin(115200);  
  //pinMode(mic1, INPUT);
  //pinMode(mic2, INPUT);

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

int MIC_REF=440;

int SAMPLES=1000;


byte d1[1000];

int t=0;
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

    int threshold=160;
      if (max1>threshold &&  max2>threshold){
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)


      int dtt=2;
      int dt=t1-t2;
      if (dt>dtt){
        digitalWrite(7, HIGH); 
        digitalWrite(8, LOW);  
      } else if (dt<-dtt){
        digitalWrite(8, HIGH); 
        digitalWrite(7, LOW);  
      } else {
        digitalWrite(8, HIGH); 
        digitalWrite(7, HIGH);  
      }
 
 
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
       digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

        digitalWrite(8, LOW); 
        digitalWrite(7, LOW);  
        
        Serial.print(".");        
      }

      reset();
    
   }

   
    
   int n1=analogRead(mic1)/4;
//   int n=h-MIC_REF;
  // if (n<0) n=-n;

   if (n1>max1){

       
        max1=n1;
        t1=t;
      
    
   }
 //    d1[t]=n1;



delayMicroseconds(5);
//
//   
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
