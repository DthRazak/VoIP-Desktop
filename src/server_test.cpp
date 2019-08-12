#include <iostream>

#include "server.h"
#include "mulawdecoder.h"
#include "mulawencoder.h"

static ThreadSafeCircleBuffer<AudioBlock> buff(5);
static std::mutex mut;

static void testPlayback(PaStream *stream, PaStreamParameters &outputParameters, PaError &err) {
	AudioBlock	block;
	buff.read(block);
	block.frameIndex = 0;

	std::unique_lock<std::mutex> lock(mut);
	err = Pa_OpenStream(
		&stream,
		NULL, /* no input */
		&outputParameters,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		paClipOff,      /* we won't output out of range samples so don't bother clipping them */
		playCallback,
		&block);
	lock.unlock();

	if (err != paNoError) {
		return;
	}

	for (size_t i = 0; i < 9; ++i) {

		if (stream) {
			err = Pa_StartStream(stream);
			if (err != paNoError) {
				break;
			}

			while ((err = Pa_IsStreamActive(stream)) == 1)
				Pa_Sleep(100);

			if (err < 0) {
				break;
			}

			err = Pa_StopStream(stream);
			if (err != paNoError) {
				break;
			}
			buff.read(block);
			block.frameIndex = 0;
		}
	}

	lock.lock();
	err = Pa_CloseStream(stream);
	lock.unlock();
	if (err != paNoError) return;
}

static void testRecord(PaStream *stream, PaStreamParameters &inputParameters, PaError &err) {
	AudioBlock block;
	block.clear();
	block.frameIndex = 0;

	std::unique_lock<std::mutex> lock(mut);
	err = Pa_OpenStream(
		&stream,
		&inputParameters,
		NULL,                   /* &outputParameters, */
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		paClipOff,				/* we won't output out of range samples so don't bother clipping them */
		recordCallback,
		&block);
	lock.unlock();

	if (err != paNoError) {
		return;
	}

	for (int i = 0; i < 10; ++i) {

		err = Pa_StartStream(stream);
		if (err != paNoError) {
			break;
		}

		Pa_Sleep(NUM_SECONDS * 1000);
		while ((err = Pa_IsStreamActive(stream)) == 1)
			Pa_Sleep(100);

		if (err < paNoError) {
			break;
		}


		err = Pa_StopStream(stream);
		if (err != paNoError) {
			break;
		}

		buff.write(block);
		block.frameIndex = 0;
	}

	lock.lock();
	err = Pa_CloseStream(stream);
	lock.unlock();
	if (err != paNoError) return;
}

void Server::concurrentTest() {
	PaStreamParameters  inputParameters,
		outputParameters;
	PaStream			*istream = nullptr, *ostream = nullptr;
	PaError             err = paNoError;
	AudioBlock			block;

	err = initializeAudio();
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	err = initializeInputAudio(inputParameters);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	err = initializeOutputAudio(outputParameters);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	std::thread t1(testRecord, istream, std::ref(inputParameters), std::ref(err));
	testPlayback(ostream, outputParameters, err);

	if (t1.joinable())
		t1.join();

	terminateAudio(err);
}

void Server::testAudio() {
	PaStreamParameters  inputParameters,
		outputParameters;
	PaStream*			stream;
	PaError             err = paNoError;
	AudioBlock			block(8 * SAMPLE_RATE, 8 * SAMPLE_RATE * NUM_CHANNELS);

	err = initializeAudio();
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	err = initializeInputAudio(inputParameters);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	err = Pa_OpenStream(
		&stream,
		&inputParameters,
		NULL,                   /* &outputParameters, */
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		paClipOff,				/* we won't output out of range samples so don't bother clipping them */
		recordCallback,
		&block);

	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	err = Pa_StartStream(stream);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	std::cout << "Now speak into microphone." << std::endl;

	Pa_Sleep(8 * 1000);
	while ((err = Pa_IsStreamActive(stream)) == 1)
		Pa_Sleep(100);

	if (err < paNoError) {
		terminateAudio(err);
		return;
	}

	err = Pa_StopStream(stream);

	err = Pa_CloseStream(stream);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	block.frameIndex = 0;

	err = initializeOutputAudio(outputParameters);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	err = Pa_OpenStream(
		&stream,
		NULL, /* no input */
		&outputParameters,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		paClipOff,      /* we won't output out of range samples so don't bother clipping them */
		playCallback,
		&block);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	if (stream) {
		err = Pa_StartStream(stream);
		if (err != paNoError) {
			terminateAudio(err);
			return;
		}

		std::cout << "Waiting for playback to finish." << std::endl;

		while ((err = Pa_IsStreamActive(stream)) == 1) Pa_Sleep(100);
		if (err < 0) {
			terminateAudio(err);
			return;
		}

		err = Pa_CloseStream(stream);
		if (err != paNoError) {
			terminateAudio(err);
			return;
		}

		std::cout << "Done." << std::endl;
	}

	terminateAudio(err);
}


void Server::testAudioWithEncoder() {
	PaStreamParameters  inputParameters,
		outputParameters;
	PaStream			*istream, *ostream;
	PaError             err = paNoError;
	AudioBlock			block;
	ThreadSafeCircleBuffer<AudioBlock> localBuff(6);

	err = initializeAudio();
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	err = initializeInputAudio(inputParameters);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	err = Pa_OpenStream(
		&istream,
		&inputParameters,
		NULL,                   /* &outputParameters, */
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		paClipOff,				/* we won't output out of range samples so don't bother clipping them */
		recordCallback,
		&block);

	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	for (size_t i = 0; i < 5; ++i) {
		err = Pa_StartStream(istream);
		if (err != paNoError) {
			terminateAudio(err);
			return;
		}

		Pa_Sleep(NUM_SECONDS * 1000);
		while ((err = Pa_IsStreamActive(istream)) == 1)
			Pa_Sleep(100);

		if (err < paNoError) {
			terminateAudio(err);
			return;
		}

		err = Pa_StopStream(istream);

		localBuff.write(block);
		block.frameIndex = 0;
	}

	err = Pa_CloseStream(istream);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	//block.frameIndex = 0;

	err = initializeOutputAudio(outputParameters);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	uint8_t *b1 = new uint8_t[block.blockSize];
	localBuff.read(block);
	block.frameIndex = 0;
	muLawEncode(block.data, b1, block.blockSize);
	muLawDecode(b1, block.data, block.blockSize);

	err = Pa_OpenStream(
		&ostream,
		NULL, /* no input */
		&outputParameters,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		paClipOff,      /* we won't output out of range samples so don't bother clipping them */
		playCallback,
		&block);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	for (size_t i = 0; i < 4; ++i) {

		if (ostream) {

			err = Pa_StartStream(ostream);
			if (err != paNoError) {
				terminateAudio(err);
				return;
			}

			while ((err = Pa_IsStreamActive(ostream)) == 1) Pa_Sleep(100);
			if (err < 0) {
				terminateAudio(err);
				return;
			}

			err = Pa_StopStream(ostream);
			if (err != paNoError) {
				terminateAudio(err);
				return;
			}

			localBuff.read(block);
			muLawEncode(block.data, b1, block.blockSize);
			muLawDecode(b1, block.data, block.blockSize);
			block.frameIndex = 0;
		}
	}

	err = Pa_CloseStream(ostream);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	terminateAudio(err);
}
