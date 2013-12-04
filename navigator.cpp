#include "navigator.h"
#include <QMessageBox>

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
