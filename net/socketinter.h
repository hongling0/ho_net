#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#ifdef _cpluscplus
extern "C" {
#endif

	typedef int (*protocol_call)();


	typedef struct socket_opt
	{
		protocol_call recv;
	} socket_opt;

	int start_socketserver(int thr);
	int stop_socketserver();
	int start_listen(int logic, const char * addr, int port, int backlog, socket_opt* opt, int* id);
	int start_connet(int logic, const char * addr, int port, socket_opt* opt, int* id);
	int start_send(int fd, char* data, int sz);
	int start_close(int fd);


#ifdef _cpluscplus
}
#endif

#endif //SOCKET_SERVER_H

