#include "logic.h"
#include "logic_store.h"


namespace frame
{
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
		array.insert(msg_id, &h);
	}

	void logic::on_logic(logic_msg* msg)
	{
		msg_handle* h = (msg_handle*)array.find(msg->msg_id);
		if (h)
		{
			(*h)(this, msg);
		}
		else
			assert(false);
	}

}
