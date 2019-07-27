#ifndef POP3SERVER_MAILHANDLER_H
#define POP3SERVER_MAILHANDLER_H
#include "util.h"
#include "User.h"
#include <string>
#include <thread>
#include <vector>
#include <mutex>
using namespace std;

// Handles POP3 sessions. Each client connection runs clientSession()
// in its own thread; the session walks through the three POP3 states:
//
//   AUTHORIZATION --USER/PASS--> TRANSACTION --QUIT--> UPDATE
//
// Commands: USER, PASS, STAT, LIST, RETR, DELE, RSET, NOOP, CAPA, QUIT.
class MailHandler
{
public:
	MailHandler();
	void clientSession(SOCKET client_socket);
	thread createThread(SOCKET client_socket);

private:
	enum class State { Authorization, Transaction, Update };

	// Per-session data kept on the handler thread's stack.
	struct Session {
		SOCKET sock;
		State state;
		string pendingUser; // name accepted by USER, waiting for PASS
		int userIndex;      // index into users[] once authenticated, else -1
	};

	int findUser(const string &name) const;
	User &sessionUser(Session &s);

	// AUTHORIZATION state
	void cmdUser(Session &s, const string &arg);
	void cmdPass(Session &s, const string &arg);

	// TRANSACTION state
	void cmdStat(Session &s);
	void cmdList(Session &s, const string &arg);
	void cmdRetr(Session &s, const string &arg);
	void cmdDele(Session &s, const string &arg);
	void cmdRset(Session &s);

	// any state
	void cmdNoop(Session &s);
	void cmdCapa(Session &s);
	void cmdQuit(Session &s);

	vector<User> users;
	// users[] is shared between client threads; every command that
	// touches it runs under this mutex.
	mutex usersMutex;
};
#endif
