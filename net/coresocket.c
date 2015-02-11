#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include <winsock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <winsock.h>

#include "coresocket.h"
#include "corepoller.h"
#include "corebuffer.h"

#define MAX_SOCKET (1 << 16)
#define HASH_ID(id) ((id) & (MAX_SOCKET-1))
#define get_ss() IO?&SERVER:NULL;assert(IO)
#define get_io() IO;

#define check_warpcall(func,s,ctx,err) \
	do{\
		if (socketcheck_addref(s) == 0) {\
			err = func(s, ctx);\
			if (err != NO_ERROR) {\
				report_socketerr(s->logic, s->id, err);\
						}\
			socketcheck_delref(s, err == NO_ERROR ? 1 : 2);\
						}\
			}while(0)

struct socket;
typedef struct socket_ctx
{
	coremsg_head;
	volatile long ready;
	union {
		char buf[2 * (sizeof(struct sockaddr_in) + 16)];
		struct socket* newsock;
		struct buffer_node* buffer;
	};
} socket_ctx;

typedef struct socket
{
	int id;
	int logic;
	SOCKET fd;
	struct socket_ctx read_op;
	struct socket_ctx write_op;
	volatile long type;
	volatile long pending;
	volatile long closing;
	unsigned long r_byte;
	unsigned long w_byte;
	unsigned long read_size;
	corebuffer read_buf;
	corebuffer write_buf;
	SOCKADDR_IN local;
	SOCKADDR_IN remote;
} ;

typedef struct socketserver
{
	volatile uint32_t alloc_id;
	struct socket slot[MAX_SOCKET];
} socketserver;

static socketserver SERVER;
static struct core_poller* IO;

static LPFN_TRANSMITFILE			sTransmitFile;
static LPFN_ACCEPTEX				sAcceptEx;
static LPFN_GETACCEPTEXSOCKADDRS	sGetAcceptExSockaddrs;
static LPFN_TRANSMITPACKETS			sTransmitPackets;
static LPFN_CONNECTEX				sConnectEx;
static LPFN_DISCONNECTEX			sDisconnectEx;
static LPFN_WSARECVMSG				sWSARecvMsg;

enum
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
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidTransmitFile, sizeof(guidTransmitFile), &sTransmitFile, sizeof(sTransmitFile), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx), &sAcceptEx, sizeof(sAcceptEx), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAcceptExSockaddrs, sizeof(guidGetAcceptExSockaddrs), &sGetAcceptExSockaddrs, sizeof(sGetAcceptExSockaddrs), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidTransmitPackets, sizeof(guidTransmitPackets), &sTransmitPackets, sizeof(sTransmitPackets), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidTransmitFile, sizeof(guidTransmitFile), &sTransmitFile, sizeof(sTransmitFile), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx, sizeof(guidConnectEx), &sConnectEx, sizeof(sConnectEx), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidDisconnectEx, sizeof(guidDisconnectEx), &sDisconnectEx, sizeof(sDisconnectEx), &bytes, NULL, NULL);
	WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidWSARecvMsg, sizeof(guidWSARecvMsg), &sWSARecvMsg, sizeof(sWSARecvMsg), &bytes, NULL, NULL);
	closesocket(fd);

	return 0;
}


static void resetfd(struct socket* s)
{
	assert(0);
}

static void force_close(struct socket * s)
{
	socketserver* ss = get_ss();
	if (InterlockedCompareExchange(&s->read_op.ready, 0, 1)) {
		CancelIoEx((HANDLE)s->fd, &s->read_op.op);
	}
	if (InterlockedCompareExchange(&s->write_op.ready, 0, 1)) {
		CancelIoEx((HANDLE)s->fd, &s->write_op.op);
	}
	shutdown(s->fd, SD_BOTH);
	resetfd(s);
}

static int socketcheck_addref(struct socket* s)
{
	if (s->type != SOCKET_TYPE_INVALID) {
		InterlockedIncrement(&s->pending);
		return 0; // todo
	}
	return 0;
}

