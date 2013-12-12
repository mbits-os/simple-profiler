#include "call_tree_model.h"
#include "algorithm"
#include <QIcon>
#include <QDebug>
#include <profile/profile.hpp>

namespace call_tree
{
	namespace build
	{
		struct call;

		typedef std::weak_ptr<call>       call_weak;
		typedef std::shared_ptr<call>     call_ptr;

		typedef std::map<profiler::function_id, function_ptr> functions;
		typedef std::vector<function_ptr>                     function_list;
		typedef std::map<profiler::call_id, call_ptr>         calls;
		typedef std::vector<call_ptr>                         call_list;

		struct call
		{
			profiler::call_id id;
			profiler::call_id parent_id;
			unsigned int flags;

			call_weak parent;
			function_weak owner;
			call_list calls;

			call(const profiler::call& src, const function_ptr& owner)
				: id(src.id())
				, parent_id(src.parent())
				, flags(src.flags())
				, owner(owner)
			{}
		};

		struct function
		{
			profiler::function_id id;

			call_list occurence;
			build::function_weak_list calls;
			build::function_weak_list called_by;

			call_tree::function_ptr dst;

			function(const profiler::function& src, item* _parent, size_t row)
				: id(src.id())
				, dst(std::make_shared<call_tree::function>(src.id(), src.name(), _parent, row))
			{
				if (src.is_section())
					dst->_icon = icons::EIconType_Section;
			}

			void build_refs()
			{
				bool is_any_syscall = false;
				for (auto&& occ: occurence)
				{
					if (occ->flags & profile::ECallFlag_SYSCALL)
						is_any_syscall = true;
					auto parent = occ->parent.lock();
					if (parent)
					{
						auto fun = parent->owner.lock();
						if (fun)
							called_by[fun->dst->_name] = fun;
					}

					for (auto&& c : occ->calls)
					{
						auto fun = c->owner.lock();
						if (fun)
							calls[fun->dst->_name] = fun;
					}
				}

				if (dst->_icon == icons::EIconType_Function && is_any_syscall)
					dst->_icon = icons::EIconType_Syscall;
			}
		};
	}

	void function::copy_refs(const build::function_ptr& src, const function_list& list)
	{
		calls->clear();
		called_by->clear();

		copy(calls, src->calls, list);
		copy(called_by, src->called_by, list);

		size_t row = 0;
		called_by->setRow(called_by->empty() ? 3 : row++);
		calls->setRow(calls->empty() ? 3 : row++);
	}

	void function::copy(function_ref_list_ptr& dst, const build::function_weak_list& src, const function_list& list)
	{
		for (auto&& pair: src)
		{
			auto fun = pair.second.lock();
			if (!fun)
				continue;

			auto call = find(list, fun->id);
			if (call)
				dst->push_back(call);
		}
	}

	function_ptr function::find(const function_list& list, profiler::function_id id)
	{
		for (auto&& f: list)
		{
			if (f->id == id)
				return f;
		}

		return nullptr;
	}

	void database::setData(const profiler::data_ptr& data)
	{
		m_functions.clear();

		build::calls    calls;
		build::functions functions;

		const auto& prof_functions = data->functions();
		const auto& prof_calls = data->calls();

		for (auto&& f : prof_functions)
		{
			if (!f) continue;
			functions[f->id()] = std::make_shared<build::function>(*f.get(), this, functions.size());
		}

		for (auto&& c : prof_calls)
		{
			if (!c) continue;

			auto it = functions.find(c->functionId());
			if (it == end(functions) || !it->second)
			{
				continue;
			}

			auto call = std::make_shared<build::call>(*c.get(), it->second);
			calls[c->id()] = call;
			it->second->occurence.push_back(call);
		}

		for (auto&& c : calls)
		{
			auto&& cc = c.second;
			if (!cc->parent_id)
				continue;

			auto it = calls.find(cc->parent_id);
			if (it == end(calls))
			{
				continue;
			}

			cc->parent = it->second;
			it->second->calls.push_back(cc);
		}

		m_functions.reserve(functions.size());

		for (auto&& f : functions)
		{
			f.second->build_refs();
			m_functions.push_back(f.second->dst);
		}

		for (auto&& f : functions)
			f.second->dst->copy_refs(f.second, m_functions);
	}

	// tree items:
	QString function_ref::name() const
	{
		auto function = ref.lock();
		if (!function)
			return "<deleted>";
		return function->name();
	}

	QString function_ref_list::name() const
	{
		return type == ERefType_CALLS ? "Calls" : "Called by";
	}

}

CallTreeModel::CallTreeModel(QObject *parent) :
	QAbstractListModel(parent)
{
}

void CallTreeModel::setData(const profiler::data_ptr& data)
{
	beginResetModel();
	m_db.setData(data);
	endResetModel();
}

bool CallTreeModel::findLink(QModelIndex& inoutIndex)
{
	if (!inoutIndex.isValid())
		return false;

	const call_tree::item* item = static_cast<const call_tree::item*>(inoutIndex.internalPointer());
	auto fun = item->link();

	if (!fun)
		return false;

	call_tree::item* link = fun.get();

	if (!link || link == &m_db)
		return false;

	inoutIndex = createIndex(link->row(), 0, link);
	return true;
}

QIcon iconFile(const QString& off, const QString& on)
{
	QIcon icon;
	icon.addFile(off);
	icon.addFile(on, QSize(), QIcon::Normal, QIcon::On);
	return icon;
}

QVariant CallTreeModel::data(const QModelIndex &index, int role) const
{
	const call_tree::item* item = nullptr;
	if (index.isValid())
		item = static_cast<const call_tree::item*>(index.internalPointer());

	if (!item)
		return QVariant();

	switch (role)
	{
	case Qt::DisplayRole:
		return item->name();
	case Qt::DecorationRole:
		return icons::icon(item->icon());
	}

	return QVariant();
}

QVariant CallTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0)
		return m_db.name();
	return QVariant();
}

QModelIndex CallTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	const call_tree::item* item = &m_db;
	if (parent.isValid())
		item = static_cast<const call_tree::item*>(parent.internalPointer());

	call_tree::item* child = item->at(row);
	if (child)
		return createIndex(row, column, child);

	return QModelIndex();
}

QModelIndex CallTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	const call_tree::item* item = static_cast<const call_tree::item*>(index.internalPointer());
	call_tree::item* parent = item->parent();

	if (!parent || parent == &m_db)
		return QModelIndex();

	return createIndex(parent->row(), 0, parent);
}

int CallTreeModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0)
		return 0;

	const call_tree::item* item = &m_db;
	if (parent.isValid())
		item = static_cast<const call_tree::item*>(parent.internalPointer());

	return item->count();
}

int CallTreeModel::columnCount(const QModelIndex &) const
{
	return 1;
}

bool CallTreeModel::hasChildren(const QModelIndex &parent) const
{
	return rowCount(parent) != 0;
}
