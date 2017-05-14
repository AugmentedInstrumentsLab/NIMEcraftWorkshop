/*
		RTDSP FINAL PROJECT - karplus-strong implementation with thumb piano interface
		Jacob Harrison

		- electret microphone sensors
		- f0 values derived from MIDI-note numbers
		- tuning using all-pass filter [1]
		- two voices
		- state machine for 
		- change of global pitch using potentiometer

		[1] Extensions of the Karplus-Strong Plucked-String Algorithm, David A. Jaffe and Julius O. Smith

*/
#include <Bela.h>
#include <math.h>	// need this for floor()
#include <Scope.h>
#include "00_PreProcess.h"

Scope scope;

// 2 seconds playback buffer
#define PLAYBACK_BUFFER_SIZE 88200

// two strings/voices
#define NUM_STRINGS 4

// analog input pins
int gMicPin[NUM_STRINGS] = {0,1,2,3};
int gPotPin = 4;

// analog input values
float gMicVal[NUM_STRINGS] = {0};
float gPotVal = 0;

// DC blocking (Mic & hall sensor)
float gMicConditioned[NUM_STRINGS] = {0};
float gMicPostProcessed[NUM_STRINGS] = {0};
float gDCxm1[NUM_STRINGS] = {0}; // Mics
float gDCym1[NUM_STRINGS] = {0}; // Mics
float a = 0.995;

// timing and sample counting
int gSampleCount = 0;
int gReadPointers[NUM_STRINGS] = {0};
int gAudioFramesPerAnalogFrame;

// karplus-strong coefficients
float gf0[NUM_STRINGS] = {0};
int gDelayLength[NUM_STRINGS] = {0};
float gDampingFactor[NUM_STRINGS] = {0};

// MIDI note tuning
const int A4 = 440;
float gF0ForMIDINote[127] = {0};

// string tuning with all-pass filter
float gC[NUM_STRINGS] = {0};
float gAPYm1[NUM_STRINGS] = {0};
float gAPXm1[NUM_STRINGS] = {0};

// output buffers
float gOutputBuffer[PLAYBACK_BUFFER_SIZE][NUM_STRINGS];

// isRunning state for preventing strings being excited before DC-blocking takes effect:
int isRunning;

// global f0 change
int gPotValInt;
int gPotValIntPrev;

int gAudioSampleRate;

void updateNotes(int);

bool setup(BelaContext *context, void *userData)
{
	// for handling analog inputs
	gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	
	// for use in other functions
	gAudioSampleRate = context->audioSampleRate;
	
	// calculate frequencies from MIDI note values
	// solution found in http://newt.phys.unsw.edu.au/jw/notes.html
	float semiTonesFromA4;
	for (int i = 1; i <= 127; i++){
		semiTonesFromA4 = i - 69;
		gF0ForMIDINote[i] = pow(2,(semiTonesFromA4/12.0))*A4;
	}

	updateNotes(50);	// C2
	
	for (int i = 0; i < NUM_STRINGS; i++){
		gDampingFactor[i] = 0.989;
	}
	
	// set isRunning state to 0
	isRunning = 0;

	scope.setup(4, context->audioSampleRate);

	return true;
}

