#include "profiler_model.h"

ProfilerModel::ProfilerModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_second(1)
    , m_max(1)
{
}

void ProfilerModel::setProfileView(const FunctionsPtr& data)
{
    m_data = data;
    if (m_data)
        m_sorted.assign(m_data->begin(), m_data->end());
    else
        m_sorted.clear();

    m_sorter.sort(m_sorted);
}


void ProfilerModel::addColumn(int pos, const ColumnPtr& col)
{
    if ((size_t)pos > m_columns.size())
        pos = m_columns.size();

    beginInsertColumns(QModelIndex(), pos, pos);
    auto it = m_columns.begin();
    std::advance(it, pos);
    m_columns.insert(it, col);
    endInsertColumns();
}

void ProfilerModel::removeColumn(int pos)
{
    if ((size_t)pos >= m_columns.size())
        pos = m_columns.size() - 1;

    beginRemoveColumns(QModelIndex(), pos, pos);
    auto it = m_columns.begin();
    std::advance(it, pos);
    m_columns.erase(it);
    // TODO: what if we removed sorted column?
    endRemoveColumns();
}

QVariant ProfilerModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && (size_t)index.column() < m_columns.size() && index.row() < rowCount(index.parent()))
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            auto function = m_sorted.at(index.row());

            if (!function)
                break;

            return m_columns.at(index.column())->getDisplayData(this, *function.get());
        }
        case Qt::TextAlignmentRole:
        {
            switch (m_columns.at(index.column())->getAlignment())
            {
            default:
            case EAlignment_Left:
                return QVariant(Qt::AlignTop | Qt::AlignLeft);
            case EAlignment_Right:
                return QVariant(Qt::AlignTop | Qt::AlignRight);
            }
        }
        };
    }

    return QVariant();
}

QVariant ProfilerModel::headerData(int section, Qt::Orientation o, int role) const
{
    if (role == Qt::DisplayRole && (size_t)section < m_columns.size())
        return m_columns.at(section)->title();

    return QAbstractListModel::headerData(section, o, role);
}

int ProfilerModel::rowCount(const QModelIndex &) const
{
    if (m_data)
        return m_data->size();
    return 0;
}

int ProfilerModel::columnCount(const QModelIndex &) const
{
    return m_columns.size();
}

void ProfilerModel::sort(int column, Qt::SortOrder order)
{
    if ((size_t)column >= m_columns.size())
        return;

    if (column == m_sorter.lastColumn() && order == m_sorter.sortOrder())
        return;

    if (column == -1)
    {
        m_sorter.clear();
        setProfileView(m_data); // this will re-populate the m_sorted
        return;
    }

    bool reverse = column == m_sorter.lastColumn() && order != m_sorter.sortOrder();

    m_sorter.setSort(column, m_columns[column], order);

    beginResetModel();
    if (reverse)
        m_sorter.reverse(m_sorted);
    else
        m_sorter.sort(m_sorted);
    endResetModel();
}


QString Columns::impl::timeFormat(profiler::time_t second, profiler::time_t ticks)
{
    profiler::time_t seconds = ticks/second;

    if (seconds > 59)
    {
        ticks -= (seconds/60)*60*second;
        return QString("%1m%2.%3s")
                .arg(seconds/60)
                .arg(ticks/second)
                .arg(profiler::time_t((ticks*1000 / second) % 1000), 3, 10, QLatin1Char('0'));
    }

    const char* freq = "s";
    if (ticks*10 / second < 9)
    {
        freq = "ms";
        ticks *= 1000;
        if (ticks*10 / second < 9)
        {
            freq = "us";
            ticks *= 1000;
            if (ticks*10 / second < 9)
            {
                freq = "ps";
                ticks *= 1000;
            }
        }
    }

    return QString("%1.%2%3")
            .arg(ticks/second)
            .arg(profiler::time_t((ticks*1000 / second) % 1000), 3, 10, QLatin1Char('0'))
            .arg(freq);
}

QString Columns::impl::usageFormat(profiler::time_t max, profiler::time_t total, profiler::time_t own)
{
    return QString("%1.%2% / %3.%4%")
            .arg(total * 100 / max)
            .arg((total * 10000 / max) % 100, 2, 10, QLatin1Char('0'))
            .arg(own * 100 / max)
            .arg((own * 10000 / max) % 100, 2, 10, QLatin1Char('0'));
}
