#pragma once

#include "typedef.h"
#include "buffer.h"


namespace frame
{
	struct event_head;
	typedef void(*event_call)(void*, event_head*, size_t, errno_type);
	struct event_head
	{
		OVERLAPPED op;
		event_call call;
	};

	namespace detail
	{
		struct logic_msg_id_alloc
		{
			static int alloc() { return id++; }
		private:
			static int id;
		};
	}

	template<typename T>
	struct logic_msg_id
	{
		typedef T type;
		static const int id;
	};
	template<typename T> const int logic_msg_id<T>::id = detail::logic_msg_id_alloc::alloc();

	struct logic_msg
	{
		logic_msg(int id) :msg_id(id){}
		virtual ~logic_msg(){}
		const int msg_id;
	};

	struct event_logic : public event_head
	{
		event_logic(){ call = logic_call; }
		logic_msg * msg;
	protected:
		static void logic_call(void* data, event_head* head, size_t s, errno_type e);
	};

	template<class T>
	struct logic_template : public logic_msg
	{
		typedef logic_msg_id<T> msg_id_type;
		logic_template() :logic_msg(msg_id_type::id){}
	};

#define LOGIC_MSG(n) struct n : public logic_template<n>

	LOGIC_MSG(logic_accept)
	{
		int id;
		int listenid;
		errno_type err;
	};

	LOGIC_MSG(logic_connect)
	{
		int id;
		errno_type err;
	};

	LOGIC_MSG(logic_recv)
	{
		int id;
		ring_buffer* buffer;
	};

	LOGIC_MSG(logic_socketerr)
	{
		int id;
		errno_type err;
	};
}