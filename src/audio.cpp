#include "audio.h"
#include <memory>
#include <mutex>


static std::mutex mut;
static bool isInitialized = false, isTerminated = false;

/*******************************************************************/

AudioBlock::AudioBlock() :
	data(new SAMPLE[NUM_SECONDS * SAMPLE_RATE * NUM_CHANNELS]),
	blockSize(NUM_SECONDS * SAMPLE_RATE * NUM_CHANNELS),
	maxFrameIndex(NUM_SECONDS * SAMPLE_RATE),
	frameIndex(0)
{}

AudioBlock::AudioBlock(size_t mfi, size_t bs) :
	frameIndex(0),
	maxFrameIndex(mfi),
	blockSize(bs),
	data(new SAMPLE[blockSize])
{}

AudioBlock::AudioBlock(AudioBlock && other) noexcept :
	data(nullptr)
{
	*this = std::move(other);
}

AudioBlock::~AudioBlock() {
	if (data != nullptr)
		delete[] data;
}

AudioBlock & AudioBlock::operator=(AudioBlock && other) {
	if (this != &other) {
		delete[] data;

		data = other.data;
		maxFrameIndex = other.maxFrameIndex;
		frameIndex = other.frameIndex;
		blockSize = other.blockSize;

		other.data = nullptr;
		other.blockSize = 0;
		other.frameIndex = 0;
		other.maxFrameIndex = 0;
	}

	return *this;
}

AudioBlock & AudioBlock::operator=(AudioBlock & other) {

	if (this != &other) {
		maxFrameIndex = other.maxFrameIndex;
		frameIndex = other.frameIndex;
		blockSize = other.blockSize;
		std::copy(other.data, other.data + blockSize, data);
	}

	return *this;
}

void AudioBlock::clear() {
	frameIndex = 0;

	for (size_t i = 0; i < maxFrameIndex * NUM_CHANNELS; i++) 
		data[i] = 0;
}

/*******************************************************************/

int recordCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	AudioBlock *block = (AudioBlock*)userData;
	const SAMPLE *rptr = (const SAMPLE*)inputBuffer;
	SAMPLE *wptr = &block->data[block->frameIndex * NUM_CHANNELS];
	long framesToCalc;
	long i;
	int finished;
	unsigned long framesLeft = block->maxFrameIndex - block->frameIndex;

	(void)outputBuffer; /* Prevent unused variable warnings. */
	(void)timeInfo;
	(void)statusFlags;
	(void)userData;

	if (framesLeft < framesPerBuffer)
	{
		framesToCalc = framesLeft;
		finished = paComplete;
	}
	else
	{
		framesToCalc = framesPerBuffer;
		finished = paContinue;
	}

	if (inputBuffer == NULL)
	{
		for (i = 0; i < framesToCalc; i++)
		{
			*wptr++ = SAMPLE_SILENCE;  /* left */
			if (NUM_CHANNELS == 2) *wptr++ = SAMPLE_SILENCE;  /* right */
		}
	}
	else
	{
		for (i = 0; i < framesToCalc; i++)
		{
			*wptr++ = *rptr++;  /* left */
			if (NUM_CHANNELS == 2) *wptr++ = *rptr++;  /* right */
		}
	}
	block->frameIndex += framesToCalc;
	return finished;
}

int playCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	AudioBlock *block = (AudioBlock*)userData;
	SAMPLE *rptr = &block->data[block->frameIndex * NUM_CHANNELS];
	SAMPLE *wptr = (SAMPLE*)outputBuffer;
	unsigned int i;
	int finished;
	unsigned int framesLeft = block->maxFrameIndex - block->frameIndex;
	(void)inputBuffer; /* Prevent unused variable warnings. */
	(void)timeInfo;
	(void)statusFlags;
	(void)userData;

	if (framesLeft < framesPerBuffer)
	{
		/* final buffer... */
		for (i = 0; i < framesLeft; i++)
		{
			*wptr++ = *rptr++;  /* left */
			if (NUM_CHANNELS == 2) *wptr++ = *rptr++;  /* right */
		}
		for (; i < framesPerBuffer; i++)
		{
			*wptr++ = 0;  /* left */
			if (NUM_CHANNELS == 2) *wptr++ = 0;  /* right */
		}
		block->frameIndex += framesLeft;
		finished = paComplete;
	}
	else
	{
		for (i = 0; i < framesPerBuffer; i++)
		{
			*wptr++ = *rptr++;  /* left */
			if (NUM_CHANNELS == 2) *wptr++ = *rptr++;  /* right */
		}
		block->frameIndex += framesPerBuffer;
		finished = paContinue;
	}
	return finished;
}

/*******************************************************************/

PaError initializeAudio() {
	PaError err = paNoError;
	std::lock_guard<std::mutex> lock(mut);
	if (!isInitialized){
		err = Pa_Initialize();
		isInitialized = true;
		isTerminated = false;
	}
	return err;
}


void terminateAudio(PaError err) {
	if (err != PaErrorCode::paNotInitialized) {
		std::unique_lock<std::mutex> lock(mut);
		if (!isTerminated) {
			Pa_Terminate();
			isTerminated = true;
			isInitialized = false;
		}
		lock.unlock();
	}

	if (err != paNoError) {
		fprintf(stderr, "An error occured while using the portaudio stream\n");
		fprintf(stderr, "Error number: %d\n", err);
		fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
	}
}

PaError initializeInputAudio(PaStreamParameters & param) {
	PaError err = paNoError;

	std::unique_lock<std::mutex> lock(mut);
	param.device = Pa_GetDefaultInputDevice();    /* default input device */
	lock.unlock();

	if (param.device == paNoDevice) {
		fprintf(stderr, "Error: No default input device.\n");
		err = PaErrorCode::paInvalidDevice;
	}

	if (err == paNoError) {
		param.channelCount = 2;                    /* stereo input */
		param.sampleFormat = PA_SAMPLE_TYPE;
		param.suggestedLatency = Pa_GetDeviceInfo(param.device)->defaultLowInputLatency;
		param.hostApiSpecificStreamInfo = NULL;
	}

	return err;
}

PaError initializeOutputAudio(PaStreamParameters & param) {
	PaError err = paNoError;

	std::unique_lock<std::mutex> lock(mut);
	param.device = Pa_GetDefaultOutputDevice(); /* default output device */
	lock.unlock();

	if (param.device == paNoDevice) {
		fprintf(stderr, "Error: No default output device.\n");
		err = PaErrorCode::paInvalidDevice;
	}

	if (err == paNoError) {
		param.channelCount = 2;                     /* stereo output */
		param.sampleFormat = PA_SAMPLE_TYPE;
		param.suggestedLatency = Pa_GetDeviceInfo(param.device)->defaultLowOutputLatency;
		param.hostApiSpecificStreamInfo = NULL;
	}

	return err;
}

/*******************************************************************/