#ifndef PROFILER_MODEL_H
#define PROFILER_MODEL_H

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QTreeView>
#include "navigator.h"
#include <algorithm>
#include <QAction>
#include <QDebug>
#include <QSettings>

class MainWindow;

enum EAlignment
{
	EAlignment_Left,
	EAlignment_Right
};

class ProfilerModel;

namespace Qt {
	enum
	{
		IsProgressRole = UserRole + 1,
		ProgressMaxRole,
		PrimaryDisplayRole,
		SecondaryDisplayRole
	};
}

struct Column
{
	virtual ~Column() {}
	virtual QString title() const = 0;
	virtual QVariant getDisplayData(const ProfilerModel* parent, const Function&) const = 0;
	virtual QVariant getPrimaryData(const ProfilerModel* parent, const Function&) const = 0;
	virtual QVariant getSecondaryData(const ProfilerModel* parent, const Function&) const = 0;
	virtual EAlignment getAlignment() const = 0;
	virtual bool isProgress() const = 0;
	virtual profiler::time_type maxDuration(const ProfilerModel* parent) const = 0;
	virtual bool less(const Function& lhs, const Function& rhs) const = 0;
};
typedef std::shared_ptr<Column> ColumnPtr;

class ColumnBag
{
	struct ColDef
	{
		bool m_used;
		ColumnPtr m_col;
		QAction* m_action;

	public:
		ColDef(): m_used(false), m_action(nullptr) {}
		ColDef(const ColumnPtr& col): m_used(false), m_col(col), m_action(nullptr) {}

		QAction* getAction(QObject *parent);

		ColumnPtr col() const { return m_col; }
		bool used() const { return m_used; }

		bool use()
		{
			if (m_used)
				return false;

			m_used = true;
			if (m_action)
				m_action->setChecked(m_used);
			return true;
		}

		bool stop()
		{
			if (!m_used)
				return false;

			m_used = false;
			if (m_action)
				m_action->setChecked(m_used);
			return true;
		}
	};
	typedef std::vector<ColDef> ColDefs;

	ProfilerModel* m_model;
	ColDefs m_defs;

	template <typename T>
	void add()
	{
		if (T::created().get())
			return;

		T::bagIndex().set(m_defs.size());
		m_defs.push_back(T::create());
		T::created().set(true);
	}

	long long positionOf(const ColumnPtr& col) const;
public:
	ColumnBag();

	void setModel(ProfilerModel* model) { m_model = model; }
	void buildColumnMenu(QObject* parent, QMenu* menu);

	void defaultColumns();
	bool loadSettings(QSettings& settings);
	void storeSettings(QSettings& settings) const;

	void use(long long ndx);
	void stop(long long ndx);

	template <typename T> void use() { use(T::bagIndex().get()); }
	template <typename T> void stop() { stop(T::bagIndex().get()); }

	template <typename T> ColumnPtr get()
	{
		auto ndx = T::bagIndex().get();
		if (ndx < 0 || ndx >= (long long)m_defs.size())
			return nullptr;

		return m_defs[(size_t)ndx].col();
	}
};

class ProfilerModel : public QAbstractListModel
{
	friend class MainWindow;
	Q_OBJECT

	ColumnBag                m_column_bag;
	std::vector<ColumnPtr>   m_columns;
	profiler::time_type      m_second;
	profiler::time_type      m_max;
	FunctionsPtr             m_data;
	std::vector<FunctionPtr> m_sorted;

	class Sorter
	{
		int m_column;
		Qt::SortOrder m_sortOrder;
		ColumnPtr m_less;
	public:
		Sorter()
			: m_column(-1)
			, m_sortOrder(Qt::AscendingOrder)
		{}

		void setSort(int column, ColumnPtr less, Qt::SortOrder sortOrder)
		{
			m_column = column;
			m_less = less;
			m_sortOrder = sortOrder;
		}

		void clear()
		{
			m_less.reset();
			m_column = -1;
			m_sortOrder = Qt::AscendingOrder;
		}

		int lastColumn() const { return m_column; }
		Qt::SortOrder sortOrder() const { return m_sortOrder; }

		void sort(std::vector<FunctionPtr>& items)
		{
			if (!m_less)
				return;

			std::sort(std::begin(items), std::end(items), *this);
		}

		void reverse(std::vector<FunctionPtr>& items)
		{
			std::reverse(std::begin(items), std::end(items));
		}

