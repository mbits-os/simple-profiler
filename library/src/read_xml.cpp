#include <profile/read.hpp>
#include <iostream>
#include "expat.hpp"
#include "reader.hpp"

namespace profile { namespace io { namespace xml {

#define EXPECT_BREAK(_name) if (!strcmp(name, _name)) break
#define EXPECT(_name) if (strcmp(name, _name)) { ok = false; return; }
#define FOR_EACH_ATTR() for (; *attrs; attrs += 2)
#define ATTR(name) if (!strcmp(attrs[0], #name)) set(name, attrs[1]); else

	class ProfilerParser : public ::xml::ExpatBase<ProfilerParser>
	{
		file_contents& out;
		reader::profile builder;
		unsigned int flags;
		bool ok;

		enum Stage
		{
			OUTSIDE,
			STATS,
			FUNCTIONS,
			FUN_READ,
			CALLS,
			CALLS_READ,
			ALL_READ
		};

		Stage stage;

		template <typename T>
		bool _atoUI(T& var, const char* c)
		{
			while(*c)
			{
				switch(*c)
				{
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					var *= 10;
					var += *c - '0';
					break;
				default:
					return false;
				}

				++c;
			}
			return true;
		}

		void set(std::string& var, const XML_Char* attr) { var = attr; }
		void set(unsigned int& var, const XML_Char* attr) { ok = _atoUI(var, attr); }
		void set(unsigned long long& var, const XML_Char* attr) { ok = _atoUI(var, attr); }

		void readStats(const XML_Char **attrs)
		{
			time::type second = 0;

			FOR_EACH_ATTR()
			{
				ATTR(second)
				{}
			}

			if (!second)
				second = 1;

			out.m_second = second;
		}

		void readFunction(const XML_Char **attrs)
		{
			function_id id = 0;
			std::string name;
			std::string suffix;

			FOR_EACH_ATTR()
			{
				ATTR(id)
				ATTR(name)
				ATTR(suffix)
				{}
			}

			if (!id)
			{
				ok = false;
				return;
			}

			ok = builder.function(id, name, suffix, flags);
		}

		void readCall(const XML_Char **attrs)
		{
			call_id id = 0;
			call_id parent = 0;
			function_id function = 0;
			unsigned int flags = 0;
			time::type duration = 0;

			FOR_EACH_ATTR()
			{
				ATTR(id)
				ATTR(parent)
				ATTR(function)
				ATTR(flags)
				ATTR(duration)
				{}
			}

			if (!id || !function)
			{
				ok = false;
				return;
			}

			ok = builder.call(id, parent, function, flags, duration, this->flags);
		}

	public:

		ProfilerParser(file_contents& out, unsigned int flags)
			: out(out)
			, builder(out.m_profile)
			, flags(flags)
			, ok(true)
			, stage(OUTSIDE)
		{}

		operator bool () const { return ok; }

		void onStartElement(const XML_Char *name, const XML_Char **attrs)
		{
			if (!ok) return;
			switch(stage)
			{
			case OUTSIDE:
				EXPECT("stats");
				stage = STATS;
				readStats(attrs);
				break;

			case STATS:
				EXPECT("functions");
				stage = FUNCTIONS;
				break;

			case FUNCTIONS:
				EXPECT("fn");
				readFunction(attrs);
				break;

			case FUN_READ:
				EXPECT("calls");
				stage = CALLS;
				break;

			case CALLS:
				EXPECT("call");
				readCall(attrs);
				break;

			default:
				ok = false;
			}
		}

		void onEndElement(const XML_Char *name)
		{
			if (!ok) return;
			switch(stage)
			{
			case FUNCTIONS:
				EXPECT_BREAK("fn");
				EXPECT("functions");
				stage = FUN_READ;
				break;

			case CALLS:
				EXPECT_BREAK("call");
				EXPECT("calls");
				stage = CALLS_READ;
				break;

			case CALLS_READ:
				EXPECT("stats");
				stage = ALL_READ;
				break;

			default:
				ok = false;
			}
		}
	};

#undef EXPECT_BREAK
#undef EXPECT
#undef FOR_EACH_ATTR
#undef ATTR

	bool read(std::istream& is, file_contents& out, int flags)
	{
		ProfilerParser parser(out, flags);
		if (!parser.create(nullptr)) return false;
		parser.enableElementHandler();

		char buffer[8192];
		std::streamsize read;
		while ((read = is.read(buffer, sizeof(buffer)).gcount()) != 0)
		{
			if (!parser.parse(buffer, (int) read, false) || !parser)
				return false;
		}

		return parser.parse(buffer, 0) && parser;
	}

}}} // profile::io::xml
