#pragma once
#include <boost/asio.hpp>

#include "buffer.hpp"
#include "audio.h"

using namespace boost::asio::ip;

class Server {
private:
	address serverAddress;
	std::string clientAddress;
	uint16_t serverPort, clientPort;
	std::atomic<bool> runningFlag, backgroundFlag;
	bool incomingCallFlag, otherServerReady;
	mutable std::mutex mut;
	boost::asio::io_service service;
	tcp::acceptor acceptor;
	tcp::socket socket;
	ThreadSafeCircleBuffer<AudioBlock> buff;

	static boost::asio::ip::address getIP();
	void sendResponse();
	void disableRunningFlag();
	void playback(PaStream *stream, PaStreamParameters &outputParameters, PaError &err);
public:
	Server();
	Server(std::string& address, uint16_t port);
	~Server();

	bool getIncomingCallFlag() const;
	bool getRunningFlag() const;
	bool isOtherServerReady() const;
	uint16_t getServerPort() const;
	uint16_t getClientPort() const;
	std::string getClientAddress() const;

	void stop();

	void disableIncomingCallFlag();
	void endCall();

	void testAudio();
	void testAudioWithEncoder();
	void concurrentTest();

	void handleConnection();
	void handleCallData();
};	