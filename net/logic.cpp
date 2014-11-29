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
<<<<<<< HEAD
int logic::send(int destination, int type, int session, void* data, size_t sz)
{
	logic_msg * msg = new logic_msg;
	msg->source = logic_id;
	msg->destination = destination;
	msg->type = type;
	msg->data = data;
	msg->sz = sz;
}
=======
>>>>>>> 4cb2f784f7b60b755d8e7b46c7ee301bfed25311


void logic::on_msg(logic_msg* msg)
{

}
