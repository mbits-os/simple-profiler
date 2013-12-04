#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <QObject>
#include "profiler.h"
#include <stack>
#include <functional>
#include <QThread>

typedef std::vector<profiler::call_id> CalledAs;

class Function
{
    profiler::function_ptr m_function;
    CalledAs m_calls;
    long long   m_call_count; // needed? we have m_calls.size()...
    profiler::time_t m_duration;
    profiler::time_t m_ownTime;
    profiler::time_t m_longest;
    profiler::time_t m_shortest;

public:
    Function(const profiler::function_ptr& function, const profiler::call_ptr& calledAs);
    void update(const profiler::call_ptr& calledAs);
    profiler::function_id id() const { return m_function->id(); }

    QString name() const { return m_function ? m_function->name() : QString(); }
    const CalledAs& calls() const { return m_calls; }
    long long call_count() const { return m_call_count; }
    profiler::time_t duration() const { return m_duration; }
    profiler::time_t ownTime() const { return m_ownTime; }
    profiler::time_t longest() const { return m_longest; }
    profiler::time_t shortest() const { return m_shortest; }
};

typedef std::shared_ptr<Function> FunctionPtr;

class Functions
{
    typedef std::vector<FunctionPtr> functions;
    functions m_functions;
    profiler::time_t m_max_duration;
public:
    void update(const profiler::functions& functions, const profiler::call_ptr& calledAs);
    size_t size() const { return m_functions.size(); }
    FunctionPtr at(size_t ndx) const { return m_functions.at(ndx); }
    functions::const_iterator begin() const { return m_functions.begin(); }
    functions::const_iterator end() const { return m_functions.end(); }
    void normalize();
    profiler::time_t max_duration() const { return m_max_duration; }
};

typedef std::shared_ptr<Functions> FunctionsPtr;

class HistoryItem
{
public:
    HistoryItem(): m_hasCache(false) {}
    HistoryItem(const CalledAs& calls): m_calls(calls), m_hasCache(false) {}

    const CalledAs& get_calls() const { return m_calls; }
    FunctionsPtr get_cached() const { return m_cached; }
    bool is_cached() const { return m_cached != nullptr; }

    void update(const profiler::functions& functions, const profiler::call_ptr& calledAs)
    {
        if (!m_cached)
            m_cached = std::make_shared<Functions>();
        m_cached->update(functions, calledAs);
    }

    void normalize() { if (m_cached) m_cached->normalize(); }
    profiler::time_t max_duration() const { return m_cached ? m_cached->max_duration() : 1; }
private:
    CalledAs m_calls;
    FunctionsPtr m_cached;
    bool m_hasCache;
};

typedef std::shared_ptr<HistoryItem> HistoryItemPtr;
typedef std::stack<HistoryItemPtr> History;

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
    HistoryItemPtr m_currentView;

    void select(const HistoryItemPtr& item, std::function<void ()> cont);
    void doSelect(const HistoryItemPtr& item);
public:
    explicit Navigator(QObject *parent = 0);
    void setData(const profiler::data_ptr& data) { m_data = data; }
    HistoryItemPtr current() { return m_currentView; }

signals:
    void hasHistory(bool);
    void selectStarted();
    void selectStopped();

public slots:
    void back();
    void home();
    void navigateTo(size_t);
    void cancel();

private slots:
    void onSelected(NavFunction* cont);
};

#endif // NAVIGATOR_H
