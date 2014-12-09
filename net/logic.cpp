#include "logic.h"
#include "logic_store.h"


namespace frame
{
	int detail::logic_msg_id_alloc::id = 100000000;

	void event_logic::logic_call(void* data, event_head* head, size_t s, errno_type e)
	{
		logic* lgc = (logic*)data;
		event_logic* ev = (event_logic*)head;
		lgc->on_logic(ev->msg);
		delete ev;
	}


	struct msg_handle_store
	{
		size_t slot_sz;
		size_t slot_max;
		msg_handle handler;
		int msg_id;
	};


	static logic_store store;

	logic* grub_logic(int logic_id)
	{
		return store.grub(logic_id);
	}

	logic::logic(iocp& e) :logic_id(store.reg(this)), io(e)
	{

	}

	void logic::addhandler(int msg_id, msg_handle h)
	{
		array.insert(msg_id, h);
	}

	void logic::on_logic(logic_msg* msg)
	{
		msg_handle h = array.find(msg->msg_id);
		if (h)
		{
			(h)(this, msg);
		}
		else
			assert(false);
	}

	int logic::post(int destination, logic_msg* msg)
	{
		event_logic* lgc = new event_logic;
		lgc->msg = msg;
		logic* recv = grub_logic(destination);
		if (!recv)
		{
			delete lgc;
			assert(false);
			return FRAME_POLLER_NOT_FOUND;
		}
		else
		{
			int err=io.post(recv, lgc, 0, 0);
			if (err != NO_ERROR&&err != WSA_IO_PENDING)
				delete lgc;
			return err;
		}
	}
}
