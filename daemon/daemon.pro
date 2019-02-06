TEMPLATE = app
TARGET = patchmanager-daemon

QT = core dbus network
CONFIG += link_pkgconfig
PKGCONFIG += nemonotifications-qt5
PKGCONFIG += libsystemd-journal
PKGCONFIG += rpm

isEmpty(PROJECT_PACKAGE_VERSION) {
    BUILD_VERSION = "4.99.99"
} else {
    BUILD_VERSION = $$PROJECT_PACKAGE_VERSION
}
message("Version: $$BUILD_VERSION")

DEFINES += BUILD_VERSION=\\\"$$BUILD_VERSION\\\"

#HEADERS += \
#    src/patchmanager_include.h

SOURCES += \
    src/main.cpp \
    src/patchmanager.cpp

target.path = /usr/sbin
INSTALLS += target

# DBus service
dbusService.files = dbus/org.coderus.patchmanager.service
dbusService.path = /usr/share/dbus-1/system-services/
INSTALLS += dbusService

# DBus interface
dbusInterface.files = dbus/org.coderus.patchmanager.xml
dbusInterface.path = /usr/share/dbus-1/interfaces/
INSTALLS += dbusInterface

# DBus config
dbusConf.files = dbus/org.coderus.patchmanager.conf
dbusConf.path = /etc/dbus-1/system.d/
INSTALLS += dbusConf

# Systemd
systemd.files = \
    systemd/patchmanager.service \
    systemd/checkForUpdates-patchmanager.service \
    systemd/checkForUpdates-patchmanager.timer
systemd.path = /lib/systemd/system/
INSTALLS += systemd

# user environment
env.files = environment/01-dbus.conf
env.path = /var/lib/environment/patchmanager/
INSTALLS += env

# dbus interface
pm_dbus_adaptor.files = dbus/org.coderus.patchmanager.xml
pm_dbus_adaptor.source_flags = -c PatchManagerAdaptor
pm_dbus_adaptor.header_flags = -c PatchManagerAdaptor# -i patchmanager_include.h
DBUS_ADAPTORS += pm_dbus_adaptor

INCLUDEPATH += /usr/include/

HEADERS += \
    src/patchmanager.h