void render(BelaContext *context, void *userData)
{
	
	float testInput[NUM_STRINGS];
	
	for(unsigned int n = 0; n < context->audioFrames; n++){
		
		/* ANALOG INPUT */
		
		// if current audio frame is also an analog frame
		if(!(n%gAudioFramesPerAnalogFrame)){
			
			// read multiple voice inputs here:
			
			for (int i = 0; i < NUM_STRINGS; i++){
				gMicVal[i] = analogRead(context, n/gAudioFramesPerAnalogFrame, gMicPin[i]);
				testInput[i] = analogRead(context, n/gAudioFramesPerAnalogFrame, gMicPin[i]);

				/* SIGNAL CONDITIONING */
				float DCx = gMicVal[i];
		
				// DC-blocking difference equation from Bela examples:
				// y[n] = x[n] - x[n-1] + a*y[n-1] where a is usually set to around 0.995
				float DCy = DCx - gDCxm1[i] + a * gDCym1[i];
		
				// update x(n-1) and y(n-1) values for next pass
				gDCxm1[i] = DCx;
				gDCym1[i] = DCy;
		
				// pass output of DC-blocking filter to gMicConditioned variable (for reading to scope)
				gMicConditioned[i] = DCy;
			}
			
			// single voice input (i.e. potentiometer) here:

		}
		
		process_microphones(gMicConditioned, gMicPostProcessed, NUM_STRINGS);
		
	/* KARPLUS-STRONG */
	
	float stringValPT[NUM_STRINGS] = {0};	// PT = pre-tuned
	float stringVal[NUM_STRINGS] = {0};
	float inputVal;
	int currentReadPointer;
	
	// loop over voices
	for (int i = 0; i < NUM_STRINGS; i++){
		
		// for the first few seconds, I don't want to excite the strings as the DC blocking takes a while to come into effect
		// so I'm using a state machine here:
		if (isRunning == 0 && gSampleCount <= PLAYBACK_BUFFER_SIZE){
			// put nothing in the input buffer
			inputVal = 0.0;
		}
		if (isRunning == 0 && gSampleCount == PLAYBACK_BUFFER_SIZE - 1){
			// put the DC-blocked Mic values into the input buffer
			inputVal = gMicPostProcessed[i] * 2.0;	
			isRunning = 1;
		}
		if (isRunning == 1){
			inputVal = gMicPostProcessed[i] * 2.0;
		}

		// update read pointer and wrap around
		gReadPointers[i] = (gSampleCount - gDelayLength[i] + PLAYBACK_BUFFER_SIZE)%PLAYBACK_BUFFER_SIZE;
		
		currentReadPointer = gReadPointers[i];
		
		/* K-S ALGORITHM */
		
		// difference equation from Karplus-Strong paper: 
		// y(n) = damping*x(n)+y(n-N)+y(n-(N+1))/2
		stringValPT[i] = gDampingFactor[i] * (inputVal + gOutputBuffer[currentReadPointer][i] + gOutputBuffer[currentReadPointer+1][i])/2.0f;
		
		/* STRING TUNING USING ALL-PASS */
		
		// tuning filter difference equation from [1]: 
		// y(n) = C * [ x(n) - y(n-1) ] + x(n-1)
		stringVal[i] = gC[i] * (stringValPT[i] - gAPYm1[i]) + gAPXm1[i];
		
		gAPYm1[i] = stringVal[i];
		gAPXm1[i] = stringValPT[i];
		
		// set y(n) to current sample (for when we get to y(n+N))
		gOutputBuffer[gSampleCount][i] = stringVal[i];
	}
	
	float out = 0.0;
	
	/* OUTPUT & MIX STRINGS */
	
	for (int i = 0; i < NUM_STRINGS; i++){
		// out += stringVal[i] / NUM_STRINGS;
		out += stringVal[i];
	}
	
	/* HOUSE-KEEPING AND PLAYBACK */
	
	// wrap sample counter
	if(++gSampleCount >= PLAYBACK_BUFFER_SIZE)
		gSampleCount = 0;
	
	// scale output to prevent clipping
	out *= 0.9;
	
	// Emergency clip handling
	if(out >= 1.0){
		out = 0.9999;
		rt_printf("signal clipped \n");
	}
	if(out <= -1.0){
		out = -0.9999;
		rt_printf("signal clipped \n");
	}
	
	scope.log(testInput[0],testInput[1],testInput[2],testInput[3]);
	
	// use audioWrite() to pass output sample to output buffer
	audioWrite(context, n, 0, out);
	audioWrite(context, n, 1, out);
	}
}

// function to update all notes
void updateNotes(int root){
	int min2nd = root + 1;
	int maj2nd = root + 2;
	int min3rd = root + 3;
	int maj3rd = root + 4;
	int nat4th = root + 5;
	int dim5th = root + 6;
	int nat5th = root + 7;
	int min6th = root + 8;
	int maj6th = root + 9;
	int min7th = root + 10;
	int maj7th = root + 11;
	int octave = 12;
	
	int string1 = root;
	int string2 = maj6th;
	int string3 = maj3rd+octave;
	int string4 = nat5th+octave;
	
	// if strings go out of range:
	while(string1 > 127){ 
		string1-=octave;
	}
	while(string2 > 127){ 
		string2-=octave;
	}
	while(string3 > 127){ 
		string3-=octave;
	}
	while(string4 > 127){ 
		string4-=octave;
	}
	
	gf0[0] = gF0ForMIDINote[string1];			
	gf0[1] = gF0ForMIDINote[string2];		
	gf0[2] = gF0ForMIDINote[string3];	
	gf0[3] = gF0ForMIDINote[string4];	
	
	rt_printf("string 1 = %d, string 2 = %d, string 3 = %d, string 4 = %d \n", string1, string2, string3, string4);
	
	float P1;
	float epsilon = 0.001;
	float PcF1;
	
	for (int i = 0; i < NUM_STRINGS; i++){
		P1 = gAudioSampleRate/gf0[i];
		gDelayLength[i] = floor(P1 - 0.5 - epsilon);
		PcF1 = P1 - gDelayLength[i] - 0.5;
		gC[i] = (1 - PcF1)/(1 + PcF1);
		// rt_printf("C for string %d = %f",i,gC[i]);
	}
}


void cleanup(BelaContext *context, void *userData)
{

}