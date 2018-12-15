#ifndef POP3SERVER_MAILHANDLER_H
#define POP3SERVER_MAILHANDLER_H
#include "util.h"
#include "User.h"
#include "Responses.h"
#include <string>
#include <thread>
#include <vector>
#include <map>
using namespace std;

class MailHandler
{
public:
	MailHandler();
	DWORD WINAPI clientHandler(LPVOID param);
	thread createThread(LPVOID param);
	int checkUser(const string &name);
	int authentication(SOCKET client_socket, int& status, User& curUser);
	string connectedUser(User& curUser);
	void status(SOCKET client_socket, User& curUser);
	void getList(SOCKET client_socket, string req, User& curUser);
	void getMessage(SOCKET client_socket, string req, User& curUser);
	void deleteMessage(SOCKET client_socket, string req, User& curUser);
	void reset(SOCKET client_socket, User& curUser);
	void disconnect(SOCKET client_socket, int& status, User& curUser);

private:
	vector<User> users;
	vector<Letter> let;
};
#endif
