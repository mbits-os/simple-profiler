#include "navigator.h"
#include <QMessageBox>
#include <QDebug>

Navigator::Navigator(QObject *parent) :
    QObject(parent)
{
}

void Navigator::select(const HistoryItemPtr& item, std::function<void ()> cont)
{
    emit selectStarted();

    HistoryItemPtr local = item;
    SelectTask* task = new SelectTask(this, [this, local](){ return this->doSelect(local); }, cont);
    QObject::connect(task, SIGNAL(selected(NavFunction*)), this, SLOT(onSelected(NavFunction*)));
    connect(task, &SelectTask::finished, task, &QObject::deleteLater);
    task->start();
}

void SelectTask::run()
{
    try
    {
        call();
        NavFunction* _cont = new NavFunction(cont);
        emit selected(_cont);
    }
    catch(std::exception& e)
    {
        qDebug() << e.what();
    }
}

void Navigator::doSelect(const HistoryItemPtr& item)
{
    if (item && item->is_cached())
    {
        m_currentView = item;
        return;
    }

    CalledAs src(item ? item->get_calls() : CalledAs());
    QString name(item ? item->name() : QString());
    m_currentView = std::make_shared<HistoryItem>(src, name);

    profiler::calls calls;
    if (src.empty())
        calls = m_data->select<profiler::calls>();
    else
    {
        for (profiler::call_id call: src)
        {
            auto part = m_data->selectCalledFrom(call);
            for (auto&& c: part)
                calls.push_back(c);
        }
    }

    auto functions = m_data->functions();

    for (auto&& c: calls)
        m_currentView->update(functions, c);

    m_currentView->normalize();

    auto cached = m_currentView->get_cached();
    qDebug() << calls.size() << "calls," << (cached? cached->size() : 0) << "functions.";
}

void Navigator::onSelected(NavFunction* cont)
{
    cont->call();
    emit selectStopped();
}

void Navigator::back()
{
    if (m_history.empty())
        return;

    auto item = m_history.top();
    m_history.pop();

    if (m_history.empty())
        hasHistory(false);

    select(item, []{});
}

void Navigator::home()
{
    History empty;
    m_history.swap(empty);
    hasHistory(false);
    select(nullptr, []{});
}

void Navigator::navigateTo(size_t ndx)
{
    auto cached = m_currentView->get_cached();
    auto function = cached ? cached->at(ndx) : nullptr;
    if (!function)
        return;

    m_history.push(m_currentView);

    QString name = function->name();
    auto tmp = std::make_shared<HistoryItem>(function->calls(), name);
    select(tmp, [this, name]{ hasHistory(true); });
}

void Navigator::cancel()
{
}

Function::Function(const profiler::function_ptr& function, const profiler::call_ptr& calledAs)
    : m_function(function)
    , m_call_count(1)
    , m_duration(calledAs->duration())
    , m_ownTime(calledAs->ownTime())
    , m_longest(calledAs->duration())
    , m_shortest(calledAs->duration())
{
    m_calls.push_back(calledAs->id());
}

void Function::update(const profiler::call_ptr& calledAs)
{
    ++m_call_count;
    m_duration += calledAs->duration();
    m_ownTime  += calledAs->ownTime();
    if (m_longest < calledAs->duration())
        m_longest = calledAs->duration();
    if (m_shortest > calledAs->duration())
        m_shortest = calledAs->duration();
    m_calls.push_back(calledAs->id());
}

void Functions::update(const profiler::functions& functions, const profiler::call_ptr& calledAs)
{
    auto function_id = calledAs->functionId();
    if (!function_id)
        return;

    for (auto&& f: m_functions)
    {
        if (f->id() == function_id)
        {
            f->update(calledAs);
            return;
        }
    }

    for (auto&& f: functions)
    {
        if (f->id() == function_id)
        {
            m_functions.push_back(std::make_shared<Function>(f, calledAs));
            return;
        }
    }
}

void Functions::normalize()
{
    m_max_duration = 1;
    m_max_duration_avg = 1;
    for (auto&& f: m_functions)
    {
        auto dur = f->duration();
        if (m_max_duration < dur)
            m_max_duration = dur;

        dur /= f->call_count();
        if (m_max_duration_avg < dur)
            m_max_duration_avg = dur;
    }
}
