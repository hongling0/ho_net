#include <stdint.h>

namespace http
{
	struct request
	{
		uintptr_t state;
	};

	class ring_buffer;
	int parse_requeset(struct request* r, ring_buffer* b)
	{
		uint8_t c, ch, *p, *m;
		enum
		{
			st_start,
			st_head,
		};
		uintptr_t state = r->state;
		for (p = b->pos; p < b->last;p++) {

		}
	}
}