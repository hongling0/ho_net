#pragma once

namespace cfg
{
	class csv_parse
	{
	public:
		csv_parse(char* msg, char* err, size_t len)
		{
			parse_ptr = msg;
			errmsg = err;
			errmsg_len = len;
		}
		bool next_column(char** out);
		bool next_line()
		{
			need_nextline = 0;
			return parse_ptr != NULL && (*parse_ptr) != '\0';
		}
	private:
		char* parse_ptr;
		int need_nextline;
		char* errmsg;
		size_t errmsg_len;
	};

	class csv
	{
	public:
		csv();
		~csv();
		bool parse(char* data);
		const char* error() const { return errmsg; }

		const char* value(int line, int row, const char* def);
		int value(int line, int row, int def);
		double value(int line, int row, double def);

	protected:
		bool parse_head();
		void resize_line();
	private:
		int col_cnt;
		int line_cnt;
		int line_cap;
		char** data;
	protected:
		char errmsg[32];
	};

	class csvfile : public csv
	{
	public:
		csvfile()
		{
			buf = 0;
		}
		~csvfile()
		{
			free(buf);
		}
		bool load(const char*);
		bool save(const char*);
	private:
		char* buf;
	};
}
