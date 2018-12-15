#pragma once
#include <string>
using namespace std;

class Letter
{
public:
	Letter();
	~Letter();
//	Cstring
	void addFrom(string fr){
		from = fr;
	}

	void addSubject(string sub){
		subject = sub;
	}

	void addData(string dat){
		data += dat;
	}

	void addTo(string t){
		to = t;
	}

	void setMarker(bool m){
		marker = m;
		string t = data;
	}

	string getFrom(){
		return from;
	}

	string getData(){
		return data;
	}

	string getTo(){
		return to;
	}

	string getSubject(){
		return subject;
	}

	bool getMarker(){
		return marker;
	}

private:
	string from;
	string to;
	string subject;
	string data;
	bool marker;//метка письма, изначально должна быть FALSE
};

