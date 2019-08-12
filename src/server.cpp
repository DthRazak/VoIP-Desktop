#include "server.h"
#include "mulawdecoder.h"
#include "mulawencoder.h"
#include <iostream>


Server::Server() :
	serverAddress(getIP()),
	serverPort(8001),
	acceptor(service, tcp::endpoint(serverAddress, serverPort)),
	buff(5),
	runningFlag(true),
	backgroundFlag(true),
	incomingCallFlag(false),
	otherServerReady(false),
	socket(service)
{
	std::cout << "Server started on " << serverAddress << ':' << serverPort << std::endl;
	constructDecoder();
	constructEncoder();
}

Server::Server(std::string & address, uint16_t port) :
	serverAddress(boost::asio::ip::address::from_string(address)),
	serverPort(port),
	acceptor(service, tcp::endpoint(serverAddress, serverPort)),
	buff(5),
	runningFlag(true),
	backgroundFlag(true),
	incomingCallFlag(false),
	otherServerReady(false),
	socket(service)
{
	std::cout << "Server started on " << serverAddress << ':' << serverPort << std::endl;
	constructDecoder();
	constructEncoder();
}

Server::~Server() {
	destructEncoder();
	destructDecoder();
}

boost::asio::ip::address Server::getIP() {
	try {
		boost::asio::io_service io_sr;
		udp::resolver resolver(io_sr);
		udp::resolver::query query(udp::v4(), "google.com", "");
		udp::resolver::iterator endpoints = resolver.resolve(query);
		udp::endpoint ep = *endpoints;
		udp::socket socket(io_sr);
		socket.connect(ep);

		address addr = socket.local_endpoint().address();

		return addr;
	}
	catch (std::exception& ex) {
		std::cerr << "Couldn't get your IP for server, please check your internet connection and re-run program" << std::endl;
		std::system("pause");
		std::exit(-1);
	}
}

bool Server::getIncomingCallFlag() const {
	return incomingCallFlag;
}

bool Server::getRunningFlag() const {
	return runningFlag;
}

bool Server::isOtherServerReady() const {
	std::lock_guard<std::mutex> lock(mut);
	return otherServerReady;
}

uint16_t Server::getServerPort() const {
	return serverPort;
}

uint16_t Server::getClientPort() const {
	return clientPort;
}

std::string Server::getClientAddress() const {
	return clientAddress;
}

// Not work correctly
void Server::stop() {
	backgroundFlag = false;
}

void Server::disableIncomingCallFlag() {
	incomingCallFlag = false;
}

void Server::disableRunningFlag() {
	runningFlag = false;
}

void Server::endCall() {
	runningFlag = false;
}

void Server::sendResponse() {
	socket.send(boost::asio::buffer("ok!"));
}

void Server::handleConnection() {
	char buffer[16];
	while (backgroundFlag) {
		acceptor.accept(socket);
		
		size_t bytes = socket.read_some(boost::asio::buffer(buffer, 16));
		
		if (std::strncmp(buffer, "call", 4) == 0) {
			clientAddress = socket.remote_endpoint().address().to_string();
			auto str = std::string(buffer, bytes).substr(5, bytes - 1);
			clientPort = std::stoul(str);
			incomingCallFlag = true;

			std::cout << "Do you want to receive call from: "
				<< clientAddress << ':' << clientPort << '\n'
				<< "yes(1) or no(2)"
				<< std::endl;

			break;
		}

		if (std::strncmp(buffer, "ready", 5) == 0) {
			otherServerReady = true;
			break;
		}
	}

	runningFlag = true;
}

void Server::handleCallData() {
	PaStreamParameters  outputParameters;
	PaStream*			stream;
	PaError             err = paNoError;
	AudioBlock			block;
	
	runningFlag = true;

	err = initializeAudio();
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	err = initializeOutputAudio(outputParameters);
	if (err != paNoError) {
		terminateAudio(err);
		return;
	}

	std::thread t(&Server::playback, this, stream, std::ref(outputParameters), std::ref(err));

	uint8_t *encodedBuffer = new uint8_t[block.blockSize];

	while (runningFlag) {

		size_t bytes = socket.read_some(boost::asio::buffer(encodedBuffer, block.blockSize));
		sendResponse();

		if (bytes < 10 && (std::strncmp((char*)encodedBuffer, "end", 3) == 0)) {
			disableRunningFlag();
			continue;
		}

		muLawDecode(encodedBuffer, block.data, block.blockSize);

		buff.write(block);
	}

	if (t.joinable())
		t.join();
	terminateAudio(err);
	otherServerReady = false;
	delete[] encodedBuffer;

	socket.close();
}

void Server::playback(PaStream *stream, PaStreamParameters &outputParameters, PaError &err) {
	AudioBlock	block;

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
		disableRunningFlag();
		return;
	}

	while (runningFlag) {
		buff.read(block);
		block.frameIndex = 0;
		if (stream) {
			err = Pa_StartStream(stream);
			if (err != paNoError) {
				disableRunningFlag();
				//cnt = 5;
				continue;
			}

			while ((err = Pa_IsStreamActive(stream)) == 1)
				Pa_Sleep(100);

			if (err < 0) {
				disableRunningFlag();
				continue;
			}

			err = Pa_StopStream(stream);
			if (err != paNoError) {
				disableRunningFlag();
				continue;
			}
		}
	}

	lock.lock();
	err = Pa_CloseStream(stream);
	lock.unlock();
	if (err != paNoError) return;
}
