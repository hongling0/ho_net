#include <string>
#include <iostream>
#include "csv.h"

using namespace cfg;

#define CONTEXT \
"��ð� ,123123,��ð�����������\r\n" \
"�Һܺð�,\"��qweqw\'\"\"\",\"\"\"�Һܺá�,\"\"��,\"\"0??\"\r\n" \
"�Һܺð�,\"��qweqw\'\"\"\",\"\"\"�Һܺá�,\"\"��,\"\"0??\"\r\n" 

int main()
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

	csvfile f;
	f.load("test.csv");

	_CrtDumpMemoryLeaks();
	return 1;
}