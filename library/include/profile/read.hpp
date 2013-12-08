#ifndef __READ_HPP__
#define __READ_HPP__

#include "profile.hpp"

namespace profile { namespace io {

	struct file_contents
	{
		collecting::profile m_profile;
		time::type m_second;

		file_contents(): m_second(1) {}
	};

	bool read(const char* path, file_contents& out);

}} // profile::io

#endif // READ_HPP__
