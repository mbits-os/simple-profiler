#include "profile/profile.hpp"

#include <regex>

namespace profile { namespace io {

	static void secs(time::type second, time::type ticks)
	{
		time::type seconds = ticks / second;
		//seconds /= 1000;
		//printf("%llu/%llu/%llu\n", second, ticks, seconds);

		if (seconds > 59)
		{
			seconds /= 1000;
			ticks -= (seconds / 60) * 60 * second;
			printf("(%3llum%llu.%03llus)", seconds / 60, ticks / second, (ticks * 1000 / second) % 1000);
		}
		else
		{
			const char* freq = "s ";
			if (ticks * 10 / second < 9)
			{
				freq = "ms";
				ticks *= 1000;
				if (ticks * 10 / second < 9)
				{
					freq = "us";
					ticks *= 1000;
					if (ticks * 10 / second < 9)
					{
						freq = "ns";
						ticks *= 1000;
					}
				}
			}

			printf("(%3llu.%03llu%s)", ticks / second, (ticks * 1000 / second) % 1000, freq);
		}
	}

	std::string fold(std::string name)
	{
		//name = std::regex_replace(name, std::regex("unsigned char"), "uchar");
		//name = std::regex_replace(name, std::regex("unsigned short"), "ushort");
		//name = std::regex_replace(name, std::regex("unsigned int"), "uint");
		//name = std::regex_replace(name, std::regex("unsigned long"), "ulong");
		name = std::regex_replace(name, std::regex("(class )|(struct )|(enum )|(__thiscall )|(__cdecl )"), "");
		name = std::regex_replace(name, std::regex("\\(void\\)"), "()");
		name = std::regex_replace(name, std::regex("std::basic_string<([_a-zA-Z0-9]+),std::char_traits<\\1>,std::allocator<\\1> >"), "std::basic_string<$1>");
		name = std::regex_replace(name, std::regex("std::basic_string<char>"), "std::string");
		name = std::regex_replace(name, std::regex("std::basic_string<wchar_t>"), "std::wstring");
		name = std::regex_replace(name, std::regex("std::vector<([_a-zA-Z0-9:<>,]+),std::allocator<\\1> >"), "std::vector<$1>");
		name = std::regex_replace(name, std::regex("std::set<([_a-zA-Z0-9:<>,]+),std::less<\\1 >,std::allocator<\\1 > >"), "std::set<$1>");
		name = std::regex_replace(name, std::regex("std::map<([_a-zA-Z0-9:<>,]+),([_a-zA-Z0-9:<>,]+),std::less<\\1 >,std::allocator<std::pair<\\1 const ,\\2 > > >"), "std::map<$1,$2>");
		return name;
	}

	namespace xml
	{
		void print(const char* filename);
	}

	namespace binary
	{
		void print(const char* filename);
	}

	void xml_print(const char* filename)
	{
		collecting::call self_probe(0, 0);
		self_probe.start();

		xml::print(filename);

		self_probe.stop();
		printf("xml_print took ");
		secs(time::second(), self_probe.duration());
		printf("\n");
	}

	void binary_print(const char *filename)
	{
		collecting::call self_probe(0, 0);
		self_probe.start();

		binary::print(filename);

		self_probe.stop();
		printf("binary_print took ");
		secs(time::second(), self_probe.duration());
		printf("\n");
	}

}} // profile::io
