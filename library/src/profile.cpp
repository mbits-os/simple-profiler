#include "profile/profile.hpp"

#include <string>
#include <fstream>

namespace profile
{
	namespace collecting
	{
		call::call(call_id call, function_id fn, unsigned int flags)
			: m_call(call)
			, m_parent(0)
			, m_fn(fn)
			, m_flags(flags)
			, m_duration(0)
		{
		}

#ifdef FEATURE_IO_READ
		call::call(call_id call, call_id parent, function_id fn, unsigned int flags, time::type duration)
			: m_call(call)
			, m_parent(parent)
			, m_fn(fn)
			, m_flags(flags)
			, m_duration(duration)
		{
		}
#endif // FEATURE_IO_READ

		void call::set_parent(call_id parent) { m_parent = parent; }

		void call::start()
		{
			m_duration = time::now();
		}

		void call::stop()
		{
			m_duration = time::now() - m_duration;
		}

		function_id next_section()
		{
			static function_id next_id = 0;
			return ++next_id;
		}

		call_id next_call()
		{
			static call_id next_id = 0;
			return ++next_id;
		}

		probe*& probe::curr()
		{
			static probe* _ = nullptr;
			return _;
		}

		profile_type<const char*>& probe::profile()
		{
			static collecting::profile_type<const char*> _;
			return _;
		}

		probe::probe(const char* name, const char* nice, const char* suffix, unsigned int flags)
			: m_call(profile().call(name, nice, suffix, flags))
			, prev(curr())
		{
			curr() = this;

			if (prev)
				m_call.set_parent(prev->m_call.id());

			m_call.start();
		}

		probe::~probe()
		{
			curr() = prev;
			m_call.stop();
		}
	}
}
