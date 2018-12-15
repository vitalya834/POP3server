#ifndef SERVER_RESPONSES_H
#define SERVER_RESPONSES_H

//512 - максимальная длина каждого элемента
const char responses[][512] = {
	{ "-ERR no such message\r\n" },//0 - ошибка, нет такого сообщения
	{ "+OK " },//1 - часть ответа от сервера
	{ "+OK POP3 server ready\r\n" },//2 сервер готов к работе с клиентом
	{ "+OK name is a valid mailbox\r\n" },//3 положительный ответ на команду USER
	{ "-ERR never heard of mailbox name\r\n" },//4 отрицательный ответ на команду USER
	{ "+OK %s's maildrop has %d messages (%d octets)\n" },//5 доступ к почтовому серверу открыт - ответ на команду PASS
	{ "-ERR invalid password\r\n" },//6 неправильный пароль - ответ на команду PASS
	{ "-ERR unable to lock maildrop\n" },//7 нет доступа к почтовому ящику - ответ на команду PASS
	{ "+OK %d %d\n" },//8 ответ на команду STAT
	{ "+OK %s\n" },//9 ответ на команду LIST
	{ "+OK %d octets\n%s\n" },//10 ответ на команду RETR
	{ "+OK message " },//11 ответ на команду DELE
	{ "+OK\n" },//12 ответ на команду NOOP
	{ "+OK maildrop has " },//13 ответ на команду RSET
	{ "+OK\n%s\n" },//14 ответ на команду TOP
	{ "+OK %d %s\n" },//15 ответ на команду UIDL
	{ "+OK dewey POP3 server signing off\n" },//16 ответ на команду QUIT в состоянии авторизации
	{ "'s maildrop has " },//17
	{ " messages (" },//18
	{ "+OK dewey POP3 server signing off (" },//19 ответ на команду QUIT при отсоединении пользователя
	{ "+OK dewey POP3 server signing off (maildrop empty)" },//20 ответ на команду QUIT при отсоединении пользователя при пустом ящике
	{ "+OK Capability list follows\r\n" }//21 CAPA - расширение для POP3
};

#endif /*SERVER_RESPONSES_H*/