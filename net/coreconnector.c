#include <assert.h>
#include <stdio.h>
#include "coreconnector.h"
#include "coresocket.h"
#include "coreerrmsg.h"
#include "coremodule.h"

struct core_connector
{
	char ip[16];
	int port;
	int socket;
	protocol_on_connect on_connect;
	protocol_on_recv on_recv;
	protocol_on_socketerr on_socketerr;
};

int coreconnector_start(struct core_poller* io, struct core_connector* co, void* param)
{
	int err;
	if (co->socket) {
		assert(0);
		return 0;
	}
	err = start_connet(UB2LOGIC(co)->logic_id, co->ip, co->port, NULL, &co->socket);
	fprintf(stdout, "|start_connet|%d|%s:%d|%d\n", UB2LOGIC(co)->logic_id, co->ip, co->port, co->socket);
	if (!err && socket > 0) {
		return 0;
	}
	fprintf(stderr, "|start_connet|%d failure|%s:%d|%d %s\n", UB2LOGIC(co)->logic_id, co->ip, co->port, co->socket, errno_str(err));
	return err;
}

int coreconnector_stop(struct core_poller* io, struct core_connector* co, void* param)
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

int coreconnector_send(struct core_poller* io, struct core_connector* co, void* param)
{
	corebuf* buf = (corebuf*)param;
	int err = start_send(co->socket, buf);
	if (err) {
		fprintf(stderr, "coreconnector_send failure [%s]\n", errno_str(err));
	}
	return err;
}

typedef int(*coreconnector_cmd)(struct core_poller* io, struct core_connector* co, void* param);

static coreconnector_cmd CMD[] =
{
	[connector_start] = coreconnector_start,
	[connector_stop] = coreconnector_stop,
	[connector_send] = coreconnector_send,
};

static int coreconnector_cmd_handler(struct core_poller* io, void* ub, int cmd, void* param)
{
	const char* args = (const char*)param;
	if (cmd <= connector_max) {
		coreconnector_cmd call = CMD[cmd];
		if (call) {
			return call(io, (struct core_connector*)ub, param);
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
	struct core_connector* ret = (struct core_connector*)malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));
	return ret;
}

static void coreconnector_logic_handler(struct core_poller* io, void* ub, int sender, int session, void* data, size_t sz)
{
	struct core_connector* co = (struct core_connector*)ub;
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
	struct core_connector* co = (struct core_connector*)ins;
	int ret = sscanf_s(args, "%s %d", co->ip, &co->port);
	if (ret != 2) {
		fprintf(stderr, "bad args for %%s %%d (%s)\n", args);
		return -1;
	}
	lgc->call = coreconnector_logic_handler;
	lgc->cmd = coreconnector_cmd_handler;
	return 0;
}

struct core_module connector_module =
{
	.name = "connector",
	.init = init,
	.create = create,
	.release = release
};