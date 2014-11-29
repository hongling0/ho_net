#include "logic.h"
#include "logic_store.h"


using namespace frame;

static logic_store store;

logic* grub_logic(int logic_id)
{
	return store.grub(logic_id);
}

logic::logic(iocp& e) :logic_id(store.reg(this)), io(e)
{

}
int logic::send(int destination, int type, int session, void* data, size_t sz)
{
	logic_msg * msg = new logic_msg;
	msg->source = logic_id;
	msg->destination = destination;
	msg->type = type;
	msg->data = data;
	msg->sz = sz;
}


void logic::on_msg(logic_msg* msg)
{

}
