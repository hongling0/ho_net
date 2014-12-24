#pragma once

namespace cfg
{
	class csv_parse
	{
	public:
		csv_parse(char* msg)
		{
			data = msg;
			parse_ptr = msg;
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

	class csv
	{
	public:
		struct line
		{
			struct line * next;
			char * column[1];
		};
		csv();
		~csv();
		bool parse(char* data);

	protected:
		bool parse_head();
		void resize_line();
	private:
		int col_cnt;
		int line_cnt;
		int line_cap;
		struct line ** list;
	};

	class csvfile : public csv
	{
	public:
		bool load(const char*);
		bool save(const char*);
	};
}
