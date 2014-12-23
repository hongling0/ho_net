#pragma once

namespace cfg
{
	class ini
	{
		struct keyval
		{
			struct keyval* next;
			char* key;
			char* val;
		};
		struct section
		{
			struct section* next;
			char* name;
			struct keyval* kv;
		};
	public:
		ini();
		~ini();

		bool parse(char* str);
		void clear();
		const char * error() const { return errmsg; }

		double value(const char* sec, const char* key, double def);
		int value(const char* sec, const char* key, int def);
		const char* value(const char* sec, const char* key, char* def);
		
		bool next_section();
		double value(const char* key, double def);
		int value(const char* key, int def);
		const char* value(const char* key, char* def);

	protected:
		bool parse_section(char* str);
		bool parse_keyval(char* str);

		char* trim(char* str);
		char* ltrim(char* str);
		void rtrim(char* str);

	protected:
		struct section* base;
		struct section* cache;
		struct section* next;
		char errmsg[16];
	};

	class inifile : public ini
	{
	public:
		inifile();
		~inifile();
		bool load(const char* file);
	private:
		char * buf;
	};
}