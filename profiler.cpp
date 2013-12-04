#include "profiler.h"
#include <QFile>
#include <QDomDocument>
#include <QDebug>

namespace profiler
{
    static bool read(QDomDocument& doc, const QString& path)
    {
        QFile file(path);
        if (!file.open(QFile::ReadOnly))
            return false;

        return doc.setContent(&file);
    }

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

    bool data::open(const QString& path)
    {
        QDomDocument doc("stats");

        if (!read(doc, path))
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

        qDebug() << "Got " << m_calls.size() << " calls and " << m_functions.size() << " functions.\n";

        return true;
    }
}
