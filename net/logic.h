#pragma once

#include "typedef.h"
#include "lock.h"
#include "poller.h"

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

	enum logic_msg_type
	{
		PTYPE_SYSTEM,
		PTYPE_SOCKET,
		PTYPE_TEXT,
	};

	struct event_head
	{
		OVERLAPPED op;
		int type;
	};

	struct logic_msg : public event_head
	{
		int destination;
		int source;
		void* data;
		size_t sz;
	};

	class logic : public nocopyable
	{
	public:
		logic(iocp& io);
		int send(int destination, int type, int session, void* data, size_t sz);
		int handle() const
		{
			return logic_id;
		}
		void on_message(event_head* head, size_t sz, errno_type err);
		virtual void on_logic(logic_msg* msg);
	protected:
		iocp& io;
	private:
		const int logic_id;
	};

	logic* grub_logic(int logic_id);
}