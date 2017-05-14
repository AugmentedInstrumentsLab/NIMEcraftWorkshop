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
	
#if 0
	// Passthrough mapping with gain
	for(int i = 0; i < channels; i++)
		outputs[i] = inputs[i] * gain;
#else
	// A more complex option involving ducking all but loudest signal:
	const float threshold = 0.15;
    const float amountBelow = 0.1;
    const float rollOff = 0.0007;
	static float inputsProcessed[8];
	static float inputsFiltered[8];
	static float stringPairs[4];
	static float onsetDetected[8];

	for(int p = 0; p < channels; p++) {

		inputsProcessed[p] = inputs[p];

		// Full wave rectify
		if(inputsProcessed[p] <= 0){
			inputsProcessed[p] = -inputsProcessed[p];
		}
		
		// Moving average filter
		inputsFiltered[p] = gMovingAverages[p].add(inputsProcessed[p]);
		
		// Raw Thresholding
		if(inputsProcessed[p] <= 0.05) {
			inputsProcessed[p] = 0;
		}
	}

	// Pair the filtered analog readings according to their position to make horizontal strings
	for(int k = 0; k < (NUMBER_OF_STRINGS * 0.5); k++){
		stringPairs[k] = inputsFiltered[k*2] + inputsFiltered[(k*2)+1];
	}
	
	// Onset detection on the filtered total of each pair.
	for(int j = 0; j < NUMBER_OF_STRINGS; j++) {
		onsetDetected[j] = gOnsetDetection[j].processRamp(inputsFiltered[j], threshold, amountBelow, rollOff, j);
		outputs[j] = inputs[j] * gain * onsetDetected[j];
	}
#endif
}