#include "headers/util.h"

int sendAll(SOCKET sock, const char *buf, int len) {
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

// A single blocking recv() per byte keeps the reader trivially correct:
// nothing is read past the line terminator, so consecutive commands are
// never swallowed. Fine for an educational server.
int recvLine(SOCKET sock, std::string &line) {
	line.clear();
	char ch;
	for (;;) {
		int n = recv(sock, &ch, 1, 0);
		if (n == 0) {
			return 0; // connection closed by peer
		}
		if (n == SOCKET_ERROR) {
			return -1;
		}
		if (ch == '\n') {
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			return 1;
		}
		if (line.size() >= MAX_LINE_LEN) {
			return -1; // line too long, treat as protocol error
		}
		line.push_back(ch);
	}
}

int sendLine(SOCKET sock, const std::string &text) {
	std::string out = text;
	if (out.size() < 2 || out.compare(out.size() - 2, 2, "\r\n") != 0) {
		out += "\r\n";
	}
	return sendAll(sock, out.c_str(), (int)out.size());
}
