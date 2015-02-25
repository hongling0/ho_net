#define  _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <winsock.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "corepoller.h"
#include "coreerrmsg.h"

struct core_poller
{
	HANDLE fd;
	uint8_t thr;
	OVERLAPPED quit;
	OVERLAPPED wake;
	volatile int quited;
	//long in_timer;
	//timer tm;
};

struct core_poller* corepoller_new(void)
{
	HANDLE h;
	struct core_poller* ret;
	DWORD last_error;

	SetLastError(0);
	h= CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	if (h == NULL) {
		last_error = GetLastError();
		fprintf(stderr, "corepoller_new CreateIoCompletionPort failure %s\n", errno_str(last_error));
		return NULL;
	}
	ret = (struct core_poller*)malloc(sizeof(*ret));
	if (ret == NULL) {
		fprintf(stderr, "corepoller_new malloc failure\n");
		return NULL;
	}
	ret->fd = h;
	ret->thr = 0;
	ret->quited = 1;
	return ret;
}

void corepoller_delete(struct core_poller* io)
{
	assert(io);
	CloseHandle(io->fd);
	free(io);
}

static void THREAD_START_ROUTINE(void* param)
{
	DWORD bytes;
	DWORD last_error;
	DWORD completion_key;
	LPOVERLAPPED op;
	BOOL ret;
	struct msghead* msg;

	struct core_poller* io = (struct core_poller*)param;
	assert(io);

	for (;;) {
		
		/*
		if (InterlockedCompareExchange(&in_timer, 1, 0) == 0) {
		tm.update(*this);
		InterlockedExchange(&in_timer, 0);
		}
		*/
		completion_key = 0;
		SetLastError(0);
		ret = GetQueuedCompletionStatus(io->fd, &bytes, &completion_key, &op, 500);
		last_error = GetLastError();
		if (ret != TRUE) {
			if (last_error == WAIT_TIMEOUT) {
				continue;
			}
		}
		if (op == &io->quit) {
			if (--bytes > 0) {
				PostQueuedCompletionStatus(io->fd, bytes, (ULONG_PTR)io, &io->quit);
			}
			else {
				io->quited = 1;
			}
			break;
		}
		else if (op) {
			msg = (struct msghead*)op;
			msg->call(io, (void*)completion_key, msg, bytes, last_error);
		}
		else {
			fprintf(stderr, "GetQueuedCompletionStatus %s\n", errno_str(last_error));
			assert(FALSE);
		}
	}
}

void corepoller_start_thread(struct core_poller* io, uint8_t n)
{
	assert(io);
	io->quited = 0;
	n = (n == 0) ? 1 : n;
	for (; io->thr < n; io->thr++) {
		_beginthread(THREAD_START_ROUTINE, 0, io);
	}
	if (io->thr>n) {
		PostQueuedCompletionStatus(io->fd, io->thr - n, (ULONG_PTR)io, &io->quit);
	}
	io->thr = n;
}

void corepoller_stop_thread(struct core_poller* io)
{
	assert(io);
	if (io->thr > 0) {
		PostQueuedCompletionStatus(io->fd, io->thr, (ULONG_PTR)io, &io->quit);
		io->thr = 0;
	}
	while (io->quited == 0) Sleep(1);
}
int corepoller_post(struct core_poller* io, void* ctx,struct msghead* ev, size_t bytes, int e)
{
	DWORD last_error;

	assert(io);
	if (io->thr == 0) {
		return -1;
	}
	SetLastError(0);
	PostQueuedCompletionStatus(io->fd, bytes, (ULONG_PTR)ctx, &ev->op);
	last_error = GetLastError();
	if (last_error == WSA_IO_PENDING) last_error = NO_ERROR;
	return last_error;
}
int corepoller_append_socket(struct core_poller* io, SOCKET s, void* context)
{
	HANDLE h;
	assert(io);
	h = CreateIoCompletionPort((HANDLE)s, io->fd, (ULONG_PTR)context, 0);
	if (h == NULL) {
		fprintf(stderr, "corepoller_new CreateIoCompletionPort failure %s\n", strerror(errno));
		return -1;
	}
	return 0;
}
