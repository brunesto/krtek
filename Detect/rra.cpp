#include <Arduino.h>

#include "common.h"

#include "rra.h";





// -- mic configuration --------------------------------------------


void setupMics() {
  for (int i = 0; i < MAX_MICS; i++)
    pinMode(MIC_DIGITAL_PIN(i), INPUT);

}

// -- recording --------------------------------------------------

//
// bucket recording information for a given mic
//

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

// record the values from mic1, for debug only
byte d1[SAMPLES_REC];

// -- stats ----------------------------------------
/**
   total number of buckets recorded in the current mode loop (detect, info, oscilo,...)
*/
int buckets;

/**
   total amount of time while recording, in the current mode loop
*/
long recordingAccMs;

// -- recording -------------------------------------
/**
   sampling number inside the bucket
*/
int t = 0;

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


float getBps() {
  return (buckets * 1000.0) / (recordingAccMs);
}


void rra() {
  reset();
  record();
  analyze();
}
