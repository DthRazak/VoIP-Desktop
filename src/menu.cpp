#include <iostream>
#include <thread>

#include "menu.h"
#include "server.h"
#include "client.h"


enum Options { CALL = 1, TEST, EXIT };

std::istream& operator >> (std::istream& is, Options& op) {
	char c;
	is >> c;
	while (c < '1' || c > '3') {
		std::cout << "Please choose the correct options\n";
		is >> c;
	}
	switch (c)
	{
	case '1':
		op = CALL;
		break;
	case '2':
		op = TEST;
		break;
	case '3':
		op = EXIT;
		break;
	}
	
	if (std::cin.get() != '\n')
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	return is;
}

void receiveCall(Server & s, char c) {

	// TODO: Add some time check

	while (c != '1' && c != '2') {
		std::cin >> c;
		if (std::cin.get() != '\n')
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	if (c == '1') {
		std::string address = s.getClientAddress();
		uint16_t port = s.getClientPort();

		Client client(address, port);
		client.connect();

		client.setServerReady();

		std::thread serverThread(&Server::handleCallData, &s); // 2 acceptors
		std::thread clientThread(&Client::run, &client);

		std::cout << "Call started\n"
			<< "1: End call"
			<< std::endl;

		do {
			std::cin >> c;
			if (std::cin.get() != '\n')
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		} while (c != '1');

		client.endCall();
		// TODO: Check for deadlock, call doesn't end correctly
		s.endCall();

		if (clientThread.joinable())
			clientThread.join();
		if (serverThread.joinable())
			serverThread.join();
	}

	s.disableIncomingCallFlag();
}


void call(Server & server) {
	std::cout << "Please input caller's ip and port bellow\n"
		<< "Example: 192.168.0.1:8080"
		<< std::endl;
	std::string		s, ip;
	uint16_t		port;

	// TODO: IP and port verification

	std::cin >> s;
	ip = s.substr(0, s.find(":"));
	port = std::stoul(s.substr(s.find(":") + 1, s.length() - 1));

	Client client(ip, port);
	client.connect();
	client.makeCall(server.getServerPort());

	// TODO: Check for rejected call
	while (!server.isOtherServerReady())
		std::this_thread::sleep_for(std::chrono::seconds(1));

	std::thread serverThread(&Server::handleCallData, &server);
	std::thread clientThread(&Client::run, &client);

	std::cout << "Call started\n"
		<< "1: End call"
		<< std::endl;
	char c;
	do {
		std::cin >> c;
		if (std::cin.get() != '\n')
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	} while (c != '1');

	client.endCall();
	// TODO: Check for deadlock, call doesn't end correctly
	server.endCall();

	if (clientThread.joinable())
		clientThread.join();
	if (serverThread.joinable())
		serverThread.join();
}

void testMicro(Server & server) {
	std::cout << "What test do you want to run?\n" 
		<< "1) Record and playback;\n"
		<< "2) Record and playback with encoding;\n"
		<< "3) Record and playback at the same time;"
		<< std::endl;
	
	char c = '0';
	while (c != '1' && c != '2' && c != '3') {
		std::cin >> c;
		if (std::cin.get() != '\n')
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	switch (c)
	{
	case '1':
		server.testAudio();
		break;
	case '2':
		server.testAudioWithEncoder();
		break;
	case '3':
		server.concurrentTest();
		break;
	}
}

void runServerInBackground(Server &s) {
	std::thread serverThread(&Server::handleConnection, &s);
	serverThread.detach();
}

void stopServer(Server &s) {
	s.stop();
}

void mainMenu(Server &s) {
	runServerInBackground(s);

	Options op;

	do
	{
		std::cout << "Please choose option:\n"
			<< "1: Call\n" << "2: Test\n" << "3: Exit"
			<< std::endl;

		std::cin >> op;
		if (s.getIncomingCallFlag()) {
			if (op != EXIT)
				if (op == CALL) {
					receiveCall(s, '1');
					runServerInBackground(s);
				}
				else
					receiveCall(s, '2');
		}
		else
			switch (op) {
			case CALL:
				call(s);
				runServerInBackground(s);
				break;
			case TEST:
				testMicro(s);
				break;
			}
	} while (op != EXIT);

	stopServer(s);
}

char runType() {
	std::cout << "Welcome to VoIP-Desktop\n"
		<< "Do you want start server manual(1) or automatically(2)?\n"
		<< "In this case you must enter your ip and port where you want the server to start,\n"
		<< "When you start program automatically, you must be connected to the internet."
		<< std::endl;
	char c;
	do {
		std::cin >> c;
		if (std::cin.get() != '\n')
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	} while (c != '1' && c != '2');

	return c;
}

void runManual() {
	std::cout << "Please input ip and port bellow\n"
		<< "Example: 192.168.0.1:8080"
		<< std::endl;
	std::string		str, ip;
	uint16_t		port;
	
	// TODO: IP and port verification

	std::cin >> str;
	ip = str.substr(0, str.find(":"));
	port = std::stoul(str.substr(str.find(":") + 1, str.length() - 1));

	Server s(ip, port);

	mainMenu(s);

}

void runAutomaticaly() {
	Server s;

	mainMenu(s);
}

int run() {

	if (runType() == '1')
		runManual();
	else
		runAutomaticaly();

	return 0;
}