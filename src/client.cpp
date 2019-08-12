#include <iostream>

#include "client.h"
#include "mulawencoder.h"

Client::Client(std::string & address, uint16_t port) :
	ep(address::from_string(address), port),
	sock(service),
	buff(5),
	runningFlag(true)
{
	constructEncoder();
}

Client::~Client() {
	destructEncoder();
}

void Client::send(uint8_t *data, size_t blockSize) {
	sock.send(boost::asio::buffer(data, blockSize));
}


void Client::connect() {
	sock.connect(ep);
}

void Client::makeCall(uint16_t port) {
	sock.send(boost::asio::buffer("call " + std::to_string(port)));
}

void Client::setServerReady() {
	sock.send(boost::asio::buffer("ready"));
}

void Client::waitForResponse() {
	boost::asio::streambuf b;
	boost::asio::read_until(sock, b, "ok!");
}
void Client::endCall() {
	sock.send(boost::asio::buffer("end"));
}

bool Client::getRunningFlag() const {
	return runningFlag;
}

void Client::disableRunningFlag() {
	runningFlag = false;
}

void Client::run() {
	PaStreamParameters  inputParameters;
	PaStream*			stream;
	PaError             err = paNoError;

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

	std::thread t(&Client::record, this, stream, std::ref(inputParameters), std::ref(err));
	
	AudioBlock block;
	uint8_t * encodedBuffer = new uint8_t[block.blockSize];

	while (runningFlag) {

		buff.read(block);
		
		muLawEncode(block.data, encodedBuffer, block.blockSize);
		send(encodedBuffer, block.blockSize);
		waitForResponse();

	}

	if (t.joinable())
		t.join();
	terminateAudio(err);

	delete[] encodedBuffer;
}

void Client::record(PaStream *stream, PaStreamParameters &inputParameters, PaError &err) {
	AudioBlock block;

	std::unique_lock<std::mutex> lock(mut);
	err = Pa_OpenStream(
		&stream,
		&inputParameters,
		NULL,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		paClipOff,
		recordCallback,
		&block);
	lock.unlock();

	if (err != paNoError) {
		disableRunningFlag();
		return;
	}

	while (runningFlag) {

		err = Pa_StartStream(stream);
		if (err != paNoError) {
			disableRunningFlag();
			continue;
		}

		Pa_Sleep(NUM_SECONDS * 1000);
		while ((err = Pa_IsStreamActive(stream)) == 1)
			Pa_Sleep(100);

		if (err < paNoError) {
			disableRunningFlag();
			continue;
		}
		
		err = Pa_StopStream(stream);
		if (err != paNoError) {
			disableRunningFlag();
			continue;
		}

		buff.write(block);
		block.frameIndex = 0;

	}

	lock.lock();
	err = Pa_CloseStream(stream);
	lock.unlock();
	if (err != paNoError) return;
}
