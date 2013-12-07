#include "mainwindow.h"
#include <QApplication>
#include <profile/profile.hpp>
#include <profile/write.hpp>

int main(int argc, char *argv[])
{
	profile::io::writer stats("stats");
	stats;

	//FUNCTION_PROBE();

	QApplication a(argc, argv);
	MainWindow w;
	w.show();
	w.open();

	return a.exec();
}
