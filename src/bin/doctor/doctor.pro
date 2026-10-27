TEMPLATE = app
QT -= gui
QT += dbus
CONFIG += command_line
CONFIG += link_pkgconfig
QMAKE_CFLAGS += -Werror

INCLUDEPATH += $$_PRO_FILE_PWD_/../../

SOURCES += \
    src/main.cpp \
    src/patchmanagerdoctor.cpp
HEADERS += \
    src/patchmanagerdoctor.h

TARGET = patchmanagerdoctor
target.path = /usr/sbin

INSTALLS = target

# DBus
pd_dbus_adaptor.files = dbus/org.SfietKonstantin.patchmanagerDoctor.xml
pd_dbus_adaptor.source_flags = -c PatchmanagerDoctorAdaptor
#pd_dbus_adaptor.header_flags = -c PatchmanagerDoctorAdaptor -i patchmanagerdoctor_include.h
DBUS_ADAPTORS += pd_dbus_adaptor

