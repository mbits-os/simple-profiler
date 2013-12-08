#include <profile/read.hpp>
#include "binary.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include "reader.hpp"

namespace profile { namespace io { namespace binary {

	std::string str(const std::vector<char>& strings, u32 offset)
	{
		if (offset >= strings.size())
			return std::string();

		return std::string(&strings[offset]);
	}

	bool read(std::istream& is, file_contents& out, int flags)
	{
		u64 magic = 0xC0C0C0C0C0C0C0C0ull;
		if (!read(is, magic) || magic != file::MAGIC)
			return false;

		file::header h;
		if (!read(is, h))
			return false;

		if (h.version != file::VERSION)
			return false;

		if (h.function_offset % 4)
			return false;

		if ((h.call_offset - h.function_offset) / sizeof(file::function) != h.function_count)
			return false;

		if (!h.second)
			h.second = 1;

		out.m_second = h.second;

		std::vector<char> strings(h.function_offset + 1);
		std::vector<file::function> funcs(h.function_count);

		if (funcs.size() < h.function_count)
			return false;

		if (strings.size() <= h.function_count)
			return false;

		if (is.read(&strings[0], h.function_offset).gcount() != h.function_offset)
			return false;

		strings[h.function_offset] = 0;

		u64 size = h.function_count * sizeof(file::function);
		if (is.read((char*) &funcs[0], size).gcount() != size)
			return false;

		reader::profile builder(out.m_profile);
		for (auto&& fun: funcs)
		{
			builder.function(str(strings, fun.name)).section(str(strings, fun.suffix), fun.id);
		}

		for (u32 i = 0; i < h.call_count; ++i)
		{
			file::call c;
			if (!read(is, c))
				return false;

			try
			{
				builder.section(c.function).call(c.id, c.parent, c.flags, c.duration);
			}
			catch(reader::bad_section)
			{
				if (flags & FAIL_UNKNOWN_FUNCTION)
					return false;

				std::ostringstream os;
				os << "<unknown-" << c.function << ">";
				try
				{
					builder
						.function(os.str())
						.section(std::string(), c.function)
						.call(c.id, c.parent, c.flags, c.duration);
				}
				catch(reader::bad_section)
				{
					return false;
				}
			}
		}

		return true;
	}

}}} // profile::io::binary

