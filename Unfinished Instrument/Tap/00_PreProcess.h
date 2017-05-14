/**
 * NIMEcraft workshop
 * 15 May 2017
 *
 * Edit 00_PreProcess.cpp to preprocess the microphone data before
 * it goes to the waveguide models
 */

#include <RampGenerator.h>
#include <stats.hpp>

#define NUMBER_OF_STRINGS 8

extern RampGenerator gOnsetDetection[NUMBER_OF_STRINGS];;
extern MovingAverage<float> gMovingAverages[NUMBER_OF_STRINGS];
	
void process_microphones(float *inputs, float *outputs, int channels);