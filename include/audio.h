#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>

#define SAMPLE_RATE	(44100)
#define FRAMES_PER_BUFFER (512)
#define NUM_SECONDS (2)
#define NUM_CHANNELS (2)
/* #define DITHER_FLAG     (paDitherOff) */
#define DITHER_FLAG (0)

 /* sample format. */
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE (0)


struct AudioBlock
{
	size_t      frameIndex;		/* Index into sample array. */
	size_t		maxFrameIndex;
	size_t		blockSize;
	SAMPLE		*data;

	AudioBlock();
	AudioBlock(size_t mfi, size_t bs);
	AudioBlock(AudioBlock &&other) noexcept;
	~AudioBlock();

	AudioBlock& operator = (AudioBlock && other);
	AudioBlock& operator = (AudioBlock & other);

	void clear();
};


int recordCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData);

int playCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData);

PaError initializeAudio();

void terminateAudio(PaError err);

PaError initializeInputAudio(PaStreamParameters & param);

PaError initializeOutputAudio(PaStreamParameters & param);