static int socketcheck_delref(struct socket*s, int ref)
{
	if (InterlockedExchangeAdd(&s->pending, 0 - ref) <= (int)ref)
		force_close(s);
	return 0;
}

static int wsa_release()
{
	WSACleanup();
	return 0;
}

static void reset_socket_ctx(struct socket_ctx* ctx)
{
	memset(ctx, 0, sizeof(*ctx));
}

static struct socket* grub_socket(int id)
{
	socketserver* ss = get_ss();
	struct socket * s = &ss->slot[HASH_ID(id)];
	InterlockedIncrement(&s->pending);
	if (s->type != SOCKET_TYPE_INVALID&&s->id == id) {
		return s;
	}
	socketcheck_delref(s, 1);
	return NULL;
}

static int report_accept(int logic, int newid, int id, int e)
{
	// todo
	return 0;
}
int report_socketerr(int logic, int id, int err)
{
	// todo
	return 0;
}
int report_connect(int logic, int id, int e)
{
	// todo
	return 0;
}

int start_socketserver(int thr)
{
	struct core_poller* io;
	socketserver* ss;
	wsa_init();
	io = IO;
	if (!io) {
		io = corepoller_new();
		if (!io) {
			return -1;
		}
		ss = &SERVER;
		memset(ss, 0, sizeof(*ss));
		IO = io;
	}
	corepoller_start_thread(io, thr);
	return 0;
}

int stop_socketserver()
{
	struct socket *s;
	socketserver* ss = get_ss();
	for (int i = 0; i < MAX_SOCKET; i++) {
		s = &ss->slot[i];
		if (s->type != SOCKET_TYPE_RESERVE) {
			force_close(s);
		}
	}
	wsa_release();
	return 0;
}

static SOCKET do_listen(const char * host, int port, int backlog)
{
	// only support ipv4
	// todo: support ipv6 by getaddrinfo
	int reuse;
	uint32_t addr;
	SOCKET listen_fd;
	struct sockaddr_in my_addr;

	addr = INADDR_ANY;
	if (host[0]) {
		addr = inet_addr(host);
	}
	WSASetLastError(0);
	listen_fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (listen_fd == INVALID_SOCKET) {
		return INVALID_SOCKET;
	}
	reuse = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(int)) == -1) {
		goto _failed;
	}

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

static int reserve_id()
{
	int i, id;
	struct socket *sock;
	socketserver* ss = get_ss();

	for (i = 0; i < MAX_SOCKET; i++) {
		id = InterlockedIncrement(&ss->alloc_id);
		if (id < 0) {
			id = InterlockedAdd(&ss->alloc_id, 0x7fffffff);
		}
		sock = &ss->slot[HASH_ID(id)];
		if (sock->type == SOCKET_TYPE_RESERVE) {
			if (InterlockedCompareExchange(&sock->type, SOCKET_TYPE_INVALID, SOCKET_TYPE_RESERVE)
				== SOCKET_TYPE_RESERVE) {
				sock->id = id;
				sock->fd = INVALID_SOCKET;
				sock->pending = 1;
				return id;
			} else { // retry
				i--;
			}
		}
	}
	return -1;
}

static struct socket * new_fd(int id, int logic_id, SOCKET fd, int add)
{
	struct core_poller *io;
	socketserver* ss;
	struct socket * s;
	int err;

	io = get_io();
	ss = get_ss();
	s = &ss->slot[HASH_ID(id)];
	assert(s->type == SOCKET_TYPE_INVALID);

	if (add) {
		WSASetLastError(0);
		corepoller_append_socket(io, fd, s);
		err = WSAGetLastError();
		if (err) {
			s->type = SOCKET_TYPE_RESERVE;
			return NULL;
		}
	}

	s->id = id;
	s->fd = fd;
	s->logic = logic_id;
	s->pending = 1;
	return s;
}

