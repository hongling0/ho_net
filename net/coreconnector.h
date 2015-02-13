#ifndef CORE_CONNECTOR_H
#define CORE_CONNECTOR_H

#include "corelogic.h"

#ifdef _cpluscplus
extern "C" {
#endif

	enum connector_cmd
	{
		connector_start,//NULL
		connector_stop,//NULL
		connector_send,// struct corebuf*
		connector_max,
	};


	int coreconnector_start(struct core_poller* io, struct core_connector* co, void* param);
	int coreconnector_stop(struct core_poller* io, struct core_connector* co, void* param);

	int coreconnector_send(struct core_poller* io, struct core_connector* co, void/*(corebuf)*/* buf);

	typedef int(*protocol_call)();

	typedef void(*protocol_on_connect)(int id, int err);
	typedef void(*protocol_on_recv)(int id, void* data, size_t sz);
	typedef void(*protocol_on_socketerr)(int id, int err);

	

	
#ifdef _cpluscplus
}
#endif

#endif //SOCKET_SERVER_H
