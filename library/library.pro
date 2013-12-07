#-------------------------------------------------
#
# Project created by QtCreator 2013-12-06T14:48:15
#
#-------------------------------------------------

QT       -= core gui

TARGET = profile
TEMPLATE = lib
CONFIG += staticlib

SOURCES += src/profile.cpp \
    src/print.cpp \
    src/print_xml.cpp \
    src/print_binary.cpp \
    src/read.cpp \
    src/read_xml.cpp \
    src/read_binary.cpp

HEADERS += include/profile/profile.hpp \
    include/profile/ticker.hpp \
    include/profile/print.hpp \
    include/profile/read.hpp \
    src/binary.hpp

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

win32 {
SOURCES += src/win32_ticker.cpp
}

INCLUDEPATH += \
    ../library/include
