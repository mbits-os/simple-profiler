#ifndef __BINARY_HPP__
#define __BINARY_HPP__

#include <iostream>

namespace profile { namespace print { namespace binary {

    template <typename T>
    void write(std::ostream& os, const T& t)
    {
        os.write((const char*) &t, sizeof(t));
    }

    template <typename T>
    bool read(std::istream& is, T& t)
    {
        return is.read((char*)&t, sizeof(T)).gcount() == sizeof(T);
    }

    typedef unsigned int u32;
    typedef unsigned long long u64;

    namespace file
    {
        static const u64 MAGIC = 0x1A454C49464F5250ull;
        static const u32 VERSION = 0x00010000; // 1.0

        struct header
        {
            u32 version;
            u32 function_count;
            u32 call_count;
            u32 function_offset;
            u32 call_offset;
            u64 second;
        };

        struct function
        {
            u32 id;
            u32 name;
            u32 suffix;
        };

        struct call
        {
            u32 id;
            u32 parent;
            u32 function;
            u32 flags;
            u64 duration;
        };
    }

}}} // profile::print::binary

#endif // __BINARY_HPP__
