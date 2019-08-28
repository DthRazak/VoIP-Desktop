#include <iostream>
#include <thread>
#include <conio.h>

#include "menu.h"
#include "server.h"
#include "client.h"


enum Options { CALL = 1, TEST, EXIT };

enum State { MENU, ACCEPT, DECLINE };

State state = State::MENU;

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

void clearInput() {
	if (std::cin.get() != '\n')
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void runServerInBackground(Server &s) {
	std::thread serverThread(&Server::handleConnection, &s);
	serverThread.detach();
}

void receiveCall(Server & s) {
	while (true) {
		while (!s.getIncomingCallFlag())
			std::this_thread::sleep_for(std::chrono::seconds(2));

		short timeout = 8;
		while (state != State::ACCEPT && state != State::DECLINE && timeout-- > 0)
			std::this_thread::sleep_for(std::chrono::seconds(1));

		if (state == State::ACCEPT) {
			std::string address = s.getClientAddress();
			uint16_t port = s.getClientPort();

			Client client(address, port);
			client.connect();

			client.setServerReady();

			std::thread serverThread(&Server::handleCallData, &s);
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
			s.endCall();

			if (clientThread.joinable())
				clientThread.join();
			if (serverThread.joinable())
				serverThread.join();
		}

		if (timeout < 0)
			std::cout << "Time is up\n"
			<< "Please choose option:\n"
			<< "1: Call\n" << "2: Test\n" << "3: Exit" 
			<< std::endl;

		state = State::MENU;
		s.disableIncomingCallFlag();
		runServerInBackground(s);
	}
}


void call(Server & server) {
	std::cout << "Please input caller's ip and port bellow\n"
		<< "Example: 192.168.0.1:8080"
		<< std::endl;
	std::string		s, ip;
	uint16_t		port;

	do {
		std::cin >> s;
		try {
			boost::asio::ip::address::from_string(s.substr(0, s.find(":")));
			ip = s.substr(0, s.find(":"));
			port = std::stoul(s.substr(s.find(":") + 1, s.length() - 1));
			break;
		}
		catch (...) {
			std::cout << "Incorrect input, try again" << std::endl;
		}
	} while (true);

	Client client(ip, port);
	client.connect();
	client.makeCall(server.getServerPort());

	short timeout = 10;
	while (!server.isOtherServerReady() && timeout-- != 0 && !client.isCallRejected() && state != State::DECLINE)
		std::this_thread::sleep_for(std::chrono::seconds(1));

	if (timeout == -1) {
		std::cout << "Call wasn't accepted" << std::endl;
		return;
	}

	if (client.isCallRejected() || state == State::DECLINE) {
		std::cout << "Call was rejected" << std::endl;
		return;
	}

	std::thread serverThread(&Server::handleCallData, &server);
	std::thread clientThread(&Client::run, &client);

	std::cout << "Call started\n"
		<< "1: End call"
		<< std::endl;
	char c;
	do {
		std::cin >> c;
		clearInput();
	} while (c != '1');

	client.endCall();
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
		clearInput();
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

void runReceiveCallInBackground(Server &s) {
	std::thread receiveCallThread(receiveCall, std::ref(s));
	receiveCallThread.detach();
}

void stopServer(Server &s) {
	s.stop();
}


void mainMenu(Server &s) {
	runServerInBackground(s);
	
	runReceiveCallInBackground(s);

	Options op;
	do
	{
		std::cout << "Please choose option:\n"
			<< "1: Call\n" << "2: Test\n" << "3: Exit"
			<< std::endl;

		state = State::MENU;
		std::cin >> op;
		if (s.getIncomingCallFlag()) {
			if (op != EXIT) {
				if (op == CALL) {
					state = State::ACCEPT;
				}
				else
					state = State::DECLINE;

				while (s.getIncomingCallFlag())
					std::this_thread::sleep_for(std::chrono::seconds(5));
			}
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
		<< "How do you want to start server manual(1) or automatically(2)?\n"
		<< "In manual case you must enter your ip and port where you want the server to start.\n"
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
	std::string		s, ip;
	uint16_t		port;
	
	do {
		std::cin >> s;
		try {
			boost::asio::ip::address::from_string(s.substr(0, s.find(":")));
			ip = s.substr(0, s.find(":"));
			port = std::stoul(s.substr(s.find(":") + 1, s.length() - 1));
			break;
		}
		catch (...) {
			std::cout << "Incorrect input, try again" << std::endl;
		}
	} while (true);

	Server server(ip, port);

	mainMenu(server);

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