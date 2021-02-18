TEMPLATE = lib
QT =
CONFIG += plugin
CONFIG += link_pkgconfig
PKGCONFIG += libshadowutils
INCLUDEPATH += /usr/include/libshadowutils
QMAKE_CFLAGS += -std=c11

LIBS = -ldl

SOURCES += \
    src/preloadpatchmanager.c

TARGET = preloadpatchmanager
target.path = /usr/lib

INSTALLS = target
