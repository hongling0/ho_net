#include <assert.h>
#include <stdio.h>
#include "corelistener.h"
#include "coresocket.h"
#include "coreerrmsg.h"
#include "coremodule.h"

struct core_listener
{
	char ip[16];
	int port;
	int socket;
	protocol_on_connect on_connect;
	protocol_on_recv on_recv;
	protocol_on_socketerr on_socketerr;
};

int corelistener_start(struct core_listener* co, void* param)
{
	int err;
	if (co->socket) {
		assert(0);
		return 0;
	}
	fprintf(stdout, "%d start_listen %s:%d\n", UB2LOGIC(co)->logic_id, co->ip, co->port);
	err = start_listen(UB2LOGIC(co)->logic_id, co->ip, co->port, 1024, NULL, &co->socket);
	if (!err) return 0;

	fprintf(stderr, "start listen %s:%d failure ! %s\n", co->ip, co->port, errno_str(err));
	return err;
}

int corelistener_stop(struct core_listener* co, void* param)
{
	if (co->socket) {
		assert(0);
		return 0;
	}
	if (!co->socket) return 0;
	start_close(co->socket);
	co->socket = 0;
	return 0;
}

int corelistener_send(struct core_listener* co, void* param)
{
	corebuf* buf = (corebuf*)param;
	int err = start_send(co->socket, buf);
	if (err) {
		fprintf(stderr, "corelistener_send failure [%s]\n", errno_str(err));
	}
	return err;
}

typedef int(*corelistener_cmd)(struct core_listener* co, void* param);

static corelistener_cmd CMD[] =
{
	[listener_start] = corelistener_start,
	[listener_stop] = corelistener_stop,
	[listener_send] = corelistener_send,
};
static int corelistener_cmd_handler(struct core_logic* lgc, int cmd, void* param)
{
	const char* args = (const char*)param;
	if (cmd <= listener_max) {
		corelistener_cmd call = CMD[cmd];
		if (call) {
			return call((struct core_listener*)lgc->inst, param);
		}
	}
	return -1;
}


static void release(void* ins)
{
	free(ins);
}

static void* create(void)
{
	struct core_listener* ret = (struct core_listener*)malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));
	return ret;
}

static void corelistener_logic_handler(struct core_poller* io, struct core_logic * lgc,int sender, int session, void* data, size_t sz)
{
	struct core_listener* co = (struct core_listener*)lgc->inst;
	struct socket_msg* m = (struct socket_msg*)data;
	switch (m->type) {
	case socketmsg_connect:
		{
			co->on_connect(m->id, m->err);
		}
	case socketmsg_socketerr:
		{
			co->on_socketerr(m->id, m->err);
		}
	case socketmsg_recv:
		{
			co->on_recv(m->id, m->data, m->size);
		}
		break;
	default:
		{
			fprintf(stderr, "unknow msgtype\n");
		}
		break;
	}
}

static int init(void* ins, struct core_logic* lgc, void* param)
{
	const char* args = (const char*)param;
	struct core_listener* co = (struct core_listener*)ins;
	int ret = sscanf_s(args, "%s %d", co->ip,sizeof(co->ip),&co->port);
	if (ret != 2) {
		fprintf(stderr, "bad args for %%s %%d (%s) %s\n", args,errno_str(errno));
		return -1;
	}
	lgc->call = corelistener_logic_handler;
	lgc->cmd = corelistener_cmd_handler;
	return 0;
}

struct core_module listener_module =
{
	.name = "listener",
	.init = init,
	.create = create,
	.release = release
};
