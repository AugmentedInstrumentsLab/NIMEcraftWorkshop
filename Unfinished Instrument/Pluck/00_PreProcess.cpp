/**
 * NIMEcraft workshop
 * 15 May 2017
 *
 * Edit this file to preprocess the microphone data before
 * it goes to the waveguide models
 */

#include <00_PreProcess.h>

// process_microphones()
// This function is called once per sample to
// preprocess the microphone signals before they
// are sent to the string models.
//
// inputs: an array of samples from each microphone (nominally -1 to 1 range)
// outputs: an array of samples to send to each waveguide
// channels: the number of channels in each of the above arrays

void process_microphones(float *inputs, float *outputs, int channels)
{
	float gain = 1.0;
	
	// Passthrough mapping with gain
	for(int i = 0; i < channels; i++)
		outputs[i] = inputs[i] * gain;
}