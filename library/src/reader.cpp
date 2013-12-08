#ifdef FEATURE_IO_READ

#include "reader.hpp"
#include <profile/read.hpp>
#include <sstream>

namespace profile { namespace io {

	reader::section_t::section_t(collecting::section_type<std::string>& ref) : ref(ref) {}

	void reader::section_t::call(call_id call, call_id parent, unsigned int flags, time::type duration)
	{
		add_call(ref, call, parent, flags, duration);
	}

	reader::function_t::function_t(collecting::function_type<std::string>& ref) : ref(ref) {}

	reader::section_t reader::function_t::section(const std::string& name, function_id id)
	{
		return section_t(add_section(ref, name, id));
	}

	reader::profile::profile(collecting::profile_type<std::string>& ref) : ref(ref) {}

	reader::function_t reader::profile::function(const std::string& name) { return function_t(ref.function(name, std::string())); }

	reader::section_t reader::profile::section(function_id id)
	{
		for (auto&& f: ref.items())
		{
			for (auto&& s: f.items())
			{
				if (s.id() == id)
					return section_t(s);
			}
		}
		throw bad_section(id);
	}

	bool reader::profile::function(function_id id, const std::string &name, const std::string &suffix, unsigned int /*reader_flags*/)
	{
		function(name).section(suffix, id);
		return true;
	}

	bool reader::profile::call(call_id id, call_id parent, function_id function, unsigned int call_flags, time::type duration, unsigned int reader_flags)
	{
		try
		{
			section(function).call(id, parent, call_flags, duration);
		}
		catch(reader::bad_section)
		{
			if (reader_flags & FAIL_UNKNOWN_FUNCTION)
				return false;

			std::ostringstream os;
			os << "<unknown-" << function << ">";
			try
			{
				this->function(os.str())
					.section(std::string(), function)
					.call(id, parent, call_flags, duration);
			}
			catch(reader::bad_section)
			{
				return false;
			}
		}

		return true;
	}

}}

#endif // FEATURE_IO_READ
