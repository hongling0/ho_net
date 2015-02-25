#pragma once
#include "typedef.h"
#include "atomic.h"

#ifndef CORE_BUFFER_H
#define CORE_BUFFER_H

#ifdef _cpluscplus
extern "C" {
#endif

	typedef struct buffer_node {
		struct buffer_node * next;
		unsigned long size;
		unsigned long in;
		unsigned long out;
		char buf[1];
	} buffer_node;

	struct buffer_node* buffernode_new(unsigned long len);
	void buffernode_delete(struct buffer_node* node);
	unsigned long buffernode_write(struct buffer_node* node, char* data, unsigned long sz);
	unsigned long buffernode_read(struct buffer_node* node, char* data, unsigned long sz);
	unsigned long buffernode_startread(struct buffer_node* node, char** ptr);
	int buffernode_readok(struct buffer_node* node, unsigned long sz);
	unsigned long buffernode_startwrite(struct buffer_node* node, char** ptr);
	int buffernode_writeok(struct buffer_node* node, unsigned long sz);

	unsigned long buffernode_size(struct buffer_node* node);
	unsigned long buffernode_canwrite(struct buffer_node* node);
	unsigned long buffernode_canread(struct buffer_node* node);
	void buffernode_clean(struct buffer_node* node);

	typedef struct corebuffer
	{
		struct buffer_node* head;
		struct buffer_node* tail;
		int lock;
		uint32_t count;
	} corebuffer;

	void corebuffer_init(struct corebuffer* b);
	void corebuffer_releae(struct corebuffer* b);
	void corebuffer_pushback(struct corebuffer* b, struct buffer_node* node);
	void corebuffer_pushback_safe(struct corebuffer* b, struct buffer_node* node);
	void corebuffer_pushfront(struct corebuffer* b, struct buffer_node* node);
	void corebuffer_pushfront_safe(struct corebuffer* b, struct buffer_node* node);
	struct buffer_node* corebuffer_popfront(struct corebuffer* b);	
	struct buffer_node* corebuffer_popfront_safe(struct corebuffer* b);

#ifdef _cpluscplus
}
#endif

#endif //CORE_BUFFER_H
