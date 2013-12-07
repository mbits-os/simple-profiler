#ifndef __READ_HPP__
#define __READ_HPP__

namespace profile { namespace collecting {
    class profile;
}}

namespace profile { namespace io {
    bool read(const char* path, collecting::profile& out);
}} // profile::io

#endif // READ_HPP__
