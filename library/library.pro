#-------------------------------------------------
#
# Project created by QtCreator 2013-12-06T14:48:15
#
#-------------------------------------------------

QT       -= core gui

TARGET = profile
TEMPLATE = lib
CONFIG += staticlib

SOURCES += profile.cpp

HEADERS += profile.hpp \
    ticker.hpp

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

win32 {
SOURCES += win32_ticker.cpp
}