#include <assert.h>
#include <stdio.h>
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
					SET_CHAR(ptr, bytes, '\0');
					assert(with_quote>0);
					parse_ptr = ptr + 2;
					need_nextline = 1;
					return true;
				}
				else if (*(ptr + 1) == '\"')
				{
					MV_CHAR(ptr, bytes);
					bytes++;
					ptr++;
				}
				else
				{
					sprintf_s(errmsg, sizeof(errmsg), "unexpected char %c on %s", *ptr, ptr);
					*out = NULL;
					return false;
				}
			}
			break;
			case '\r':
			{
				if (with_quote&&*(ptr + 1) == '\n')
				{
					SET_CHAR(ptr, bytes, '\0');
					assert(with_quote > 0);
					parse_ptr = ptr + 2;
					return true;
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
}