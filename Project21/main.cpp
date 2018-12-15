#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "headers/util.h"
#include "headers/MailHandler.h"
using namespace std;

HANDLE mutex;
HANDLE allhandlers[numCl];
int numClients = 0;

int main(void) {
	MailHandler mh;
	//используется для инициализации библиотеки сокетов 
//	char buf[SIZE_OF_BUF]; //буфер приема и передачи сообщения 
//	int readbytes; //число прочитанных байт 
	WSADATA WSStartData; //Инициализация WinSock и проверка его запуска 
	if (WSAStartup(MAKEWORD(2, 0), &WSStartData) != 0) {
		printf("WSAStartup failed with error: %d\n", GetLastError());
		return 100;
	} //создание сокета 
	SOCKET server_socket; //по умолчанию используется протокол tcp 
	printf("Server is started.\nTry to create socket -----------------");
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printf("error with creation socket. GetLasterror= %d\n", GetLastError());
		return 1000;
	}
	printf("CHECK\n"); //Привязывание сокета конкретному IP и номеру порта
	struct sockaddr_in sin;
	sin.sin_addr.s_addr = INADDR_ANY; // используем все интерфейсы
	sin.sin_port = htons(120); // используем порт протокола POP3
	sin.sin_family = AF_INET; printf("Try to bind socket -------------------");
	if (::bind(server_socket, (struct sockaddr *)&sin, sizeof(sin)) == SOCKET_ERROR) {
		printf("error with bind socket. GetLasterror= %d\n", GetLastError());
		return 1001;
	}
	printf("CHECK\n"); //делаем сокет прослушиваемым
	printf("Try to set socket listening ----------");
	if (listen(server_socket, 5) != 0) {
		printf("error with listen socket. GetLasterror= %d\n", GetLastError());
		return 1002;
	}
	printf("CHECK\n");
	printf("Server starts listening\n"); //Ждем клиента. Создаем пустую структуру, которая будет содержать параметры сокета, 
	//инициирующего соединение 
	struct sockaddr_in from;
	int fromlen = sizeof(from); // начинаем "слушать" входящие запросы на подключение
	int ind = 0;
	//на каждое подключение к серверному сокету
	while (SOCKET client_socket = accept(server_socket, (struct sockaddr*)&from, &fromlen)){
		//создаём новый поток, в котором работает clientHandler() из MailHandler, 
		//detach отправляет его выполняться независимо от главного потока
		mh.createThread((LPVOID)client_socket).detach();
		cout << "Client connect to server\n" <<endl;
	}
	closesocket(server_socket);
	return 0;
}
