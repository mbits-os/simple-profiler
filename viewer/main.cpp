#include "mainwindow.h"
#include <QApplication>
#include <profile.hpp>

int main(int argc, char *argv[])
{
    profile::print::printer stats("stats", profile::print::EPrinter_BIN);
    stats;

    //FUNCTION_PROBE();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    w.open();

    return a.exec();
}
