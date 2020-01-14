
// -- mic configuration --------------------------------------------

/**
  maximum number of microphones
*/
#define MAX_MICS 3

/**
  pins of mic in analog mode
*/
#define MIC_ANALOG_PIN(x) (x+14)

/**
  pins of mic in digital mode
*/
#define MIC_DIGITAL_PIN(x) (x+2)

#define LED_PIN(x) (x+10)


// -- recording --------------------------------------------------

//
// bucket recording information for a given mic
//
typedef struct Mic {
  byte pin;
  int vmax;
  int tmax;
  int vmin;
  int tmin;
  int tamax;
  int vamax;
  //  int vat; // values above threshold
  long acc; // acc for values. used to compute avg
} Mic;

extern struct Mic mics[MAX_MICS];

void resetMic(int i);

// -- analysis config -----------------------------------------

extern char *version;

typedef struct AnalysisConfig {

  /**
    are the mic analog or digital
  */
  boolean digital;

  /**
    number of mics beeing used
  */
  int MICS;

  /**
    number of samples per bucket, i.e. per period
  */
  int SAMPLES;

  // volume threshold
  int threshold;

  /**
    max difference in time (in number of recording loops)
    between the peaks. If one mic has a peak which is too far away it is discarded
  */
  int maxdt;

  /**
    recording loop delay in micro seconds
  */
  int loopDelayUs;

} AnalysisConfig;

extern AnalysisConfig config;

/**
  reset the analysis config
*/


#define CONFIG_VERSION 'a'



/**
   recording buffer.
   used in oscilo mode to store reording values
*/

#define SAMPLES_REC 80

// record the values from mic1, for debug only
extern byte d1[];

// -- stats ----------------------------------------
/**
   total number of buckets recorded in the current mode loop (detect, info, oscilo,...)
*/
extern int buckets;

/**
   total amount of time while recording, in the current mode loop
*/
extern long recordingAccMs;

// -- recording -------------------------------------
/**
   sampling number inside the bucket
*/
extern int t;

// -- analysis --------------------------------------

/**
   firstMic is the main analysis result
   if >=0 it indicates the mic which is closest to the noise source
   if <0 it indicates reason for no detection
*/

// index of first mic

#define ALL_UNDER_THRESHOLD -2
#define SOME_UNDER_THRESHOLD -3
#define DTT_TOO_HIGH -4

extern  int firstMic;

// dt to first
extern  int dtf[];


void setupMics();
void setupLeds();

void setupAnalysisConfig(boolean digital);

void resetBpsTimer();

float getBps();
void reset();
void record();
void analyze();
int findLoudestPeakMicV();
void rra();
