#include "profile/profile.hpp"
#include "binary.hpp"

#include <regex>
#include <fstream>

namespace profile { namespace io {
    std::string fold(std::string name);
}} // profile::io

namespace profile { namespace io { namespace binary {

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

    void print(const char* filename)
    {
        auto profile = collecting::probe::profile();

        std::ofstream os(std::string(filename) + ".count", std::ios::out | std::ios::binary);

        file::header h = { file::VERSION }; // version

        strings str;
        std::vector<file::function> functions;

        for (auto& f : profile)
        {
            for (auto& s : f)
            {
                file::function fun = { s.id(), 0, 0 };
                fun.name = str.add(fold(f.nice()));
                if (s.name() && *s.name())
                    fun.suffix = str.add(s.name());
                functions.push_back(fun);

                ++h.function_count;
                for (auto&& c : s)
                {
                    c, ++h.call_count;
                }
            }
        }

        h.function_offset = ((str.offset + 3) >> 2) << 2;
        h.call_offset = h.function_offset + functions.size() * sizeof(file::function);
        h.second = time::second();

        write(os, file::MAGIC);
        write(os, h);

        for (auto& s : str.value)
            os.write(s.value.c_str(), s.value.length() + 1);
        os.write("\xFF\xFF\xFF\xFF", h.function_offset - str.offset); // pad, if needed

        for (auto& f: functions)
            write(os, f);

        for (auto& f : profile) for (auto& s : f) for (auto& c : s)
        {
            file::call _c =
            {
                c.id(),
                c.parent(),
                c.function(),
                c.flags(),
                c.duration()
            };
            write(os, _c);
        }
    }

}}} // profile::io::binary
