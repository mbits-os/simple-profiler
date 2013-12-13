#include "profiler_model.h"
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QMenu>
#include <sstream>

ProfilerModel::ProfilerModel(QObject *parent)
	: QAbstractListModel(parent)
	, m_second(1)
	, m_max(1)
{
	m_column_bag.setModel(this);
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

void ProfilerModel::removeColumn(const ColumnPtr &col)
{
	auto it = m_columns.begin();
	auto end = m_columns.end();
	for (; it != end; ++it)
	{
		if (*it == col)
			break;
	}

	if (it == end)
		return;

	auto colid = std::distance(m_columns.begin(), it);
	beginRemoveColumns(QModelIndex(), colid, colid);
	m_columns.erase(it);
	// TODO: what if we removed sorted column?
	endRemoveColumns();
}

FunctionPtr ProfilerModel::getItem(const QModelIndex& index)
{
	if (index.isValid() && (size_t)index.row() < m_sorted.size())
		return m_sorted[index.row()];

	return nullptr;
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
		case Qt::DecorationRole:
		{
			auto function = m_sorted.at(index.row());

			if (!function)
				break;

			return icons::icon(m_columns.at(index.column())->getIcon(*function.get()));
		}
		case Qt::PrimaryDisplayRole:
		{
			auto function = m_sorted.at(index.row());

			if (!function)
				break;

			return m_columns.at(index.column())->getPrimaryData(this, *function.get());
		}
		case Qt::SecondaryDisplayRole:
		{
			auto function = m_sorted.at(index.row());

			if (!function)
				break;

			return m_columns.at(index.column())->getSecondaryData(this, *function.get());
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
			};
			break;
		}
		case Qt::IsProgressRole:
		{
			return m_columns.at(index.column())->isProgress();
		}
		case Qt::ProgressMaxRole:
		{
			return m_columns.at(index.column())->maxDuration(this);
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

ProfilerDelegate::ProfilerDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{
}

void ProfilerDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	static QColor COLOR_PRIMARY(0x00, 0x80, 0x00);
	static QColor COLOR_SECONDARY(0x00, 0x40, 0x00);

	QStyledItemDelegate::paint(painter, option, index);

	if (index.data(Qt::IsProgressRole).toBool())
	{
		bool has_max = true, has_primary = true, has_secondary = true;

		qulonglong max_progress = index.data(Qt::ProgressMaxRole).toULongLong(&has_max);
		qulonglong progress = index.data(Qt::PrimaryDisplayRole).toULongLong(&has_primary);
		qulonglong secondary_progress = index.data(Qt::SecondaryDisplayRole).toULongLong(&has_secondary);

		if (!has_secondary)
			secondary_progress = 0;

		if (has_max && has_primary)
		{
			auto rect = option.rect.normalized();
			int adjX = 2, adjY = 2;
			if (rect.width() <= 6) adjX = 0;
			if (rect.height() <= 6) adjY = 0;
			rect.adjust(adjX, adjY, -adjX, -adjY);

			auto rectPrimary = rect;
			auto rectSecondary = rect;

			rectPrimary.setWidth(rectPrimary.width() * progress / max_progress);
			rectSecondary.setWidth(rectPrimary.width() * secondary_progress / max_progress);

			painter->fillRect(rectPrimary, COLOR_PRIMARY);
			painter->fillRect(rectSecondary, COLOR_SECONDARY);
		}
	}
}

QAction* ColumnBag::ColDef::getAction(QObject* parent)
{
	if (!m_action)
	{
		m_action = new QAction(parent);
		m_action->setCheckable(true);
		m_action->setChecked(m_used);
		if (m_col)
			m_action->setText(m_col->title());
	}
	return m_action;
}

ColumnBag::ColumnBag()
{
	using namespace Columns;
	add<Name>();
	add<Count>();
	add<Subcalls>();
	add<TotalTime>();
	add<OwnTime>();
	add<LongestTime>();
	add<ShortestTime>();
	add<TotalTimeAvg>();
	add<OwnTimeAvg>();
	add<Graph>();
	add<GraphAvg>();
	add<Type>();
}

void ColumnBag::buildColumnMenu(QObject* parent, QMenu* menu)
{
	long long ndx = 0;
	for (auto&& def: m_defs)
	{
		auto action = def.getAction(parent);
		if (action)
		{
			action->setData(ndx);
			menu->addAction(action);
		}
		++ndx;
	}
}

long long ColumnBag::positionOf(const ColumnPtr& col) const
{
	long long pos = 0;
	for (auto&& def: m_defs)
	{
		if (def.col() == col)
			return pos;
		++pos;
	}

	return pos;
}

void ColumnBag::defaultColumns()
{
	using namespace Columns;
	use<Count>();
	use<TotalTime>();
	use<OwnTime>();
	use<Graph>();
	use<Name>();
}

bool ColumnBag::loadSettings(QSettings& settings)
{
	if (settings.contains("columns"))
	{
		QByteArray used = settings.value("columns").toByteArray();
		if ((used.size() % sizeof(long long)) == 0)
		{
			const long long* index = (const long long*)used.data();
			size_t count = used.size() / sizeof(long long);
			for (size_t i = 0; i < count; ++i, ++index)
			{
				use(*index);
			}
			return true;
		}
	}

	defaultColumns();
	return false;
}

void ColumnBag::storeSettings(QSettings& settings) const
{
	size_t count = m_model->getColumnCount();
	std::vector<long long> data(count);

	if (data.size() != count)
		return;

	for (size_t i = 0; i < count; ++i)
	{
		ColumnPtr col = m_model->getColumn(i);
		if (!col)
			return;

		auto ndx = positionOf(col);
		if (ndx >= m_defs.size())
			return;

		data[i] = ndx;
	}

	QByteArray indices((const char*)data.data(), data.size() * sizeof(long long));
	settings.setValue("columns", indices);
}


void ColumnBag::use(long long ndx)
{
	if (ndx < 0 || ndx >= (long long)m_defs.size())
		return;

	auto& def = m_defs[(size_t)ndx];

	if (!def.use())
		return;

	m_model->appendColumn(def.col());
}

void ColumnBag::stop(long long ndx)
{
	if (ndx < 0 || ndx >= (long long)m_defs.size())
		return;

	auto& def = m_defs[(size_t)ndx];

	if (!def.stop())
		return;

	m_model->removeColumn(def.col());
}


QString Columns::impl::timeFormat(profiler::time_type second, profiler::time_type ticks)
{
	profiler::time_type seconds = ticks/second;

	if (seconds > 59)
	{
		ticks -= (seconds/60)*60*second;
		return QString("%1m%2.%3s")
				.arg(seconds/60)
				.arg(ticks/second)
				.arg(profiler::time_type((ticks*1000 / second) % 1000), 3, 10, QLatin1Char('0'));
	}

	QString freq = "s";
	if (ticks*10 / second < 9)
	{
		freq = "ms";
		ticks *= 1000;
		if (ticks*10 / second < 9)
		{
			freq = QString("µs");
			ticks *= 1000;
			if (ticks*10 / second < 9)
			{
				freq = "ns";
				ticks *= 1000;
			}
		}
	}

	return QString("%1.%2%3")
			.arg(ticks/second)
			.arg(profiler::time_type((ticks*1000 / second) % 1000), 3, 10, QLatin1Char('0'))
			.arg(freq);
}

QString Columns::impl::usageFormat(profiler::time_type max, profiler::time_type total, profiler::time_type own)
{
	return QString("%1.%2% / %3.%4%")
			.arg(total * 100 / max)
			.arg((total * 10000 / max) % 100, 2, 10, QLatin1Char('0'))
			.arg(own * 100 / max)
			.arg((own * 10000 / max) % 100, 2, 10, QLatin1Char('0'));
}
