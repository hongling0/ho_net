#pragma once


#include "logic.h"
#include "socketinterface.h"
#include "socket.h"



namespace frame
{



	const int MAX_SOCKET_P = 16;
	const int MAX_SOCKET = (1 << MAX_SOCKET_P);
	const int MIN_READ_BUFFER = 64;

	class socket_server : public logic
	{
	public:
		socket_server(iocp& e);
		~socket_server();
		int start_listen(int logic, const char * addr, int port, int backlog, const socket_opt& opt, errno_type& e);
		int start_connet(int logic, const char * addr, int port, const socket_opt& opt, errno_type& e);
		errno_type start_send(int fd, char* data, size_t sz);
		errno_type start_close(int fd);

		errno_type ev_listen_start(socket* s, io_event* ev);
		errno_type ev_send_start(socket* s);
		errno_type ev_recv_start(socket* s);

		void force_close(socket * s);
	protected:
		int reserve_id();
		socket* new_fd(int id, int logic_id, socket_type fd, const socket_opt& opt, bool add);
		socket* getsocket(int id);

		void socket_error(socket* s, errno_type e);
		bool post2logic(int logic, event_head* ev, errno_type err);
		void on_msg(logic_msg* msg);
	public:
		LPFN_TRANSMITFILE						TransmitFile;
		LPFN_ACCEPTEX								AcceptEx;
		LPFN_GETACCEPTEXSOCKADDRS	GetAcceptExSockaddrs;
		LPFN_TRANSMITPACKETS				TransmitPackets;
		LPFN_CONNECTEX							ConnectEx;
		LPFN_DISCONNECTEX						DisconnectEx;
		LPFN_WSARECVMSG						WSARecvMsg;
	private:
		atomic_type alloc_id;
		socket* slot[MAX_SOCKET];
	};


}