static int ev_listen_start(struct socket* s, struct socket_ctx* ctx);
static int ev_recv_start(struct socket* s, struct socket_ctx* ctx);
static void on_ev_listen(struct core_poller* io, void* data, struct msghead* msg, size_t sz, int e)
{
	struct socket *newsock, *s;
	struct socket_ctx* ctx;
	SOCKADDR_IN* local_addr, *remote_addr;
	int locallen, remotelen, id, err;

	ctx = (struct socket_ctx*)msg;
	s = (struct socket*)data;
	id = s->id;
	newsock = ctx->newsock;
	assert(s->pending >= 1 && newsock);

	if (e) {
		report_accept(s->logic, newsock->id, id, e);
	} else {
		locallen = sizeof(SOCKADDR_IN);
		remotelen = sizeof(SOCKADDR_IN);
		GetAcceptExSockaddrs(ctx->buf,
			sizeof(ctx->buf) - 2 * (sizeof(SOCKADDR_IN) + 16),
			sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16,
			(SOCKADDR**)&local_addr, &locallen,
			(SOCKADDR**)&remote_addr, &remotelen);

		memcpy(&s->local, local_addr, sizeof(SOCKADDR_IN));
		memcpy(&s->remote, remote_addr, sizeof(SOCKADDR_IN));
		//fprintf(stdout,"remote[%s:%d]->local[%s:%d]\n", inet_ntoa(local_addr->sin_addr),
		//(int)ntohs(local_addr->sin_port), inet_ntoa(remote_addr->sin_addr), (int)ntohs(remote_addr->sin_port));

		newsock->type = SOCKET_TYPE_PACCEPT;
		report_accept(s->logic, newsock->id, s->id, 0);
		InterlockedIncrement(&newsock->pending);

		check_warpcall(ev_recv_start, newsock, &newsock->read_op, err);
	}

	InterlockedExchange(&ctx->ready, 0);

	check_warpcall(ev_listen_start, s, ctx, err);
}

static int ev_listen_start(struct socket* s, struct socket_ctx* ctx)
{
	int err;
	SOCKET fd;
	DWORD bytes;
	struct socket* news;

	WSASetLastError(0);
	fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	err = WSAGetLastError();
	if (fd == INVALID_SOCKET) {
		ctx->ready = 0;
		return err;
	}

	news = new_fd(reserve_id(), s->logic, fd, 1);
	if (!news) {
		ctx->ready = 0;
		err = -1;// todo no news can allocated
		resetfd(news);
		return err;
	}
	news->type = SOCKET_TYPE_PLISTEN;
	
	reset_socket_ctx(ctx);
	ctx->ready = 1;
	ctx->call = on_ev_listen;

	WSASetLastError(0);
	bytes = 0;
	setsockopt(news->fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&s->fd, sizeof(s->fd));
	sAcceptEx(s->fd, news->fd, ctx->buf, 0, sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16, &bytes, &ctx->op);
	err = WSAGetLastError();
	if (err && err != WSA_IO_PENDING) {
		resetfd(news);
		ctx->ready = 0;
		return err;
	}
	ctx->ready = 1;
	return NO_ERROR;
}

int start_listen(int logic, const char * addr, int port, int backlog, socket_opt* opt, int* id)
{
	SOCKET fd;
	struct socket* sock;
	int err;
	socketserver* ss = &SERVER;
	WSASetLastError(0);
	fd = do_listen(addr, port, backlog);
	if (fd == INVALID_SOCKET) {
		id = 0;
		return WSAGetLastError();
	}

	*id = reserve_id();
	sock = new_fd(*id, logic, fd, 1);
	if (!sock) {
		*id = 0;
		resetfd(sock);
		return WSAGetLastError();
	}

	// todo double listen op
	sock->type = SOCKET_TYPE_LISTEN;
	if (socketcheck_addref(sock) == 0) {
		err = ev_listen_start(sock, &sock->read_op);
		socketcheck_delref(sock, err == NO_ERROR ? 1 : 2);
		if (err == NO_ERROR&&socketcheck_addref(sock) == 0) {
			err = ev_listen_start(sock, &sock->write_op);
			socketcheck_delref(sock, err == NO_ERROR ? 1 : 2);
		}
	}
	return err;
}

