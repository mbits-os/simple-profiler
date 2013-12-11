#ifndef CALL_TREE_MODEL_H
#define CALL_TREE_MODEL_H

#include <QAbstractListModel>
#include "profiler.h"
#include <memory>

namespace call_tree
{
	struct function;
	struct function_ref;
	struct function_ref_list;

	typedef std::weak_ptr<function>            function_weak;
	typedef std::shared_ptr<function>          function_ptr;
	typedef std::shared_ptr<function_ref>      function_ref_ptr;
	typedef std::shared_ptr<function_ref_list> function_ref_list_ptr;
	typedef std::vector<function_ptr>          function_list;
	typedef std::vector<function_ref_ptr>      function_refs;

	namespace build
	{
		struct function;

		typedef std::weak_ptr<build::function>          function_weak;
		typedef std::shared_ptr<build::function>        function_ptr;
		typedef std::map<QString, build::function_weak> function_weak_list;
	}

	enum ERefType
	{
		ERefType_CALLED_BY,
		ERefType_CALLS
	};

	enum EIconType
	{
		EIconType_Folder,
		EIconType_FolderOpened,
		EIconType_Function,
		EIconType_Reference
	};

	struct item
	{
		item* _parent;
		size_t _row;
		item(item* _parent, size_t _row = 0): _parent(_parent), _row(_row) {}
		void setRow(size_t _row) { this->_row = _row; }

		virtual QString name() const = 0;
		virtual size_t count() const = 0;
		virtual item* at(size_t i) const = 0;
		virtual EIconType icon(bool expanded) const = 0;
		virtual item* link() const { return nullptr; }
		function_ptr parent() const { return _parent; }
		size_t row() const { return _row; }
	};

	struct function_ref: item
	{
		function_weak ref;
		function_ref(const function_ptr& ref, item* _parent, size_t _row): item(_parent, _row), ref(ref) {}

		QString name() const;
		size_t count() const { return 0; }
		item* at(size_t) const { return nullptr; }
		EIconType icon(bool) const { return EIconType_Reference; }
		virtual function_ptr link() const { return ref.lock(); }
	};

	struct function_ref_list: item
	{
		ERefType type;
		function_refs items;

		function_ref_list(ERefType type, item* _parent): item(_parent), type(type) {}

		void push_back(const function_ptr& ref)
		{
			auto tmp = std::make_shared<function_ref>(ref, this, items.size());
			items.push_back(tmp);
		}
		void clear() { items.clear(); }
		bool empty() const { return items.empty(); }

		QString name() const;
		size_t count() const { return items.size(); }
		item* at(size_t i) const { return i < items.size() ? items.at(i).get() : nullptr; }
		EIconType icon(bool expanded) const { return expanded ? EIconType_FolderOpened : EIconType_Folder; }
	};

	struct function: item
	{
		profiler::function_id id;
		QString _name;

		function_ref_list_ptr calls;
		function_ref_list_ptr called_by;

		function(profiler::function_id id, const QString& name, item* _parent, size_t _row)
			: item(_parent, _row)
			, id(id)
			, _name(name)
			, calls(std::make_shared<function_ref_list>(ERefType_CALLS, this))
			, called_by(std::make_shared<function_ref_list>(ERefType_CALLED_BY, this))
		{}

		void copy_refs(const build::function_ptr& src, const function_list& list);

		QString name() const { return _name; }
		size_t count() const
		{
			size_t ret = 0;
			if (!called_by->empty()) ret++;
			if (!calls->empty()) ret++;
			return ret;
		}
		item* at(size_t i) const
		{
			if (!called_by->empty())
			{
				if (!i)
					return called_by.get();
				--i;
			}
			if (!calls->empty())
			{
				if (!i)
					return calls.get();
				--i;
			}
			return nullptr;
		}
		EIconType icon(bool) const { return EIconType_Function; }
	private:
		static void copy(function_ref_list_ptr& dst, const build::function_weak_list& src, const function_list& list);
		static function_ptr find(const function_list& list, profiler::function_id id);
	};

	struct database: item
	{
		function_list m_functions;

		database(): item(nullptr) {}

		void setData(const profiler::data_ptr& data);

		QString name() const { return "Name"; }
		size_t count() const { return m_functions.size(); }
		item* at(size_t i) const { return i < m_functions.size() ? m_functions.at(i).get() : nullptr; }
		EIconType icon(bool) const { return EIconType_Function; }
	};
}

class CallTreeModel : public QAbstractListModel
{
	Q_OBJECT

	call_tree::database m_db;
public:
	explicit CallTreeModel(QObject *parent = 0);

	void setData(const profiler::data_ptr& data);

	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &) const { return Qt::ItemIsEnabled | Qt::ItemIsSelectable; }
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	QModelIndex index(int row, int column, const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	bool hasChildren(const QModelIndex &parent) const;
};

#endif // CALL_TREE_MODEL_H
