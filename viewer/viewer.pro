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

INCLUDEPATH += ../library/include

CONFIG(debug, debug|release) {
unix:LIBS += -L../library/debug -lprofile -lexpat
windows:LIBS += ../library/debug/profile.lib ../3rdparty/libexpat/debug/expat.lib
}
else {
unix:LIBS += -L../library/release -lprofile -lexpat
windows:LIBS += ../library/release/profile.lib ../3rdparty/libexpat/release/expat.lib
}

