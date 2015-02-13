#ifndef CORE_CONNECTOR_H
#define CORE_LISTENER_H

#include "corelogic.h"

#ifdef _cpluscplus
extern "C" {
#endif

	enum listener_cmd
	{
		listener_start,//NULL
		listener_stop,//NULL
		listener_send,// struct corebuf*
		listener_max,
	};

	typedef void(*protocol_on_connect)(int id, int err);
	typedef void(*protocol_on_recv)(int id, void* data, size_t sz);
	typedef void(*protocol_on_socketerr)(int id, int err);

#ifdef _cpluscplus
}
#endif

#endif //SOCKET_SERVER_H