#include <iostream>
#include <boost/thread.hpp>

#include "client.h"
#include "mulawencoder.h"

Client::Client(std::string & address, uint16_t port) :
	ep(address::from_string(address), port),
	socket(new tcp::socket(service)),
	buff(5),
	runningFlag(true)
{
	constructEncoder();
}

Client::~Client() {
	destructEncoder();
}

void Client::send(uint8_t *data, size_t blockSize) {
	socket->send(boost::asio::buffer(data, blockSize));
}


void Client::connect() {
	socket->connect(ep);
}

void Client::makeCall(uint16_t port) {
	socket->send(boost::asio::buffer("call " + std::to_string(port)));
}

void Client::setServerReady() {
	socket->send(boost::asio::buffer("ready"));
}

void Client::waitForResponse() {
	boost::asio::streambuf b;
	boost::asio::read_until(*socket, b, "ok!");
}
void Client::endCall() {
	disableRunningFlag();
	if (socket->is_open())
		socket->send(boost::asio::buffer("end"));
}

bool Client::getRunningFlag() const {
	return runningFlag;
}

bool Client::isCallRejected() const {
	return !socket->is_open();
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

	boost::thread t(&Client::record, this, stream, std::ref(inputParameters), std::ref(err));
	
	AudioBlock block;
	uint8_t * encodedBuffer = new uint8_t[block.blockSize];

	while (runningFlag) {

		buff.read(block);
		
		if (!runningFlag)
			break;

		muLawEncode(block.data, encodedBuffer, block.blockSize);
		send(encodedBuffer, block.blockSize);
		waitForResponse();

	}

	if (t.joinable())
		t.join();
	terminateAudio(err);

	delete[] encodedBuffer;
}

bool Client::try_close() {
	if (socket->is_open()) {
		socket->close();
		socket.reset(new tcp::socket(service));  
		return true;
	}
	return false;
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

	buff.write(block); // Prevent deadlock in Client::run() thread

	lock.lock();
	err = Pa_CloseStream(stream);
	lock.unlock();
}
