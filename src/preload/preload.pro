TEMPLATE = lib
QT =
CONFIG += plugin
QMAKE_CFLAGS += -std=c11

LIBS = -ldl

SOURCES += \
    src/preloadpatchmanager.c

TARGET = preloadpatchmanager
target.path = /usr/lib

INSTALLS = target
