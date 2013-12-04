#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <QObject>
#include "profiler.h"
#include <stack>

class HistoryItem
{
public:
    typedef std::vector<profiler::call_id> calls;

    const calls& get_calls() const { return m_calls; }

private:
    calls m_calls;
};

typedef std::stack<HistoryItem> History;

class Navigator : public QObject
{
    Q_OBJECT

    profiler::data_ptr m_data;
    History m_history;

    void select(const HistoryItem& item);
public:
    explicit Navigator(QObject *parent = 0);
    void setData(const profiler::data_ptr& data) { m_data = data; }

signals:
    void hasHistory(bool);

public slots:
    void back();
    void home();
    void cancel();
};

#endif // NAVIGATOR_H
