#include "profile.hpp"

#include <regex>
#include <fstream>

namespace profile { namespace print {
    std::string fold(std::string name);
}} // profile::print

namespace profile { namespace print { namespace binary {

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
"func offset   20     4    call-count * 24\n"
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
"function       8     4\n"
"flags         12     4\n"
"duration      16     8\n"
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
                write(os, c.function());  // u32
                write(os, c.flags());     // u32
                write(os, c.duration());  // u64
            }
        }

        for (auto& f : functions)
            write(os, f);

        for (auto& s : str.value)
            os.write(s.value.c_str(), s.value.length() + 1);

        os << doc;
    }

}}} // profile::print::binary
