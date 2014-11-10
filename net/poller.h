#include "typedef.h"
#include "lock.h"

namespace net
{
	class poller
	{
	public:
		poller()
		{
			fd = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
		}
		~poller()
		{
			CloseHandle(fd);
		}
		void start_thread(uint8_t n)
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
		void stop_thread()
		{
			if (thr>0){
				PostQueuedCompletionStatus(fd, thr, (ULONG_PTR)this, &quit);
			}
		}
		static void THREAD_START_ROUTINE(LPVOID lpThreadParameter)
		{
			poller* p = (poller*)lpThreadParameter;
			
		}
		void run()
		{
			DWORD bytes;
			DWORD completion_key = 0;
			LPOVERLAPPED op;
			for (;;)
			{
				SetLastError(0);
				BOOL ret = GetQueuedCompletionStatus(fd, &bytes, &completion_key, &op, 500);
				DWORD last_error = ::GetLastError();
				if (!ret)
				{
					if
				}
			}
		}
	private:
		handle_type fd;
		uint8_t thr;
		OVERLAPPED quit;
	};

}