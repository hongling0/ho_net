#pragma once

#include "typedef.h"
#include "lock.h"

namespace frame
{

	struct logic_msg
	{
		int destination;
		int source;
		int type;
		void* data;
		size_t sz;
	};

	class logic
	{
	public:
		logic();
		int send(int source, int destination, int type, int session, void* data, size_t sz);
		int handle() const
		{
			return logic_id;
		}
	private:
		virtual void on_msg(logic_msg* msg);
	private:
		const int logic_id;
	};
}