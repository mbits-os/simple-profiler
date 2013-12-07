#include <profile/read.hpp>
#include "binary.hpp"
#include <fstream>

namespace profile { namespace io {

	namespace xml
	{
		bool read(std::istream& is, collecting::profile& out);
	}

	namespace binary
	{
		bool read(std::istream& is, collecting::profile& out);
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

	bool read(const char* path, collecting::profile& out)
	{
		std::ifstream is(path, std::ios::in | std::ios::binary);

		if (is_binary(is))
			return binary::read(is, out);
		else
			return xml::read(is, out);
	}

}} // profile::io
