#include "typedef.h"
#include "lock.h"
#include "poller.h"
#include "lock.h"

#define MAX_POLLER 256

using namespace sys;
namespace net
{

	class iocp_list
	{
	public:
		iocp_list()
		{
			cnt = 0;
		}
		~iocp_list()
		{
			for (size_t i = 0; i < cnt; i++)
			{
				delete list[i];
			}
		}
		int reg(iocp* e)
		{
			m.wlock();
			if (cnt >= MAX_POLLER-1)
			{
				m.unlock();
				return 0;
			}
			list[cnt++] = e;
			return cnt;
		}
		void unreg(iocp* e)
		{
			m.wlock();
			for (size_t i = 0; i < MAX_POLLER; i++)
			{
				if (list[i] == e)
				{
					list[i] = NULL;
					m.unlock();
					return;
				}
			}
			m.unlock();
		}
		iocp * grub(int id)
		{
			if (id == 0) return NULL;
			m.rlock();
			if (id > cnt)
			{
				iocp * ret = list[id - 1];
				m.unlock();
				return ret;
			}
			m.unlock();
			return NULL;
		}
	private:
		iocp* list[MAX_POLLER];
		uint8_t cnt;
		rwlock m;
	};

	static iocp_list T;

	int create_poller(iocp_handle h)
	{
		iocp* p = new iocp(h);
		return T.reg(p);
	}

	iocp* grub_poller(int id)
	{
		return T.grub(id);
	}
}