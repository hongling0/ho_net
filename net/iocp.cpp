


//#include <winsock.h>
//#include <WinSock2.h>
//#pragma comment(lib, "Ws2_32.lib")
#include "iocp.h"
#include "socket.h"
#include "atomic.h"
//#include <ws2tcpip.h>
//#include <ws2def.h>
//#include <mswsock.h>

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


namespace net
{

	struct startnetwork
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
	};

	startnetwork autoswitch;

	iocp::iocp()
	{
		SetLastError(0);
		event_fd = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (event_fd == 0)
		{
			fprintf(stderr, "iocp: CreateIoCompletionPort failed. %s\n", errno_str(GetLastError()));
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
			socket *s = &slot[i];
			if (s->type != SOCKET_TYPE_RESERVE) {
				s->reset();
			}
		}
		delete[] slot;
		CloseHandle(event_fd);
	}

	int iocp::reserve_id() 
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

	socket * iocp::new_fd(int id, socket_type fd, const socket_opt& opt, bool add)
	{
		socket * s = &slot[HASH_ID(id)];
		assert(s->type == SOCKET_TYPE_RESERVE);

		if (add) {
			SetLastError(0);
			if (!CreateIoCompletionPort((HANDLE)fd, event_fd, (ULONG_PTR)s, 0)) {
				s->type = SOCKET_TYPE_INVALID;
				return NULL;
			}
		}

		s->id = id;
		s->fd = fd;
		s->opt = opt;
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

	static void event_accept(iocp* poller, io_event* ev, socket* s, errno_type err, size_t sz)
	{
		LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockAddrs;
		GUID guid = WSAID_GETACCEPTEXSOCKADDRS;
		DWORD bytes = 0;
		SOCKADDR_IN* local_addr;
		SOCKADDR_IN* remote_addr;
		int locallen = sizeof(SOCKADDR_IN);
		int remotelen = sizeof(SOCKADDR_IN);
		WSAIoctl(s->fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid),&GetAcceptExSockAddrs, sizeof(GetAcceptExSockAddrs), &bytes, NULL, NULL);

		GetAcceptExSockAddrs(ev->buf, sizeof(ev->buf) - 2 * (sizeof(SOCKADDR_IN) + 16), sizeof(sockaddr_in) + 16,
			sizeof(sockaddr_in) + 16, (sockaddr**)&local_addr, &locallen, (sockaddr**)&remote_addr, &remotelen);

		printf("remote[%s:%d]->local[%s:%d]\n", inet_ntoa(local_addr->sin_addr),
			(int)ntohs(local_addr->sin_port), inet_ntoa(remote_addr->sin_addr), (int)ntohs(remote_addr->sin_port));

		socket * newsocket = (socket*)ev->u;
		newsocket->type = SOCKET_TYPE_PACCEPT;
		if (s->opt.accept)
			s->opt.accept(s, newsocket,err);
		errno_type dummy;
		poller->relisten(s, ev, dummy);
		poller->post_recv(newsocket, dummy);
	}

	int iocp::start_listen(const char * addr, int port, int backlog, const socket_opt& opt, errno_type& e)
	{
		SetLastError(0);
		SOCKET fd = do_listen(addr, port, backlog);
		if (fd == INVALID_SOCKET)
		{
			e = GetLastError();
			return 0;
		}
			
		int id = reserve_id();
		socket* s = new_fd(id, fd,opt, true);
		if (!s)
		{
			e = GetLastError();
			return 0;
		}
		s->type = SOCKET_TYPE_LISTEN;
		if (!relisten(s, &s->op[ioevent_read],e))
		{
			s->reset();
			return 0;
		}
		return id;
	}

	bool iocp::relisten(socket* s, io_event* ev,errno_type& err)
	{
		SetLastError(0);
		SOCKET fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (fd == INVALID_SOCKET) {
			err = GetLastError();
			return false;
		}
		socket* news = new_fd(reserve_id(), fd,s->opt,true);
		news->type = SOCKET_TYPE_PLISTEN;
		ev->call = event_accept;
		ev->u = news;
		DWORD bytes = 0;
		memset(&ev->op, 0, sizeof(ev->op));
		SetLastError(0);
		setsockopt(news->fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&s->fd, sizeof(s->fd));
		if (!AcceptEx(s->fd, news->fd, ev->buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &bytes, &ev->op))
		{
			err = GetLastError();
			if (err != WSA_IO_PENDING)
			{
				ev->u = NULL;
				news->reset();
				return false;
			}			
		}
		
		err = NO_ERROR;
		return true;
	}

	static DWORD WINAPI THREAD_START_ROUTINE(LPVOID part)
	{
		iocp * e = (iocp *)part;
		e->poll();
		return 0;
	}

	bool iocp::start_run(size_t t)
	{
		for (size_t i = 0; i < t; i++)
		{
			CreateThread(NULL, 0, &THREAD_START_ROUTINE, this, 0, NULL);
		}
		return true;
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
				if (last_error == WAIT_TIMEOUT) continue;// It was not an Time out event we wait for ever (INFINITE) 
				//TRACE(_T("GetQueuedCompletionStatus errorCode: %i, %s\n"), WSAGetLastError(), pThis->ErrorCode2Text(dwIOError));
				// if we have a pointer & This is not an shut down.. 
				//if (lpClientContext!=NULL && pThis->m_bShutDown == false)
				if (completion_key != NULL)
				{
					/*
					* ERROR_NETNAME_DELETED Happens when the communication socket
					* is cancelled and you have pendling WSASend/WSARead that are not finished.
					* Then the Pendling I/O (WSASend/WSARead etc..) is cancelled and we return with
					* ERROR_NETNAME_DELETED..
					*/
					if (last_error == ERROR_NETNAME_DELETED)
					{

						//TRACE("ERROR_NETNAME_DELETED\r\n");
//							force_close(context);
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
//					pOverlapBuff = NULL;
//					if (overlapped != NULL)
//						pOverlapBuff = CONTAINING_RECORD(overlapped, CIOCPBuffer, m_ol);
//					if (pOverlapBuff != NULL)
//						pThis->ReleaseBuffer(pOverlapBuff);
//					continue;
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

	static void event_connect(iocp* poller, io_event* ev, socket* s, errno_type err, size_t sz)
	{
		s->type = SOCKET_TYPE_CONNECTED;
		if (s->opt.connect)
			s->opt.connect(s,err);
		if (err == NO_ERROR)
		{
			errno_type dummy;
			poller->post_recv(s, dummy);
		}
		else
			;// todo: close socket;
	}

	int iocp::start_connet(const char * host, int port, const socket_opt& opt, errno_type& err)
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
		s->op[ioevent_read].call = event_connect;
		
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

	static void event_send(iocp* poller, io_event* ev, socket* s, errno_type err, size_t sz)
	{
		if (err!=NO_ERROR)
		{
			// todo: close
		}
		else
		{
			s->wb.read_ok(sz);
			errno_type dummy;
			poller->post_send(s, dummy);
		}
	}

	bool iocp::post_send(socket* s,errno_type& err)
	{
		char* ptr;
		size_t sz;
		if (!s->wb.readbuffer(&ptr, &sz))  return false;

		DWORD bytes_transferred = 0;
		DWORD send_flags = 0;// flags;
		io_event* ev = &s->op[ioevent_write];
		ev->wsa.buf = ptr;
		ev->wsa.len = sz;
		ev->call = event_send;
		if (WSASend(s->fd, &ev->wsa, 1, &bytes_transferred, send_flags, &ev->op, 0)!=0)
		{
			err = GetLastError();
			if (err != WSA_IO_PENDING)
			{
				s->reset();
				return false;
			}
		}
		return true;
	}

	static void event_recv(iocp* poller, io_event* ev, socket* s, errno_type err, size_t sz)
	{
		s->rb.write_ok(sz);
		s->opt.recv(s,err);
		if (err!=NO_ERROR)
		{
			// todo: close
		}
		else
		{
			errno_type dummy;
			poller->post_recv(s, dummy);
		}
	}

	bool iocp::post_recv(socket* s,errno_type& err)
	{
		
		char* ptr;
		size_t sz;
		if (!s->rb.writebuffer(&ptr, &sz))
			return false;

		io_event * ev = &s->op[ioevent_read];
		ev->call = event_recv;
		ev->wsa.len = sz;
		ev->wsa.buf = ptr;

   		DWORD bytes_transferred = 0;
		DWORD read_flags = 0;// flags;
		SetLastError(0);
		if (WSARecv(s->fd, &ev->wsa, 1, &bytes_transferred, &read_flags, &ev->op, 0)!=0)
		{
			err = GetLastError();
			if (err != WSA_IO_PENDING)
			{
				s->reset();
				return false;
			}
		}
		err = NO_ERROR;
		return true;
	}

	int iocp::send(int id, char* data, size_t sz,errno_type& err)
	{
		socket * s = getsocket(id);
		s->wb.write(data, sz);
		post_send(s,err);
		return 1;
	}

	socket* iocp::getsocket(int id)
	{
		socket * s = &slot[HASH_ID(id)];
		if (s->type != SOCKET_TYPE_INVALID&&s->id == id)
			return s;
		return NULL;
	}
	
	void iocp::force_close(socket * s)
	{
		shutdown(s->fd, SD_BOTH);
		if (s->op[ioevent_read].ready)
		{
			CancelIoEx((HANDLE)s->fd, &s->op[ioevent_read].op);
		}
		if (s->op[ioevent_write].ready)
		{
			CancelIoEx((HANDLE)s->fd, &s->op[ioevent_write].op);
		}
		s->reset();
	}

	int iocp::post_close(int id)
	{
		return 0;
	}
}