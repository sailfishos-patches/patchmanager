TARGET = patchmanager
PLUGIN_IMPORT_PATH = org/SfietKonstantin/patchmanager

TEMPLATE = lib
QT = core qml network dbus gui
CONFIG += qt plugin hide_symbols

HEADERS += \
    patchmanager.h \
    webcatalog.h \
    webpatchesmodel.h \
    webdownloader.h \
    patchmanagermodel.h \
    PatchObject.hpp

SOURCES += \
    plugin.cpp \
    patchmanager.cpp \
    webpatchesmodel.cpp \
    webdownloader.cpp \
    patchmanagermodel.cpp \
    PatchObject.cpp

DISTFILES += qmldir \
    PatchManagerPage.qml \
    AboutPage.qml \
    DevelopersPage.qml \
    RestartServicesDialog.qml \
    LegacyPatchPage.qml \
    WebCatalogPage.qml \
    WebPatchPage.qml \
    ItemErrorComponent.qml \
    NewPatchPage.qml \
    ScreenshotsPage.qml \
    SettingsPage.qml

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH

qmlfiles.files += $${DISTFILES}
qmlfiles.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH

INSTALLS += target qmlfiles

pm_dbus_interface.files = ../bin/patchmanager-daemon/dbus/org.SfietKonstantin.patchmanager.xml
pm_dbus_interface.source_flags = -c PatchManagerInterface
pm_dbus_interface.header_flags = -c PatchManagerInterface -i ../bin/patchmanager-daemon/patchmanager_include.h
DBUS_INTERFACES += pm_dbus_interface
