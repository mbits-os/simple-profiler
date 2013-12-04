#include "navigator.h"

Navigator::Navigator(QObject *parent) :
    QObject(parent)
{
}

void Navigator::select(const HistoryItem& item)
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

void Navigator::back()
{
    if (m_history.empty())
        return;

    auto item = m_history.top();
    m_history.pop();

    if (m_history.empty())
        hasHistory(false);

    select(item);
}

void Navigator::home()
{
    History empty;
    m_history.swap(empty);
    hasHistory(false);
    select(HistoryItem());
}

void Navigator::cancel()
{
}
