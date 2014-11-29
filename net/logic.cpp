#include "logic.h"
#include "logic_store.h"


using namespace frame;

static logic_store store;

logic::logic() :logic_id(store.reg(this))
{

}


void logic::on_msg(logic_msg* msg)
{

}
