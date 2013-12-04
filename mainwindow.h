#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "profiler.h"
#include "navigator.h"
#include <functional>
#include <QThread>

namespace Ui {
class MainWindowEx;
}

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

public slots:
    void back() { emit onBack(); }
    void home() { emit onHome(); }
    void open();
    void aTaskStarted();
    void aTaskStopped();

private slots:
    void hasHistory(bool value);
    void onOpened(bool success, QString fileName);

signals:
    void onBack();
    void onHome();

private:
    Ui::MainWindowEx *ui;
    profiler::data_ptr m_data;
    Navigator* m_nav;
    unsigned long m_animationCount;

    void doOpen(const QString& fileName);
};

#endif // MAINWINDOW_H
