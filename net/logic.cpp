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


void logic::on_msg(logic_msg* msg)
{

}
