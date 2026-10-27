TEMPLATE = app
QT -= gui
QT += dbus
CONFIG += command_line
# CONFIG += link_pkgconfig
QMAKE_CFLAGS += -Werror

INCLUDEPATH += \
    $$_PRO_FILE_PWD_/../patchmanager-daemon \
    $$_PRO_FILE_PWD_/../../ \
    $$_PRO_FILE_PWD_/..

DEFINES += PATCHMANAGERDOCTOR_STANDALONE

SOURCES += \
    main.cpp \
    patchmanagerdoctor.cpp
HEADERS += \
    patchmanagerdoctor.h

TARGET = patchmanagerdoctor
target.path = /usr/libexec

INSTALLS = target
