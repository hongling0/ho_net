#define  _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ini.h"

namespace cfg
{

	ini::ini()
	{
		base = NULL;
	}
	ini::~ini()
	{
		clear();
	}
	
	bool ini::parse_section(char* str)
	{
		char* nm = str + 1;
		char* end = strchr(nm, ']');
		if (!end)
		{
			sprintf_s(errmsg, sizeof(errmsg), "not found section end flag ]");
			return false;
		}
		*end = '\0';
		nm = trim(nm);
		if (*nm == '\0')
		{
			sprintf_s(errmsg, sizeof(errmsg), "empty section name");
			return false;
		}
		section* sec = (section*)malloc(sizeof(*sec));
		sec->name = nm;
		sec->kv = NULL;
		sec->next = base;
		base = sec;
		return true;
	}
	bool ini::parse_keyval(char* str)
	{
		char* key = str;
		char* val = strchr(key, '=');
		if (!val)
		{
			sprintf_s(errmsg, sizeof(errmsg), "not found key value flag =");
			return false;
		}
		*val = '\0';
		key = trim(key);
		if (*key == '\0')
		{
			sprintf_s(errmsg, sizeof(errmsg), "empty keyval name");
			return false;
		}
		val = trim(val + 1);
		keyval * n = (keyval*)malloc(sizeof(*n));
		n->key = key;
		n->val = (*val == '\0') ? NULL : val;
		n->next = base->kv;
		base->kv = n;
		return true;
	}
	bool ini::parse(char* str)
	{
		clear();
		section* sec = (section*)malloc(sizeof(*sec));
		sec->kv = NULL;
		sec->name = "";
		sec->next = base;
		base = sec;
		char* line = str;
		while (line)
		{
			char* nextline = strchr(line, '\n');
			if (nextline)
			{
				nextline[0] = '\0';
				nextline++;
			}
			char* comment = strchr(line, ';');
			if (comment)
				comment[0] = '\0';
			line = trim(line);
			switch (line[0])
			{
			case '\0':
				break;
			case '[':
				if (!parse_section(line))
					return false;
				break;
			default:
				if (!parse_keyval(line))
					return false;
				break;
			}
			line = nextline;
		}
		return true;
	}
	char* ini::trim(char* str)
	{
		char* ret = ltrim(str);
		rtrim(ret);
		return ret;
	}
	char* ini::ltrim(char* str)
	{
		int i = 0;
		for (; str[i] != '\0'; i++)
		{
			if (!isspace(str[i]))
				break;
		}
		return str + i;
	}
	void ini::rtrim(char* str)
	{
		int e = -1;
		for (int i = 0; str[i] != '\0'; i++)
		{
			if (isspace(str[i]))
			{
				if (e < 0)
					e = i;
			}
			else
			{
				if (e >= 0)
					e = -1;
			}
		}
		if (e >= 0)
			str[e] = '\0';
	}
	void ini::clear()
	{
		for (section* sec = base; sec;)
		{
			section* next = sec;
			sec = sec->next;
			for (keyval* kv = next->kv; kv;)
			{
				keyval* n = kv;
				kv = kv->next;
				free(n);
			}
			free(next);
		}
		base = NULL;
		cache = NULL;
		next = NULL;
		errmsg[0] = '\0';
	}
	double ini::value(const char* sec, const char* key, double def)
	{
		const char* val = value(sec, key, (char*)NULL);
		if (!val) return def;
		char* endptr = NULL;
		double n = strtod(val, &endptr);
		if (*endptr != '\0')
		{
			fprintf(stderr, "can't convert to number %s\n", val);
			return def;
		}
		return n;
	}
	int ini::value(const char* sec, const char* key, int def)
	{
		const char* val = value(sec, key, (char*)NULL);
		if (!val) return def;
		char* endptr = NULL;
		long n = strtol(val, &endptr, 0);
		if (*endptr != '\0')
		{
			fprintf(stderr, "can't convert to number %s\n", val);
			return def;
		}
		return n;
	}
	const char* ini::value(const char* sec, const char* key, char* def)
	{
		section * node = NULL;
		if (cache&&strcmp(sec, cache->name) == 0)
			node = cache;
		else
		{
			for (section * n = base; n; n = n->next)
			{
				if (strcmp(sec, n->name) == 0)
					node = cache = n;
			}
		}
		for (keyval * n = node?node->kv:NULL; n; n = n->next)
		{
			if (strcmp(key, n->key) == 0)
				return n->val ? n->val : def;
		}
		return def;
	}

	bool ini::next_section()
	{
		next = next ? next->next : base;
		return next != NULL;
	}
	double ini::value(const char* key, double def)
	{
		const char* val = value(key, (char*)NULL);
		if (!val) return def;
		char* endptr = NULL;
		double n = strtod(val, &endptr);
		if (*endptr != '\0')
		{
			fprintf(stderr, "can't convert to number %s\n", val);
			return def;
		}
		return n;
	}
	int ini::value(const char* key, int def)
	{
		const char* val = value(key, (char*)NULL);
		if (!val) return def;
		char* endptr = NULL;
		long n = strtol(val, &endptr,0.0);
		if (*endptr != '\0')
		{
			fprintf(stderr, "can't convert to number %s\n", val);
			return def;
		}
		return n;
	}
	const char* ini::value(const char* key, char* def)
	{
		for (keyval * n = next?next->kv:NULL; n; n = n->next)
		{
			if (strcmp(key, n->key) == 0)
				return n->val?n->val:def;
		}
		return def;
	}

	inifile::inifile()
	{
		buf = NULL;
		next = NULL;
	}
	inifile::~inifile()
	{
		free(buf);
	}
	bool inifile::load(const char* file)
	{
		FILE* f = fopen(file, "rb");
		if (!f)
		{
			sprintf_s(errmsg, sizeof(errmsg), strerror(errno));
			return false;
		}
		fseek(f, 0, SEEK_END);
		size_t len = ftell(f);
		char*b = (char*)malloc(len + 1);
		b[len] = '\0';
		size_t r = fread(b, 1, len, f);
		if (r != len)
		{
			sprintf_s(errmsg, sizeof(errmsg), strerror(errno));
			fclose(f);
			free(b);
			return false;
		}
		fclose(f);
		if (!parse(b))
		{
			clear();
			free(b);
			return false;
		}
		buf = b;
		return true;
	}

}