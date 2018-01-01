TARGET = patchmanager
PLUGIN_IMPORT_PATH = org/SfietKonstantin/patchmanager

TEMPLATE = lib
QT = core qml network dbus
CONFIG += qt plugin hide_symbols

HEADERS += \
    patchmanager.h \
    webcatalog.h \
    webpatchesmodel.h \
    webpatchdata.h \
    webdownloader.h \
    patchmanagermodel.h

SOURCES += \
    plugin.cpp \
    patchmanager.cpp \
    webpatchesmodel.cpp \
    webpatchdata.cpp \
    webdownloader.cpp \
    patchmanagermodel.cpp

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
    ScreenshotsPage.qml

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH

qmlfiles.files += $${DISTFILES}
qmlfiles.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH

translations.files += translations/*.qm
translations.path = /usr/share/translations

INSTALLS += target qmlfiles translations

pm_dbus_interface.files = ../bin/patchmanager-daemon/dbus/org.SfietKonstantin.patchmanager.xml
pm_dbus_interface.source_flags = -c PatchManagerInterface
pm_dbus_interface.header_flags = -c PatchManagerInterface -i ../bin/patchmanager-daemon/patchmanager_include.h
DBUS_INTERFACES += pm_dbus_interface
