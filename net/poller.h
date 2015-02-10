#pragma once

#include "typedef.h"
#include "event.h"
#include "timer.h"

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
		void wait();
		int post(void* context, event_head* ev, size_t bytes, errno_type e);
		bool append_socket(socket_type s, void* context);
		uint32_t start_timer(timer_call call, void* u, uint32_t wait = 0);
		void stop_timer(uint32_t idx);
	protected:
		handle_type fd;
		uint8_t thr;
		OVERLAPPED quit;
		OVERLAPPED wake;
		volatile int quited;

		long in_timer;
		timer tm;
	};
}
