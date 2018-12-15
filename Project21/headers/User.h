#pragma once
#include <string>
#include "headers/Mailbox.h"
using namespace std;
class User
{
public:
	User();
	User(const string& name, const string& pass);
	~User();
	string getName(){
		return userName;
	}

	string getPass(){
		return password;
	}

	//добавление письма в ящик
	void addLetter(const Letter& l){
		box.addLetterToMailbox(l);
	}

	//удаление письма из ящика
	void eraseLetter(int num){
		box.eraseLetterFromMailbox(num);
	}

	//возвращает количество писем в ящике у пользователя
	int lettersCount(){
		return box.returnCountOfLetters();
	}
	
	//возвращает общий размер всех писем в ящике
	int overallSize(){
		return box.returnSizeOfAllLetters();
	}

	//возвращает размер кокретного письма
	int letterSize(int i){
		return box.returnSizeOfLetter(i);
	}

	//возвращает письмо
	Letter getLetter(int i){
		Letter l = Letter();
		l = box.returnLetter(i);
		return l;
	}

	void deleteLetter(int i){
		box.setFlag(i,true);
	}

	void recoverLetter(int i){
		box.setFlag(i,false);
	}

	void setOnline(){
		isOnline = true;
	}

	void setOfline(){
		isOnline = false;
	}

private:
	string userName;
	string password;
	Mailbox box;
	bool isOnline;
};

