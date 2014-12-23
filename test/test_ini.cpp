#include <string>
#include <iostream>
#include "ini.h"

using namespace cfg;

#define CONTEXT \
" [ main]\n" \
" author = wh\n"\
"\rtime = 2014 \n"\
"[te st ] \n;qweqweqweqwe\n"\
"result=false\n"

int main()
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

	for(int j=0;j<10000000;++j)
	{
		cfg::ini i;
		char ctx[] = { CONTEXT };
		if (i.parse(ctx) == false)
			fprintf(stderr, i.error());
		int l = i.value("main", "time", 0);
		double d = i.value("main", "time", 0.0);
		const char* c = i.value("te st", "result", "null");
	}

	_CrtDumpMemoryLeaks();
	return 1;
}