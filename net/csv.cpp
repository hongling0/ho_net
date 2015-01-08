#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv.h"
#include "stringutil.h"

using namespace frame;

#define SET_CHAR(ptr,n,c) do{ *(ptr-n)=c; } while(0)
#define MV_CHAR(ptr,n) do{ if(n>0)*(ptr-n)=*ptr; } while(0)

namespace cfg
{
	bool csv_parse::next_column(char** out)
	{
		int with_quote, bytes;
		char* ptr;

		*out = NULL;
		if (!parse_ptr) return true;
		if (need_nextline) return true;
		with_quote = *parse_ptr == '"' ? 1 : 0;
		ptr = with_quote > 0 ? (parse_ptr + 1) : parse_ptr;
		*out = ptr;
		bytes = 0;
	loop:
		switch (*ptr) {
		case '\0':
			{
				parse_ptr = NULL;
				return true;
			}
			break;
		case '\"':
			{
				switch (*(ptr + 1)) {
				case  ',':
					if (with_quote == 0)
						goto parse_error;
					else {
						SET_CHAR(ptr, bytes, '\0');
						parse_ptr = ptr + 2;
						return true;
					}
					break;
				case '\r':
					if (*(ptr + 2) == '\n') {
						if (with_quote == 0)
							goto parse_error;
						else {
							SET_CHAR(ptr, bytes, '\0');
							parse_ptr = ptr + 3;
							need_nextline = 1;
							return true;
						}
					}
					else
						goto parse_error;
					break;
				case  '\"':
					{
						MV_CHAR(ptr, bytes);
						bytes++;
						ptr++;
					}
					break;
				default:
					goto parse_error;
					break;
				}
			}
			break;
		case '\r':
			{
				if (with_quote == 0 && *(ptr + 1) == '\n') {
					SET_CHAR(ptr, bytes, '\0');
					parse_ptr = ptr + 2;
					need_nextline = 1;
					return true;
				}
				else
					MV_CHAR(ptr, bytes);
			}
			break;
		case ',':
			{
				if (with_quote == 0) {
					SET_CHAR(ptr, bytes, '\0');
					parse_ptr = ptr + 1;
					return true;
				}
				else
					MV_CHAR(ptr, bytes);
			}
			break;
		default:
			MV_CHAR(ptr, bytes);
			break;
		}
		++ptr;
		goto loop;
	parse_error:
		if (errmsg&&errmsg_len>0)
			_snprintf(errmsg, errmsg_len, "unexpected char %c on %s", *ptr, ptr);
		*out = NULL;
		return false;
	}
	csv::csv()
	{
		line_cnt = 0;
		col_cnt = 0;
		data = NULL;
	}
	csv::~csv()
	{
		free(data);
	}

	struct column_node
	{
		struct column_node * next;
		char * data;
	};

	struct line_node
	{
		struct line_node * next;
		struct column_node head;
		struct column_node* tail;
	};

	bool csv::parse(char* str)
	{
		csv_parse p(str, errmsg, sizeof(errmsg));
		int line_num = 0;
		struct line_node line_head;
		struct line_node* tail;

		line_head.next = NULL;
		tail = &line_head;

		for (; p.next_line();) {
			struct line_node * n = (struct line_node*)malloc(sizeof(*n));
			n->head.next = NULL;
			n->tail = &n->head;
			tail->next = n;
			tail = n;
			int col_num = 0;
			for (;;) {
				char *ptr = NULL;
				if (!p.next_column(&ptr))
					// todo memory leak
					return false;
				if (!ptr)
					break;
				struct column_node * n = (struct column_node*)malloc(sizeof(*n));
				n->next = NULL;
				n->data = ptr;
				tail->tail->next = n;
				tail->tail = n;
				++col_num;
			}
			if (col_num>col_cnt)
				col_cnt = col_num;
			++line_num;
		}
		line_cnt = line_num;
		data = (char**)malloc(sizeof(char*)*line_cnt*col_cnt);
		int line = 0;
		for (struct line_node* cur = line_head.next; cur; ++line) {
			struct line_node* next = cur->next;
			int row = 0;
			for (struct column_node* c = cur->head.next; c; ++row) {
				struct column_node* n = c->next;
				data[line*col_cnt + row] = c->data;
				free(c);
				c = n;
			}
			for (; row < col_cnt; ++row)
				data[line*col_cnt + row] = "";
			free(cur);
			cur = next;
		}
		return true;
	}

	const char* csv::value(int line, int row, const char* def)
	{
		if (line >= line_cnt || row >= col_cnt) return def;
		return data[line*col_cnt + row];
	}

	int csv::value(int line, int row, int def)
	{
		const char* val = value(line, row, (const char*)NULL);
		if (!val) return def;
		return tointeger(val, def);
	}
	double csv::value(int line, int row, double def)
	{
		const char* val = value(line, row, (const char*)NULL);
		if (!val) return def;
		return todouble(val, def);
	}

	bool csvfile::load(const char* path)
	{
		size_t len, r;
		char  *b;
		FILE* f = fopen(path, "rb");
		if (!f)
			goto do_errno;

		fseek(f, 0, SEEK_END);
		len = ftell(f);
		b = (char*)malloc(len + 1);
		b[len] = '\0';
		fseek(f, 0, SEEK_SET);
		r = fread(b, 1, len, f);
		if (r != len)
			goto file_errno;
		fclose(f);
		if (!parse(b)) {
			free(b);
			return false;
		}
		buf = b;
		return true;
	file_errno:
		free(b);
		fclose(f);
	do_errno:
		_snprintf(errmsg, sizeof(errmsg), strerror(errno));
		return false;
	}
}
