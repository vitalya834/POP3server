#include "headers/MailHandler.h"
#include <fstream>
#include<string.h>
using namespace std;
MailHandler::MailHandler()
{
	// в конструкторе добавляются такие юзера в users, users - private поле в заголовочном
	users.push_back(User("wladez", "password"));
	users.push_back(User("azat", "12345"));
	users.push_back(User("lera", "1q2w3e"));
	// FOR TESTS!
	//и нулевому юзеру добавляются два тестовых письма
	Letter letter = Letter();
	letter.addFrom((string)"lera");
	letter.addSubject((string)"Just for fun");
	letter.addTo((string)"wladez");
	letter.addData((string)"Hi! How are you?");
	users[0].addLetter(letter);
	Letter letter1 = Letter();
	letter1.addFrom((string)"azat");
	letter1.addSubject((string)"Study");
	letter1.addTo((string)"wladez");

	letter1.addData((string)"You need to get zachot for seti!");
	users[0].addLetter(letter1);
}

DWORD WINAPI MailHandler::clientHandler(LPVOID param){
	SOCKET client_socket = (SOCKET)param;
	if (client_socket == INVALID_SOCKET) {
		printf("error with accept socket. GetLasterror= %d\n", GetLastError());
		return 1003;
	}
	char buf[SIZE_OF_BUF] = "/0";
	//пересылаем код ответа номер 2 (ready) через клиентский сокет
	//POP3-коды ответа отпределены в Responses.h как двумерный массив
	sendLine(client_socket, responses[2]);
	//значение устанавливается в answer(), по смыслу номер состояния после принятия запроса
	int status = 1;
	int aut;
	User curUs = User();
	do {
		aut = authentication(client_socket, status, curUs); //процесс аутентификации клиента
		if (status == 0) {
			break;//Проверка на выход
		}
	} while (aut < 0);
	while (recvLine(client_socket, buf, SIZE_OF_BUF)>0){
		//принимаем сообщения размером buf из клиентского сокета, записываем в buf
		int len = recvLine(client_socket, buf, SIZE_OF_BUF);
		if (len > 2){
			//если что-то пришло, отвечаем
			string request=string(buf);
			request.erase(request.size() - 1);
			if (request.find("STAT") != string::npos){
				MailHandler::status(client_socket, curUs);
			}
			else if (request.find("LIST") != string::npos){
				getList(client_socket, request, curUs);
			}
			else if (request.find("RETR") != string::npos){
				getMessage(client_socket, request, curUs);
			}
			else if (request.find("DELE") != string::npos){
				deleteMessage(client_socket, request, curUs);
			}
			else if (request.find("RSET") != string::npos){
				reset(client_socket, curUs);
			}
			else if (request.find("QUIT") != string::npos){
				disconnect(client_socket, status, curUs);
			}
		}
		if (status == 0) {
			break;//Проверка на выход
		}
	}
	closesocket(client_socket);
}

//процесс аутентификации
int MailHandler::authentication(SOCKET client_socket, int& status, User& curUser){
	char buf[SIZE_OF_BUF] = "/0";
	int len = recvLine(client_socket, buf, SIZE_OF_BUF);
	cout << buf << endl;  
	int res=-1;
	if (len>0){
		string request = string(buf);
		string pass;		
		request.erase(request.size() - 1);
		cout << "request = " << request << endl;
		if (request.find("USER") != string::npos){
			request.erase(request.begin(), request.begin() + 5);
			int check = checkUser(request);
			if (check == -1){
				sendLine(client_socket, responses[4]);//ответ о том, что нет такого пользователя
				res = -1;
			}
			else{
				sendLine(client_socket, responses[3]);//ответ о том, что есть такой пользователь
				len = recvLine(client_socket, buf, SIZE_OF_BUF);
				if (len>0){
					pass = string(buf);
					pass.erase(pass.size() - 1);
					if (pass.find("PASS") != string::npos){
						pass.erase(pass.begin(), pass.begin() + 5);
						if (users[check].getPass() == pass){
							curUser = users[check];
							curUser.setOnline();
							string answer = connectedUser(curUser);
							sendLine(client_socket, answer.c_str());//правильный пароль
							res = 1;
						}
						else {
							sendLine(client_socket, responses[6]);//неправильный пароль
							res = -1;
						}
					}
					else if (pass.find("QUIT") != string::npos){
						status = 0;
						sendLine(client_socket, responses[16]);
					}
				}
			}
		}
		else if (request.find("QUIT") != string::npos){
			status = 0;
			sendLine(client_socket, responses[16]);
		}
		else if (request.find("CAPA") != string::npos){
			string ans;
			ans.append("+OK Capability list follows\r\nUSER\r\n.");
			sendLine(client_socket, ans.c_str());
		}
	}
	return res;
}

string MailHandler::connectedUser(User& curUser){
	int cnt = 0, cap = 0;
	cnt = curUser.lettersCount();
	cap = curUser.overallSize();
	string answer;
	answer.append(responses[1] + curUser.getName() + responses[17] + to_string(cnt) + responses[18] + to_string(cap) + " octets)\r\n");
	return answer;
}

