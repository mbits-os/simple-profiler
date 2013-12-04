#-------------------------------------------------
#
# Project created by QtCreator 2013-12-03T20:28:26
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ProfileViewer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    profiler.cpp

HEADERS  += mainwindow.h \
    profiler.h

FORMS    += mainwindow.ui

RESOURCES += \
    ProfileViewer.qrc
