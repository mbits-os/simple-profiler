#include "profile.hpp"

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

        void call::set_parent(call_id parent) { m_parent = parent; }

        void call::start()
        {
            m_duration = time::now();
        }

        void call::stop()
        {
            m_duration = time::now() - m_duration;
        }

        static function_id next_section()
        {
            static function_id next_id = 0;
            return ++next_id;
        }

        section::section(const char* name)
            : m_name(name)
            , m_id(next_section())
        {
        }

        call& section::call(unsigned int flags)
        {
            static call_id next_id = 0;
            call_id id = ++next_id;
            m_items.emplace_back(id, m_id, flags);
            return m_items.back();
        }

        void section::update(const collecting::call& c)
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

        function::function(const char* name, const char* nice)
            : m_name(name)
            , m_nice(nice)
        {
        }

        probe*& probe::curr()
        {
            static probe* _ = nullptr;
            return _;
        }

        profile& probe::profile()
        {
            static collecting::profile _;
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