//ответ на команду STAT
void MailHandler::status(SOCKET client_socket, User& curUser){
	int cnt = 0, cap = 0;
	cnt = curUser.lettersCount();
	cap = curUser.overallSize();
	string answer;
	answer.append(responses[1]+to_string(cnt)+" "+to_string(cap)+"\r\n");
	sendLine(client_socket, answer.c_str());
}

//ответ на команду LIST
void MailHandler::getList(SOCKET client_socket, string req, User& curUser){
	string request = req;
	string answer;
	int cnt = 0, cap = 0;
	cnt = curUser.lettersCount();
	cap = curUser.overallSize();
	if (request.length() < 5){
		answer.append(responses[1] + to_string(cnt) + " messages (" + to_string(cap) + " octets)\r\n");
		for (int i = 1; i <= cnt; i++){
			answer.append(to_string(i) + " " + to_string(curUser.letterSize(i)) + "\r\n");
		}		
	}
	else {
		request.erase(request.begin(), request.begin() + 5);
		int num = stoi(request);
		if (num <= cnt && num>0) answer.append(responses[1] + to_string(num) + " " + to_string(curUser.letterSize(num)) + "\r\n");
		else answer.append(responses[0]); 
	}
	sendLine(client_socket, answer.c_str());
}

//ответ на команду RETR
void MailHandler::getMessage(SOCKET client_socket, string req, User& curUser){
	string request = req;
	string answer;
	int cnt = 0, cap = 0, num=0;
	cnt = curUser.lettersCount();
	if (req.size() > 5){
		string temp = request.substr(5, 1);
		try{
			num = stoi(temp, nullptr);			
		}
		catch (invalid_argument){
			cout << "Error! Invalid argument!" << endl;
			answer.append("-ERR Invalid argument!");
		}
		if (num <= cnt && num > 0) {			
			Letter let = Letter();
			let = curUser.getLetter(num);
			if (let.getMarker() == false){
				cout << "number is " << num << endl;
				answer.append(responses[1] + to_string(curUser.letterSize(num)) + " octets\r\n");
				answer.append("From: " + let.getFrom());
				answer.append("\r\nTo: " + let.getTo());
				answer.append("\r\nSubject: " + let.getSubject());
				answer.append("\r\nData: " + let.getData() + "\r\n.");
			}
			else answer.append("-ERR message " + to_string(num) + " was deleted\r\n");			
		}
		else answer.append(responses[0]);
	}
	else answer.append(responses[0]);
	sendLine(client_socket, answer.c_str());
}

//ответ на команду DELE
void MailHandler::deleteMessage(SOCKET client_socket, string req, User& curUser){
	string request = req;
	string answer;
	int cnt = 0, num = 0;
	cnt = curUser.lettersCount();
	string temp = request.substr(5, 1);
	try{
		num = stoi(temp, nullptr);
	}
	catch (invalid_argument){
		cout << "Error! Invalid argument!" << endl;
		answer.append("-ERR Invalid argument!");
	}
	if (num <= cnt && num > 0){
		if (curUser.getLetter(num).getMarker() == false){
			curUser.deleteLetter(num);
			answer.append(responses[11] + to_string(num) + " deleted\r\n");
		}
		else answer.append("-ERR message " + to_string(num) + " already deleted\r\n");
	}
	else answer.append(responses[0]);
	sendLine(client_socket, answer.c_str());
}

//сброс установленных флагов на удаление
void MailHandler::reset(SOCKET client_socket, User& curUser){
	string answer;
	int cnt = 0;
	cnt = curUser.lettersCount();
	for (int i = 1; i <= cnt; i++){
		if (curUser.getLetter(i).getMarker() == true){
			//curUser.getLetter(i).setMarker(false);
			curUser.recoverLetter(i);
		}
	}
	answer.append(responses[13] + to_string(cnt) + " messages\r\n");
	sendLine(client_socket, answer.c_str());
}

//завершение сеанса
void MailHandler::disconnect(SOCKET client_socket, int& status, User& curUser){
	string answer;
	int cnt = 0;
	cnt = curUser.lettersCount();
	for (int i = cnt; i > 0; i--){
		if (curUser.getLetter(i).getMarker() == true){
			curUser.eraseLetter(i-1);
		}
	}
	int fin = curUser.lettersCount();
	if (fin == 0) answer.append(responses[20]);
	else answer.append(responses[19] + to_string(fin) + " messages left)");
	curUser.setOfline();
	status = 0;
	sendLine(client_socket, answer.c_str());
}

//возвращает индекс первого юзера в массиве, имя которого (возвращаемое getName()) совпало с аргументом, если таких нет, то -1
int MailHandler::checkUser(const string &name){
	for (int i = 0; i < users.size(); ++i){
		if (users[i].getName() == name){
			return i;
		}
	}
	return -1;
}
thread MailHandler::createThread(LPVOID param){
	//при использовании из main.cpp param - client_socket оттуда
	return thread(&MailHandler::clientHandler, this, param);
}