static void event_connect(struct core_poller* io,void* data, struct msghead* msg, size_t sz, int e)
{
	int err;
	struct socket_ctx* ctx = (struct socket_ctx*)msg;
	struct socket* s = (struct socket*)data;
	ctx->ready = 0;
	report_connect(s->logic, s->id, e);
	if (e == NO_ERROR) {
		s->type = SOCKET_TYPE_CONNECTED;
		check_warpcall(ev_recv_start, s, ctx, err);
	} else {
		socketcheck_delref(s, 1);
	}
}

int start_connet(int logic, const char * addr, int port, socket_opt* opt, int* id)
{
	int err;
	struct socket* s;
	SOCKET fd;
	DWORD bytes;
	SOCKADDR_IN localaddr;

	WSASetLastError(0);
	fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	err = WSAGetLastError();
	if (fd == INVALID_SOCKET)
		return err;

	memset(&localaddr, 0, sizeof(localaddr));
	localaddr.sin_family = AF_INET;

	WSASetLastError(0);
	if (bind(fd, (SOCKADDR*)&localaddr, sizeof(localaddr)) == SOCKET_ERROR) {
		closesocket(fd);
		err = WSAGetLastError();
		return err;
	}

	localaddr.sin_addr.s_addr = inet_addr(addr);
	localaddr.sin_port = htons(port);

	s = new_fd(reserve_id(), logic, fd, 1);
	if (!s) {
		resetfd(s);
		return WSAGetLastError();
	}

	*id = s->id;
	s->type = SOCKET_TYPE_CONNECTING;
	reset_socket_ctx(&s->read_op);
	s->read_op.ready = 1;
	s->read_op.call = event_connect;

	InterlockedIncrement(&s->pending);

	bytes = 0;
	WSASetLastError(0);
	sConnectEx(fd, (SOCKADDR*)&localaddr, sizeof(localaddr), 0, 0, &bytes, &s->read_op.op);
	err = WSAGetLastError();
	if (err&&err != WSA_IO_PENDING) {
		*id = 0;
		resetfd(s);
		return err;
	}
	assert(*id > 0);
	return NO_ERROR;
}

static int ev_send_start(struct socket* s, struct socket_ctx* ctx);
int start_send(int fd, char* data, unsigned long sz)
{
	int err;
	struct socket* s = grub_socket(fd);
	struct buffer_node* node = buffernode_new(sz);
	buffernode_write(node, data, sz);
	corebuffer_pushback_safe(&s->read_buf, node);
	err = ev_send_start(s, &s->write_op);
	socketcheck_delref(s, err == NO_ERROR ? 0 : 1);
	return err;
}

static void on_ev_send(struct core_poller* io,void* data, struct msghead* msg, size_t sz, int e)
{
	struct buffer_node* node;
	uint32_t lsz;
	int err;
	struct socket_ctx* ctx = (struct socket_ctx*)msg;
	struct socket* s = (struct socket*)data;
	if (e != NO_ERROR) {
		InterlockedExchange(&ctx->ready, 0);
		report_socketerr(s->logic, s->id, e);
		socketcheck_delref(s, 1);
	} else {
		node = ctx->buffer;
		buffernode_readok(node, sz);
		lsz = s->w_byte;
		s->w_byte += sz;
		if (lsz / 1024 / 1024 / 80 != s->w_byte / 1024 / 1024 / 80)
			fprintf(stdout, "on_ev_send %d0 MB\n", s->w_byte / 1024 / 1024 / 80);

		if (buffernode_canread(node)==0) {
			ctx->buffer = NULL;
			buffernode_delete(node);
		}
		InterlockedExchange(&ctx->ready, 0);
		check_warpcall(ev_send_start, s, ctx, err);
	}
}

