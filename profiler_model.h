#ifndef PROFILER_MODEL_H
#define PROFILER_MODEL_H

#include <QAbstractListModel>
#include "navigator.h"

class MainWindow;

enum EAlignment
{
    EAlignment_Left,
    EAlignment_Right
};

class ProfilerModel;

struct Column
{
    virtual ~Column() {}
    virtual QString title() const = 0;
    virtual QVariant getDisplayData(const ProfilerModel* parent, const Function&) const = 0;
    virtual EAlignment getAlignment() const = 0;
};
typedef std::shared_ptr<Column> ColumnPtr;

class ProfilerModel : public QAbstractListModel
{
    friend class MainWindow;
    Q_OBJECT

    std::vector<ColumnPtr> m_columns;
    profiler::time_t m_second;
    profiler::time_t m_max;
    FunctionsPtr     m_data;
public:
    explicit ProfilerModel(QObject *parent = 0);

    void setSecond(profiler::time_t second) { m_second = second; }
    void setMaxDuration(profiler::time_t max) { m_max = max; }
    void setProfileView(const FunctionsPtr& data) { m_data = data; }
    void appendColumn(const ColumnPtr& col) { addColumn(m_columns.size(), col); }
    void addColumn(int pos, const ColumnPtr& col);
    void removeColumn(int pos);

    profiler::time_t second() const { return m_second; }
    profiler::time_t max_duration() const { return m_data ? m_data->max_duration() : 1; }

    //Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
};

namespace Columns
{
    namespace impl
    {
        QString timeFormat(profiler::time_t second, profiler::time_t ticks);
        QString usageFormat(profiler::time_t max, profiler::time_t total, profiler::time_t own);

        template <typename T>
        struct Impl: Column
        {
            QString title() const { return T::title(); }
            QVariant getDisplayData(const ProfilerModel* parent, const Function& f) const { return T::getDisplayData(parent, f); }
            EAlignment getAlignment() const { return T::getAlignment(); }
        };

        template <typename Final, EAlignment Alignment = EAlignment_Left>
        struct ColumnInfo
        {
            static ColumnPtr create() { return std::make_shared< Impl<Final> >();}
            static QVariant getDisplayData(const ProfilerModel*, const Function& f)
            {
                return Final::getData(f);
            }

            static EAlignment getAlignment()
            {
                return Alignment;
            }
        };

        template <typename Final, EAlignment Alignment = EAlignment_Right>
        struct TimeColumnInfo: ColumnInfo<Final, Alignment>
        {
            static QVariant getDisplayData(const ProfilerModel* parent, const Function& f)
            {
                return timeFormat(parent->second(), Final::getData(f));
            }
        };

        template <typename Final, EAlignment Alignment = EAlignment_Right>
        struct AvgTimeColumnInfo: ColumnInfo<Final, Alignment>
        {
            static QVariant getDisplayData(const ProfilerModel* parent, const Function& f)
            {
                return timeFormat(parent->second() * 1000, Final::getData(f) * 1000 / f.call_count());
            }
        };
    }

    struct Name: impl::ColumnInfo<Name>
    {
        static QString title() { return "Name"; }
        static QString getData(const Function& f) { return f.name(); }
    };

    struct Count: impl::ColumnInfo<Count, EAlignment_Right>
    {
        static QString title() { return "Calls"; }
        static long long getData(const Function& f) { return f.call_count(); }
    };

    struct TotalTime: impl::TimeColumnInfo<TotalTime>
    {
        static QString title() { return "Total time"; }
        static profiler::time_t getData(const Function& f) { return f.duration(); }
    };

    struct OwnTime: impl::TimeColumnInfo<OwnTime>
    {
        static QString title() { return "Self time"; }
        static profiler::time_t getData(const Function& f) { return f.ownTime(); }
    };

    struct TotalTimeAvg: impl::AvgTimeColumnInfo<TotalTimeAvg>
    {
        static QString title() { return "Total time (average)"; }
        static profiler::time_t getData(const Function& f) { return f.duration(); }
    };

    struct OwnTimeAvg: impl::AvgTimeColumnInfo<OwnTimeAvg>
    {
        static QString title() { return "Self time (average)"; }
        static profiler::time_t getData(const Function& f) { return f.ownTime(); }
    };

    struct Graph: impl::ColumnInfo<Graph>
    {
        static QString title() { return "Time"; }

        static QVariant getDisplayData(const ProfilerModel* model, const Function& f)
        {
            return impl::usageFormat(model->max_duration(), f.duration(), f.ownTime());
        }
    };
}

#endif // PROFILER_MODEL_H
