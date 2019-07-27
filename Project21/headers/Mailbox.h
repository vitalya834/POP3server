#pragma once
#include <vector>
#include <string>
#include "headers/Letter.h"

// A user's maildrop. Message numbers used by the POP3 commands are
// 1-based and stay stable for the whole session: DELE only marks a
// message, the actual erase happens in the UPDATE state (see purge()).
class Mailbox
{
public:
	Mailbox();
	~Mailbox();

	void addLetterToMailbox(const Letter &let){
		letters.push_back(let);
	}

	// Total number of messages, including ones marked as deleted.
	int totalCount() const {
		return (int)letters.size();
	}

	// Number of messages not marked as deleted.
	int activeCount() const {
		int cnt = 0;
		for (size_t i = 0; i < letters.size(); ++i){
			if (!letters[i].getMarker()) ++cnt;
		}
		return cnt;
	}

	// Combined size (octets) of messages not marked as deleted.
	int activeSize() const {
		int res = 0;
		for (size_t i = 0; i < letters.size(); ++i){
			if (!letters[i].getMarker()) res += letters[i].size();
		}
		return res;
	}

	bool validNumber(int num) const {
		return num >= 1 && num <= (int)letters.size();
	}

	// `num` is the 1-based POP3 message number in all accessors below.
	int letterSize(int num) const {
		return letters.at(num - 1).size();
	}

	const Letter &letter(int num) const {
		return letters.at(num - 1);
	}

	bool isDeleted(int num) const {
		return letters.at(num - 1).getMarker();
	}

	void markDeleted(int num, bool deleted){
		letters.at(num - 1).setMarker(deleted);
	}

	// RSET: clear every deletion mark.
	void unmarkAll(){
		for (size_t i = 0; i < letters.size(); ++i){
			letters[i].setMarker(false);
		}
	}

	// UPDATE state: physically remove everything marked as deleted.
	void purge(){
		for (int i = (int)letters.size() - 1; i >= 0; --i){
			if (letters[i].getMarker()){
				letters.erase(letters.begin() + i);
			}
		}
	}

private:
	vector<Letter> letters;
};
