#ifndef __READ_HPP__
#define __READ_HPP__

#ifdef FEATURE_IO_READ

#include "profile.hpp"
#include <string>

namespace profile { namespace io {

	struct file_contents
	{
		collecting::profile_type<std::string> m_profile;
		time::type m_second;

		file_contents(): m_second(1) {}
	};

	enum
	{
		FAIL_UNKNOWN_FUNCTION = 0x00000001
	};

	bool read(const char* path, file_contents& out, unsigned int flags = FAIL_UNKNOWN_FUNCTION);

}} // profile::io

#endif // FEATURE_IO_READ

#endif // __READ_HPP__
