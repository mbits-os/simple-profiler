#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <QObject>
#include "profiler.h"
#include <stack>
#include <functional>
#include <QThread>

typedef std::vector<profiler::call_id> CalledAs;

class Function
{
	profiler::function_ptr m_function;
	CalledAs               m_calls;
	CalledAs               m_subcalls;
	unsigned long long     m_call_count;
	unsigned long long     m_sub_call_count;
	profiler::time_type    m_duration;
	profiler::time_type    m_ownTime;
	profiler::time_type    m_longest;
	profiler::time_type    m_shortest;
	bool                   m_at_least_one_syscall;

public:
	Function(const profiler::function_ptr& function, const profiler::call_ptr& calledAs);
	void update(const profiler::call_ptr& calledAs);
	void updateSubcalls(const CalledAs& subcalls) { m_subcalls.insert(end(m_subcalls), begin(subcalls), end(subcalls)); }
	profiler::function_id id() const { return m_function->id(); }

	QString name() const { return m_function ? m_function->name() : QString(); }
	const CalledAs& calls() const { return m_calls; }
	const CalledAs& subcalls() const { return m_subcalls; }
	unsigned long long call_count() const { return m_call_count; }
	unsigned long long sub_call_count() const { return m_sub_call_count; }
	profiler::time_type duration() const { return m_duration; }
	profiler::time_type ownTime() const { return m_ownTime; }
	profiler::time_type longest() const { return m_longest; }
	profiler::time_type shortest() const { return m_shortest; }
	bool is_section() const { return m_function->is_section(); }
	bool has_at_least_one_syscall() const {return m_at_least_one_syscall; }
};

typedef std::shared_ptr<Function> FunctionPtr;

class Functions
{
	typedef std::vector<FunctionPtr> functions;
	functions m_functions;
	profiler::time_type m_max_duration;
	profiler::time_type m_max_duration_avg;
public:
	void update(const profiler::functions& functions, const profiler::call_ptr& calledAs, const CalledAs& subcalls);
	size_t size() const { return m_functions.size(); }
	FunctionPtr at(size_t ndx) const { return m_functions.at(ndx); }
	functions::const_iterator begin() const { return m_functions.begin(); }
	functions::const_iterator end() const { return m_functions.end(); }
	void normalize();
	profiler::time_type max_duration() const { return m_max_duration; }
	profiler::time_type max_duration_avg() const { return m_max_duration_avg; }
};

typedef std::shared_ptr<Functions> FunctionsPtr;

class HistoryItem
{
public:
	HistoryItem() {}
	HistoryItem(const CalledAs& calls, QString name): m_calls(calls), m_name(name) {}

	const CalledAs& get_calls() const { return m_calls; }
	FunctionsPtr get_cached() const { return m_cached; }
	bool is_cached() const { return m_cached != nullptr; }
	const QString& name() const { return m_name; }

	void update(const profiler::functions& functions, const profiler::call_ptr& calledAs, const CalledAs& subcalls)
	{
		if (!m_cached)
			m_cached = std::make_shared<Functions>();
		m_cached->update(functions, calledAs, subcalls);
	}

	void normalize() { if (m_cached) m_cached->normalize(); }
	profiler::time_type max_duration() const { return m_cached ? m_cached->max_duration() : 1; }
private:
	CalledAs m_calls;
	FunctionsPtr m_cached;
	QString m_name;
};

typedef std::shared_ptr<HistoryItem> HistoryItemPtr;
typedef std::stack<HistoryItemPtr> History;

struct NavFunction
{
	std::function<void ()> f;
	NavFunction( std::function<void ()> f ): f(f) {}

	void call()
	{
		f();
		delete this;
	}
};

class SelectTask: public QThread
{
	Q_OBJECT;

	std::function<void ()> call;
	std::function<void ()> cont;
public:
	SelectTask(QObject *parent, std::function<void ()> call, std::function<void ()> cont): QThread(parent), call(call), cont(cont) {}

	void run();

signals:
	void selected(NavFunction* cont);
};

class Navigator : public QObject
{
	Q_OBJECT

	profiler::data_ptr m_data;
	History m_history;
	HistoryItemPtr m_currentView;

	void select(const HistoryItemPtr& item, std::function<void ()> cont);
	void doSelect(const HistoryItemPtr& item);
public:
	explicit Navigator(QObject *parent = 0);
	void setData(const profiler::data_ptr& data) { m_data = data; }
	HistoryItemPtr current() { return m_currentView; }

signals:
	void hasHistory(bool);
	void selectStarted();
	void selectStopped();

public slots:
	void back();
	void home();
	void navigateTo(size_t);
	void cancel();

private slots:
	void onSelected(NavFunction* cont);
};

#endif // NAVIGATOR_H
