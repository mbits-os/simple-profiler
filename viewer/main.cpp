#include "mainwindow.h"
#include <QApplication>
#include <profile/profile.hpp>
#include <profile/print.hpp>

int main(int argc, char *argv[])
{
    profile::print::printer stats("stats");
    stats;

    //FUNCTION_PROBE();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    w.open();

    return a.exec();
}
