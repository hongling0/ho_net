#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include <winsock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <winsock.h>

#include "socketinterface.h"
#include "corepoller.h"

#define MAX_SOCKET (1 << 16)
#define HASH_ID(id) ((id) & (MAX_SOCKET-1))


typedef struct socket
{
	int id;
	int logic;
	SOCKET fd;
	volatile uint8_t type;
	volatile uint8_t pending;
	volatile uint8_t closing;
	uint32_t w_byte;
	uint32_t r_byte;
	socket_opt opt;
	SOCKADDR_IN local;
	SOCKADDR_IN remote;
} socket;

typedef struct socketserver
{
	volatile uint32_t alloc_id;
	socket slot[MAX_SOCKET];
}socketserver;


static LPFN_TRANSMITFILE			TransmitFile;
static LPFN_ACCEPTEX				AcceptEx;
static LPFN_GETACCEPTEXSOCKADDRS	GetAcceptExSockaddrs;
static LPFN_TRANSMITPACKETS			TransmitPackets;
static LPFN_CONNECTEX				ConnectEx;
static LPFN_DISCONNECTEX			DisconnectEx;
static LPFN_WSARECVMSG				WSARecvMsg;

enum socket_type_enum
{
	SOCKET_TYPE_INVALID = 0,
	SOCKET_TYPE_RESERVE,
	SOCKET_TYPE_PLISTEN,
	SOCKET_TYPE_LISTEN,
	SOCKET_TYPE_CONNECTING,
	SOCKET_TYPE_CONNECTED,
	SOCKET_TYPE_HALFCLOSE,
	SOCKET_TYPE_PACCEPT,
	SOCKET_TYPE_BIND,
};

static int wsa_init()
{
	WSADATA data;
	SOCKET fd;
	DWORD bytes;

	GUID guidTransmitFile = WSAID_TRANSMITFILE;
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	GUID guidTransmitPackets = WSAID_TRANSMITPACKETS;
	GUID guidConnectEx = WSAID_CONNECTEX;
	GUID guidDisconnectEx = WSAID_DISCONNECTEX;
	GUID guidWSARecvMsg = WSAID_WSARECVMSG;

	WSAStartup((WORD)MAKELONG(1, 2), &data);

	fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidTransmitFile, sizeof(guidTransmitFile), &TransmitFile, sizeof(TransmitFile), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx), &AcceptEx, sizeof(AcceptEx), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAcceptExSockaddrs, sizeof(guidGetAcceptExSockaddrs), &GetAcceptExSockaddrs, sizeof(GetAcceptExSockaddrs), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidTransmitPackets, sizeof(guidTransmitPackets), &TransmitPackets, sizeof(TransmitPackets), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidTransmitFile, sizeof(guidTransmitFile), &TransmitFile, sizeof(TransmitFile), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx, sizeof(guidConnectEx), &ConnectEx, sizeof(ConnectEx), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidDisconnectEx, sizeof(guidDisconnectEx), &DisconnectEx, sizeof(DisconnectEx), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidWSARecvMsg, sizeof(guidWSARecvMsg), &WSARecvMsg, sizeof(WSARecvMsg), &bytes, NULL, NULL);

	closesocket(fd);

	return 0;
}

static int wsa_release()
{
	WSACleanup();
	return 0;
}

static socketserver SERVER;
struct core_poller* IO;

int start_socketserver(int thr)
{
	wsa_init();
	struct core_poller* io = IO;
	if (!io) {
		io = corepoller_new();
		if (!io) {
			return -1;
		}
		socketserver* ss = &SERVER;
		memset(ss, 0, sizeof(*ss));
		IO = io;
	}
	corepoller_start_thread(io, thr);
}

int stop_socketserver()
{
	socketserver* ss = &SERVER;
	assert(ss);

	for (int i = 0; i < MAX_SOCKET; i++) {
		socket *s = &ss->slot[i];
		if (s->type != SOCKET_TYPE_RESERVE) {
			force_close(s);
		}
	}
	wsa_release();
}

