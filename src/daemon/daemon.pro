TEMPLATE = app
TARGET = patchmanager-daemon

QT = core dbus

HEADERS += \
    patchmanagerobject.h

SOURCES += \
    main.cpp \
    patchmanagerobject.cpp

OTHER_FILES += dbus/org.SfietKonstantin.patchmanager.xml \
    dbus/org.SfietKonstantin.patchmanager.service \
    dbus/org.SfietKonstantin.patchmanager.conf \
    config/applied_patches.json


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

# Config
config.files = config/applied_patches.json
config.path = /var/lib/patchmanager
INSTALLS += config