		bool operator()(FunctionPtr lhs, FunctionPtr rhs)
		{
			if (m_sortOrder == Qt::DescendingOrder)
				std::swap(lhs, rhs);

			if (!lhs)
				return !!rhs; // nullptr < valid, !(nullptr < nullptr)

			return m_less->less(*lhs.get(), *rhs.get());
		}

	} m_sorter;

	void addColumn(int pos, const ColumnPtr& col);

	template <typename T>
	int getIndex()
	{
		ColumnPtr col = m_column_bag.get<T>();
		if (!col)
			return -1;

		int ndx = 0;
		for (auto&& c: m_columns)
		{
			if (c == col)
				return ndx;
			++ndx;
		}

		return -1;
	}

public:
	explicit ProfilerModel(QObject *parent = 0);

	void defaultColumns() { m_column_bag.defaultColumns(); }
	bool loadSettings(QSettings& settings) { return m_column_bag.loadSettings(settings); }
	void storeSettings(QSettings& settings) const { m_column_bag.storeSettings(settings); }
	void buildColumnMenu(QObject* parent, QMenu* menu) { m_column_bag.buildColumnMenu(parent, menu); }
	void useColumn(long long ndx) { m_column_bag.use(ndx); }
	void stopUsing(long long ndx) { m_column_bag.stop(ndx); }

	void setSecond(profiler::time_type second) { m_second = second; }
	void setMaxDuration(profiler::time_type max) { m_max = max; }
	void setProfileView(const FunctionsPtr& data);
	long long appendColumn(const ColumnPtr& col) { addColumn(m_columns.size(), col); return m_columns.size() - 1; }
	void removeColumn(const ColumnPtr& col);
	ColumnPtr getColumn(int pos) const { return m_columns[pos]; }
	size_t getColumnCount() const { return m_columns.size(); }

	template <typename T>
	void sortColumn(QTreeView* tree, Qt::SortOrder order)
	{
		int ndx = getIndex<T>();
		if (ndx < 0)
			return;

		tree->sortByColumn(ndx, order);
	}

	profiler::time_type second() const { return m_second; }
	profiler::time_type max_duration() const { return m_data ? m_data->max_duration() : 1; }
	profiler::time_type max_duration_avg() const { return m_data ? m_data->max_duration_avg() : 1; }

	//Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
};

class ProfilerDelegate: public QStyledItemDelegate
{
	Q_OBJECT;
public:
	explicit ProfilerDelegate(QObject *parent = 0);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

namespace Columns
{
	namespace impl
	{
		QString timeFormat(profiler::time_type second, profiler::time_type ticks);
		QString usageFormat(profiler::time_type max, profiler::time_type total, profiler::time_type own);

		struct Direct { enum { SCALE = 1 }; };
		struct Scaled { enum { SCALE = 1000 }; };

		template <typename T>
		class Property
		{
			T value;
		public:
			Property(T defValue): value(defValue) {}
			T get() const { return value; }
			void set(T val) { value = val; }
		};

		template <typename T>
		struct Impl: Column
		{
			QString title() const { return T::title(); }
			QVariant getDisplayData(const ProfilerModel* parent, const Function& f) const { return T::getDisplayData(parent, f); }
			QVariant getPrimaryData(const ProfilerModel* parent, const Function& f) const { return T::getPrimaryData(parent, f); }
			QVariant getSecondaryData(const ProfilerModel* parent, const Function& f) const { return T::getSecondaryData(parent, f); }
			EAlignment getAlignment() const { return T::getAlignment(); }
			bool isProgress() const { return T::isProgress(); }
			profiler::time_type maxDuration(const ProfilerModel* parent) const { return T::maxDuration(parent); }
			bool less(const Function& lhs, const Function& rhs) const { return T::less(lhs, rhs); }
		};

		template <typename Final, typename Scale = Direct, EAlignment Alignment = EAlignment_Left>
		struct ColumnInfo: Scale
		{
			static ColumnPtr create() { return std::make_shared< Impl<Final> >();}

			static Property<bool>& created()
			{
				static Property<bool> _(false);
				return _;
			}

			static Property<long long>& bagIndex()
			{
				static Property<long long> _(-1);
				return _;
			}

			static QVariant getDisplayData(const ProfilerModel*, const Function& f)
			{
				return Final::getData(f);
			}

			static QVariant getPrimaryData(const ProfilerModel*, const Function&)
			{
				return QVariant();
			}

			static QVariant getSecondaryData(const ProfilerModel*, const Function&)
			{
				return QVariant();
			}

