TEMPLATE = app
TARGET = patchmanager

QT = core dbus

#SOURCES += \
#    notification.cpp \
#    notificationmanagerproxy.cpp

#HEADERS += \
#    notification.h \
#    notificationmanagerproxy.h

HEADERS += \
#    adaptor.h \
    patchmanagerobject.h

SOURCES += \
    main.cpp \
#    adaptor.cpp \
    patchmanagerobject.cpp

OTHER_FILES += dbus/org.SfietKonstantin.patchmanager.xml \
    dbus/org.SfietKonstantin.patchmanager.service \
    dbus/org.SfietKonstantin.patchmanager.conf

target.path = /usr/sbin
INSTALLS += target

# DBus service
dbusService.files = dbus/org.SfietKonstantin.patchmanager.service
dbusService.path = /usr/share/dbus-1/system-services/
INSTALLS += dbusService

# DBus interface
dbusInterface.files = dbus/org.SfietKonstantin.patchmanager.xml
dbusInterface.path = /usr/share/dbus-1/interfaces/
INSTALLS += dbusInterface

# DBus config
dbusConf.files = dbus/org.SfietKonstantin.patchmanager.conf
dbusConf.path = /etc/dbus-1/system.d/
INSTALLS += dbusConf

systemd.files = systemd/dbus-org.SfietKonstantin.patchmanager.service
systemd.path = /lib/systemd/system/
INSTALLS += systemd

env.files = environment/10-dbus.conf
env.path = /var/lib/environment/patchmanager/
INSTALLS += env

# Tools
tools.files = check-package.sh
tools.path = /usr/share/patchmanager/tools
INSTALLS += tools

# Patch
patch.path = /usr/share/patchmanager/patches/sailfishos-patchmanager-unapplyall
patch.files = patch/patch.json patch/unified_diff.patch
INSTALLS += patch

# DBus
#system(qdbusxml2cpp dbus/org.SfietKonstantin.patchmanager.xml -i patchmanagerobject.h -a adaptor)
#system(qdbusxml2cpp org.freedesktop.Notifications.xml -p notificationmanagerproxy -c NotificationManagerProxy -i notification.h)

pm_dbus_adaptor.files = dbus/org.SfietKonstantin.patchmanager.xml
pm_dbus_adaptor.source_flags = -c PatchManagerAdaptor
pm_dbus_adaptor.header_flags = -c PatchManagerAdaptor -i patchmanagerobject.h
DBUS_ADAPTORS += pm_dbus_adaptor

#dbus_interface.files = org.freedesktop.Notifications.xml
#dbus_interface.source_flags = -p notificationmanagerproxy -c NotificationManagerProxy
#dbus_interface.header_flags = -p notificationmanagerproxy -c NotificationManagerProxy -i notification.h
#DBUS_INTERFACES += dbus_interface

