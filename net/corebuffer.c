#include "corebuffer.h"

#define CORE_MIN(a,b) ((a)<(b))?(a):(b)
#define LOCK(l)
#define UNLOCK(l)


static unsigned long next_power(unsigned long sz)
{
	unsigned long i = 64;
	if (sz >= LONG_MAX) return LONG_MAX;
	for (; i < sz;) {
		i *= 2;
	}
	return i;
}

struct buffer_node* buffernode_new(unsigned long len)
{
	struct buffer_node * ret;
	len = next_power(len);
	ret = (struct buffer_node *)malloc(sizeof(*ret) + len);
	ret->next = NULL;
	ret->size = len;
	ret->in = 0;
	ret->out = 0;
	return ret;
}

void buffernode_delete(struct buffer_node* node)
{
	assert(node);
	free(node);
}

unsigned long buffernode_write(struct buffer_node* node, char* data, unsigned long sz)
{
	unsigned long all, l;

	assert(node);
	all = node->size - (node->in - node->out);
	if (all < sz) return -1;

	l = CORE_MIN(sz, node->size - (node->in&(node->size - 1)));
	memcpy(node->buf + (node->in&(node->size - 1)), data, l);
	memcpy(node->buf, data + l, sz - l);

	node->in += sz;
	return 0;
}

unsigned long buffernode_read(struct buffer_node* node, char* data, unsigned long sz)
{
	unsigned long all, l;

	all = node->in - node->out;
	if (all < sz) return -1;

	l = CORE_MIN(sz, node->size - (node->out&(node->size - 1)));
	memcpy(data, node->buf + (node->out&(node->size - 1)), l);
	memcpy(data + l, data, sz - l);

	node->out += sz;
	return 0;
}

unsigned long buffernode_startread(struct buffer_node* node, char** ptr)
{
	unsigned long all, in, out;

	all = node->in - node->out;
	if (all == 0) return 0;
	in = node->in%node->size;
	out = node->out %node->size;
	*ptr = &node->buf[out];
	return in>out ? (in - out) : (node->size - out);
}

int buffernode_readok(struct buffer_node* node, unsigned long sz)
{
	unsigned long all = node->in - node->out;
	if (sz > all) return -1;
	node->out += sz;
	return 0;
}

unsigned long buffernode_startwrite(struct buffer_node* node, char** ptr)
{
	unsigned long all, in, out;

	all = node->size - (node->in - node->out);
	if (all == 0) return 0;
	in = node->in&(node->size - 1);
	out = node->out&(node->size - 1);
	*ptr = &node->buf[in];
	return in < out ? (node->size - in - 1) : (node->out - in);
}

int buffernode_writeok(struct buffer_node* node, unsigned long sz)
{
	unsigned long all = node->in - node->out;
	if (sz > all) return -1;
	node->in += sz;
	return 0;
}

unsigned long buffernode_size(struct buffer_node* node)
{
	return node->size;
}
unsigned long buffernode_canwrite(struct buffer_node* node)
{
	return node->size - (node->in - node->out);
}
unsigned long buffernode_canread(struct buffer_node* node)
{
	return node->in - node->out;
}
void buffernode_clean(struct buffer_node* node)
{
	node->in = 0;
	node->out = 0;
}

void corebuffer_init(struct corebuffer* b)
{
	b->head = NULL;
	b->tail = NULL;
	b->lock = 0;
	b->count = 0;
}

void corebuffer_releae(struct corebuffer* b)
{
	struct buffer_node * cur, *next;
	cur = b->head;
	for (;;) {
		next = cur->next;
		if (cur == NULL) {
			break;
		}
		buffernode_delete(cur);
		cur = next;
	}
	b->head = NULL;
	b->tail = NULL;
	b->count = 0;
}

void corebuffer_pushback_safe(struct corebuffer* b, struct buffer_node* node)
{
	struct buffer_node * tail;
	uint32_t count = 1;
	tail = node;
	while (tail->next) {
		tail = tail->next;
		count++;
	}

	LOCK(&b->lock);
	if (b->head == NULL) {
		b->head = node;
	} else {
		b->tail->next = node;
	}
	b->tail = tail;
	b->count += count;
	UNLOCK(&b->lock);
}

void corebuffer_pushfront_safe(struct corebuffer* b, struct buffer_node* node)
{
	struct buffer_node * tail;
	uint32_t count = 1;
	tail = node;
	while (tail->next) {
		tail = tail->next;
		count++;
	}
	LOCK(&b->lock);
	if (b->tail == NULL) {
		b->tail = tail;
	} else {
		tail->next = b->head;
	}
	b->head = node;
	b->count += count;
	UNLOCK(&b->lock);
}

struct buffer_node* corebuffer_popfront_safe(struct corebuffer* b)
{
	struct buffer_node* node;
	LOCK(&b->lock);
	if (b->count == 0) {
		UNLOCK(&b->lock);
		return NULL;
	}
	node = b->head;
	b->head = node->next;
	if (b->head == NULL) {
		b->tail = NULL;
	}
	UNLOCK(&b->lock);
	return node;
}


void corebuffer_pushback(struct corebuffer* b, struct buffer_node* node)
{
	struct buffer_node * tail;
	uint32_t count = 1;
	tail = node;
	while (tail->next) {
		tail = tail->next;
		count++;
	}

	if (b->head == NULL) {
		b->head = node;
	} else {
		b->tail->next = node;
	}
	b->tail = tail;
	b->count += count;
}

void corebuffer_pushfront(struct corebuffer* b, struct buffer_node* node)
{
	struct buffer_node * tail;
	uint32_t count = 1;
	tail = node;
	while (tail->next) {
		tail = tail->next;
		count++;
	}

	if (b->tail == NULL) {
		b->tail = tail;
	} else {
		tail->next = b->head;
	}
	b->head = node;
	b->count += count;

}

struct buffer_node* corebuffer_popfront(struct corebuffer* b)
{
	struct buffer_node* node;

	if (b->count == 0) {
		return NULL;
	}
	node = b->head;
	b->head = node->next;
	if (b->head == NULL) {
		b->tail = NULL;
	}
	return node;
}