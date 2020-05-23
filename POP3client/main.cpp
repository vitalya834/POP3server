// Interactive console POP3 client for the educational POP3 server.
//
// Usage: POP3client.exe [host] [port]
//   host defaults to 127.0.0.1, port to 8110.
//
// The client connects, prints the server greeting, then reads commands
// from stdin, sends them CRLF-terminated and prints the reply. For the
// multi-line replies (LIST/UIDL without argument, RETR, TOP, CAPA) it
// keeps reading until the terminating "." line, undoing byte-stuffing.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

#pragma comment (lib, "Ws2_32.lib")

static const char *DEFAULT_HOST = "127.0.0.1";
static const unsigned short DEFAULT_PORT = 8110;
static const size_t MAX_LINE_LEN = 4096;

static int sendAll(SOCKET sock, const char *buf, int len) {
	int total = 0;
	while (total < len) {
		int n = send(sock, buf + total, len - total, 0);
		if (n == SOCKET_ERROR) {
			return -1;
		}
		total += n;
	}
	return total;
}

static int sendLine(SOCKET sock, const string &text) {
	string out = text + "\r\n";
	return sendAll(sock, out.c_str(), (int)out.size());
}

// 1 = line read, 0 = connection closed, -1 = error.
static int recvLine(SOCKET sock, string &line) {
	line.clear();
	char ch;
	for (;;) {
		int n = recv(sock, &ch, 1, 0);
		if (n == 0) return 0;
		if (n == SOCKET_ERROR) return -1;
		if (ch == '\n') {
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			return 1;
		}
		if (line.size() >= MAX_LINE_LEN) return -1;
		line.push_back(ch);
	}
}

static string upperVerb(const string &line) {
	string verb = line.substr(0, line.find(' '));
	for (size_t i = 0; i < verb.size(); ++i) {
		verb[i] = (char)toupper((unsigned char)verb[i]);
	}
	return verb;
}

// Multi-line replies exist only for these verbs and only on +OK.
static bool isMultiLine(const string &verb, bool hasArg) {
	if (verb == "RETR" || verb == "TOP" || verb == "CAPA") return true;
	if ((verb == "LIST" || verb == "UIDL") && !hasArg) return true;
	return false;
}

// Reads lines until the "." terminator, removing byte-stuffing.
// Returns false if the connection broke.
static bool printMultiLine(SOCKET sock) {
	string line;
	for (;;) {
		if (recvLine(sock, line) <= 0) {
			return false;
		}
		if (line == ".") {
			return true;
		}
		if (!line.empty() && line[0] == '.') {
			line.erase(line.begin());
		}
		cout << line << "\n";
	}
}

int main(int argc, char *argv[]) {
	const char *host = (argc > 1) ? argv[1] : DEFAULT_HOST;
	const char *portStr = "8110";
	if (argc > 2) {
		char *end = nullptr;
		long value = strtol(argv[2], &end, 10);
		if (end == argv[2] || *end != '\0' || value < 1 || value > 65535) {
			fprintf(stderr, "Usage: %s [host] [port]\n  port: 1-65535, default %u\n",
				argv[0], (unsigned)DEFAULT_PORT);
			return 1;
		}
		portStr = argv[2];
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed with error: %d\n", WSAGetLastError());
		return 1;
	}

	addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	addrinfo *result = nullptr;
	if (getaddrinfo(host, portStr, &hints, &result) != 0 || result == nullptr) {
		fprintf(stderr, "Cannot resolve %s:%s\n", host, portStr);
		WSACleanup();
		return 1;
	}

	SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sock == INVALID_SOCKET) {
		fprintf(stderr, "socket() failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		fprintf(stderr, "Cannot connect to %s:%s (error %d)\n",
			host, portStr, WSAGetLastError());
		freeaddrinfo(result);
		closesocket(sock);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);

	string line;
	if (recvLine(sock, line) <= 0) {
		fprintf(stderr, "Server closed the connection before greeting\n");
		closesocket(sock);
		WSACleanup();
		return 1;
	}
	cout << line << "\n";
	cout << "Commands: USER, PASS, STAT, LIST [n], RETR n, DELE n, RSET, NOOP, CAPA, QUIT\n";

	int exitCode = 0;
	string input;
	for (;;) {
		cout << "pop3> " << flush;
		if (!getline(cin, input)) {
			// stdin closed (Ctrl+Z): leave politely.
			sendLine(sock, "QUIT");
			recvLine(sock, line);
			cout << line << "\n";
			break;
		}
		// trim trailing CR/whitespace pasted from elsewhere
		while (!input.empty() && (input.back() == '\r' || input.back() == ' ')) {
			input.pop_back();
		}
		if (input.empty()) {
			continue;
		}
		if (sendLine(sock, input) < 0) {
			fprintf(stderr, "Connection lost\n");
			exitCode = 1;
			break;
		}
		if (recvLine(sock, line) <= 0) {
			fprintf(stderr, "Connection closed by server\n");
			exitCode = 1;
			break;
		}
		cout << line << "\n";

		string verb = upperVerb(input);
		bool hasArg = input.find(' ') != string::npos;
		if (line.compare(0, 3, "+OK") == 0 && isMultiLine(verb, hasArg)) {
			if (!printMultiLine(sock)) {
				fprintf(stderr, "Connection closed by server\n");
				exitCode = 1;
				break;
			}
		}
		if (verb == "QUIT" ) {
			break;
		}
	}

	shutdown(sock, SD_BOTH);
	closesocket(sock);
	WSACleanup();
	return exitCode;
}
