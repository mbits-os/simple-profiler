#ifndef __READER_HPP__
#define __READER_HPP__

#include <profile/profile.hpp>
#include <string>

namespace profile { namespace io {

	class reader
	{
		static collecting::section_type<std::string>& add_section(
				collecting::function_type<std::string>& fun,
				const std::string& name, function_id id)
		{
			return fun.add_section(name, id);
		}

		static void add_call(
				collecting::section_type<std::string>& section,
				call_id call, call_id parent, unsigned int flags, time::type duration)
		{
			section.add_call(call, parent, flags, duration);
		}

	public:

		class bad_section: public std::runtime_error
		{
			function_id m_id;
		public:
			explicit bad_section(function_id id): std::runtime_error("Bad function id"), m_id(id) {}
			function_id id() const { return m_id; }
		};

		struct section_t
		{
			collecting::section_type<std::string>& ref;

			section_t(collecting::section_type<std::string>& ref) : ref(ref) {}
			void call(call_id call, call_id parent, unsigned int flags, time::type duration)
			{
				add_call(ref, call, parent, flags, duration);
			}
		};

		struct function_t
		{
			collecting::function_type<std::string>& ref;

			function_t(collecting::function_type<std::string>& ref) : ref(ref) {}
			section_t section(const std::string& name, function_id id)
			{
				return section_t(add_section(ref, name, id));
			}
		};

		struct profile
		{
			collecting::profile_type<std::string>& ref;

			profile(collecting::profile_type<std::string>& ref) : ref(ref) {}
			function_t function(const std::string& name) { return function_t(ref.function(name, std::string())); }

			section_t section(function_id id)
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
		};
	};

}}

#endif // __READER_HPP__