int start_listen(int logic, const char * addr, int port, int backlog, socket_opt* opt, int* id);
int start_connet(int logic, const char * addr, int port, socket_opt* opt, int* id);
int start_send(int fd, char* data, int sz);
int start_close(int fd);



		

	socket_server::~socket_server()
	{
		for (int i = 0; i < MAX_SOCKET; i++) {
			socket *s = slot[i];
			if (s->type != SOCKET_TYPE_RESERVE) {
				force_close(s);
			}
			delete s;
		}
	}

	int socket_server::reserve_id()
	{
		for (int i = 0; i < MAX_SOCKET; i++) {
			int id = InterlockedIncrement(&alloc_id);
			if (id < 0) {
				id = InterlockedAdd(&alloc_id, 0x7fffffff);
			}
			socket *s = slot[HASH_ID(id)];
			if (s->type == SOCKET_TYPE_RESERVE) {
				if (InterlockedCompareExchange(&s->type, SOCKET_TYPE_INVALID, SOCKET_TYPE_RESERVE) == SOCKET_TYPE_RESERVE) {
					s->id = id;
					s->fd = INVALID_SOCKET;
					s->pending = 1;
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
		assert(s->type == SOCKET_TYPE_INVALID);

		if (add) {
			WSASetLastError(0);
			io.append_socket(fd, s);
			errno_type err = WSAGetLastError();
			if (err) {
				s->type = SOCKET_TYPE_RESERVE;
				return NULL;
			}
		}

		s->id = id;
		s->fd = fd;
		s->opt = opt;
		s->logic = logic_id;
		s->pending = 1;
		return s;
	}

	bool socket_server::report_accept(int logic, int id, int listenid, errno_type err)
	{
		logic_accept* ev_logic = new logic_accept;
		ev_logic->id = err == NO_ERROR ? id : 0;
		ev_logic->listenid = listenid;
		ev_logic->err = err;
		return POST2LOGIC(logic, listenid, ev_logic);
	}

	bool socket_server::report_connect(int logic, int id, errno_type err)
	{
		logic_connect* ev_logic = new logic_connect;
		ev_logic->id = id;
		ev_logic->err = err;
		return POST2LOGIC(logic, id, ev_logic);
	}
	bool socket_server::report_socketerr(int logic, int id, errno_type err)
	{
		logic_socketerr* ev_logic = new logic_socketerr;
		ev_logic->id = id;
		ev_logic->err = err;
		return POST2LOGIC(logic, id, ev_logic);
	}

	bool socket_server::report_recv(int logic, int id, logic_recv* ev_logic)
	{
		return POST2LOGIC(logic, id, ev_logic);
	}

	static SOCKET do_listen(const char * host, int port, int backlog)
	{
		// only support ipv4
		// todo: support ipv6 by getaddrinfo
		uint32_t addr = INADDR_ANY;
		if (host[0]) {
			addr = inet_addr(host);
		}
		WSASetLastError(0);
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
		int id = s->id;
		assert(s->pending >= 1);

		socket * newsocket = (socket*)ev->u;
		if (err) {
			if (io.report_accept(s->logic, newsocket->id, id, err)) {
				io.dec_socket(newsocket->id);
			}
		}
		else {
			SOCKADDR_IN* local_addr;
			SOCKADDR_IN* remote_addr;
			int locallen = sizeof(SOCKADDR_IN);
			int remotelen = sizeof(SOCKADDR_IN);

			io.GetAcceptExSockaddrs(ev->buf, sizeof(ev->buf) - 2 * (sizeof(SOCKADDR_IN) + 16), sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16, (SOCKADDR**)&local_addr, &locallen, (SOCKADDR**)&remote_addr, &remotelen);

			memcpy(&s->local, local_addr, sizeof(SOCKADDR_IN));
			memcpy(&s->remote, remote_addr, sizeof(SOCKADDR_IN));
			//fprintf(stdout,"remote[%s:%d]->local[%s:%d]\n", inet_ntoa(local_addr->sin_addr),
			//(int)ntohs(local_addr->sin_port), inet_ntoa(remote_addr->sin_addr), (int)ntohs(remote_addr->sin_port));

			newsocket->type = SOCKET_TYPE_PACCEPT;
			if (!io.report_accept(s->logic, newsocket->id, s->id, 0))
				io.dec_socket(newsocket->id);
			else {
				int newid = newsocket->id;
				InterlockedIncrement(&newsocket->pending);
				int e = io.ev_recv_start(newsocket);
				if (e != NO_ERROR) {
					io.report_socketerr(newsocket->logic, newid, e);
					io.dec_socket(newid);
				}
			}
		}
		//InterlockedExchange(&ev->ready, 0);
		ev->ready = 0;

		if (InterlockedIncrement(&s->pending) == 2) return io.inner_close(s, 2);
		int e = io.ev_listen_start(s, ev);
		InterlockedDecrement(&s->pending);
		if (e != NO_ERROR)
			io.inner_close(s, 2);
	}

	errno_type socket_server::ev_listen_start(socket* s, io_event* ev)
	{
		//if (InterlockedCompareExchange(&ev->ready, 1, 0) != 0) return FRAME_IO_PENDING;
		ev->ready = 1;

		WSASetLastError(0);
		SOCKET fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		errno_type err = WSAGetLastError();
		if (fd == INVALID_SOCKET) {
			closesocket(fd);
			//InterlockedExchange(&ev->ready, 0);
			ev->ready = 0;
			report_socketerr(s->logic, s->id, err);
			return err;
		}

		socket* news = new_fd(reserve_id(), s->logic, fd, s->opt, true);
		news->type = SOCKET_TYPE_PLISTEN;
		ev->call = on_ev_listen;
		ev->u = news;

		memset(&ev->op, 0, sizeof(ev->op));

		WSASetLastError(0);
		DWORD bytes = 0;
		setsockopt(news->fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&s->fd, sizeof(s->fd));
		AcceptEx(s->fd, news->fd, ev->buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &bytes, &ev->op);
		err = WSAGetLastError();
		if (err && err != WSA_IO_PENDING) {
			ev->u = NULL;
			news->reset();
			//InterlockedExchange(&ev->ready, 0);
			ev->ready = 0;
			report_socketerr(s->logic, s->id, err);
			return err;
		}

		return NO_ERROR;
	}

	errno_type socket_server::start_listen(int logic, const char * addr, int port, int backlog, const socket_opt& opt, int& id)
	{
		WSASetLastError(0);
		SOCKET fd = do_listen(addr, port, backlog);
		if (fd == INVALID_SOCKET)
			return WSAGetLastError();

		id = reserve_id();
		socket* s = new_fd(id, logic, fd, opt, true);
		if (!s) {
			id = 0;
			inner_close(s);
			return WSAGetLastError();
		}
		s->type = SOCKET_TYPE_LISTEN;
		InterlockedIncrement(&s->pending);
		errno_type e = ev_listen_start(s, &s->op[socket_ev_read]);
		assert(e == NO_ERROR);
		InterlockedIncrement(&s->pending);
		e = ev_listen_start(s, &s->op[socket_ev_write]);
		assert(e == NO_ERROR);
		//if (e)
		//	inner_close(s);
		return NO_ERROR;
	}

	bool socket_server::post2logic(int id, int logic, logic_msg* ev, const char* flag, const char* file, int line)
	{
		int err = post(logic, ev);
		if (err) {
			fprintf(stderr, "|post2logic failure,start_close %d|%s|%s|%s:%d\n", id, flag, errno_str(err), file, line);
			//assert(false);
		}
		return err == NO_ERROR;
	}

	static void event_connect(void* data, event_head* head, size_t sz, errno_type err)
	{
		socket* s = (socket*)data;
		io_event* ev = (io_event*)head;
		socket_server& io = s->io;

		io.report_connect(s->logic, s->id, err);

		if (err == NO_ERROR) {
			s->type = SOCKET_TYPE_CONNECTED;
			//InterlockedExchange(&ev->ready, 0);
			ev->ready = 0;
			if (InterlockedIncrement(&s->pending) == 2) return io.inner_close(s, 2);
			int e = io.ev_recv_start(s);
			if (e != NO_ERROR)
				io.inner_close(s, 2);
		}
		else {
			io.inner_close(s, 1);
		}
	}

	errno_type socket_server::start_connet(int logic, const char * host, int port, const socket_opt& opt, int& id)
	{
		WSASetLastError(0);
		SOCKET fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		errno_type err = WSAGetLastError();
		if (fd == INVALID_SOCKET)
			return err;

		SOCKADDR_IN localaddr;
		memset(&localaddr, 0, sizeof(localaddr));
		localaddr.sin_family = AF_INET;

		WSASetLastError(0);
		if (bind(fd, (SOCKADDR*)&localaddr, sizeof(localaddr)) == SOCKET_ERROR) {
			closesocket(fd);
			err = WSAGetLastError();
			return err;
		}

		localaddr.sin_addr.s_addr = inet_addr(host);
		localaddr.sin_port = htons(port);

		id = reserve_id();
		socket * s = new_fd(id, logic, fd, opt, true);
		//fprintf(stdout, "|start_connet|%d|%d %p\n", logic, id, s);
		if (!s) {
			id = 0;
			inner_close(s);
			return WSAGetLastError();
		}

		s->type = SOCKET_TYPE_CONNECTING;
		s->op[socket_ev_read].call = event_connect;
		s->op[socket_ev_read].ready = 1;
		InterlockedIncrement(&s->pending);

		DWORD bytes = 0;
		WSASetLastError(0);
		ConnectEx(fd, (SOCKADDR*)&localaddr, sizeof(localaddr), 0, 0, &bytes, &s->op[socket_ev_read].op);
		err = WSAGetLastError();
		if (err&&err != WSA_IO_PENDING) {
			id = 0;
			inner_close(s);
			return err;
		}
		assert(id > 0);
		return NO_ERROR;
	}

	static void on_ev_send(void* data, event_head* head, size_t sz, errno_type err)
	{
		socket* s = (socket*)data;
		io_event* ev = (io_event*)head;
		socket_server& io = s->io;
		if (err != NO_ERROR) {
			InterlockedExchange(&ev->ready, 0);
			io.report_socketerr(s->logic, s->id, err);
			io.inner_close(s);
		}
		else {
			ring_buffer* b = ev->b;
			b->read_ok(sz);
			uint32_t lsz = s->w_byte;
			s->w_byte += sz;
			if (lsz / 1024 / 1024 / 80 != s->w_byte / 1024 / 1024 / 80) fprintf(stdout, "on_ev_send %d0 MB\n", s->w_byte / 1024 / 1024 / 80);
			if (b->empty()) {
				ev->b = NULL;
				delete b;
			}
			InterlockedExchange(&ev->ready, 0);

			if (InterlockedIncrement(&s->pending) == 2) return io.inner_close(s, 2);
			int e = io.ev_send_start(s);
			InterlockedDecrement(&s->pending);
			if (e != NO_ERROR && e != FRAME_IO_PENDING && e != FRAME_BUFF_EMPTY)
				io.inner_close(s, 1);
		}
	}

	errno_type socket_server::ev_send_start(socket* s)
	{
		io_event* ev = &s->op[socket_ev_write];
		ev->call = on_ev_send;
		if (InterlockedCompareExchange(&ev->ready, 1, 0) != 0) {
			if (InterlockedIncrement(&ev->ready) != 1) {
				return FRAME_IO_PENDING;
			}
		}

		char* ptr;
		size_t sz;
		ring_buffer* b = NULL;
		while (true) {
			if (!ev->b) {
				b = s->wb.pop_front();
				if (b == NULL) {
					if (InterlockedDecrement(&ev->ready) > 0)
						continue;
					return FRAME_BUFF_EMPTY;
				}
				ev->b = b;
			}
			break;
		}

		if (!ev->b->readbuffer(&ptr, &sz))
			assert(false); // can't failure

		DWORD bytes_transferred = 0;
		DWORD send_flags = 0;// flags;

		WSABUF wsa;
		wsa.buf = ptr;
		wsa.len = sz;

		WSASetLastError(0);
		WSASend(s->fd, &wsa, 1, &bytes_transferred, send_flags, &ev->op, 0);
		errno_type err = WSAGetLastError();
		if (err&&err != WSA_IO_PENDING) {
			report_socketerr(s->logic, s->id, err);
			return err;
		}
		return NO_ERROR;
	}

	static void on_ev_recv(void* data, event_head* head, size_t sz, errno_type err)
	{
		socket* s = (socket*)data;
		io_event* ev = (io_event*)head;
		socket_server& io = s->io;

		if (err == NO_ERROR&&sz>0) {
			ev->b->write_ok(sz);
			s->rb.push_back(ev->b);
			uint32_t lsz = s->r_byte;
			s->r_byte += sz;
			if (lsz / 1024 / 1024 / 80 != s->r_byte / 1024 / 1024 / 80) fprintf(stdout, "on_ev_recv %d MB\n", s->r_byte / 1024 / 1024 / 80);
			ev->b = NULL;
			logic_recv* ev_logic = NULL;
			if (s->opt.recv(s->id, &s->rb, &ev_logic) == false) {
				//InterlockedExchange(&ev->ready, 0);
				ev->ready = 0;
				io.report_socketerr(s->logic, s->id, FRAME_IO_PARSEERR);
				return io.inner_close(s);
			}
			io.report_recv(s->logic, s->id, ev_logic);
			//InterlockedExchange(&ev->ready, 0);
			ev->ready = 0;

			if (InterlockedIncrement(&s->pending) == 2) return io.inner_close(s, 2);
			int e = io.ev_recv_start(s);
			InterlockedDecrement(&s->pending);
			if (e != NO_ERROR)
				io.inner_close(s);
		}
		else {
			io.report_socketerr(s->logic, s->id, err);
			io.inner_close(s);
		}
	}

	errno_type socket_server::ev_recv_start(socket* s)
	{
		io_event * ev = &s->op[socket_ev_read];
		ev->call = on_ev_recv;
		//if (InterlockedCompareExchange(&ev->ready, 1, 0) != 0) return FRAME_IO_PENDING;
		assert(ev->ready == 0);
		ev->ready = 1;
		assert(ev->b == NULL);
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
		WSASetLastError(0);
		WSARecv(s->fd, &wsa, 1, &bytes_transferred, &read_flags, &ev->op, 0);
		errno_type err = WSAGetLastError();
		if (err&&err != WSA_IO_PENDING) {
			report_socketerr(s->logic, s->id, err);
			ev->ready = 0;
			return err;
		}
		return NO_ERROR;
	}

	errno_type socket_server::start_send(int id, char* data, size_t sz)
	{
		socket * s = grub_socket(id);
		if (!s)
			return FRAME_INVALID_SOCKET;
		s->wb.push_back(new ring_buffer(data, sz));
		int e = ev_send_start(s);
		if (e != NO_ERROR && e != FRAME_IO_PENDING && e != FRAME_BUFF_EMPTY)
			dec_socket(id);
		return e;
	}

	socket* socket_server::grub_socket(int id)
	{
		socket * s = slot[HASH_ID(id)];
		InterlockedIncrement(&s->pending);
		if (s->type != SOCKET_TYPE_INVALID&&s->id == id) {
			return s;
		}
		inner_close(s, 1);
		return NULL;
	}

	void socket_server::dec_socket(int id)
	{
		socket * s = slot[HASH_ID(id)];
		if (s->type != SOCKET_TYPE_INVALID&&s->id == id) {
			if (InterlockedDecrement(&s->pending) == 0)
				force_close(s);
		}
	}


	void socket_server::force_close(socket * s)
	{
		shutdown(s->fd, SD_BOTH);
		if (InterlockedCompareExchange(&s->op[socket_ev_read].ready, 0, 1)) {
			CancelIoEx((HANDLE)s->fd, &s->op[socket_ev_read].op);
		}
		if (InterlockedCompareExchange(&s->op[socket_ev_write].ready, 0, 1)) {
			CancelIoEx((HANDLE)s->fd, &s->op[socket_ev_write].op);
		}
		s->reset();
	}

	void socket_server::inner_close(socket *s, unsigned ref)
	{
		if (InterlockedExchangeAdd(&s->pending, 0 - ref) <= (int)ref)
			force_close(s);
	}

	errno_type socket_server::start_close(int id)
	{
		socket * s = grub_socket(id);
		if (!s) return FRAME_INVALID_SOCKET;
		if (s->type != SOCKET_TYPE_PLISTEN
			&& s->type != SOCKET_TYPE_LISTEN
			&& s->type != SOCKET_TYPE_CONNECTING
			&& s->type != SOCKET_TYPE_CONNECTED
			&& s->type != SOCKET_TYPE_PACCEPT)
			return FRAME_INVALID_SOCKET_TYPE;
		if (!s->closing) {
			s->closing = true;
			inner_close(s, 2);
		}
		else
			inner_close(s, 1);
		return NO_ERROR;
	}
}