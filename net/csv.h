#pragma once

namespace cfg
{
	class csv_parse
	{
	public:
		csv_parse(char* msg)
		{
			data = msg;
		}
		bool next_column(char** out);
		bool next_line()
		{
			need_nextline = 0;
			return parse_ptr != NULL;
		}
		const char * error() const { return errmsg; }
	private:
		char* data;
		char* parse_ptr;
		int need_nextline;
		char errmsg[32];
	};

}