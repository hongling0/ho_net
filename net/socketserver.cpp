
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "socketserver.h"
#include "socket.h"
#include "atomic.h"
#include "poller.h"



#define HASH_ID(id) (((unsigned)id) % MAX_SOCKET)


namespace frame
{

	static struct startnetwork
	{
		startnetwork()
		{
			WSADATA data;
			WSAStartup((WORD)MAKELONG(1, 2), &data);
		}
		~startnetwork()
		{
			WSACleanup();
		}
	}  autoswitch;

	socket_server::socket_server(iocp& e) :logic(e)
	{
		alloc_id = 0;
		GUID guidTransmitFile = WSAID_TRANSMITFILE;
		GUID guidAcceptEx = WSAID_ACCEPTEX;
		GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
		GUID guidTransmitPackets = WSAID_TRANSMITPACKETS;
		GUID guidConnectEx = WSAID_CONNECTEX;
		GUID guidDisconnectEx = WSAID_DISCONNECTEX;
		GUID guidWSARecvMsg = WSAID_WSARECVMSG;

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

		closesocket(fd);

		for (int i = 0; i < MAX_SOCKET; i++)
		{
			slot[i] = new socket(*this);
		}
	}

	socket_server::~socket_server()
	{
		for (int i = 0; i < MAX_SOCKET; i++) {
			socket *s = slot[i];
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
			socket *s = slot[HASH_ID(id)];
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

	socket * socket_server::new_fd(int id, int logic_id, socket_type fd, const socket_opt& opt, bool add)
	{
		socket * s = slot[HASH_ID(id)];
		assert(s->type == SOCKET_TYPE_RESERVE);

		if (add) {
			SetLastError(0);
			if (!io.append_socket(fd, s))
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

	static void on_ev_listen(void* data, event_head* head, size_t sz, errno_type err)
	{
		socket* s = (socket*)data;
		io_event* ev = (io_event*)head;
		socket_server& io = s->io;

		SOCKADDR_IN* local_addr;
		SOCKADDR_IN* remote_addr;
		int locallen = sizeof(SOCKADDR_IN);
		int remotelen = sizeof(SOCKADDR_IN);

		io.GetAcceptExSockaddrs(ev->buf, sizeof(ev->buf) - 2 * (sizeof(SOCKADDR_IN) + 16), sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16, (SOCKADDR**)&local_addr, &locallen, (SOCKADDR**)&remote_addr, &remotelen);

		printf("remote[%s:%d]->local[%s:%d]\n", inet_ntoa(local_addr->sin_addr),
			(int)ntohs(local_addr->sin_port), inet_ntoa(remote_addr->sin_addr), (int)ntohs(remote_addr->sin_port));

		socket * newsocket = (socket*)ev->u;
		newsocket->type = SOCKET_TYPE_PACCEPT;

		logic_accept* ev_logic = new logic_accept;
		ev_logic->id = newsocket->id;
		ev_logic->listenid = s->id;
		ev_logic->err = err;

		if (io.post(s->logic, ev_logic))
		{
			io.force_close(s);
		}

		io.ev_listen_start(s, ev);
		io.ev_recv_start(newsocket);
	}

	errno_type socket_server::ev_listen_start(socket* s, io_event* ev)
	{
		SetLastError(0);
		SOCKET fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (fd == INVALID_SOCKET) {
			return WSAGetLastError();
		}
		socket* news = new_fd(reserve_id(), s->logic, fd, s->opt, true);
		news->type = SOCKET_TYPE_PLISTEN;
		ev->call = on_ev_listen;
		ev->u = news;
		DWORD bytes = 0;
		memset(&ev->op, 0, sizeof(ev->op));
		SetLastError(0);
		setsockopt(news->fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&s->fd, sizeof(s->fd));
		if (!AcceptEx(s->fd, news->fd, ev->buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &bytes, &ev->op))
		{
			errno_type err = WSAGetLastError();
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
			e = WSAGetLastError();
			return 0;
		}

		int id = reserve_id();
		socket* s = new_fd(id, logic, fd, opt, true);
		if (!s)
		{
			e = WSAGetLastError();
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

	static void event_connect(void* data, event_head* head, size_t sz, errno_type err)
	{
		socket* s = (socket*)data;
		io_event* ev = (io_event*)head;
		socket_server& io = s->io;

		s->type = SOCKET_TYPE_CONNECTED;

		logic_connect* ev_logic = new logic_connect;
		ev_logic->id = s->id;
		ev_logic->err = err;
		if (io.post(s->logic, ev_logic))
		{
			io.force_close(s);
		}// todo 

		if (err == NO_ERROR)
		{
			io.ev_recv_start(s);
		}
		else
			;// todo: close socket;
	}

	int socket_server::start_connet(int logic, const char * host, int port, const socket_opt& opt, errno_type& err)
	{
		SetLastError(0);

		SOCKET fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (fd == INVALID_SOCKET)
		{
			err = WSAGetLastError();
			return 0;
		}

		SOCKADDR_IN localaddr;
		memset(&localaddr, 0, sizeof(localaddr));
		localaddr.sin_family = AF_INET;

		if (bind(fd, (SOCKADDR*)&localaddr, sizeof(localaddr)) == SOCKET_ERROR)
		{
			closesocket(fd);
			err = WSAGetLastError();
			return 0;
		}

		localaddr.sin_addr.s_addr = inet_addr(host);
		localaddr.sin_port = htons(port);

		int id = reserve_id();
		socket * s = new_fd(id, logic, fd, opt, true);
		if (!s)
		{
			err = WSAGetLastError();
			return 0;
		}
		s->type = SOCKET_TYPE_CONNECTING;
		s->op[socket_ev_read].call = event_connect;

		DWORD bytes = 0;
		BOOL result = ConnectEx(fd, (SOCKADDR*)&localaddr, sizeof(localaddr), 0, 0, &bytes, &s->op[socket_ev_read].op);
		if (!result)
		{
			err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				s->reset();
				return 0;
			}
		}
		err = NO_ERROR;
		return id;
	}

	static void on_ev_send(void* data, event_head* head, size_t sz, errno_type err)
	{
		socket* s = (socket*)data;
		io_event* ev = (io_event*)head;
		socket_server& io = s->io;
		if (err != NO_ERROR)
		{
			logic_socketerr* ev_logic = new logic_socketerr;
			ev_logic->id = s->id;
			ev_logic->err = err;
			if (io.post(s->logic, ev_logic))
			{
				io.force_close(s);
			}
		}
		else
		{
			ring_buffer* b = ev->b;
			b->read_ok(sz);
			if (b->empty())
			{
				ev->b = NULL;
				delete b;
			}
			InterlockedExchange(&ev->ready, 0);
			io.ev_send_start(s);
		}
	}

	errno_type socket_server::ev_send_start(socket* s)
	{
		io_event* ev = &s->op[socket_ev_write];
		ev->call = on_ev_send;
		if (InterlockedCompareExchange(&ev->ready, 1, 0)!= 0) return NO_ERROR;

		char* ptr;
		size_t sz;
		if (!ev->b)
		{
			ev->b = s->wb.pop_front();
		}
		if (ev->b == NULL)
		{
			InterlockedExchange(&ev->ready, 0);
			return 0;
		}

		if (!ev->b->readbuffer(&ptr, &sz))
			assert(false); // can't failure

		DWORD bytes_transferred = 0;
		DWORD send_flags = 0;// flags;

		WSABUF wsa;
		wsa.buf = ptr;
		wsa.len = sz;

		if (WSASend(s->fd, &wsa, 1, &bytes_transferred, send_flags, &ev->op, 0) != 0)
		{
			errno_type err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				force_close(s);
				return err;
			}
		}
		return NO_ERROR;
	}

	static void on_ev_recv(void* data, event_head* head, size_t sz, errno_type err)
	{
		socket* s = (socket*)data;
		io_event* ev = (io_event*)head;
		socket_server& io = s->io;

		ev->b->write_ok(sz);
		s->rb.push_back(ev->b);
		ev->b = NULL;

		logic_recv* ev_logic = NULL;
		if (s->opt.recv(s->id, &s->rb, &ev_logic) == false)
		{
			//parse error;
		}
		if (io.post(s->logic, ev_logic))
		{
			io.force_close(s);
		}
		InterlockedExchange(&ev->ready, 0);
		if (err != NO_ERROR)
		{
			logic_socketerr* ev_logic = new logic_socketerr;
			ev_logic->id = s->logic;
			ev_logic->err = err;
			if (io.post(s->logic, ev_logic))
			{
				io.force_close(s);
			}
		}
		else
		{
			io.ev_recv_start(s);
		}
	}

	errno_type socket_server::ev_recv_start(socket* s)
	{
		io_event * ev = &s->op[socket_ev_read];
		ev->call = on_ev_recv;
		if (InterlockedCompareExchange(&ev->ready, 1, 0) != 0) return NO_ERROR;
		ev->b = new ring_buffer;

		char* ptr;
		size_t sz;
		if (!ev->b->writebuffer(&ptr, &sz))
			assert(false);

		WSABUF wsa;
		wsa.len = sz;
		wsa.buf = ptr;

		DWORD bytes_transferred = 0;
		DWORD read_flags = 0;// flags;
		SetLastError(0);
		if (WSARecv(s->fd, &wsa, 1, &bytes_transferred, &read_flags, &ev->op, 0) != 0)
		{
			errno_type err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				s->reset();
				return err;
			}
		}
		return NO_ERROR;
	}

	errno_type socket_server::start_send(int id, char* data, size_t sz)
	{
		socket * s = getsocket(id);
		ring_buffer* b = new ring_buffer(data,sz);
		s->wb.push_back(b);
		return ev_send_start(s);
	}

	socket* socket_server::getsocket(int id)
	{
		socket * s = slot[HASH_ID(id)];
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

	errno_type socket_server::start_close(int fd)
	{
		return NULL;
	}

	/*	int socket_server::post_close(int id)
	{
	return 0;
	}
	*/

}