static int ev_send_start(struct socket* s, struct socket_ctx* ctx)
{
	DWORD bytes_transferred, send_flags;
	WSABUF wsa;
	int err;

	if (InterlockedCompareExchange(&ctx->ready, 1, 0) != 0) {
		if (InterlockedIncrement(&ctx->ready) != 1) {
			return NO_ERROR;// todo FRAME_IO_PENDING;
		}
	}

	if (ctx->buffer == NULL) {
		ctx->buffer = corebuffer_popfront_safe(&s->write_buf);
	}
	wsa.len = buffernode_startread(ctx->buffer, &wsa.buf);
	assert(wsa.len > 0);

	reset_socket_ctx(ctx);
	ctx->call = on_ev_send;

	send_flags = 0;
	WSASetLastError(0);
	WSASend(s->fd, &wsa, 1, &bytes_transferred, send_flags, &ctx->op, 0);
	err = WSAGetLastError();
	if (err&&err != WSA_IO_PENDING) {
		return err;
	}
	return NO_ERROR;
}

static void on_ev_recv(struct core_poller* io, void* data, struct msghead* msg, size_t sz, int e)
{
	struct buffer_node* node;
	unsigned long lsz;
	int err;
	struct socket_ctx* ctx = (struct socket_ctx*)msg;
	struct socket* s = (struct socket*)data;

	node = ctx->buffer;
	assert(node);
	err = buffernode_readok(node, sz);
	assert(err == 0);
	corebuffer_pushback(&s->read_buf, node);
	ctx->buffer = NULL;
	if (e == NO_ERROR&&sz>0) {
		lsz = s->r_byte;
		s->r_byte += sz;
		s->read_size = (7 * s->read_size + sz) >>2;
		if (lsz / 1024 / 1024 / 80 != s->r_byte / 1024 / 1024 / 80)
			fprintf(stdout, "on_ev_recv %d MB\n", s->r_byte / 1024 / 1024 / 80);

		//logic_recv* ev_logic = NULL;
		//if (s->opt.recv(s->id, &s->rb, &ev_logic) == false) {
		//InterlockedExchange(&ev->ready, 0);
		//	ev->ready = 0;
		//	io.report_socketerr(s->logic, s->id, FRAME_IO_PARSEERR);
		//	return io.inner_close(s);
		//}
		//io.report_recv(s->logic, s->id, ev_logic);
		//InterlockedExchange(&ev->ready, 0);
		ctx->ready = 0;

		if (socketcheck_addref(s) == 0) {
			err = ev_recv_start(s, &s->read_op); // todo
			if (err != NO_ERROR) {
				report_socketerr(s->logic, s->id, err);
			}
			socketcheck_delref(s, err == NO_ERROR ? 1 : 2);
		}
	} else {
		report_socketerr(s->logic, s->id, err);
		socketcheck_delref(s, 1);
	}
}

static int ev_recv_start(struct socket* s, struct socket_ctx* ctx)
{
	DWORD bytes_transferred, read_flags;
	WSABUF wsa;
	int err;

	if (InterlockedCompareExchange(&ctx->ready, 1, 0) != 0) {
		assert(0);// todo
	}

	reset_socket_ctx(ctx);
	ctx->ready = 1;
	ctx->call = on_ev_recv;
	ctx->buffer = buffernode_new(s->read_size);

	wsa.len = buffernode_startwrite(ctx->buffer, &wsa.buf);
	assert(wsa.len);

	bytes_transferred = 0;
	read_flags = 0;
	WSASetLastError(0);
	WSARecv(s->fd, &wsa, 1, &bytes_transferred, &read_flags, &ctx->op, 0);
	err = WSAGetLastError();
	if (err&&err != WSA_IO_PENDING) {
		ctx->ready = 0;
		return err;
	}
	return NO_ERROR;
}

int start_close(int fd)
{
	struct socket * s = grub_socket(fd);
	if (!s) return -1;// FRAME_INVALID_SOCKET;
	if (s->type != SOCKET_TYPE_PLISTEN
		&& s->type != SOCKET_TYPE_LISTEN
		&& s->type != SOCKET_TYPE_CONNECTING
		&& s->type != SOCKET_TYPE_CONNECTED
		&& s->type != SOCKET_TYPE_PACCEPT)
		return -1;// FRAME_INVALID_SOCKET_TYPE;
	if (!s->closing) {
		s->closing = 1;
		socketcheck_delref(s, 2);
	} else {
		socketcheck_delref(s, 1);
	}
	return NO_ERROR;
}