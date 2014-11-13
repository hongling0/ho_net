#include "typedef.h"
#include "lock.h"

#include "socket.h"

namespace net
{
	typedef void(*iocp_handle)(void* context, event_head* ev, size_t bytes, errno_type e);
	class iocp
	{
	public:
		iocp(iocp_handle h) :handle(h)
		{
			_id = 0;
			fd = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
		}
		~iocp()
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
				thr = 0;
			}
		}
		static void THREAD_START_ROUTINE(LPVOID lpThreadParameter)
		{
			poller* p = (poller*)lpThreadParameter;
			p->run();
		}
		void run()
		{
			for (;;)
			{
				DWORD bytes;
				DWORD completion_key = 0;
				LPOVERLAPPED op;
				SetLastError(0);
				BOOL ret = GetQueuedCompletionStatus(fd, &bytes,&completion_key, &op, 500);
				DWORD last_error = ::GetLastError();
				if (!ret)
				{
					if (last_error == WAIT_TIMEOUT)
						continue;
					else
						assert(false);
				}
				if (op==&quit)
				{
					if (--bytes > 0)
						PostQueuedCompletionStatus(fd, bytes, (ULONG_PTR)this, &quit);
					break;
				}
				else if (op)
				{
					event_head* ev = CONTAINING_RECORD(op, event_head, op);
					handle((void*)completion_key, ev, bytes, last_error);
				}
			}
		}
		bool post(void* context, event_head* ev, size_t bytes, errno_type e)
		{
			if (thr = 0)
				return false;
			return PostQueuedCompletionStatus(fd, bytes, (ULONG_PTR)context, &ev->op);
		}
		bool append_socket(socket_type s, void* context)
		{
			return CreateIoCompletionPort((HANDLE)s, fd, (ULONG_PTR)context, 0);
		}
		uint8_t id() const { return _id; }
	protected:
		handle_type fd;
		uint8_t thr;
		OVERLAPPED quit;
		iocp_handle handle;
		uint8_t _id;
	};

//	int register_poller(iocp*  e);
//	void unregister_poller(iocp* e);
	iocp* grub_poller(int id);
	void drop_poller(iocp*);
	int create_poller(iocp_handle h);
}
