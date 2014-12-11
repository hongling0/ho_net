#pragma once

#include "typedef.h"
#include "lock.h"
#include "poller.h"
#include "binary_arry.h"
#include "errmsg.h"

namespace frame
{
	class nocopyable
	{
	public:
		nocopyable(){}
		virtual~nocopyable(){}
		nocopyable(const nocopyable&);
	protected:
		nocopyable operator = (const nocopyable&);
	};

	struct event_logic;
	struct logic_msg;
	class logic;
	typedef void(*msg_handle)(logic*,struct logic_msg*);
	class logic : public nocopyable
	{
	public:
		logic(iocp& io);
		int post(int destination,logic_msg* msg);
		void addhandler(int msg_id, msg_handle h);
		const int logic_id;
	protected:
		friend event_logic;
		void on_logic(struct logic_msg* msg);
	protected:
		iocp& io;
		binary_arry<msg_handle> array;
	};


	
}