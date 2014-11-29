#pragma once

#include "typedef.h"
#include "lock.h"

namespace frame
{
	class logic_msg;
	class logic
	{
	public:
		logic();
		int send(int source, int destination, int type, int session, void* data, size_t sz);
		int handle() const
		{
			return logic_id;
		}
	protected:
		friend logic_msg;
		virtual void on_msg(logic_msg* msg);
	private:
		const int logic_id;
	};


	typedef void(*event_call)(void*, event_head*, size_t, errno_type);

	struct event_head
	{
		OVERLAPPED op;
		event_call call;
	};

	struct msg_id_alloc
	{
		static int alloc() { return id++; }
	private:
		static int id;
	};
	int msg_id_alloc::id = 0;

	template<class T>
	struct logic_msg_id
	{
		static const int msg_id;
	};
	template<class T> logic_msg_id::msg_id = msg_id_alloc::alloc();

	struct logic_msg : public event_head
	{
		logic_msg(int id) :msg_id(id){ call = logic_call; }
		const int msg_id;
	private:
		static void logic_call(void* data, event_head* head, size_t s, errno_type e)
		{
			logic* lgc = (logic*)data;
			lgc->on_msg((logic_msg*)head);
		}
	};

	template<class T>
	struct logic_template : public logic_msg
	{
		logic_template():logic_msg(typename logic_msg_id<T>::msg_id){}
	};

#define LOGIC_MSG(n) struct n : public logic_template<n>

	LOGIC_MSG(logic_recv)
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

	LOGIC_MSG(logic_read)
	{
		int id;
		//ring_buffer* buffer;
	};

	LOGIC_MSG(logic_socketerr)
	{
		int id;
		errno_type err;
	};
}