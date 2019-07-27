#include "headers/MailHandler.h"
#include <iostream>
#include <sstream>
using namespace std;

namespace {

// Splits "RETR 1" into an upper-cased verb ("RETR") and the raw
// argument string ("1"). Extra whitespace is trimmed.
void parseCommand(const string &line, string &verb, string &arg) {
	size_t sp = line.find(' ');
	verb = line.substr(0, sp);
	for (size_t i = 0; i < verb.size(); ++i) {
		verb[i] = (char)toupper((unsigned char)verb[i]);
	}
	arg.clear();
	if (sp != string::npos) {
		size_t start = line.find_first_not_of(' ', sp);
		if (start != string::npos) {
			size_t end = line.find_last_not_of(' ');
			arg = line.substr(start, end - start + 1);
		}
	}
}

// Parses a whole-token positive message number; rejects "1abc", "-2", "".
bool parseMessageNumber(const string &arg, int &num) {
	if (arg.empty() || arg.size() > 9) return false;
	for (size_t i = 0; i < arg.size(); ++i) {
		if (!isdigit((unsigned char)arg[i])) return false;
	}
	num = stoi(arg);
	return num >= 1;
}

// RFC 1939 byte-stuffing: a body line starting with '.' is sent as "..".
void sendMessageText(SOCKET sock, const string &text) {
	istringstream in(text);
	string bodyLine;
	while (getline(in, bodyLine)) {
		if (!bodyLine.empty() && bodyLine.back() == '\r') {
			bodyLine.pop_back();
		}
		if (!bodyLine.empty() && bodyLine[0] == '.') {
			bodyLine.insert(bodyLine.begin(), '.');
		}
		sendLine(sock, bodyLine);
	}
}

} // namespace

MailHandler::MailHandler()
{
	// Educational server: accounts and demo mail live in memory.
	users.push_back(User("wladez", "password"));
	users.push_back(User("azat", "12345"));
	users.push_back(User("lera", "1q2w3e"));

	Letter letter;
	letter.addFrom("lera");
	letter.addSubject("Just for fun");
	letter.addTo("wladez");
	letter.addData("Hi! How are you?");
	users[0].addLetter(letter);

	Letter letter1;
	letter1.addFrom("azat");
	letter1.addSubject("Study");
	letter1.addTo("wladez");
	letter1.addData("You need to get zachot for seti!");
	users[0].addLetter(letter1);
}

thread MailHandler::createThread(SOCKET client_socket) {
	return thread(&MailHandler::clientSession, this, client_socket);
}

int MailHandler::findUser(const string &name) const {
	for (size_t i = 0; i < users.size(); ++i) {
		if (users[i].getName() == name) {
			return (int)i;
		}
	}
	return -1;
}

User &MailHandler::sessionUser(Session &s) {
	return users[s.userIndex];
}

void MailHandler::clientSession(SOCKET client_socket) {
	Session s;
	s.sock = client_socket;
	s.state = State::Authorization;
	s.userIndex = -1;

	sendLine(s.sock, "+OK POP3 server ready");

	string line, verb, arg;
	while (s.state != State::Update) {
		int r = recvLine(s.sock, line);
		if (r <= 0) {
			// Client vanished without QUIT: abort the session, do NOT
			// enter UPDATE - marked messages must survive (RFC 1939).
			break;
		}
		if (line.empty()) {
			sendLine(s.sock, "-ERR empty command");
			continue;
		}
		parseCommand(line, verb, arg);

		lock_guard<mutex> guard(usersMutex);
		if (verb == "QUIT") {
			cmdQuit(s);
		} else if (verb == "NOOP") {
			cmdNoop(s);
		} else if (verb == "CAPA") {
			cmdCapa(s);
		} else if (verb == "USER") {
			if (s.state == State::Authorization) cmdUser(s, arg);
			else sendLine(s.sock, "-ERR command USER is valid only in AUTHORIZATION state");
		} else if (verb == "PASS") {
			if (s.state == State::Authorization) cmdPass(s, arg);
			else sendLine(s.sock, "-ERR command PASS is valid only in AUTHORIZATION state");
		} else if (verb == "STAT" || verb == "LIST" || verb == "RETR"
				|| verb == "DELE" || verb == "RSET") {
			if (s.state != State::Transaction) {
				sendLine(s.sock, "-ERR command " + verb + " is valid only in TRANSACTION state");
			} else if (verb == "STAT") {
				cmdStat(s);
			} else if (verb == "LIST") {
				cmdList(s, arg);
			} else if (verb == "RETR") {
				cmdRetr(s, arg);
			} else if (verb == "DELE") {
				cmdDele(s, arg);
			} else {
				cmdRset(s);
			}
		} else {
			sendLine(s.sock, "-ERR unknown command");
		}
	}

	// Session ended without a clean UPDATE: release the maildrop lock
	// and forget the deletion marks.
	if (s.userIndex >= 0) {
		lock_guard<mutex> guard(usersMutex);
		sessionUser(s).mailbox().unmarkAll();
		sessionUser(s).unlock();
	}

	shutdown(client_socket, SD_BOTH);
	closesocket(client_socket);
	cout << "Client disconnected" << endl;
}

