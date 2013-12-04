#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "profiler.h"
#include "navigator.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    void onOpened(bool success, const QString& fileName);
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void back() { onBack(); }
    void home() { onHome(); }
    void open();

private slots:
    void hasHistory(bool value);

signals:
    void onBack();
    void onHome();

private:
    Ui::MainWindow *ui;
    profiler::data m_data;
    Navigator* m_nav;
};

#endif // MAINWINDOW_H
