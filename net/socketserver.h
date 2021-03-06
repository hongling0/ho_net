#pragma once


#include "logic.h"
#include "socketinterface.h"
#include "socket.h"



namespace frame
{

	const int MAX_SOCKET = (1 << 16);

	class socket_server : public logic
	{
	public:
		socket_server(iocp& e);
		~socket_server();
		errno_type start_listen(int logic, const char * addr, int port, int backlog, const socket_opt& opt, int& id);
		errno_type start_connet(int logic, const char * addr, int port, const socket_opt& opt, int& id);
		errno_type start_send(int fd, char* data, size_t sz);
		errno_type start_close(int fd);

		errno_type ev_listen_start(socket* s, io_event* ev);
		errno_type ev_send_start(socket* s);
		errno_type ev_recv_start(socket* s);

		void force_close(socket * s);
		socket* grub_socket(int id);
		void dec_socket(int id);

		void inner_close(socket *s, unsigned ref=1);
		
		bool report_accept(int logic, int id, int listenid, errno_type err);
		bool report_connect(int logic, int id, errno_type err);
		bool report_socketerr(int logic, int id, errno_type err);
		bool report_recv(int logic, int id, logic_recv* ev);

		bool post2logic(int id, int logic, logic_msg* ev, const char* flag, const char* file, int line);
	protected:
		int reserve_id();
		socket* new_fd(int id, int logic_id, socket_type fd, const socket_opt& opt, bool add);

		void socket_error(socket* s, errno_type e);
		
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
