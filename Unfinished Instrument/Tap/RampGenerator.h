/*** RampGenerator.h ***/
/* RampGenerator.h */

#ifndef RAMPGENERATOR_H_
#define RAMPGENERATOR_H_

//struct RampConfig;

class RampGenerator	{

public:
	RampGenerator();
	~RampGenerator();
	void config(int numReadings, float thresholdToTrigger, float amountBelowPeak, float rolloffRate, float readings[], int index, float avrTotal, float average, int previousSample, float peakValue, int triggered, float line, float amps, int ADSRStates, int sampleCount, int attack, int debounceSampleCount, int debounceThresh, int onsetFinished, int debounce, int startPlaying);
	float processRamp(float in, float thresholdToTrigger,float amountBelowPeak, float rolloffRate, int index);

private:

	int numReadings;
	float thresholdToTrigger;
	float amountBelowPeak;
	float rolloffRate;

	/* Average */
	float readings[50]; // array to store readings from the analog input
	int index; // index of the current reading
	float avrTotal; // running total
	float average; // the average
	/* -------------------- */

	/* Onset Detection */
	int previousSample;   // Value of the sensor the last time around
	float peakValue;
	int triggered;
	int startPlaying;
	/* -------------------- */

	/* Envelope Trigger */
	float line;
	float amps;
	int ADSRStates;
	int sampleCount;
	enum {
		kStateOff = 0,
		kStateAttack
	};
	int attack;
	/* -------------------- */

	/* Debounce */
	int debounceSampleCount; // Sample counter for avoiding double onsets
	int debounceThresh; //1500 Duration in samples of window in which a second onset can't be triggered (equates to a trill of ~22Hz)
	int onsetFinished;
	int debounce;
	/* -------------------- */
	
	/*  DC OFFSET FILTER */
    float prevReadingDCOffset;
    float prevPiezoReading;
    float readingDCOffset;
    float R;
    /* ----------------- */
    
    float piezoAmp;
    
    // DEBUG
    float peakValueDebug;
};

#endif /* RAMPGENERATOR_H_ */