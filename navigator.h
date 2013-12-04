#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <QObject>
#include "profiler.h"

class Navigator : public QObject
{
    Q_OBJECT

    profiler::data* m_data;
public:
    explicit Navigator(QObject *parent = 0);
    void setData(profiler::data* data) { m_data = data; }

signals:
    void hasHistory(bool);

public slots:
    void back();
    void home();
};

#endif // NAVIGATOR_H
