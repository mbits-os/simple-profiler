TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    library \
    viewer

viewer.depends = library
