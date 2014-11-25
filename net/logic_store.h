#pragma once

#include "typedef.h"
#include "lock.h"

namespace frame
{
	class logic;

	class logic_store
	{
		int resize();
	public:
		logic_store();
		~logic_store();

		int reg(logic* lgc);
		logic* grub(int id);
	private:
		sys::rwlock lock;
		logic** slot;
		int slot_size;
		int index;
	};
}