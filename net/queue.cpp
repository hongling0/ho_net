#include "typedef.h"
#include "lock.h"

namespace net
{

	struct message
	{
		volatile message* next;
		uint8_t type;
		void* data;
	};



	class queue
	{
	public:
		void push(message* q)
		{
			q->next = NULL;
			message * p;
			p = tail;
			oldp = p
				do {
				while (p->next != NULL)
					p = p->next;
			} while (atomic_cmp_set(&tail->next, NULL, p));
			atomic_cmp_set(&tail, n, p);
		}
		message* pop()
		{

		}
	private:
		atomic_type ref;
		message * head;
		message * tail;
	};

}