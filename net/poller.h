#include "typedef.h"
#include "lock.h"

#include "socket.h"

namespace frame
{
	typedef void(*iocp_handle)(void* context, event_head* ev, size_t bytes, errno_type e);
	class iocp
	{
	public:
		iocp();
		~iocp();
		void start_thread(uint8_t n);
		void stop_thread();
		void run();
		bool post(void* context, event_head* ev, size_t bytes, errno_type e);
		bool append_socket(socket_type s, void* context);
		uint8_t id() const { return _id; }
		bool reghandle(uint8_t type, iocp_handle h);
	protected:
		handle_type fd;
		uint8_t thr;
		OVERLAPPED quit;
		iocp_handle handle[io_event_type_max];
		uint8_t _id;
	};
}
