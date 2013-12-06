#-------------------------------------------------
#
# Project created by QtCreator 2013-12-03T20:28:26
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ProfileViewer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    profiler.cpp \
    navigator.cpp \
    profiler_model.cpp

HEADERS  += mainwindow.h \
    profiler.h \
    navigator.h \
    profiler_model.h

FORMS    += mainwindow.ui

RESOURCES += \
    ProfileViewer.qrc

RC_FILE = ProfileViewer.rc

win32 {
SOURCES += win32_setIcon.cpp
}
