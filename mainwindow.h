#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "profiler.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void onBack();
    void onHome();
    void onOpen();

private:
    Ui::MainWindow *ui;
    profiler::data m_data;
};

#endif // MAINWINDOW_H
