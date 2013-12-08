#ifdef FEATURE_IO_READ

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

		if (strings.size() <= h.function_count)
			return false;

		if (is.read(&strings[0], h.function_offset).gcount() != h.function_offset)
			return false;

		strings[h.function_offset] = 0;

		reader::profile builder(out.m_profile);
		for (u32 i = 0; i < h.function_count; ++i)
		{
			file::function fun;
			if (!read(is, fun))
				return false;

			if (!builder.function(fun.id, str(strings, fun.name), str(strings, fun.suffix), flags))
				return false;
		}

		for (u32 i = 0; i < h.call_count; ++i)
		{
			file::call c;
			if (!read(is, c))
				return false;

			if (!builder.call(c.id, c.parent, c.function, c.flags, c.duration, flags))
				return false;
		}

		return true;
	}

}}} // profile::io::binary

#endif // FEATURE_IO_READ
