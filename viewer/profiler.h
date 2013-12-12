#ifndef PROFILER_H
#define PROFILER_H

#include <memory>
#include <vector>
#include <QString>
#include <QFile>
#include "profile/profile.hpp"

namespace profile { namespace io { struct file_contents; }}

namespace profiler
{
	typedef unsigned long long call_id;
	typedef unsigned long long function_id;
	typedef unsigned long long time_type;

	template <typename C, typename R> R extract_ret_type(R (C::*)() const);

#define FIELD(klass, name, accessor) \
	struct name \
	{ \
		typedef std::decay<decltype(extract_ret_type(&klass::accessor))>::type type; \
		typedef std::vector<type> vector; \
		static type select(const klass& i) { return i.accessor(); } \
	}

	class function
	{
		QString m_name;
		function_id m_id;
		bool m_is_section;
	public:
		function() {}
		function(function_id id, const QString& name, bool is_section): m_name(name), m_id(id), m_is_section(is_section) {}

		const QString& name() const { return m_name; }
		function_id id() const { return m_id; }
		bool is_section() const { return m_is_section; }

		FIELD(function, name_field,     name);
		FIELD(function, parent_field,   id);
	};

	typedef std::shared_ptr<function> function_ptr;
	typedef std::weak_ptr<function> weak_function_ptr;
	typedef std::vector<function_ptr> functions;

	class call
	{
		call_id      m_id;
		call_id      m_parent;
		function_id  m_function;
		time_type    m_duration;
		time_type    m_detract;
		size_t       m_subcalls;
		unsigned int m_flags;
	public:
		call() {}
		call(call_id id, call_id parent, function_id function, time_type duration, unsigned int flags)
			: m_id(id)
			, m_parent(parent)
			, m_function(function)
			, m_duration(duration)
			, m_detract(0)
			, m_subcalls(0)
			, m_flags(flags)
		{}

		void detract(time_type amount) { m_detract += amount; ++m_subcalls; }

		call_id id() const { return m_id; }
		call_id parent() const { return m_parent; }
		function_id functionId() const { return m_function; }
		time_type duration() const { return m_duration; }
		time_type ownTime() const { return m_duration - m_detract; }
		size_t subcalls() const { return m_subcalls; }
		unsigned int flags() const { return m_flags; }
		bool is_syscall() const { return m_flags & profile::ECallFlag_SYSCALL; }

		FIELD(call, id_field,         id);
		FIELD(call, parent_field,     parent);
		FIELD(call, function_field,   functionId);
		FIELD(call, duration_field,   duration);
		FIELD(call, ownTime_field,    ownTime);
	};

	typedef std::shared_ptr<call> call_ptr;
	typedef std::weak_ptr<call> weak_call_ptr;
	typedef std::vector<call_ptr> calls;

	template <typename T>
	struct vector_selector
	{
		T& ref;
		vector_selector(T& ref): ref(ref) {}

		template <typename F>
		T where(F clause)
		{
			T out;
			for (auto&& e: ref)
			{
				if (e && clause(*e.get()))
					out.push_back(e);
			}

			return out;
		}

		T get() { return ref; }

		operator T() { return ref; }
	};

	template <typename T, typename P>
	struct projection_selector
	{
		typedef typename P::type   type;
		typedef typename P::vector vector;

		T& ref;
		projection_selector(T& ref): ref(ref) {}

		template <typename F>
		vector where(F clause)
		{
			vector out;
			for (auto&& e: ref)
			{
				if (e && clause(*e.get()))
					out.push_back(P::select(*e.get()));
			}

			return out;
		}

		vector get()
		{
			vector out;
			for (auto&& e: ref)
				out.push_back(P::select(e));

			return out;
		}
	};

	template <typename T>
	struct item_selector
	{
		typedef typename T::value_type value_type;
		T& ref;
		item_selector(T& ref): ref(ref) {}

		template <typename F>
		value_type where(F clause)
		{
			for (auto&& e: ref)
			{
				if (e && clause(*e.get()))
					return e;
			}
			return value_type();
		}

		value_type get()
		{
			if (ref.size())
				return ref.front();
			return value_type();
		}
	};

	class data
	{
		profiler::functions m_functions;
		profiler::calls m_calls;
		time_type m_second;

		template <typename T> struct select_data;
		template <> struct select_data<call> {
			typedef item_selector<calls> selector;
			static calls& get_vector(data* pThis) { return pThis->m_calls; }
		};
		template <> struct select_data<function> {
			typedef item_selector<functions> selector;
			static functions& get_vector(data* pThis) { return pThis->m_functions; }
		};
		template <typename T> struct select_data< std::vector< std::shared_ptr<T> > > {
			typedef vector_selector< std::vector< std::shared_ptr<T> > > selector;
			static std::vector< std::shared_ptr<T> >& get_vector(data* pThis) { return select_data<T>::get_vector(pThis); }
		};

		bool xml_open(QFile& file);
		bool bin_open(QFile& file);
		void rebuild_profile(const profile::io::file_contents& file);
	public:
		data(): m_second(1) {}
		bool open(const QString& path);

		time_type second() const { return m_second; }

		template <typename T>
		typename select_data<T>::selector select()
		{
			return typename select_data<T>::selector(select_data<T>::get_vector(this));
		}

		template <typename T, typename P>
		projection_selector<T, P> select()
		{
			return projection_selector<T, P>(select_data<T>::get_vector(this));
		}

		const profiler::functions& functions() const { return m_functions; }
		const profiler::calls& calls() const { return m_calls; }

		profiler::calls selectFunctionCalls(function_id fn)
		{
			return select<profiler::calls>().where([=](const call& c){ return c.functionId() == fn; });
		}

		profiler::calls selectFunctionCalls(function_id fn, call_id called_from)
		{
			return select<profiler::calls>().where([=](const call& c){ return c.functionId() == fn && c.parent() == called_from; });
		}

		call::id_field::vector selectIdsOfFunctionCalls(function_id fn)
		{
			return select<profiler::calls, call::id_field>().where([=](const call& c){ return c.functionId() == fn; });
		}

		call::id_field::vector selectIdsOfFunctionCalls(function_id fn, call_id called_from)
		{
			return select<profiler::calls, call::id_field>().where([=](const call& c){ return c.functionId() == fn && c.parent() == called_from; });
		}

		profiler::calls selectCalledFrom(call_id called_from)
		{
			return select<profiler::calls>().where([=](const call& c){ return c.parent() == called_from; });
		}

		call::id_field::vector selectIdsOfCalledFrom(call_id called_from)
		{
			return select<profiler::calls, call::id_field>().where([=](const call& c){ return c.parent() == called_from; });
		}
	};

	typedef std::shared_ptr<data> data_ptr;
}

#endif // PROFILER_H
