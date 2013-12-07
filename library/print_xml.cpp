#include "profile.hpp"

#include <regex>
#include <fstream>

namespace profile { namespace print {
    std::string fold(std::string name);
}} // profile::print

namespace profile { namespace print { namespace xml {

    static std::string xml(std::string attr)
    {
        attr = std::regex_replace(attr, std::regex("&"), "&amp;");
        attr = std::regex_replace(attr, std::regex("<"), "&lt;");
        attr = std::regex_replace(attr, std::regex(">"), "&gt;");
        return std::regex_replace(attr, std::regex("\""), "&quot;");
    }

    void print(const char* filename)
    {
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
                os << " duration=\"" << c.duration() << "\" />\n";
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
    }

}}} // profile::print::xml
