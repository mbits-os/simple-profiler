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

			section_t(collecting::section_type<std::string>& ref);
			void call(call_id call, call_id parent, unsigned int flags, time::type duration);
		};

		struct function_t
		{
			collecting::function_type<std::string>& ref;

			function_t(collecting::function_type<std::string>& ref);
			section_t section(const std::string& name, function_id id);
		};

		class profile
		{
			collecting::profile_type<std::string>& ref;

			function_t function(const std::string& name);
			section_t section(function_id id);
		public:
			profile(collecting::profile_type<std::string>& ref);
			bool function(function_id id, const std::string &name, const std::string &suffix, unsigned int reader_flags);
			bool call(call_id id, call_id parent, function_id function, unsigned int call_flags, time::type duration, unsigned int reader_flags);
		};
	};

}}

#endif // __READER_HPP__
