#ifdef FEATURE_IO_WRITE

#include "profile/profile.hpp"

#include <regex>
#include <fstream>

namespace profile { namespace io {
    std::string fold(std::string name);
}} // profile::io

namespace profile { namespace io { namespace xml {

    static std::string xml(std::string attr)
    {
        attr = std::regex_replace(attr, std::regex("&"), "&amp;");
        attr = std::regex_replace(attr, std::regex("<"), "&lt;");
        attr = std::regex_replace(attr, std::regex(">"), "&gt;");
        return std::regex_replace(attr, std::regex("\""), "&quot;");
    }

    void write(const char* filename)
    {
        auto profile = collecting::probe::profile();

        std::ofstream os(std::string(filename) + ".xcount");
        os << "<stats second=\"" << time::second() << "\">\n\t<functions>\n";
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

        os << "\t</functions>\n\t<calls>\n";

        for (auto& f : profile) for (auto& s : f) for (auto& c : s)
        {
            os << "\t\t<call id=\"" << c.id() << "\"";
            if (c.parent())
                os << " parent=\"" << c.parent() << "\"";
            if (c.function())
                os << " function=\"" << c.function() << "\"";
            os << " duration=\"" << c.duration() << "\"";
            if (c.isSysCall())
                os << " syscall=\"true\"";
            os << " />\n";
        }
        os << "\t</calls>\n</stats>\n";
    }

}}} // profile::io::xml

#endif // FEATURE_IO_WRITE
