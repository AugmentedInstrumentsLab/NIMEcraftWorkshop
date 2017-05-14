/*** RampGenerator.cpp ***/
/* RampGenerator.cpp */

#include "RampGenerator.h"

extern float gTriggered[];


RampGenerator::RampGenerator()	{
    
    numReadings = 20;
	thresholdToTrigger = 0.005;
	amountBelowPeak = 0.0001;
	rolloffRate = 0.0004;
	
	/* Average */
	index = 0; // index of the current reading
	avrTotal = 0; // running total
	average = 0; // the average
	
	for(int i=0;i<50;i++)
	    readings[i] = 0;
	/* -------------------- */

	/* Onset Detection */
	previousSample = 0;   // Value of the sensor the last time around
    peakValue = 0;
	triggered = 0;
	startPlaying = 0;
	/* -------------------- */

	/* Envelope Trigger */
	line = 0.0;
	sampleCount = 0;
	attack = 20000;
	/* -------------------- */

	/* Debounce */
	debounceSampleCount = 0; // Sample counter for avoiding double onsets
	debounceThresh = 1500; //1500 Duration in samples of window in which a second onset can't be triggered (equates to a trill of ~22Hz)
	onsetFinished = 1;
	debounce = 0;
	/* -------------------- */
	
	// DC OFFSET FILTER
	prevReadingDCOffset = 0;
	prevPiezoReading = 0;
	readingDCOffset = 0;
	R = 1 - (250/44100);
	
	//float piezoAmp;
	
	// DEBUG
	//float peakValueDebug;

}

RampGenerator::~RampGenerator()	{

}

void RampGenerator::config(int numReadings, float thresholdToTrigger, float amountBelowPeak, float rolloffRate, float readings[], int index, float avrTotal, float average, int previousSample, float peakValue, int triggered, float line, float amps, int ADSRStates, int sampleCount, int attack, int debounceSampleCount, int debounceThresh, int onsetFinished, int debounce, int startPlaying)	{
	this->numReadings = numReadings;
	this->thresholdToTrigger = thresholdToTrigger;
	this->amountBelowPeak = amountBelowPeak;
	this->rolloffRate = rolloffRate;
	//this->readings[4] = readings[4];
	this->index = index;
	this->avrTotal = avrTotal;
	this->average = average;
	this->previousSample = previousSample;
	this->peakValue = peakValue;
	this->triggered = triggered;
	this->line = line;
	this->amps = amps;
	this->ADSRStates = ADSRStates;
	this->sampleCount = sampleCount;
	this->attack = attack;
	this->debounceSampleCount = debounceSampleCount;
	this->debounceThresh = debounceThresh;
	this->onsetFinished = onsetFinished;
	this->debounce = debounce;
	this->peakValueDebug = peakValueDebug;
	// ...
}

float RampGenerator::processRamp(float in, float thresholdToTrigger, float amountBelowPeak, float rolloffRate, int index)	{

    float smoothedReading = in;
	
    // ONSET DETECTION
	if(smoothedReading >= peakValue) // Record the highest incoming sample
	{ 
	    peakValue = smoothedReading;
	    triggered = 0;
	    //startPlaying = 0;
  	}
	else if(peakValue >= thresholdToTrigger) // But have the peak value decay over time
		peakValue -= rolloffRate;           // so we can catch the next peak later

	if(smoothedReading < peakValue - amountBelowPeak && peakValue >= thresholdToTrigger && !triggered) 
	{
		triggered = 1; // Indicate that we've triggered and wait for the next peak before triggering again.
		gTriggered[index] = peakValue;
    }
    
    if(peakValue < thresholdToTrigger) {
	    gTriggered[index] = 0;
    }
    
    return peakValue;

}

