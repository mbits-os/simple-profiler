#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <QObject>
#include "profiler.h"
#include <stack>
#include <functional>
#include <QThread>

class HistoryItem
{
public:
    typedef std::vector<profiler::call_id> calls;

    const calls& get_calls() const { return m_calls; }

private:
    calls m_calls;
};

typedef std::stack<HistoryItem> History;

struct NavFunction
{
    std::function<void ()> f;
    NavFunction( std::function<void ()> f ): f(f) {}

    void call()
    {
        f();
        delete this;
    }
};

class SelectTask: public QThread
{
    Q_OBJECT;

    std::function<void ()> call;
    std::function<void ()> cont;
public:
    SelectTask(QObject *parent, std::function<void ()> call, std::function<void ()> cont): QThread(parent), call(call), cont(cont) {}

    void run();

signals:
    void selected(NavFunction* cont);
};

class Navigator : public QObject
{
    Q_OBJECT

    profiler::data_ptr m_data;
    History m_history;

    void select(const HistoryItem& item, std::function<void ()> cont);
    void doSelect(const HistoryItem& item);
public:
    explicit Navigator(QObject *parent = 0);
    void setData(const profiler::data_ptr& data) { m_data = data; }

signals:
    void hasHistory(bool);
    void selectStarted();
    void selectStopped();

public slots:
    void back();
    void home();
    void cancel();

private slots:
    void onSelected(NavFunction* cont);
};

#endif // NAVIGATOR_H
