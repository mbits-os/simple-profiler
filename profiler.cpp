#include "profiler.h"
#include <QFile>
#include <QDomDocument>

namespace profiler
{
    static bool read(QDomDocument& doc, const QString& path)
    {
        QFile file(path);
        if (!file.open(QFile::ReadOnly))
            return false;

        return doc.setContent(&file);
    }

    bool data::open(const QString& path)
    {
        QDomDocument doc("stats");

        if (!read(doc, path))
            return false;

        auto root = doc.documentElement();
        auto sSecond = root.attribute("second");

        return true;
    }
}
