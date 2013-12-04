#include "navigator.h"
#include <QMessageBox>
#include <QDebug>

Navigator::Navigator(QObject *parent) :
    QObject(parent)
{
}

void Navigator::select(const HistoryItem& item, std::function<void ()> cont)
{
    emit selectStarted();

    SelectTask* task = new SelectTask(this, [this, item](){ return this->doSelect(item); }, cont);
    QObject::connect(task, SIGNAL(selected(NavFunction*)), this, SLOT(onSelected(NavFunction*)));
    connect(task, &SelectTask::finished, task, &QObject::deleteLater);
    task->start();
}

void SelectTask::run()
{
    call();
    NavFunction* _cont = new NavFunction(cont);
    emit selected(_cont);
}

void Navigator::doSelect(const HistoryItem& item)
{
    if (item.is_cached())
    {
        m_currentView = item;
        return;
    }

    m_currentView = HistoryItem(item.get_calls());

    profiler::calls calls;
    if (item.get_calls().empty())
        calls = m_data->select<profiler::calls>();
    else
    {
        for (profiler::call_id call: item.get_calls())
        {
            auto part = m_data->selectCalledFrom(call);
            for (auto&& c: part)
                calls.push_back(c);
        }
    }

    auto functions = m_data->functions();

    for (auto&& c: calls)
        m_currentView.update(functions, c);

    m_currentView.normalize();

    qDebug() << calls.size() << "calls," << m_currentView.get_cached().size() << "functions.";
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
    select(HistoryItem(), []{});
}

void Navigator::cancel()
{
}

Function::Function(const profiler::function_ptr& function, const profiler::call_ptr& calledAs)
    : m_function(function)
    , m_call_count(1)
    , m_duration(calledAs->duration())
    , m_ownTime(calledAs->ownTime())
{
    m_calls.push_back(calledAs->id());
}

void Function::update(const profiler::call_ptr& calledAs)
{
    ++m_call_count;
    m_duration += calledAs->duration();
    m_ownTime  += calledAs->ownTime();
    m_calls.push_back(calledAs->id());
}

void Functions::update(const profiler::functions& functions, const profiler::call_ptr& calledAs)
{
    auto function_id = calledAs->functionId();
    if (!function_id)
        return;

    for (auto&& f: m_functions)
    {
        if (f.id() == function_id)
        {
            f.update(calledAs);
            return;
        }
    }

    for (auto&& f: functions)
    {
        if (f->id() == function_id)
        {
            m_functions.emplace_back(f, calledAs);
            return;
        }
    }
}

void Functions::normalize()
{
    m_max_duration = 1;
    for (auto&& f: m_functions)
        if (m_max_duration < f.duration())
            m_max_duration = f.duration();
}
