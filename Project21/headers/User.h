#pragma once
#include <string>
#include "headers/Mailbox.h"
using namespace std;

class User
{
public:
	User();
	User(const string &name, const string &pass);
	~User();

	string getName() const {
		return userName;
	}

	string getPass() const {
		return password;
	}

	Mailbox &mailbox(){
		return box;
	}

	const Mailbox &mailbox() const {
		return box;
	}

	void addLetter(const Letter &l){
		box.addLetterToMailbox(l);
	}

	// POP3 allows only one session per maildrop; PASS acquires the
	// lock and QUIT (or a dropped connection) releases it.
	bool isLocked() const {
		return locked;
	}

	void lock(){
		locked = true;
	}

	void unlock(){
		locked = false;
	}

private:
	string userName;
	string password;
	Mailbox box;
	bool locked;
};