void MailHandler::cmdUser(Session &s, const string &arg) {
	if (arg.empty()) {
		sendLine(s.sock, "-ERR USER requires a mailbox name");
		return;
	}
	if (findUser(arg) == -1) {
		s.pendingUser.clear();
		sendLine(s.sock, "-ERR never heard of mailbox " + arg);
		return;
	}
	s.pendingUser = arg;
	sendLine(s.sock, "+OK " + arg + " is a valid mailbox");
}

void MailHandler::cmdPass(Session &s, const string &arg) {
	if (s.pendingUser.empty()) {
		sendLine(s.sock, "-ERR send USER first");
		return;
	}
	int idx = findUser(s.pendingUser);
	s.pendingUser.clear();
	if (idx == -1 || users[idx].getPass() != arg) {
		sendLine(s.sock, "-ERR invalid password");
		return;
	}
	if (users[idx].isLocked()) {
		sendLine(s.sock, "-ERR unable to lock maildrop");
		return;
	}
	users[idx].lock();
	s.userIndex = idx;
	s.state = State::Transaction;
	Mailbox &box = users[idx].mailbox();
	sendLine(s.sock, "+OK " + users[idx].getName() + "'s maildrop has "
		+ to_string(box.activeCount()) + " messages ("
		+ to_string(box.activeSize()) + " octets)");
}

void MailHandler::cmdStat(Session &s) {
	Mailbox &box = sessionUser(s).mailbox();
	sendLine(s.sock, "+OK " + to_string(box.activeCount()) + " "
		+ to_string(box.activeSize()));
}

void MailHandler::cmdList(Session &s, const string &arg) {
	Mailbox &box = sessionUser(s).mailbox();
	if (arg.empty()) {
		sendLine(s.sock, "+OK " + to_string(box.activeCount()) + " messages ("
			+ to_string(box.activeSize()) + " octets)");
		for (int i = 1; i <= box.totalCount(); ++i) {
			if (!box.isDeleted(i)) {
				sendLine(s.sock, to_string(i) + " " + to_string(box.letterSize(i)));
			}
		}
		sendLine(s.sock, ".");
		return;
	}
	int num;
	if (!parseMessageNumber(arg, num) || !box.validNumber(num) || box.isDeleted(num)) {
		sendLine(s.sock, "-ERR no such message");
		return;
	}
	sendLine(s.sock, "+OK " + to_string(num) + " " + to_string(box.letterSize(num)));
}

void MailHandler::cmdRetr(Session &s, const string &arg) {
	Mailbox &box = sessionUser(s).mailbox();
	int num;
	if (!parseMessageNumber(arg, num) || !box.validNumber(num)) {
		sendLine(s.sock, "-ERR no such message");
		return;
	}
	if (box.isDeleted(num)) {
		sendLine(s.sock, "-ERR message " + to_string(num) + " already deleted");
		return;
	}
	sendLine(s.sock, "+OK " + to_string(box.letterSize(num)) + " octets");
	sendMessageText(s.sock, box.letter(num).render());
	sendLine(s.sock, ".");
}

void MailHandler::cmdDele(Session &s, const string &arg) {
	Mailbox &box = sessionUser(s).mailbox();
	int num;
	if (!parseMessageNumber(arg, num) || !box.validNumber(num)) {
		sendLine(s.sock, "-ERR no such message");
		return;
	}
	if (box.isDeleted(num)) {
		sendLine(s.sock, "-ERR message " + to_string(num) + " already deleted");
		return;
	}
	box.markDeleted(num, true);
	sendLine(s.sock, "+OK message " + to_string(num) + " deleted");
}

void MailHandler::cmdRset(Session &s) {
	Mailbox &box = sessionUser(s).mailbox();
	box.unmarkAll();
	sendLine(s.sock, "+OK maildrop has " + to_string(box.activeCount())
		+ " messages (" + to_string(box.activeSize()) + " octets)");
}

void MailHandler::cmdNoop(Session &s) {
	sendLine(s.sock, "+OK");
}

void MailHandler::cmdCapa(Session &s) {
	sendLine(s.sock, "+OK Capability list follows");
	sendLine(s.sock, "USER");
	sendLine(s.sock, "IMPLEMENTATION educational POP3 server");
	sendLine(s.sock, ".");
}

void MailHandler::cmdQuit(Session &s) {
	if (s.state == State::Transaction) {
		// UPDATE state: physically remove messages marked by DELE
		// from the shared user list, then release the lock.
		Mailbox &box = sessionUser(s).mailbox();
		box.purge();
		int left = box.totalCount();
		sessionUser(s).unlock();
		s.userIndex = -1;
		if (left == 0) {
			sendLine(s.sock, "+OK POP3 server signing off (maildrop empty)");
		} else {
			sendLine(s.sock, "+OK POP3 server signing off ("
				+ to_string(left) + " messages left)");
		}
	} else {
		sendLine(s.sock, "+OK POP3 server signing off");
	}
	s.state = State::Update;
}
