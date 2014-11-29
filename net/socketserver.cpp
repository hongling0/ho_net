#include "socketserver.h"
#include "socket.h"
#include "atomic.h"
#include "poller.h"


#define MAX_SOCKET_P 16
#define MAX_SOCKET (1<<MAX_SOCKET_P)
#define MIN_READ_BUFFER 64

#define HASH_ID(id) (((unsigned)id) % MAX_SOCKET)

#define SOCKET_TYPE_INVALID 0
#define SOCKET_TYPE_RESERVE 1
#define SOCKET_TYPE_PLISTEN 2
#define SOCKET_TYPE_LISTEN 3
#define SOCKET_TYPE_CONNECTING 4
#define SOCKET_TYPE_CONNECTED 5
#define SOCKET_TYPE_HALFCLOSE 6
#define SOCKET_TYPE_PACCEPT 7
#define SOCKET_TYPE_BIND 8


namespace frame
{
	static struct startnetwork
	{
		startnetwork()
		{
			WSADATA data;
			WSAStartup(MAKELONG(1, 2), &data);
		}
		~startnetwork()
		{
			WSACleanup();
		}
	}  autoswitch;

	socket_server::socket_server(iocp& e) :logic(e){}
	{
		static GUID guidTransmitFile = WSAID_TRANSMITFILE;
		static GUID guidAcceptEx = WSAID_ACCEPTEX;
		static GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
		static GUID guidTransmitPackets = WSAID_TRANSMITPACKETS;
		static GUID guidConnectEx = WSAID_CONNECTEX;
		static GUID guidDisconnectEx = WSAID_DISCONNECTEX;
		static GUID guidWSARecvMsg = WSAID_WSARECVMSG;

		SOCKET fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		DWORD bytes;
		WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidTransmitFile, sizeof(guidTransmitFile), &TransmitFile, sizeof(TransmitFile), &bytes, NULL, NULL);
		WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx), &AcceptEx, sizeof(AcceptEx), &bytes, NULL, NULL);
		WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAcceptExSockaddrs, sizeof(guidGetAcceptExSockaddrs), &GetAcceptExSockaddrs, sizeof(GetAcceptExSockaddrs), &bytes, NULL, NULL);
		WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidTransmitPackets, sizeof(guidTransmitPackets), &TransmitPackets, sizeof(TransmitPackets), &bytes, NULL, NULL);
		WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidTransmitFile, sizeof(guidTransmitFile), &TransmitFile, sizeof(TransmitFile), &bytes, NULL, NULL);
		WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx, sizeof(guidConnectEx), &ConnectEx, sizeof(ConnectEx), &bytes, NULL, NULL);
		WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidDisconnectEx, sizeof(guidDisconnectEx), &DisconnectEx, sizeof(DisconnectEx), &bytes, NULL, NULL);
		WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidWSARecvMsg, sizeof(guidWSARecvMsg), &WSARecvMsg, sizeof(WSARecvMsg), &bytes, NULL, NULL);
	}

	bool  socket_server::post2logic(int logic_id,event_head* ev,errno_type err)
	{
		logic* p = grub_logic(logic_id);
		if (!p || !p->post(0, ev, 0, err))
		{
			delete ev;
			return false;
		}
		return true;
	}

	void socket_server :: socket_error(socket* s, errno_type err)
	{
		ev_socketerr* ev_logic = new ev_socketerr;
		ev_logic->id = s->id;
		post2logic(s->logic, ev_logic, err);
	}

	socket_server::~socket_server()
	{
		for (int i = 0; i < MAX_SOCKET; i++) {
			socket *s = &slot[i];
			if (s->type != SOCKET_TYPE_RESERVE) {
				s->reset();
			}
		}
	}

	int socket_server::reserve_id()
	{
		for (int i = 0; i < MAX_SOCKET; i++) {
			int id = atomic_add_fetch(&alloc_id, 1);
			if (id < 0) {
				id = atomic_add_fetch(&alloc_id, 0x7fffffff);
			}
			socket *s = &slot[HASH_ID(id)];
			if (s->type == SOCKET_TYPE_INVALID) {
				if (atomic_cmp_set(&s->type, SOCKET_TYPE_INVALID, SOCKET_TYPE_RESERVE)) {
					s->id = id;
					s->fd = -1;
					return id;
				}
				else {// retry
					--i;
				}
			}
		}
		return -1;
	}

	socket * socket_server::new_fd(int id,int logic_id, socket_type fd, const socket_opt& opt, bool add)
	{
		socket * s = &slot[HASH_ID(id)];
		assert(s->type == SOCKET_TYPE_RESERVE);

		if (add) {
			SetLastError(0);
			if (!io.append_socket(fd,s))
			{
				s->type = SOCKET_TYPE_INVALID;
				return NULL;
			}
		}

		s->id = id;
		s->fd = fd;
		s->opt = opt;
		s->logic = logic_id;
		return s;
	}

	static SOCKET do_listen(const char * host, int port, int backlog)
	{
		// only support ipv4
		// todo: support ipv6 by getaddrinfo
		uint32_t addr = INADDR_ANY;
		if (host[0]) {
			addr = inet_addr(host);
		}
		SetLastError(0);
		SOCKET listen_fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (listen_fd == INVALID_SOCKET) {
			return INVALID_SOCKET;
		}
		int reuse = 1;
		if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(int)) == -1) {
			goto _failed;
		}
		struct sockaddr_in my_addr;
		memset(&my_addr, 0, sizeof(struct sockaddr_in));
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons(port);
		my_addr.sin_addr.s_addr = addr;
		if (bind(listen_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
			goto _failed;
		}
		if (listen(listen_fd, backlog) == -1) {
			goto _failed;
		}
		return listen_fd;
	_failed:
		closesocket(listen_fd);
		return INVALID_SOCKET;
	}

	static void on_ev_listen(socket_server* poller, io_event* ev, socket* s, errno_type err, size_t sz)
	{
		SOCKADDR_IN* local_addr;
		SOCKADDR_IN* remote_addr;
		int locallen = sizeof(SOCKADDR_IN);
		int remotelen = sizeof(SOCKADDR_IN);

		GetAcceptExSockaddrs(ev->buf, sizeof(ev->buf) - 2 * (sizeof(SOCKADDR_IN) + 16), sizeof(sockaddr_in) + 16,
			sizeof(sockaddr_in) + 16, (sockaddr**)&local_addr, &locallen, (sockaddr**)&remote_addr, &remotelen);

		printf("remote[%s:%d]->local[%s:%d]\n", inet_ntoa(local_addr->sin_addr),
			(int)ntohs(local_addr->sin_port), inet_ntoa(remote_addr->sin_addr), (int)ntohs(remote_addr->sin_port));

		socket * newsocket = (socket*)ev->u;
		newsocket->type = SOCKET_TYPE_PACCEPT;

		ev_accept* ev_logic = new ev_accept;
		ev_logic->id = newsocket->id;
		ev_logic->listenid = s->id;
		if (poller->send(s->logic, PTYPE_SOCKET, 0, ev_logic, sizeof(*ev_logic)))
		{
			delete ev_logic;
			poller->closesocket(newsocket);
		}// todo 

		poller->ev_listen_start(s, ev);
		poller->ev_recv_start(newsocket);
	}
	
	errno_type socket_server::ev_listen_start(socket* s, io_event* ev)
	{
		SetLastError(0);
		SOCKET fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (fd == INVALID_SOCKET) {
			return GetLastError();
		}
		socket* news = new_fd(reserve_id(),s->logic,fd,s->opt,true);
		news->type = SOCKET_TYPE_PLISTEN;
		ev->call = on_ev_listen;
		ev->u = news;
		DWORD bytes = 0;
		memset(&ev->op, 0, sizeof(ev->op));
		SetLastError(0);
		setsockopt(news->fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&s->fd, sizeof(s->fd));
		if (!AcceptEx(s->fd, news->fd, ev->buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &bytes, &ev->op))
		{
			errno_type err = GetLastError();
			if (err != WSA_IO_PENDING)
			{
				ev->u = NULL;
				news->reset();
				return err;
			}			
		}
		return NO_ERROR;
	}

	int socket_server::start_listen(int logic, const char * addr, int port, int backlog, const socket_opt& opt, errno_type& e)
	{
		SetLastError(0);
		SOCKET fd = do_listen(addr, port, backlog);
		if (fd == INVALID_SOCKET)
		{
			e = GetLastError();
			return 0;
		}

		int id = reserve_id();
		socket* s = new_fd(id,logic,fd, opt, true);
		if (!s)
		{
			e = GetLastError();
			return 0;
		}
		s->type = SOCKET_TYPE_LISTEN;
		e = ev_listen_start(s, &s->op[socket_ev_read]);
		if (e)
		{
			s->reset();
			return 0;
		}
		return id;
	}

	static void event_connect(socket_server* poller, io_event* ev, socket* s, errno_type err, size_t sz)
	{
		s->type = SOCKET_TYPE_CONNECTED;

		ev_connect* ev_logic = new ev_connect;
		ev_logic->id = s->id;
		if (poller->send(s->logic, PTYPE_SOCKET, 0, ev_logic, sizeof(*ev_logic)))
		{
			delete ev_logic;
			poller->closesocket(s);
		}// todo 

		if (err == NO_ERROR)
		{
			poller->ev_recv_start(s);
		}
		else
			;// todo: close socket;
	}

	int socket_server::start_connet(int logic,const char * host, int port, const socket_opt& opt, errno_type& err)
	{
		SetLastError(0);
		LPFN_CONNECTEX ConnectEx = NULL;
		GUID GuidConnectEx = WSAID_CONNECTEX;

		SOCKET fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (fd == INVALID_SOCKET)
		{
			err = GetLastError();
			return 0;
		}

		SOCKADDR_IN localaddr;
		memset(&localaddr, 0, sizeof(localaddr));
		localaddr.sin_family = AF_INET;

		if (bind(fd, (SOCKADDR*)&localaddr, sizeof(localaddr)) == SOCKET_ERROR)
		{
			closesocket(fd);
			err = GetLastError();
			return 0;
		}

		localaddr.sin_addr.s_addr = inet_addr(host);
		localaddr.sin_port = htons(port);

		int id = reserve_id();
		socket * s = new_fd(id, fd, opt,true);
		if (!s)
		{
			err = GetLastError();
			return 0;
		}
		s->type = SOCKET_TYPE_CONNECTING;
		s->op[socket_ev_read].call = event_connect;
		
		DWORD bytes = 0;
		if (WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx), &ConnectEx, sizeof(ConnectEx), &bytes, NULL, NULL) != 0)
		{
			err = GetLastError();
			s->reset();
			return 0;
		}
		BOOL result = ConnectEx(fd, (SOCKADDR*)&localaddr, sizeof(localaddr), 0, 0, &bytes, &s->op[ioevent_read].op);
		if (!result)
		{
			err = GetLastError();
			if (err != WSA_IO_PENDING)
			{
				s->reset();
				return 0;
			}
		}
		err = NO_ERROR;
		return id;
	}

	static void on_ev_send(socket_server* poller, io_event* ev, socket* s, errno_type err, size_t sz)
	{
		if (err!=NO_ERROR)
		{
			ev_socketerr* ev_logic = new ev_socketerr;
			ev_logic->id = s->id;
			iocp* p = grub_poller(s->logic);
			if (!p || !p->post(0, ev_logic, 0, err))
			{
				delete ev_logic;
			}// todo 
		}
		else
		{
			s->wb.read_ok(sz);
			poller->ev_send_start(s);
		}
	}

	errno_type socket_server::ev_send_start(socket* s)
	{
		char* ptr;
		size_t sz;
		if (!s->wb.readbuffer(&ptr, &sz))  return false;

		DWORD bytes_transferred = 0;
		DWORD send_flags = 0;// flags;
		io_event* ev = &s->op[socket_ev_write];
		ev->wsa.buf = ptr;
		ev->wsa.len = sz;
		ev->call = on_ev_send;
		if (WSASend(s->fd, &ev->wsa, 1, &bytes_transferred, send_flags, &ev->op, 0)!=0)
		{
			errno_type err = GetLastError();
			if (err != WSA_IO_PENDING)
			{
				s->reset();
				return err;
			}
		}
		return NO_ERROR;
	}

	static void on_ev_recv(socket_server* poller, io_event* ev, socket* s, errno_type err, size_t sz)
	{
		s->rb.write_ok(sz);
		s->opt.recv(s,err);
		if (err!=NO_ERROR)
		{
			// todo: close
		}
		else
		{
			poller->ev_start_recv(s);
		}
	}

	errno_type socket_server::ev_recv_start(socket* s)
	{
		char* ptr;
		size_t sz;
		if (!s->rb.writebuffer(&ptr, &sz))
			return false;

		io_event * ev = &s->op[socket_ev_read];
		ev->call = on_ev_recv;
		ev->wsa.len = sz;
		ev->wsa.buf = ptr;

   		DWORD bytes_transferred = 0;
		DWORD read_flags = 0;// flags;
		SetLastError(0);
		if (WSARecv(s->fd, &ev->wsa, 1, &bytes_transferred, &read_flags, &ev->op, 0)!=0)
		{
			errno_type err = GetLastError();
			if (err != WSA_IO_PENDING)
			{
				s->reset();
				return err;
			}
		}
		return NO_ERROR;
	}

	errno_type socket_server::start_send(int id, char* data, size_t sz )
	{
		socket * s = getsocket(id);
		s->wb.write(data, sz);
		return ev_send_start(s);
	}

	socket* socket_server::getsocket(int id)
	{
		socket * s = &slot[HASH_ID(id)];
		if (s->type != SOCKET_TYPE_INVALID&&s->id == id)
			return s;
		return NULL;
	}
	
	void socket_server::force_close(socket * s)
	{
		shutdown(s->fd, SD_BOTH);
		if (s->op[socket_ev_read].ready)
		{
			CancelIoEx((HANDLE)s->fd, &s->op[socket_ev_read].op);
		}
		if (s->op[socket_ev_write].ready)
		{
			CancelIoEx((HANDLE)s->fd, &s->op[socket_ev_write].op);
		}
		s->reset();
	}

	void socket_server::on_msg(logic_msg* msg,size_t sz,errno_type err)
	{
		switch (msg->type)
		{
		case PTYPE_SYSTEM:
		{
			io_event * ev = (io_event*)msg->data;
			ev->call(this, ev, ev->s, err, sz);
		}
			break;
		default:
			assert(false); 
		}
	}

/*	int socket_server::post_close(int id)
	{
		return 0;
	}
*/
}