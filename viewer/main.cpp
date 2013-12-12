#include "mainwindow.h"
#include <QApplication>
#include <profile/profile.hpp>
#include <profile/write.hpp>

int main(int argc, char *argv[])
{
#ifdef FEATURE_IO_WRITE
	profile::io::writer stats("stats");
	stats;
#endif // FEATURE_IO_WRITE

	FUNCTION_PROBE();

	QApplication a(argc, argv);
	QCoreApplication::setOrganizationName("midnightBITS");
	QCoreApplication::setOrganizationDomain("midnightbits.com");
	QCoreApplication::setApplicationName("Profile Viewer");

	MainWindow w;
	w.show();
	w.open();

	FUNCTION_PROBE2(exec, "exec");
	return a.exec();
}
