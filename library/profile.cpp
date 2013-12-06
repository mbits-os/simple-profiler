#include "profile.hpp"

#include <regex>
#include <string>
#include <fstream>
#include <chrono>

namespace profile
{
    namespace collecting
    {
        call::call(call_id call, function_id fn)
            : m_call(call)
            , m_parent(0)
            , m_fn(fn)
            , m_timestamp(0)
            , m_duration(0)
        {
        }

        void call::set_parent(call_id parent) { m_parent = parent; }

        void call::start()
        {
            using namespace std::chrono;
            auto duration = system_clock::now().time_since_epoch();
            m_timestamp = (time::type)duration_cast<milliseconds>(duration).count();

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

        call& section::call()
        {
            static call_id next_id = 0;
            call_id id = ++next_id;
            m_items.emplace_back(id, m_id);
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

        probe::probe(const char* name, const char* nice, const char* suffix)
            : m_call(profile().call(name, nice, suffix))
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


    namespace print
    {
        static void secs(time::type second, time::type ticks);

        static std::string fold(std::string name)
        {
            //name = std::regex_replace(name, std::regex("unsigned char"), "uchar");
            //name = std::regex_replace(name, std::regex("unsigned short"), "ushort");
            //name = std::regex_replace(name, std::regex("unsigned int"), "uint");
            //name = std::regex_replace(name, std::regex("unsigned long"), "ulong");
            name = std::regex_replace(name, std::regex("(class )|(struct )|(enum )|(__thiscall )|(__cdecl )"), "");
            name = std::regex_replace(name, std::regex("\\(void\\)"), "()");
            name = std::regex_replace(name, std::regex("std::basic_string<([_a-zA-Z0-9]+),std::char_traits<\\1>,std::allocator<\\1> >"), "std::basic_string<$1>");
            name = std::regex_replace(name, std::regex("std::basic_string<char>"), "std::string");
            name = std::regex_replace(name, std::regex("std::basic_string<wchar_t>"), "std::wstring");
            name = std::regex_replace(name, std::regex("std::vector<([_a-zA-Z0-9:<>,]+),std::allocator<\\1> >"), "std::vector<$1>");
            name = std::regex_replace(name, std::regex("std::set<([_a-zA-Z0-9:<>,]+),std::less<\\1 >,std::allocator<\\1 > >"), "std::set<$1>");
            name = std::regex_replace(name, std::regex("std::map<([_a-zA-Z0-9:<>,]+),([_a-zA-Z0-9:<>,]+),std::less<\\1 >,std::allocator<std::pair<\\1 const ,\\2 > > >"), "std::map<$1,$2>");
            return name;
        }

        namespace xml
        {
            static std::string xml(std::string attr)
            {
                attr = std::regex_replace(attr, std::regex("&"), "&amp;");
                attr = std::regex_replace(attr, std::regex("<"), "&lt;");
                attr = std::regex_replace(attr, std::regex(">"), "&gt;");
                return std::regex_replace(attr, std::regex("\""), "&quot;");
            }

            void print(const char* filename)
            {
                collecting::call self_probe(0, 0);
                self_probe.start();

                auto profile = collecting::probe::profile();

                std::ofstream os(std::string(filename) + ".xcount");
                os << "<stats second=\"" << time::second() << "\">\n\t<calls>\n";

                {
                    std::vector<collecting::call> calls;

                    for (auto& f : profile) for (auto& s : f) for (auto& c : s)
                        calls.push_back(c);

                    std::sort(begin(calls), end(calls), [](const collecting::call& lhs, const collecting::call& rhs) { return lhs.id() < rhs.id(); });

                    for (auto& c : calls)
                    {
                        os << "\t\t<call id=\"" << c.id() << "\"";
                        if (c.parent())
                            os << " parent=\"" << c.parent() << "\"";
                        if (c.function())
                            os << " function=\"" << c.function() << "\"";
                        os << " timestamp=\"" << c.timestamp() << "\" duration=\"" << c.duration() << "\" />\n";
                    }
                }

                os << "\t</calls>\n\t<functions>\n";
                for (auto& f : profile)
                {
                    for (auto& s : f)
                    {
                        os << "\t\t<fn id=\"" << s.id() << "\" name=\"" << xml(fold(f.nice()));
                        if (s.name() && *s.name())
                            os << "\"\n\t\t    suffix=\"" << s.name();
                        os << "\"/>\n";
                    }
                }

                os << "\t</functions>\n</stats>\n";

                self_probe.stop();

                printf("xml_print took ");
                secs(time::second(), self_probe.duration());
                printf("\n");
            }
        }

        namespace binary
        {
            template <typename T>
            void write(std::ofstream& os, const T& t)
            {
                os.write((const char*) &t, sizeof(t));
            }

            typedef unsigned int u32;
            typedef unsigned long long u64;

            struct string
            {
                u32 offset;
                std::string value;

                string(u32 offset, const std::string& value)
                    : offset(offset)
                    , value(value)
                {
                }
            };


            struct strings
            {
                typedef std::vector<string> strings_t;
                strings_t value;
                u32 offset;

                strings()
                    : offset(0)
                {
                    add(std::string()); // "null" string
                }

                u32 add(const std::string& s)
                {
                    for (auto&& _s : value)
                    {
                        if (s == _s.value)
                            return _s.offset;
                    }

                    u32 ret = offset;
                    value.emplace_back(offset, s);
                    offset += s.length() + 1;

                    return ret;
                }
            };

            struct binfunction
            {
                u32 id;
                u32 name;
                u32 suffix;
            };

            void print(const char* filename)
            {
                collecting::call self_probe(0, 0);
                self_probe.start();

                auto profile = collecting::probe::profile();

                const char* doc = "\n"
"FILE:\n"
"  HEADER\n"
"  CALLS\n"
"  FUNCTIONS\n"
"  STRINGS\n"
"\n"
"\n"
"HEADER:\n"
"\n"
"FIELD         OFFSET SIZE VALUE\n"
"--------------------------------\n"
"magic          0     8    \"PROFILE\" ^Z\n"
"version        8     4    1\n"
"call count    12     4    -\n"
"func count    16     4    -\n"
"func offset   20     4    call-count * 28\n"
"string offset 24     4    func-offset + func-count * 12\n"
"string size   28     4    -\n"
"second        32     8    -\n"
"\n"
"CALL:\n"
"\n"
"FIELD         OFFSET SIZE\n"
"--------------------------\n"
"id             0     4\n"
"parent         4     4\n"
"timestamp      8     8\n"
"duration      16     8\n"
"function      24     4\n"
"\n"
"FUNCTION\n"
"\n"
"FIELD         OFFSET SIZE\n"
"--------------------------\n"
"id             0     4\n"
"name           4     4\n"
"suffix         8     4\n";

                std::ofstream os(std::string(filename) + ".count", std::ios::out | std::ios::binary);
                os << "PROFILE\x1A";
                write(os, (u32)1u);

                strings str;
                std::vector<binfunction> functions;
                u32 call_count = 0, function_count = 0;

                for (auto& f : profile)
                {
                    for (auto& s : f)
                    {
                        binfunction fun = { s.id(), 0, 0 };
                        fun.name = str.add(fold(f.nice()));
                        if (s.name() && *s.name())
                            fun.suffix = str.add(s.name());
                        functions.push_back(fun);

                        ++function_count;
                        for (auto&& c : s)
                            c, ++call_count;
                    }
                }

                write(os, call_count);
                write(os, function_count);

                call_count *= sizeof(u32) * 3 + sizeof(u64) * 2;
                function_count *= sizeof(u32) * 3;
                function_count += call_count;

                write(os, call_count);     // functon offset
                write(os, function_count); // string offset
                write(os, str.offset);
                write(os, time::second());

                {
                    std::vector<collecting::call> calls;

                    for (auto& f : profile) for (auto& s : f) for (auto& c : s)
                        calls.push_back(c);

                    std::sort(begin(calls), end(calls), [](const collecting::call& lhs, const collecting::call& rhs) { return lhs.id() < rhs.id(); });

                    for (auto& c : calls)
                    {
                        write(os, c.id());        // u32
                        write(os, c.parent());    // u32
                        write(os, c.timestamp()); // u64
                        write(os, c.duration());  // u64
                        write(os, c.function());  // u32
                    }
                }

                for (auto& f : functions)
                    write(os, f);

                for (auto& s : str.value)
                    os.write(s.value.c_str(), s.value.length() + 1);

                os << doc;

                self_probe.stop();

                printf("binary_print took ");
                secs(time::second(), self_probe.duration());
                printf("\n");
            }
        }

        static void secs(time::type second, time::type ticks)
        {
            time::type seconds = ticks / second;
            //seconds /= 1000;
            //printf("%llu/%llu/%llu\n", second, ticks, seconds);

            if (seconds > 59)
            {
                seconds /= 1000;
                ticks -= (seconds / 60) * 60 * second;
                printf("(%3llum%llu.%03llus)", seconds / 60, ticks / second, (ticks * 1000 / second) % 1000);
            }
            else
            {
                const char* freq = "s ";
                if (ticks * 10 / second < 9)
                {
                    freq = "ms";
                    ticks *= 1000;
                    if (ticks * 10 / second < 9)
                    {
                        freq = "us";
                        ticks *= 1000;
                        if (ticks * 10 / second < 9)
                        {
                            freq = "ns";
                            ticks *= 1000;
                        }
                    }
                }

                printf("(%3llu.%03llu%s)", ticks / second, (ticks * 1000 / second) % 1000, freq);
            }
        }

        void xml_print(const char* filename)
        {
            xml::print(filename);
        }

        void binary_print(const char *filename)
        {
            binary::print(filename);
        }
    }

}
