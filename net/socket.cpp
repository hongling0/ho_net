#include "iocp.h"
#include "atomic.h"

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
		s->opaque = opaque;
		s->wb_size = 0;
		return s;
	}


	static socket_t do_listen(const char * host, int port, int backlog)
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

	int iocp::post_listen(uintptr_t opaque, const char * addr, int port, int backlog)
	{
		socket_t fd = do_listen(addr, port, backlog);
		if (fd==INVALID_SOCKET)
			return WSAGetLastError();
		PostQueuedCompletionStatus(event_fd, 0, NULL, );
	}

	static int
		listen_socket(struct socket_server *ss, struct request_listen * request, struct socket_message *result) {
		int id = request->id;
		int listen_fd = request->fd;
		struct socket *s = new_fd(ss, id, listen_fd, request->opaque, false);
		if (s == NULL) {
			goto _failed;
		}
		s->type = SOCKET_TYPE_PLISTEN;
		return -1;
	_failed:
		close(listen_fd);
		result->opaque = request->opaque;
		result->id = id;
		result->ud = 0;
		result->data = NULL;
		ss->slot[HASH_ID(id)].type = SOCKET_TYPE_INVALID;

		return SOCKET_ERROR;
	}

	static DWORD THREAD_START_ROUTINE(LPVOID part)
	{
		iocp * e = (iocp *)part;
		e->run();
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
				io_operation * op = (io_operation*)completion_key;
				op->call(this, last_error, bytes_transferred);
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



			if (bIORet && lpOverlapped && lpClientContext)
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


			//pThis->ReleaseBuffer(pOverlapBuff);// from previous call

		}
	}

	int iocp::async_send(socket_t s, const buf_t* bufs, size_t count,errno_t& e)
	{
		DWORD send_buf_count = static_cast<DWORD>(count);
		DWORD bytes_transferred = 0;
		DWORD send_flags = 0;// flags;
		e = ::WSASend(s, const_cast<buf_t *>(bufs),send_buf_count, &bytes_transferred, send_flags, 0, 0);
		if (e != 0)
			return SOCKET_ERROR;
		return bytes_transferred;
	}
}