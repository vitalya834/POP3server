#ifndef SERVER_UTIL_H
#define SERVER_UTIL_H
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment (lib, "Ws2_32.lib")

#define MAX_LINE_LEN 4096

// Reads one line terminated by LF (CRLF is stripped) into `line`.
// Returns 1 on success, 0 when the peer closed the connection, -1 on error
// (including a line longer than MAX_LINE_LEN).
int recvLine(SOCKET sock, std::string &line);

// Sends `text`, appending CRLF unless it already ends with it.
// Returns the number of bytes sent, or -1 on error.
int sendLine(SOCKET sock, const std::string &text);

// Sends exactly `len` bytes. Returns `len`, or -1 on error.
int sendAll(SOCKET sock, const char *buf, int len);

#endif /*SERVER_UTIL_H*/
