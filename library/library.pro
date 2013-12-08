#-------------------------------------------------
#
# Project created by QtCreator 2013-12-06T14:48:15
#
#-------------------------------------------------

QT       -= gui

TARGET = profile
TEMPLATE = lib
CONFIG += staticlib

DEFINES += XML_STATIC

SOURCES += src/profile.cpp \
    src/write.cpp \
    src/write_xml.cpp \
    src/write_binary.cpp \
    src/read.cpp \
    src/read_xml.cpp \
    src/read_binary.cpp \
    src/reader.cpp

HEADERS += include/profile/profile.hpp \
    include/profile/ticker.hpp \
    include/profile/write.hpp \
    include/profile/read.hpp \
    src/binary.hpp \
    src/reader.hpp \
    src/expat.hpp

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
    ../library/include \
    ../3rdparty/libexpat/inc
