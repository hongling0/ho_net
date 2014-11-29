#pragma once

#include "typedef.h"
#include "lock.h"
#include "poller.h"
#include "binary_arry.h"

namespace frame
{
	class nocopyable
	{
	public:
		nocopyable(){}
		nocopyable(const nocopyable&);
	protected:
		nocopyable operator = (const nocopyable&);
	};

	class logic_msg;
	class logic;
	class msg_handle_store;
	typedef void(*msg_handle)(logic*,logic_msg*);
	class logic : public nocopyable
	{
	public:
		logic(iocp& io);
		int send(int destination, int type, int session, void* data, size_t sz);
		int handle() const
		{
			return logic_id;
		}
		void addhandler(int msg_id, msg_handle h);
	protected:
		friend logic_msg;
		virtual void on_logic(logic_msg* msg);
	protected:
		iocp& io;
		const int logic_id;
		binary_arry array;
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
	struct msg_id
	{
		static const int id;
	};
	template<class T> msg_id::id = msg_id_alloc::alloc();

	struct logic_msg : public event_head
	{
		logic_msg(int id) :msg_id(id){ call = logic_call; }
		const int msg_id;
	private:
		static void logic_call(void* data, event_head* head, size_t s, errno_type e)
		{
			logic* lgc = (logic*)data;
			lgc->on_logic((logic_msg*)head);
		}
	};

	template<class T>
	struct logic_template : public logic_msg
	{
		logic_template():logic_msg(typename msg_id<T>::msg_id){}
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