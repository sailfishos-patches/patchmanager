TEMPLATE = app
TARGET = patchmanager-dialog

QT += dbus network
CONFIG += sailfishapp c++11

dbus.files = dbus/org.SfietKonstantin.patchmanager.service
dbus.path = /usr/share/dbus-1/services
INSTALLS += dbus

systemd.files = \
    systemd/dbus-org.SfietKonstantin.patchmanager.service \
    systemd/lipstick-patchmanager.service
systemd.path = /usr/lib/systemd/user
INSTALLS += systemd

icons.files = patchmanager-icon.svg
icons.path = /usr/share/patchmanager/data
INSTALLS += icons

gui.files = dialog.qml
gui.path = /usr/share/patchmanager/data
INSTALLS += gui

SOURCES += \
    main.cpp
