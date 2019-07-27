#pragma once
#include <string>
using namespace std;

class Letter
{
public:
	Letter();
	~Letter();

	void addFrom(const string &fr){
		from = fr;
	}

	void addSubject(const string &sub){
		subject = sub;
	}

	void addData(const string &dat){
		data += dat;
	}

	void addTo(const string &t){
		to = t;
	}

	void setMarker(bool m){
		marker = m;
	}

	string getFrom() const {
		return from;
	}

	string getData() const {
		return data;
	}

	string getTo() const {
		return to;
	}

	string getSubject() const {
		return subject;
	}

	bool getMarker() const {
		return marker;
	}

	// Full RFC822-style text of the message as it goes over the wire.
	string render() const {
		return "From: " + from + "\r\n"
			"To: " + to + "\r\n"
			"Subject: " + subject + "\r\n"
			"\r\n" + data + "\r\n";
	}

	// Message size in octets = length of the transmitted text,
	// not sizeof() of the object.
	int size() const {
		return (int)render().size();
	}

private:
	string from;
	string to;
	string subject;
	string data;
	bool marker; // deletion mark; false while the message is intact
};
