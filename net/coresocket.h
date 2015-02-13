#ifndef CORE_SOCKET_H
#define CORE_SOCKET_H

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

	typedef struct corebuf
	{
		void* data;
		unsigned long size;
	} corebuf;
	int start_send(int fd, struct corebuf* buf);
	int start_close(int fd);

	enum socketmsg_type
	{
		socketmsg_accept,
		socketmsg_connect,
		socketmsg_recv,
		socketmsg_socketerr,
		socketmsg_max,
	};

	struct socket_msg
	{
		uint8_t type;
		uint32_t id;
		int err;
		union {
			int listenid;
			struct buf{
				unsigned long size;
				char data[1];
			};
		};
	};

#ifdef _cpluscplus
}
#endif

#endif //SOCKET_SERVER_H
