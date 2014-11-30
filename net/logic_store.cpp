#include "logic_store.h"
#include "logic.h"

using namespace frame;


logic_store::logic_store()
{
	index = 0;
	slot = 0;
	slot_size = 0;
	resize();
}

logic_store::~logic_store()
{
	::free(slot);
}
int logic_store::resize()
{
	int new_size = slot_size ? slot_size : (1 << 2);
	new_size *= 2;
	logic** newlist = (logic**)::malloc(sizeof(logic*)*new_size);
	memset(newlist, 0, sizeof(logic*)*new_size);
	for (int i = 0; i < slot_size; i++)
	{
		int handle = slot[i]->handle();
		int hash = handle ^ ((slot_size * 2) - 1);
		newlist[hash] = slot[i];
	}
	::free(slot);
	slot = newlist;
	slot_size = new_size;
	return slot_size;
}

int logic_store::reg(logic* lgc)
{
	lock.wlock();
	for (;;)
	{
		for (int i = 0; i < slot_size; i++)
		{
			int handle = i + index;
			int hash = handle & (slot_size - 1);
			if (slot[hash] == NULL)
			{
				slot[hash] = lgc;
				index = handle + 1;
				lock.unlock();
				return handle;
			}
		}
		resize();
	}
}
logic* logic_store::grub(int id)
{
	lock.rlock();
	int hash = id&(slot_size - 1);
	logic* lgc = slot[hash];
	if (!lgc || lgc->handle() != id)
		lgc = NULL;
	lock.unlock();
	return lgc;
}