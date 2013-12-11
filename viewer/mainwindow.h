#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "profiler.h"
#include "navigator.h"
#include <functional>
#include <QThread>
#include <QModelIndex>

namespace Ui {
class MainWindowEx;
}

class ProfilerModel;
class ProfilerDelegate;
class CallTreeModel;

class OpenTask: public QThread
{
	Q_OBJECT;

	QString fileName;
	std::function<bool ()> call;
public:
	OpenTask(QObject *parent, std::function<bool ()> call, QString fileName): QThread(parent), call(call), fileName(fileName) {}

	void run();

signals:
	void opened(bool success, QString fileName);
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	void closeEvent(QCloseEvent * event);

public slots:
	void storeSettings();
	void back() { emit onBack(); }
	void home() { emit onHome(); }
	void open();
	void selected(QModelIndex);
	void aTaskStarted();
	void aTaskStopped();
	void aTaskStopped_nav();

private slots:
	void hasHistory(bool value);
	void onOpened(bool success, QString fileName);
	void onColumnChanged(QAction* action);
	void onColumnsMenu(QPoint pos);
	void onViewChanged(QAction* action);

signals:
	void onBack();
	void onHome();
	void onNavigate(size_t);

private:
	Ui::MainWindowEx *ui;
	profiler::data_ptr m_data;
	Navigator* m_nav;
	ProfilerModel* m_model;
	CallTreeModel* m_call_tree;
	ProfilerDelegate* m_delegate;
	unsigned long m_animationCount;

	void loadSettings();

	void doOpen(const QString& fileName);
};

#endif // MAINWINDOW_H
