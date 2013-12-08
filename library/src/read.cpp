#include <profile/read.hpp>
#include "binary.hpp"
#include <fstream>

namespace profile { namespace io {

	namespace xml
	{
		bool read(std::istream& is, file_contents& out, int flags);
	}

	namespace binary
	{
		bool read(std::istream& is, file_contents& out, int flags);
	}

	bool is_binary(std::istream& s)
	{
		using namespace io::binary;

		auto pos = s.tellg();
		u64 magic = 0;
		if (!io::binary::read(s, magic))
			return false;

		s.seekg(pos);

		return magic == file::MAGIC;
	}

	bool read(const char* path, file_contents& out, unsigned int flags)
	{
		std::ifstream is(path, std::ios::in | std::ios::binary);

		if (is_binary(is))
			return binary::read(is, out, flags);
		else
			return xml::read(is, out, flags);
	}

}} // profile::io
