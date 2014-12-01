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
	}
	iocp::~iocp()
	{
		CloseHandle(fd);
	}
	void iocp::start_thread(uint8_t n)
	{
		n = (n == 0) ? 1 : n;
		for (; thr < n; thr++)
		{
			_beginthread(THREAD_START_ROUTINE, 0, this);
		}
		if (thr>n){
			PostQueuedCompletionStatus(fd, thr - n, (ULONG_PTR)this, &quit);
		}
		thr = n;
	}
	void iocp::stop_thread()
	{
		if (thr > 0){
			PostQueuedCompletionStatus(fd, thr, (ULONG_PTR)this, &quit);
			thr = 0;
		}
	}

	void iocp::run()
	{
		for (;;)
		{
			DWORD bytes;
			DWORD completion_key = 0;
			LPOVERLAPPED op;
			SetLastError(0);
			BOOL ret = GetQueuedCompletionStatus(fd, &bytes, &completion_key, &op, 500);
			DWORD last_error = ::GetLastError();
			if (ret!=TRUE)
			{
				if (last_error == WAIT_TIMEOUT)
					continue;
			}
			if (op == &quit)
			{
				if (--bytes > 0)
					PostQueuedCompletionStatus(fd, bytes, (ULONG_PTR)this, &quit);
				break;
			}
			else if (op)
			{
				event_head* ev = (event_head*)op;
				ev->call ((void*)completion_key, ev, bytes, last_error);
			}
			else
			{
				fprintf(stderr, "GetQueuedCompletionStatus %s\n", errno_str(last_error));
				assert(false);
			}
		}
	}
	bool iocp::post(void* context, event_head* ev, size_t bytes, errno_type e)
	{
		if (thr = 0)
			return false;
		return PostQueuedCompletionStatus(fd, bytes, (ULONG_PTR)context, &ev->op)==TRUE;
	}
	bool iocp::append_socket(socket_type s, void* context)
	{
		return CreateIoCompletionPort((HANDLE)s, fd, (ULONG_PTR)context, 0)!=0;
	}

}
