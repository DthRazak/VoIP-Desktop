#pragma once
#include <string>
#include <boost/asio.hpp>

#include "audio.h"
#include "buffer.hpp"

using namespace boost::asio::ip;

class Client
{
private:
	tcp::endpoint ep;
	boost::asio::io_service service;
	tcp::socket sock;
	std::atomic<bool> runningFlag;
	mutable std::mutex mut;
	ThreadSafeCircleBuffer<AudioBlock> buff;;

	void waitForResponse();
	void disableRunningFlag();
	void send(uint8_t *data, size_t blockSize);
	void record(PaStream *stream, PaStreamParameters &inputParameters, PaError &err);

public:
	Client() = delete;
	Client(std::string& address, uint16_t port);

	~Client();

	void connect();
	void makeCall(uint16_t port);
	void setServerReady();
	void endCall();
	void run();

	bool getRunningFlag() const;
};

