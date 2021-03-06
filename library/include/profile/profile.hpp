#ifndef __PROFILE_HPP__
#define __PROFILE_HPP__

#include <deque>
#include "ticker.hpp"

#ifdef FEATURE_MT_ENABLED
#include <atomic>
#include <mutex>
#endif // FEATURE_MT_ENABLED

namespace profile
{
	typedef unsigned int call_id;
	typedef unsigned int function_id;

	template <typename string_t>
	struct string_ref
	{
		typedef const string_t& type;

		static bool equals(type lhs, type rhs) { return lhs == rhs; }
	};

	template <typename char_t>
	struct string_ref<const char_t*>
	{
		typedef const char_t* type;
		static bool equals(type lhs, type rhs) { return !strcmp(lhs, rhs); }
	};

	namespace impl
	{
		template <typename item>
		class container
		{
		public:
			typedef std::deque<item> items;
			typedef typename items::const_iterator const_iterator;

			const_iterator begin() const { return m_items.begin(); }
			const_iterator end() const { return m_items.end(); }

		protected:
			items m_items;
		};

		template <typename item, typename string_t>
		class findable_container: public container<item>
		{
		protected:
			typedef typename string_ref<string_t>::type string_arg;
			item& locate(string_arg name)
			{
				for (auto& i : m_items)
				{
					if (string_ref<string_t>::equals(i.name(), name))
						return i;
				}

				m_items.emplace_back(name);
				return m_items.back();
			}

			template <typename Arg>
			item& locate(string_arg name, Arg other)
			{
				for (auto& i : m_items)
				{
					if (string_ref<string_t>::equals(i.name(), name))
						return i;
				}

				m_items.emplace_back(name, other);
				return m_items.back();
			}
		};
	}

#ifdef FEATURE_IO_READ
	namespace io
	{
		class reader;
	}
#endif // FEATURE_IO_READ

	enum ECallFlag
	{
		ECallFlag_SYSCALL = 1
	};

#ifdef FEATURE_MT_ENABLED

	namespace mt
	{
		class spin_lock
		{
			std::atomic<bool> _lock;
		public:
			spin_lock(): _lock(false) {}

			void lock()
			{
				while (_lock.exchange(true)) {}
			}

			void unlock()
			{
				_lock = false;
			}
		};
	}

#endif // FEATURE_MT_ENABLED

	namespace collecting
	{
		class call
		{
			call_id      m_call;
			call_id      m_parent;
			function_id  m_fn;
			unsigned int m_flags;
			time::type   m_duration;

#ifdef FEATURE_IO_READ
			template <typename string_t>
			friend class section_type;

			call(call_id call, call_id parent, function_id fn, unsigned int flags, time::type duration);
#endif // FEATURE_IO_READ

		public:
			call(call_id call, function_id fn, unsigned int flags = 0);
			void set_parent(call_id parent);

			call_id id() const { return m_call; }
			call_id parent() const { return m_parent; }
			function_id function() const { return m_fn; }
			unsigned int flags() const { return m_flags; }
			bool isSysCall() const { return m_flags & ECallFlag_SYSCALL; }
			time::type duration() const { return m_duration; }

			void start();
			void stop();
		};

		function_id next_section();
		call_id next_call();

		template <typename string_t>
		class section_type: public impl::container<collecting::call>
		{
			string_t m_name;
			function_id m_id;

		public:
			typedef typename string_ref<string_t>::type string_arg;

			section_type(string_arg name)
				: m_name(name)
				, m_id(next_section())
			{}

			string_arg name() const { return m_name; }
			function_id id() const { return m_id; }
			collecting::call& call(unsigned int flags = 0)
			{
				call_id id = next_call();
				m_items.emplace_back(id, m_id, flags);
				return m_items.back();
			}

			void update(const collecting::call& c)
			{
				for (auto& i : m_items)
				{
					if (i.id() == c.id())
					{
						i = c;
						break;
					}
				}
			}

#ifdef FEATURE_IO_READ
		private:
			friend class io::reader;
			template <typename string_t>
			friend class function_type;

			section_type(string_arg name, function_id id)
				: m_name(name)
				, m_id(id)
			{}

			void add_call(call_id call, call_id parent, unsigned int flags, time::type duration)
			{
				m_items.push_back(collecting::call(call, parent, m_id, flags, duration));
			}
#endif // FEATURE_IO_READ
		};

		template <typename string_t>
		class function_type: public impl::findable_container<section_type<string_t>, string_t>
		{
			string_t m_name;
			string_t m_nice;

#ifdef FEATURE_IO_READ
			friend class io::reader;

			section_type<string_t>& add_section(string_arg name, function_id id)
			{
				for (auto& i : m_items)
				{
					if (string_ref<string_t>::equals(i.name(), name))
						return i;
				}

				m_items.push_back(section_type<string_t>(name, id));
				return m_items.back();
			}

			items& items() { return m_items; }
#endif // FEATURE_IO_READ

		public:
			function_type(string_arg name, string_arg nice)
				: m_name(name)
				, m_nice(nice)
			{}
			section_type<string_t>& section(string_arg name) { return locate(name); }

			string_arg name() const { return m_name; }
			string_arg nice() const { return m_nice; }
		};

		template <typename string_t>
		class profile_type: public impl::findable_container<function_type<string_t>, string_t>
		{
		public:
			function_type<string_t>& function(string_arg name, string_arg nice) { return locate(name, nice); }

			collecting::call& call(string_arg name, string_arg nice, string_arg suffix, unsigned int flags = 0)
			{
#ifdef FEATURE_MT_ENABLED
				static mt::spin_lock barrier;
				std::lock_guard<mt::spin_lock> guard(barrier);
				(void)(guard);
#endif // FEATURE_MT_ENABLED

				return function(name, nice).section(suffix).call(flags);
			}
			void update(string_arg name, string_arg nice, string_arg suffix, const collecting::call& c) { function(name, nice).section(suffix).update(c); }

#ifdef FEATURE_IO_READ
		private:
			friend class io::reader;
			items& items() { return m_items; }
#endif // FEATURE_IO_READ
		};

#ifdef FEATURE_IO_WRITE
		struct probe
		{
			call& m_call;
			probe* prev;
			static probe*& curr();
			static profile_type<const char*>& profile();

			probe(const char* name, const char* raw, const char* suffix, unsigned int flags = 0);
			~probe();
		};
#endif // FEATURE_IO_WRITE
	}
}

#ifdef FEATURE_IO_WRITE
#	define FUNCTION_PROBE() profile::collecting::probe __probe(__FUNCDNAME__, __FUNCSIG__, "")
#	define SYSCALL_PROBE() profile::collecting::probe __probe(__FUNCDNAME__, __FUNCSIG__, "", profile::ECallFlag_SYSCALL)
#	define FUNCTION_PROBE2(name, suffix) profile::collecting::probe name(__FUNCDNAME__, __FUNCSIG__, suffix)
#else
#	define FUNCTION_PROBE()
#	define SYSCALL_PROBE()
#	define FUNCTION_PROBE2(name, suffix)
#endif // FEATURE_IO_WRITE

#endif // __PROFILE_HPP__
