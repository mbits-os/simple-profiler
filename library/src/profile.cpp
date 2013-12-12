#include "profile/profile.hpp"

#include <string>
#include <fstream>

#ifdef FEATURE_MT_ENABLED
#include <thread>
#include <list>
#endif // FEATURE_MT_ENABLED

namespace profile
{
#ifdef FEATURE_MT_ENABLED

	namespace mt
	{
		struct curr
		{
			collecting::probe* m_curr;
			std::thread::id m_id;
			curr(): m_curr(nullptr), m_id(std::this_thread::get_id()) {}

			bool operator == (const std::thread::id& id) const { return m_id == id; }
		};

		class threads
		{
			std::list<curr> m_sinks;
			spin_lock m_barrier;

			static threads& inst()
			{
				static threads _;
				return _;
			}

		public:
			static curr& get()
			{
				auto& i = inst();
				std::lock_guard<spin_lock> guard(i.m_barrier);
				(void)(guard); // "unused"

				auto key = std::this_thread::get_id();
				for (auto&& sink: i.m_sinks)
				{
					if (sink == key)
						return sink;
				}

				i.m_sinks.push_back(curr());
				return i.m_sinks.back();
			}
		};
	}

#endif // FEATURE_MT_ENABLED

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

#ifdef FEATURE_IO_WRITE

		probe*& probe::curr()
		{
#ifdef FEATURE_MT_ENABLED
			return mt::threads::get().m_curr;
#else
			static probe* _ = nullptr;
			return _;
#endif
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
#endif // FEATURE_IO_WRITE
	}
}
