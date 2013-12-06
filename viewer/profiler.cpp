#include "profiler.h"
#include <QFile>
#include <QDomDocument>
#include <QDebug>
#include <cctype>

namespace profiler
{
    long long getAttr(const QDomNamedNodeMap& attrs, const QString& name, bool& ok, bool* has = nullptr)
    {
        if (has)
        {
            *has = attrs.contains(name);
            if (!*has)
                return 0;
        }

        return attrs.namedItem(name).nodeValue().toLongLong(&ok);
    }

    QString getAttr(const QDomNamedNodeMap& attrs, const QString& name, bool* has = nullptr)
    {
        if (has)
        {
            *has = attrs.contains(name);
            if (!*has)
                return QString();
        }

        return attrs.namedItem(name).nodeValue();
    }

    call_ptr find_by_id(const profiler::calls& calls, call_id id)
    {
        for (auto&& c: calls)
            if (c->id() == id)
                return c;

        return nullptr;
    }

    bool data::xml_open(QFile& file)
    {
        QDomDocument doc("stats");

        file.seek(0);
        if (!doc.setContent(&file))
            return false;

        auto root = doc.documentElement();
        auto sSecond = root.attribute("second");

        bool ok = true, has = true;

        auto second = getAttr(root.attributes(), "second", ok, &has);
        if (!has || !ok) return false;

        m_second = second;
        m_calls.clear();
        m_functions.clear();

        if (!m_second)
            m_second = 1;

        auto list = doc.elementsByTagName("call");
        auto length = list.length();
        for (decltype(length) i = 0; i < length; ++i)
        {
            auto atts = list.item(i).attributes();

            call_id id = getAttr(atts, "id", ok);
            if (!ok) return false;

            call_id parent = getAttr(atts, "parent", ok, &has);
            if (has && !ok) return false;

            function_id function = getAttr(atts, "function", ok);
            if (!ok) return false;

            time_t duration = getAttr(atts, "duration", ok);
            if (!ok) return false;

            if (!id)
                return false;

            m_calls.push_back(std::make_shared<call>(id, parent, function, duration));
        }

        for (auto&& c: m_calls)
        {
            auto parent_id = c->parent();
            if (!parent_id)
                continue;

            auto parent = find_by_id(m_calls, c->parent());
            if (parent)
                parent->detract(c->duration());
        }

        list = doc.elementsByTagName("fn");
        length = list.length();
        for (decltype(length) i = 0; i < length; ++i)
        {
            auto atts = list.item(i).attributes();

            call_id id = getAttr(atts, "id", ok);
            if (!ok) return false;

            QString name = getAttr(atts, "name", &has);
            if (!has) return false;

            QString suffix = getAttr(atts, "suffix", &has);
            if (has)
                name.append("/").append(suffix);

            if (!id)
                return false;

            m_functions.push_back(std::make_shared<function>(id, name));
        }

        qDebug() << "Got" << m_calls.size() << "calls and" << m_functions.size() << "functions.\n";

        return true;
    }

    namespace
    {
        typedef unsigned int u32;
        typedef unsigned long long u64;

        struct _header
        {
            u32 version;
            u32 call_count;
            u32 func_count;
            u32 func_offset;
            u32 string_offset;
            u32 string_size;
            u64 second;
        };

        struct _call
        {
            u32 id;
            u32 parent;
            u32 timestamp1;
            u32 timestamp2;
            u32 duration1;
            u32 duration2;
            u32 function;

            u64 duration()
            {
                u64 ret = duration2;
                ret <<= 32;
                ret |= duration1;
                return ret;
            }
        };

        struct _function
        {
            u32 id;
            u32 name;
            u32 suffix;
        };

        template <typename T>
        bool read(QFile& file, T& out)
        {
            return file.read((char*)&out, sizeof(T)) == sizeof(T);
        }
    }

    QString str(const std::vector<char>& strings, u32 offset)
    {
        if (offset >= strings.size())
            return QString();

        return QString(&strings[offset]);
    }

    bool data::bin_open(QFile &file)
    {
        _header h = {};
        if (!read(file, h))
            return false;

        if (h.call_count * sizeof(_call) != h.func_offset)
            return false;

        if (h.func_count * sizeof(_function) + h.func_offset != h.string_offset)
            return false;

        if (h.string_size + h.string_offset > file.size())
            return false;

        m_second = h.second;
        m_calls.clear();
        m_functions.clear();

        if (!m_second)
            m_second = 1;

        for (u32 i = 0; i < h.call_count; ++i)
        {
            _call c;
            if (!read(file, c))
                return false;

            m_calls.push_back(std::make_shared<call>(c.id, c.parent, c.function, c.duration()));
        }

        for (auto&& c: m_calls)
        {
            auto parent_id = c->parent();
            if (!parent_id)
                continue;

            auto parent = find_by_id(m_calls, c->parent());
            if (parent)
                parent->detract(c->duration());
        }

        std::vector<_function> funcs(h.func_count);
        std::vector<char> strings(h.string_size + 1);

        if (funcs.size() < h.func_count)
            return false;

        if (strings.size() <= h.string_size)
            return false;

        u64 size = h.func_count * sizeof(_function);
        if (file.read((char*)&funcs[0], size) != size)
            return false;

        if (file.read(&strings[0], h.string_size) != h.string_size)
            return false;

        strings[h.string_size] = 0;

        for (auto&& f: funcs)
        {
            QString name = str(strings, f.name);
            QString suffix = str(strings, f.suffix);

            if (name.isEmpty())
                return false;
            if (!suffix.isEmpty())
                name.append("/").append(suffix);

            if (!f.id)
                return false;

            m_functions.push_back(std::make_shared<function>(f.id, name));
        }

        qDebug() << "Got" << m_calls.size() << "calls and" << m_functions.size() << "functions.\n";

        return true;
    }

    static bool is_binary(QFile &file)
    {
        u64 magic;
        if (!read(file, magic))
            return false;

        return magic == 0x1A454C49464F5250ull;

        return false;
    }

    bool data::open(const QString &path)
    {
        QFile file(path);
        if (!file.open(QFile::ReadOnly))
            return false;

        if (is_binary(file))
            return bin_open(file);
        return xml_open(file);
    }
}
