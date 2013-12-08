TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    3rdparty \
    library \
    viewer

viewer.depends = 3rdparty library
