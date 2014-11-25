#include "logic.h"
#include "logic_store.h"


using namespace frame;

static logic_store store;

logic::logic() :logic_id(store.reg(this))
{

}
int logic::send(int source, int destination, int type, int session, void* data, size_t sz)
{
	if (source == 0)
		source = logic_id;
	logic_msg * msg = new logic_msg;
	msg->source = source;
	msg->destination = destination;
	msg->type = type;
	msg->data = data;
	msg->sz = sz;
}


void logic::on_msg(logic_msg* msg)
{

}
