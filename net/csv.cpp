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
						need_nextline = 1;
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
		list = NULL;
	}
	csv::~csv()
	{
		for (int i = 0; i < line_cnt; i++)
		{
			free(list[i]);
		}
		free(list);
	}

	struct column_node
	{
		struct column_node * next;
		char * data;
	};
	bool csv::parse(char* data)
	{
		csv_parse p(data);
		if(!p.next_line()) return true;
		struct column_node head_node = { NULL, NULL };
		struct column_node *tail_node = &head_node;
		for (;;)
		{
			char *ptr = NULL;
			if (!p.next_column(&ptr))
				return false;
			struct column_node * n = (struct column_node*)malloc(sizeof(*n));
			n->next = NULL;
			n->data = ptr;
			tail_node->next = n;
			tail_node = n;
			++col_cnt;
		}
		size_t sz = sizeof(struct line) + sizeof(char*)*(col_cnt - 1);
		struct line* l = (struct line*)malloc(sz);
		l->next = NULL;
		struct column_node * cur = head_node.next;
		for (int i = 0; i < col_cnt; i++)
		{
			l->column[i] = cur->data;
			struct column_node * next = cur->next;
			free(cur);
			cur = next;
		}
		tail->next = l;
		tail = l;
		line_cnt = 1;
		while (p.next_line())
		{
			line_cnt++;
			size_t sz = sizeof(struct line) + sizeof(char*)*(col_cnt - 1);
			struct line* l = (struct line*)malloc(sz);
			l->next = NULL;
			tail->next = l;
			tail = l;
			int cnt = 0;
			for (;;)
			{
				char* ptr = NULL;
				if (!p.next_column(&ptr))
					return false;
				if (cnt >= col_cnt)
					return false;
				l->column[cnt++] = ptr;
			}
		}
	}
}


