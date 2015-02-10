#include "typedef.h"
#include "lock.h"
#include "logic.h"
#include "poller.h"

using namespace sys;
namespace frame
{
	static void THREAD_START_ROUTINE(LPVOID lpThreadParameter)
	{
		iocp* p = (iocp*)lpThreadParameter;
		p->run();
	}

	iocp::iocp()
	{
		fd = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
		thr = 0;
		quited = 1;
	}
	iocp::~iocp()
	{
		CloseHandle(fd);
	}
	void iocp::start_thread(uint8_t n)
	{
		quited = 0;
		n = (n == 0) ? 1 : n;
		for (; thr < n; thr++) {
			_beginthread(THREAD_START_ROUTINE, 0, this);
		}
		if (thr>n) {
			PostQueuedCompletionStatus(fd, thr - n, (ULONG_PTR)this, &quit);
		}
		thr = n;
	}
	void iocp::stop_thread()
	{
		if (thr > 0) {
			PostQueuedCompletionStatus(fd, thr, (ULONG_PTR)this, &quit);
			thr = 0;
		}
		while (quited == 0) Sleep(1);
	}

	void iocp::run()
	{
		for (;;) {
			DWORD bytes;
			DWORD completion_key = 0;
			LPOVERLAPPED op;

			if (InterlockedCompareExchange(&in_timer, 1, 0) == 0) {
				tm.update(*this);
				InterlockedExchange(&in_timer, 0);
			}

			SetLastError(0);
			BOOL ret = GetQueuedCompletionStatus(fd, &bytes, &completion_key, &op, 500);
			DWORD last_error = ::GetLastError();
			if (ret != TRUE) {
				if (last_error == WAIT_TIMEOUT)
					continue;
			}
			if (op == &quit) {
				if (--bytes > 0)
					PostQueuedCompletionStatus(fd, bytes, (ULONG_PTR)this, &quit);
				else
					quited = 1;
				break;
			} else if (op) {
				event_head* ev = (event_head*)op;
				ev->call((void*)completion_key, ev, bytes, last_error);
			} else {
				fprintf(stderr, "GetQueuedCompletionStatus %s\n", errno_str(last_error));
				assert(false);
			}
		}
	}
	int iocp::post(void* context, event_head* ev, size_t bytes, errno_type e)
	{
		if (thr == 0)
			return FRAME_POLLER_NOT_RUN;
		SetLastError(0);
		PostQueuedCompletionStatus(fd, bytes, (ULONG_PTR)context, &ev->op);
		DWORD last_error = ::GetLastError();
		if (last_error == WSA_IO_PENDING) last_error = NO_ERROR;
		return last_error;
	}
	bool iocp::append_socket(socket_type s, void* context)
	{
		return CreateIoCompletionPort((HANDLE)s, fd, (ULONG_PTR)context, 0) != 0;
	}

	uint32_t iocp::start_timer(timer_call call, void* u, uint32_t wait/* = 0*/)
	{
		lock_guard<timer> guard(tm);
		return tm.add(call, u, wait);
	}
	void iocp::stop_timer(uint32_t idx)
	{
		lock_guard<timer> guard(tm);
		tm.del(idx);
	}
}
