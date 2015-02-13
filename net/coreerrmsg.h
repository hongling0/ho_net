#ifndef CORE_ERRORMSG_H
#define CORE_ERRORMSG_H

#ifdef _cpluscplus
extern "C" {
#endif //_cpluscplus

	int errno_append(int e,const char* msg);
	const char * errno_str(int e);

#ifdef _cpluscplus
}
#endif //_cpluscplus

#endif //COREPOLLER_H
