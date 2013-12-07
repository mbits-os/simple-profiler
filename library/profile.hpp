#ifndef __COUNTER_HPP__
#define __COUNTER_HPP__

#include <deque>
#include "ticker.hpp"

namespace profile
{
	typedef unsigned int call_id;
	typedef unsigned int function_id;

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

        template <typename item>
        class findable_container: public container<item>
        {
        protected:
            item& locate(const char* name)
            {
                for (auto& i : m_items)
                {
                    if (!strcmp(i.name(), name))
                        return i;
                }

                m_items.emplace_back(name);
                return m_items.back();
            }

            template <typename Arg>
            item& locate(const char* name, Arg other)
            {
                for (auto& i : m_items)
                {
                    if (!strcmp(i.name(), name))
                        return i;
                }

                m_items.emplace_back(name, other);
                return m_items.back();
            }
        };
    }

    enum ECallFlag
    {
        ECallFlag_SYSCALL = 1
    };

    namespace collecting
    {
        class call
        {
            call_id      m_call;
            call_id      m_parent;
            function_id  m_fn;
            unsigned int m_flags;
            time::type   m_duration;

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

        class section: public impl::container<call>
        {
            const char* m_name;
            function_id m_id;
        public:
            section(const char* name);

            const char* name() const { return m_name; }
            function_id id() const { return m_id; }
            call& call(unsigned int flags = 0);
            void update(const collecting::call& c);
        };

        class function: public impl::findable_container<section>
        {
            const char* m_name;
            const char* m_nice;
        public:

            function(const char* name, const char* nice);
            section& section(const char* name) { return locate(name); }

            const char* name() const { return m_name; }
            const char* nice() const { return m_nice; }
        };

        class profile: public impl::findable_container<function>
        {
        public:
            function& function(const char* name, const char* nice) { return locate(name, nice); }

            call& call(const char* name, const char* nice, const char* suffix, unsigned int flags = 0) { return function(name, nice).section(suffix).call(flags); }
            void update(const char* name, const char* nice, const char* suffix, const collecting::call& c) { function(name, nice).section(suffix).update(c); }
        };

        struct probe
        {
            call& m_call;
            probe* prev;
            static probe*& curr();
            static profile& profile();

            probe(const char* name, const char* raw, const char* suffix, unsigned int flags = 0);
            ~probe();
        };
    }

    namespace print
    {
        void xml_print(const char* filename);
        void binary_print(const char* filename);

        enum EPrinter
        {
            EPrinter_XML,
            EPrinter_BIN
        };

        struct printer
        {
            const char* m_filename;
            EPrinter    m_typeId;
            printer(const char* filename, EPrinter typeId = EPrinter_XML)
                : m_filename(filename)
                , m_typeId(typeId)
            {}

            ~printer()
            {
                switch (m_typeId)
                {
                case EPrinter_XML: xml_print(m_filename); break;
                case EPrinter_BIN: binary_print(m_filename); break;
                }
            }
        };
    }
}

#define FUNCTION_PROBE() profile::collecting::probe __probe(__FUNCDNAME__, __FUNCSIG__, "")
#define SYSCALL_PROBE() profile::collecting::probe __probe(__FUNCDNAME__, __FUNCSIG__, "", profile::ECallFlag_SYSCALL)
#define FUNCTION_PROBE2(name, suffix) profile::collecting::probe name(__FUNCDNAME__, __FUNCSIG__, suffix)

#endif //__COUNTER_HPP__
