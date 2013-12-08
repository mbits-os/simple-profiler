QT       -= core gui

TARGET = expat
TEMPLATE = lib
CONFIG += staticlib

DEFINES += HAVE_CONFIG_H POSIX ZLIB L_ENDIAN HAVE_MEMMOVE

SOURCES += src/xmlparse.c \
	src/xmlrole.c \
	src/xmltok.c \
	src/xmltok_impl.c \
	src/xmltok_ns.c

HEADERS += inc/amigaconfig.h \
	inc/ascii.h \
	inc/asciitab.h \
	inc/expat.h \
	inc/expat_config.h \
	inc/expat_external.h \
	inc/iasciitab.h \
	inc/internal.h \
	inc/latin1tab.h \
	inc/macconfig.h \
	inc/nametab.h \
	inc/utf8tab.h \
	inc/winconfig.h \
	inc/xmlrole.h \
	inc/xmltok.h \
	inc/xmltok_impl.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

INCLUDEPATH += inc