			static EAlignment getAlignment()
			{
				return Alignment;
			}

			static bool less(const Function& lhs, const Function& rhs)
			{
				return Final::getData(lhs) < Final::getData(rhs);
			}

			static bool isProgress()
			{
				return false;
			}

			static profiler::time_type maxDuration(const ProfilerModel* parent)
			{
				return parent->max_duration() * SCALE;
			}
		};

		template <typename Final, typename Scale = Direct, EAlignment Alignment = EAlignment_Right>
		struct NumberColumnInfo: ColumnInfo<Final, Scale, Alignment>
		{
		};

		template <typename Final, typename Scale = Direct, EAlignment Alignment = EAlignment_Right>
		struct TimeColumnInfo: ColumnInfo<Final, Scale, Alignment>
		{
			static QVariant getDisplayData(const ProfilerModel* parent, const Function& f)
			{
				return timeFormat(parent->second() * SCALE, Final::getData(f));
			}
		};

		template <typename Final, typename Total, typename Own, typename Scale = Direct, EAlignment Alignment = EAlignment_Left>
		struct GraphColumnInfo: ColumnInfo<Final, Scale, Alignment>
		{
			static QVariant getDisplayData(const ProfilerModel*, const Function&)
			{
				return QVariant();
			}

			static QVariant getPrimaryData(const ProfilerModel*, const Function& f)
			{
				return Total::getData(f);
			}

			static QVariant getSecondaryData(const ProfilerModel*, const Function& f)
			{
				return Own::getData(f);
			}

			static bool less(const Function& lhs, const Function& rhs)
			{
				return Total::getData(lhs) < Total::getData(rhs);
			}

			static bool isProgress()
			{
				return true;
			}
		};
	}

	struct Name: impl::ColumnInfo<Name>
	{
		static QString title() { return "Name"; }
		static QString getData(const Function& f) { return f.name(); }
	};

	struct Count: impl::NumberColumnInfo<Count>
	{
		static QString title() { return "Calls"; }
		static unsigned long long getData(const Function& f) { return f.call_count(); }
	};

	struct Subcalls: impl::NumberColumnInfo<Subcalls>
	{
		static QString title() { return "Subcalls"; }
		static QVariant getData(const Function& f)
		{
			auto count = f.sub_call_count();
			if (count)
				return count;
			return QVariant();
		}
		static bool less(const Function& lhs, const Function& rhs)
		{
			return lhs.sub_call_count() < rhs.sub_call_count();
		}
	};

	struct TotalTime: impl::TimeColumnInfo<TotalTime>
	{
		static QString title() { return "Total time"; }
		static profiler::time_type getData(const Function& f) { return f.duration(); }
	};

	struct OwnTime: impl::TimeColumnInfo<OwnTime>
	{
		static QString title() { return "Self time"; }
		static profiler::time_type getData(const Function& f) { return f.ownTime(); }
	};

	struct LongestTime: impl::TimeColumnInfo<LongestTime>
	{
		static QString title() { return "Longest time"; }
		static profiler::time_type getData(const Function& f) { return f.longest(); }
	};

	struct ShortestTime: impl::TimeColumnInfo<ShortestTime>
	{
		static QString title() { return "Shortest time"; }
		static profiler::time_type getData(const Function& f) { return f.shortest(); }
	};

	struct TotalTimeAvg: impl::TimeColumnInfo<TotalTimeAvg, impl::Scaled>
	{
		static QString title() { return "Total time (average)"; }
		static profiler::time_type getData(const Function& f) { return f.duration() * SCALE / f.call_count(); }
	};

	struct OwnTimeAvg: impl::TimeColumnInfo<OwnTimeAvg, impl::Scaled>
	{
		static QString title() { return "Self time (average)"; }
		static profiler::time_type getData(const Function& f) { return f.ownTime() * SCALE / f.call_count(); }
	};

	struct Graph: impl::GraphColumnInfo<Graph, TotalTime, OwnTime>
	{
		static QString title() { return "Time"; }
	};

	struct GraphAvg: impl::GraphColumnInfo<GraphAvg, TotalTimeAvg, OwnTimeAvg, impl::Scaled>
	{
		static QString title() { return "Time (average)"; }

		static profiler::time_type maxDuration(const ProfilerModel* parent)
		{
			return parent->max_duration_avg() * SCALE;
		}
	};
}

#endif // PROFILER_MODEL_H
