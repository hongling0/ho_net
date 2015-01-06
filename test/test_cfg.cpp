#include <string>
#include <iostream>
#include "csv.h"

using namespace cfg;

#define CONTEXT \
"你好啊 ,123123,你好啊，我是王红\r\n" \
"我很好啊,\"‘qweqw\'\"\"\",\"\"\"我很好”,\"\"“,\"\"0??\"\r\n" \
"我很好啊,\"‘qweqw\'\"\"\",\"\"\"我很好”,\"\"“,\"\"0??\"\r\n" 

int main()
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

	csvfile f;
	f.load("test.csv");

	_CrtDumpMemoryLeaks();
	return 1;
}