TEMPLATE = app
QT -= gui
QT += dbus
CONFIG += command_line
# CONFIG += link_pkgconfig
QMAKE_CFLAGS += -Werror

INCLUDEPATH += \
    $$_PRO_FILE_PWD_/include/ \
    $$_PRO_FILE_PWD_/../../ \
    $$_PRO_FILE_PWD_/..

SOURCES += \
    src/main.cpp \
    src/patchmanagerdoctor.cpp
HEADERS += \
    src/patchmanagerdoctor.h

TARGET = patchmanagerdoctor
target.path = /usr/libexec

INSTALLS = target
