#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include "headers/util.h"
#include "headers/MailHandler.h"
using namespace std;

static const unsigned short DEFAULT_PORT = 8110;

// Accepts "Project21.exe [port]"; falls back to DEFAULT_PORT.
static bool parsePort(int argc, char *argv[], unsigned short &port) {
	port = DEFAULT_PORT;
	if (argc < 2) {
		return true;
	}
	char *end = nullptr;
	long value = strtol(argv[1], &end, 10);
	if (end == argv[1] || *end != '\0' || value < 1 || value > 65535) {
		return false;
	}
	port = (unsigned short)value;
	return true;
}

int main(int argc, char *argv[]) {
	unsigned short port;
	if (!parsePort(argc, argv, port)) {
		fprintf(stderr, "Usage: %s [port]\n  port: 1-65535, default %u\n",
			argv[0], (unsigned)DEFAULT_PORT);
		return 1;
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed with error: %d\n", WSAGetLastError());
		return 1;
	}

	SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET) {
		fprintf(stderr, "socket() failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// Lets the server restart immediately without waiting out TIME_WAIT.
	BOOL reuse = TRUE;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
		(const char *)&reuse, sizeof(reuse));

	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	if (::bind(server_socket, (sockaddr *)&sin, sizeof(sin)) == SOCKET_ERROR) {
		fprintf(stderr, "bind() failed with error: %d\n", WSAGetLastError());
		closesocket(server_socket);
		WSACleanup();
		return 1;
	}

	if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
		fprintf(stderr, "listen() failed with error: %d\n", WSAGetLastError());
		closesocket(server_socket);
		WSACleanup();
		return 1;
	}

	printf("POP3 server listening on port %u\n", (unsigned)port);

	MailHandler mh;
	for (;;) {
		sockaddr_in from;
		int fromlen = sizeof(from);
		SOCKET client_socket = accept(server_socket, (sockaddr *)&from, &fromlen);
		if (client_socket == INVALID_SOCKET) {
			int err = WSAGetLastError();
			if (err == WSAEINTR || err == WSAECONNRESET) {
				continue; // transient, keep accepting
			}
			fprintf(stderr, "accept() failed with error: %d\n", err);
			break;
		}
		cout << "Client connected" << endl;
		// The session thread owns the client socket and closes it itself.
		mh.createThread(client_socket).detach();
	}

	closesocket(server_socket);
	WSACleanup();
	return 0;
}
