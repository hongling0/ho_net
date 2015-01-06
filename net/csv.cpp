#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv.h"


#define SET_CHAR(ptr,n,c) do{ *(ptr-n)=c; } while(0)
#define MV_CHAR(ptr,n) do{ if(n>0)*(ptr-n)=*ptr; } while(0)

namespace cfg
{
	bool csv_parse::next_column(char** out)
	{
		*out = NULL;
		if (!parse_ptr) return true;
		if (need_nextline) return true;
		int with_quote = *parse_ptr == '"' ? 1 : 0;
		char* ptr = with_quote > 0 ? (parse_ptr + 1) : parse_ptr;
		*out = ptr;
		int bytes = 0;
		for (;; ++ptr)
		{
			switch (*ptr)
			{
			case '\0':
			{
				parse_ptr = NULL;
				return true;
			}
			break;
			case '\"':
			{
				if (*(ptr + 1) == ',' || (*(ptr + 1) == '\r'&&*(ptr + 2) == '\n'))
				{
					if (with_quote==0)
					{
						*out = NULL;
						_snprintf(errmsg, sizeof(errmsg), "unexpected char %c on %s", *ptr, ptr);
						return false;
					}
					else
					{
						SET_CHAR(ptr, bytes, '\0');
						parse_ptr = ptr + 2;
						if (*(ptr + 1) == '\r'&&*(ptr + 2) == '\n')
						{
							need_nextline = 1;
							parse_ptr++;
						}
						return true;
					}
				}
				else if (*(ptr + 1) == '\"')
				{
					MV_CHAR(ptr, bytes);
					bytes++;
					ptr++;
				}
				else
				{
					_snprintf(errmsg, sizeof(errmsg), "unexpected char %c on %s", *ptr, ptr);
					*out = NULL;
					return false;
				}
			}
			break;
			case '\r':
			{
				if (with_quote==0&&*(ptr + 1) == '\n')
				{
					SET_CHAR(ptr, bytes, '\0');
					parse_ptr = ptr + 2;
					need_nextline = 1;
					return true;
				}
				else
				{
					MV_CHAR(ptr, bytes);
				}
			}
			break;
			case ',':
			{
				if (with_quote == 0)
				{
					SET_CHAR(ptr, bytes, '\0');
					parse_ptr = ptr + 1;
					return true;
				}
				else
				{
					MV_CHAR(ptr, bytes);
				}
			}
			break;
			default:
			{
				MV_CHAR(ptr, bytes);
			}
			break;
			}
		}
	}
	csv::csv()
	{
		line_cnt = 0;
		col_cnt = 0;
		line_cap = 0;
		data = NULL;
	}
	csv::~csv()
	{
		for (int i = 0; i < line_cnt; i++)
		{
			free(data[i]);
		}
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
		csv_parse p(str);
		struct line_node line_head;
		struct line_node* tail;;
		line_head.next = NULL;
		tail = &line_head;
		int line_num = 0;
		for (;p.next_line();)
		{
			struct line_node * n = (struct line_node*)malloc(sizeof(*n));
			n->head.next = NULL;
			n->tail = &n->head;
			tail->next = n;
			tail = n;
			int col_num = 0;
			for (;;)
			{
				char *ptr = NULL;
				if (!p.next_column(&ptr))
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
			col_cnt = col_num;
			++line_num;
		}
		line_cnt = line_num;
		data = (char**)malloc(sizeof(char*)*line_cnt*col_cnt);
		int line = 0;
		for (struct line_node* cur=line_head.next;cur;++line)
		{
			struct line_node* next=cur->next;
			int row = 0;
			for (struct column_node* c = cur->head.next; c;++row)
			{
				struct column_node* next = c->next;
				data[line*col_cnt + row] = c->data;
				free(c);
				c = next;
			}
			free(cur);
			cur = next;
		}
		return true;
	}

	int csvfile::load(const char* path)
	{
		FILE* f = fopen(path, "rb");
		if (!f)
			return errno;
		fseek(f, 0, SEEK_END);
		size_t len = ftell(f);
		char* b = (char*)malloc(len + 1);
		b[len] = '\0';
		fseek(f, 0, SEEK_SET);
		size_t r = fread(b, 1, len, f);
		if (r != len)
		{
			int err = errno;
			fclose(f);
			free(b);
			return err;
		}
		fclose(f);
		int err = parse(b);
		if (err)
		{
			free(b);
			return err;
		}
		buf = b;
		return 0;
	}
}


