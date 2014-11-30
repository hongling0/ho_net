#pragma once
#include <memory>


namespace frame
{
	class binary_arry
	{
		struct item
		{
			int key;
			void* val;
		};
	public:
		binary_arry(int sz=0)
		{
			slot = NULL;
			slot_sz = 0;
			slot_max = 0;
			slot_def = 2;
			while (slot_def < sz) slot_def *= 2;
		}
		~binary_arry()
		{
			free(slot);
		}
		bool insert(int key,void* val)
		{
			if (slot_sz + 1 > slot_max)
				grown();
			int p = 0;
			for (p = 0; p < slot_sz&&slot[p]->key < key; p++);
			for (int i = slot_sz; i > p; i--)
			{
				*(slot[i + 1]) = *(slot[i]);
			}
			slot[p]->key = key;
			slot[p]->val = val;
			slot_sz++;
			return true;
		}
		void* find(int i)
		{
			return binary_search(slot, 0, slot_sz, i);
		}
	protected:
		void* binary_search(item **arr, int low, int high, int key)
		{
			int mid = low + (high - low) / 2;
			if (low>high)
				return NULL;
			else{
				if (arr[mid]->key == key)
					return arr[mid]->val;
				else if(arr[mid]->key>key)
					return binary_search(arr, low, mid - 1, key);
				else
					return binary_search(arr, mid + 1, high, key);
			}
		}
		void grown()
		{
			int new_max = slot_max ? 2 * slot_max : slot_def;
			item** newslot = (item**)malloc(sizeof(item*)*new_max);
			memset(newslot, 0, sizeof(item*)*new_max);
			for (int i = 0; i < slot_sz; i++)
			{
				*(newslot[i]) = *(slot[i]);
			}
			free(slot);
			slot_max = new_max;
			slot = newslot;
		}
	private:
		int slot_def;
		item** slot;
		int slot_sz;
		int slot_max;
	};
}