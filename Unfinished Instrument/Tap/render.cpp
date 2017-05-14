/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the
Centre for Digital Music at Queen Mary University of London.
http://www.eecs.qmul.ac.uk/~andrewm

(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
	Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
	Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License
(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
*/


/*
 * AIR-HARP
 * Physically modelled strings using waveguide junctions and mass-spring-dampers
 * controllable using an accelerometer
 *
 * render.cpp
 *
 * Christian Heinrichs 04/2015
 *
 */


#include "MassSpringDamper.h"
#include "String.h"
#include "Plectrum.h"

#include <Bela.h>
#include <cmath>
#include <stdio.h>
#include <cstdlib>
#include <rtdk.h>
#include <Scope.h>
#include <stats.hpp>
#include <RampGenerator.h>
#include <00_PreProcess.h>

#define ACCEL_BUF_SIZE 8

Scope scope;

RampGenerator gOnsetDetection[NUMBER_OF_STRINGS];

// Tuning MIDI Notes
float gMidinotes[NUMBER_OF_STRINGS] = {40,45,50,55,57,60,62,64};
// float gMidinotes[NUMBER_OF_STRINGS] = {50,50,53,53,57,57,62,62};
// float gMidinotes_2[NUMBER_OF_STRINGS] = {55,55,58,58,62,62,67,67};
// float gMidinotes_3[NUMBER_OF_STRINGS] = {50,50,53,53,57,57,62,62};
// float gMidinotes_4[NUMBER_OF_STRINGS] = {45,45,49,49,52,52,57,57};


float gInverseSampleRate;
int gAudioFramesPerAnalogFrame;

float out_gain = 5.0;

int accelPin_x = 0;
int accelPin_y = 1;
int accelPin_z = 2;

MassSpringDamper msd = MassSpringDamper(1,0.1,10);// (10,0.001,10);
String strings[NUMBER_OF_STRINGS];
Plectrum plectrums[NUMBER_OF_STRINGS];

float gPlectrumDisplacement = 0;

float gAccel_x[ACCEL_BUF_SIZE] = {0};
int gAccelReadPtr = 0;

// DC BLOCK BUTTERWORTH

// Coefficients for 100hz high-pass centre frequency
float a0_l = 0.9899759179893742;
float a1_l = -1.9799518359787485;
float a2_l = 0.9899759179893742;
float a3_l = -1.979851353142371;
float a4_l = 0.9800523188151258;

float a0_r = a0_l;
float a1_r = a1_l;
float a2_r = a2_l;
float a3_r = a3_l;
float a4_r = a4_l;

float x1_l = 0;
float x2_l = 0;
float y1_l = 0;
float y2_l = 0;

float x1_r = 0;
float x2_r = 0;
float y1_r = 0;
float y2_r = 0;


// TEST PARAMS FOR TRIGGERING
int gTrigger[NUMBER_OF_STRINGS];

float gInterval = 0.1;
float gSecondsElapsed = 0;
int gCount = 0;

float gSignalDifference;
//
// float gStringPairs[4];
// float gOnsetDetected[8];

float gTriggered[8];

int gButton1Status;
int gButtonPressed;

int gChord; // stores the rotation of chords
#define NUMBER_OF_CHORDS 3

MovingAverage<float> gMovingAverages[8];
const int gMovingAverageLength = 135;

bool setup(BelaContext *context, void *userData)
{

	gInverseSampleRate = 1.0 / context->audioSampleRate;
	gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	
	// Buttons
	pinMode(context, 0, P8_07, INPUT);
	pinMode(context, 0, P8_08, INPUT);

	
	// initialise movingAverage Filter
	for(int n = 0; n < NUMBER_OF_STRINGS; n++){
		gMovingAverages[n].setLength(gMovingAverageLength);
	}
	
	// initialise onset detection
	for(int i=0;i<NUMBER_OF_STRINGS;i++)    {
        gOnsetDetection[i] = RampGenerator();
    }
    
    // setup the scope with x channels at the audio sample rate
	scope.setup(4, context->audioSampleRate);

	// initialise strings & plectrums
	for(int i=0;i<NUMBER_OF_STRINGS;i++)	{

		plectrums[i] = Plectrum();
		plectrums[i].setup(250,0.05,0.05);

		strings[i] = String();
		strings[i].setMidinote(gMidinotes[i]);

		float spacing = 2.0 / (NUMBER_OF_STRINGS+1);

		strings[i].setGlobalPosition( -1 + spacing*(i+1) );

		rt_printf("STRING %d // midinote: %f position: %f\n",i,gMidinotes[i],( -1 + spacing*(i+1) ));

	}

	return true;
}

void render(BelaContext *context, void *userData)
{
	// float lastAccel = 0;
	
	float max = 0;
	int max_index = 0;
	
	static float micInputs[8];
	static float waveguideInputs[8]; 

	for(int n = 0; n < context->audioFrames; n++) {
		

		/*
		 *
		 * READ BUTTON AND CYCLE CHORDS
		 *
		 */
		
		// Increment a counter on every frame
		gCount++;

		// Print a message every second indicating the number of seconds elapsed
		if(gCount % (int)(context->audioSampleRate*gInterval) == 0) {
		    gSecondsElapsed += gInterval;
		    //rt_printf("Time elapsed: %f %f %f %f \n",gTriggered[0],gTriggered[1],gTriggered[2],gTriggered[3]);
		    
		    // Read Buttons 1
			gButton1Status = digitalRead(context, 0, P8_07);
			
			if (gButton1Status == 0 && !gButtonPressed){
				gChord++;
				gButtonPressed = 1;
			} 
			else if (gButton1Status == 1){
				gButtonPressed = 0;
			}
			if(gChord >= NUMBER_OF_CHORDS){
				gChord = 0;
			}
			
		}
		
	
		// Read microphones
		if(!(n % gAudioFramesPerAnalogFrame)) {
			for(int ch = 0; ch < 8; ch++) {
				if(ch < context->analogInChannels)
					micInputs[ch] = analogRead(context, n / gAudioFramesPerAnalogFrame, ch);
				else
					micInputs[ch] = 0;
			}
		}
		
		// Call user-defined preprocessing
		process_microphones(micInputs, waveguideInputs, 8);

		/*
		 *
		 * PHYSICS SIMULATION
		 *
		 */

		// The horizontal force (which can be gravity if box is tipped on its side)
		// is used as the input to a Mass-Spring-Damper model
		// Plectrum displacement (i.e. when interacting with string) is included
		// float massPosition = (float)msd.update(gravity - gPlectrumDisplacement);

		float out_l = 0;
		float out_r = 0;
		// Use this parameter to quickly adjust output gain
		float gain = 0.0015;	// 0.0015 is a good value or 12 strings
		gPlectrumDisplacement = 0;

		for(int s=0;s<NUMBER_OF_STRINGS;s++)	{

			float stringPosition = strings[s].getGlobalPosition();

			//float plectrumForce = plectrums[s].update(massPosition, stringPosition);
			// gPlectrumDisplacement += strings[s].getPlectrumDisplacement();

			// calculate panning based on string position (-1->left / 1->right)
			float panRight = map(stringPosition,1,-1,0.1,1);
			float panLeft = map(stringPosition,-1,1,0.1,1);
			panRight *= panRight;
			panLeft *= panLeft;
			
			if(s < 8) {
				float out = strings[s].update(waveguideInputs[s]) * 0.1;
				out_l += out; //*panLeft;
				out_r += out; //*panRight;
			}
		}

		// APPLY DC-BLOCK FILTER TO OUTPUTS
		// LEFT CHANNEL
		float temp_in = out_l;
		/* compute result */
    	out_l = a0_l * out_l + a1_l * x1_l + a2_l * x2_l - a3_l * y1_l - a4_l * y2_l;
    	/* shift x1 to x2, sample to x1 */
    	x2_l = x1_l;
    	x1_l = temp_in;
    	/* shift y1 to y2, result to y1 */
    	y2_l = y1_l;
   	 	y1_l = out_l;

   	 	// RIGHT CHANNEL
		temp_in = out_r;
		/* compute result */
    	out_r = a0_r * out_r + a1_r * x1_r + a2_r * x2_r - a3_r * y1_r - a4_r * y2_r;
    	/* shift x1 to x2, sample to x1 */
    	x2_r = x1_r;
    	x1_r = temp_in;
    	/* shift y1 to y2, result to y1 */
    	y2_r = y1_r;
   	 	y1_r = out_r;

		// Suppress startup transient by fading in over 3 seconds
		float out_gain_adjusted = 0;
		if(context->audioFramesElapsed < context->audioSampleRate) {
			out_gain_adjusted = 0;
		}
		else if(context->audioFramesElapsed < 3 * context->audioSampleRate) {
			out_gain_adjusted = out_gain * (float)(context->audioFramesElapsed + n - context->audioSampleRate) 
										/ (2.0 * (float)context->audioSampleRate);
		}
		else {
			out_gain_adjusted = out_gain;
		}
		context->audioOut[n * context->audioOutChannels + 1] = out_l * out_gain_adjusted;
		context->audioOut[n * context->audioOutChannels + 0] = out_r * out_gain_adjusted;
		
		// Optional scope logging
	    //scope.log(gStringPairs[0], gStringPairs[1],gOnsetDetected[0], gOnsetDetected[1],gStringPairs[2], gStringPairs[3],gOnsetDetected[2], gOnsetDetected[3]);
	    //scope.log(gAnalogReadingsFiltered[0],gAnalogReadingsFiltered[1],gAnalogReadingsFiltered[2],gAnalogReadingsFiltered[3]);

	}

}


// cleanup_render() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in initialise_render().

void cleanup(BelaContext *context, void *userData)
{

}
