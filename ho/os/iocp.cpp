#include "iocp.h"
#include "atomic.h"

#include <ws2tcpip.h>
#include <mswsock.h>

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

struct write_buffer {
	struct write_buffer * next;
	char *ptr;
	int sz;
	void *buffer;
};

struct wb_list {
	struct write_buffer * head;
	struct write_buffer * tail;
};

static inline void
clear_wb_list(struct wb_list *list) {
	list->head = NULL;
	list->tail = NULL;
}



namespace net
{

	iocp::iocp()
	{
		event_fd = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (event_fd == 0)
		{
			fprintf(stderr, "socket-server: create event pool failed.\n");
			return;
		}

		slot = new socket[MAX_SOCKET];

		alloc_id = 0;
		event_n = 0;
		event_index = 0;
	}

	iocp::~iocp()
	{
		for (int i = 0; i < MAX_SOCKET; i++) {
			struct socket *s = &slot[i];
			if (s->type != SOCKET_TYPE_RESERVE) {
				force_close(s);
			}
		}
		delete[] slot;
		CloseHandle(event_fd);
	}

	int iocp::reserve_id() 
	{
		for (int i = 0; i < MAX_SOCKET; i++) {
			int id = sync_add_and_fetch(&alloc_id, 1);
			if (id < 0) {
				id = sync_add_and_fetch(&alloc_id, 0x7fffffff);
			}
			struct socket *s = &slot[HASH_ID(id)];
			if (s->type == SOCKET_TYPE_INVALID) {
				if (sync_bool_compare_and_swap(&s->type, SOCKET_TYPE_INVALID, SOCKET_TYPE_RESERVE)) {
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

	socket * iocp::new_fd(int id, int fd, uintptr_t opaque, bool add) {
		struct socket * s = &slot[HASH_ID(id)];
		assert(s->type == SOCKET_TYPE_RESERVE);

		if (add) {
			if (!CreateIoCompletionPort(s->fd, event_fd, NULL, 0)) {
				s->type = SOCKET_TYPE_INVALID;
				return NULL;
			}
		}

		s->id = id;
		s->fd = fd;
		s->size = MIN_READ_BUFFER;
		s->wb_size = 0;
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
		SOCKET listen_fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (listen_fd == INVALID_SOCKET) {
			return -1;
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
		return -1;
	}

	static void event_recv(iocp* poller, io_event* ev, socket* s, errno_t err, size_t sz)
	{
		socket * newsocket = (socket*)ev->u;
		poller->on_accept(s->id, newsocket->id);
		poller->relisten(s,ev);

		poller->post_recv(newsocket);
	}

	int iocp::start_listen(const char * addr, int port, int backlog, errno_t& e)
	{
		SetLastError(0);
		SOCKET fd = do_listen(addr, port, backlog);
		if (fd == INVALID_SOCKET)
		{
			e = GetLastError();
			return 0;
		}
			
		int id = reserve_id();
		socket* s = new_fd(id, fd, true);
		s->type = SOCKET_TYPE_LISTEN;
		s->op[0].call = event_recv;
		s->op[1].call = event_recv;
		relisten(s, &s->op[0]);
		relisten(s, &s->op[1]);
		return id;
	}

	bool iocp::relisten(socket* s, io_event* ev)
	{
		SOCKET fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (fd == INVALID_SOCKET) {
			return false;
		}
		socket* news = new_fd(reserve_id(), fd, false);
		ev->u = news;
		DWORD bytes = 0;
		memset(&ev->op, 0, sizeof(ev->op));
		AcceptEx(s->fd, news->fd,ev->buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &bytes, &ev->op);
	}

	static DWORD THREAD_START_ROUTINE(LPVOID part)
	{
		iocp * e = (iocp *)part;
		e->poll();
	}

	void iocp::poll()
	{
		bool error = false;
		while (!error)
		{
			DWORD bytes_transferred =0;
			PULONG_PTR completion_key = 0;
			LPOVERLAPPED overlapped =0;
			// Get a completed IO request.
			SetLastError(0);
			BOOL ret = ::GetQueuedCompletionStatus(event_fd, &bytes_transferred, (LPDWORD)&completion_key, &overlapped, INFINITE);
			DWORD last_error = ::GetLastError();
			if (overlapped)
			{
				socket * s = (socket*)completion_key;
				io_event* ev = CONTAINING_RECORD(overlapped, io_event, op);
				ev->call(this,ev,s,last_error,bytes_transferred);
			}
			if (!ret)
			{
				if (last_error != WAIT_TIMEOUT) // It was not an Time out event we wait for ever (INFINITE) 
				{
					//TRACE(_T("GetQueuedCompletionStatus errorCode: %i, %s\n"), WSAGetLastError(), pThis->ErrorCode2Text(dwIOError));
					// if we have a pointer & This is not an shut down.. 
					//if (lpClientContext!=NULL && pThis->m_bShutDown == false)
					if (context != NULL)
					{
						/*
						* ERROR_NETNAME_DELETED Happens when the communication socket
						* is cancelled and you have pendling WSASend/WSARead that are not finished.
						* Then the Pendling I/O (WSASend/WSARead etc..) is cancelled and we return with
						* ERROR_NETNAME_DELETED..
						*/
						if (dwIOError == ERROR_NETNAME_DELETED)
						{

							//TRACE("ERROR_NETNAME_DELETED\r\n");
							force_close(context);
							//TRACE(">IOWORKER1 (%x)\r\n", lpClientContext);
							//pThis->ReleaseClientContext(lpClientContext); //Later Implementati

						}
						else
						{ // Should not get here if we do: disconnect the client and cleanup & report. 

							//pThis->AppendLog(pThis->ErrorCode2Text(dwIOError));
							//pThis->DisconnectClient(lpClientContext);
							//TRACE(">IOWORKER2 (%x)\r\n", lpClientContext);
							//pThis->ReleaseClientContext(lpClientContext); //Should we do this ? 
						}
						// Clear the buffer if returned. 
						pOverlapBuff = NULL;
						if (lpOverlapped != NULL)
							pOverlapBuff = CONTAINING_RECORD(lpOverlapped, CIOCPBuffer, m_ol);
						if (pOverlapBuff != NULL)
							pThis->ReleaseBuffer(pOverlapBuff);
						continue;
					}
					// We shall never come here  
					// anyway this was an error and we should exit the worker thread
					//bError = true;
					//pThis->AppendLog(pThis->ErrorCode2Text(dwIOError));
					//pThis->AppendLog("IOWORKER KILLED BECAUSE OF ERROR IN GetQueuedCompletionStatus");

					//pOverlapBuff = NULL;
					//if (lpOverlapped != NULL)
					//	pOverlapBuff = CONTAINING_RECORD(lpOverlapped, CIOCPBuffer, m_ol);
					//if (pOverlapBuff != NULL)
					//	pThis->ReleaseBuffer(pOverlapBuff);
					continue;
				}
			}



	/*		if (bIORet && lpOverlapped && lpClientContext)
			{
				pOverlapBuff = CONTAINING_RECORD(lpOverlapped, CIOCPBuffer, m_ol);
				if (pOverlapBuff != NULL)
					pThis->ProcessIOMessage(pOverlapBuff, lpClientContext, dwIoSize);
			}

			if (lpClientContext == NULL&&pOverlapBuff == NULL&&pThis->m_bShutDown)
			{
				TRACE("lpClientContext==NULL \r\n");
				bError = true;
			}
	*/

			//pThis->ReleaseBuffer(pOverlapBuff);// from previous call

		}
	}

	static void event_connect(iocp* poller, io_event* ev, socket* s, errno_t err, size_t sz)
	{
		poller->on_connect(s->id);
		poller->post_recv(s);
	}

	int iocp::start_connet(const char * host, int port, errno_t& e)
	{
		LPFN_CONNECTEX ConnectEx = NULL;
		GUID GuidConnectEx = WSAID_CONNECTEX;

		uint32_t addr = inet_addr(host);

		SOCKADDR_IN ai_hints;
		memset(&ai_hints, 0, sizeof(ai_hints));
		ai_hints.sin_family = AF_INET;
		ai_hints.sin_addr.s_addr = addr;
		ai_hints.sin_port = htons(port);

		
		SOCKET fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

		int id = reserve_id();

		socket * s = new_fd(id, fd, true);
		
		DWORD bytes = 0;
		WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx), &ConnectEx, sizeof(ConnectEx), &bytes, NULL, NULL);

		s->type = SOCKET_TYPE_CONNECTING;
		s->op[0].call = NULL;
		BOOL result = ConnectEx(fd, (SOCKADDR*)&ai_hints, sizeof(ai_hints), 0, 0, 0, &s->op[1].op);
		
		return id;
	}

	bool async_recv(socket*);
	bool async_send(socket*);

	static void event_send(iocp* poller, io_event* ev, socket* s, errno_t err, size_t sz)
	{
		poller->on_send(s->id,sz);
		if (ev->wsa.len>sz)
			poller->resend(s, ev);
		else
			poller->post_send(s);
	}

	bool  iocp::resend(socket* s, io_event* ev)
	{
		DWORD bytes_transferred = 0;
		DWORD send_flags = 0;// flags;
		WSASend(s->fd, &ev->wsa, 1, &bytes_transferred, send_flags, &ev->op, 0);
		return true;
	}

	int iocp::post_send(socket* s)
	{
		write_buffer * buf = s->wb.head;
		if (!buf) return 0;
		s->wb.head = s->wb.head->next;

		io_event * ev = &s->op[ioevent_write];
		ev->call = event_send;
		ev->wsa.len = buf->sz - (buf->ptr - buf->buffer);
		ev->wsa.buf = buf->ptr;
		resend(s, ev);
		return 0;
	}

	static void event_read(iocp* poller, io_event* ev, socket* s, errno_t err, size_t sz)
	{
		poller->on_recv(s->id, sz);
		
		poller->post_recv(s);
	}

	int iocp::post_recv(socket* s)
	{
		io_event * ev = &s->op[ioevent_read];
		ev->call = event_read;
		ev->wsa.len = 0;// todo
		ev->wsa.buf = ev->buf;
		DWORD bytes_transferred = 0;
		DWORD read_flags = 0;// flags;
		WSARecv(s->fd, &ev->wsa, 1, &bytes_transferred, &read_flags, &ev->op, NULL);
	}

	int iocp::start_send(int id, void* data, size_t sz)
	{
		write_buffer * buf = (write_buffer * )malloc(sizeof(write_buffer) + sz);
		memcpy(buf + 1, data, sz);
		socket * s = getsocket(id);
		s->wb.tail->next = buf;
		s->wb.tail = buf;
		post_send(s);
		return 1;
	}
	
	int iocp::post_close(socket*)
	{

	}
}