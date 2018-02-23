TARGET = patchmanager
PLUGIN_IMPORT_PATH = org/SfietKonstantin/patchmanager

TEMPLATE = lib
QT = core qml network dbus gui
CONFIG += qt plugin hide_symbols

HEADERS += \
    patchmanager.h \
    webcatalog.h \
    webpatchesmodel.h \
    webpatchdata.h \
    webdownloader.h \
    patchmanagermodel.h \
    PatchObject.hpp

SOURCES += \
    plugin.cpp \
    patchmanager.cpp \
    webpatchesmodel.cpp \
    webpatchdata.cpp \
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
    ScreenshotsPage.qml

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH

qmlfiles.files += $${DISTFILES}
qmlfiles.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH

INSTALLS += target qmlfiles

TRANSLATIONS += \
    translations/settings-$${TARGET}-ru.ts

TS_FILE = $${_PRO_FILE_PWD_}/translations/settings-$${TARGET}.ts
HAVE_TRANSLATIONS = 0

# Translation source directories
TRANSLATION_SOURCE_CANDIDATES = $${_PRO_FILE_PWD_} $${_PRO_FILE_PWD_}/src $${_PRO_FILE_PWD_}/qml
for(dir, TRANSLATION_SOURCE_CANDIDATES) {
    exists($$dir) {
        TRANSLATION_SOURCES += $$dir
    }
}

# prefix all TRANSLATIONS with the src dir
# the qm files are generated from the ts files copied to out dir
for(t, TRANSLATIONS) {
    TRANSLATIONS_IN  += $${_PRO_FILE_PWD_}/$$t
    TRANSLATIONS_OUT += $${OUT_PWD}/$$t
    HAVE_TRANSLATIONS = 1
}

qm.files = $$replace(TRANSLATIONS_OUT, \.ts, .qm)
qm.path = /usr/share/translations
qm.CONFIG += no_check_exist

# update the ts files in the src dir and then copy them to the out dir
qm.commands += lupdate -noobsolete $${TRANSLATION_SOURCES} -ts $${TS_FILE} $$TRANSLATIONS_IN && \
    mkdir -p translations && \
    [ \"$${OUT_PWD}\" != \"$${_PRO_FILE_PWD_}\" -a $$HAVE_TRANSLATIONS -eq 1 ] && \
    cp -af $${TRANSLATIONS_IN} $${OUT_PWD}/translations || :

# create the qm files
qm.commands += ; [ $$HAVE_TRANSLATIONS -eq 1 ] && lrelease -nounfinished $${TRANSLATIONS_OUT} || :

INSTALLS += qm

pm_dbus_interface.files = ../bin/patchmanager-daemon/dbus/org.SfietKonstantin.patchmanager.xml
pm_dbus_interface.source_flags = -c PatchManagerInterface
pm_dbus_interface.header_flags = -c PatchManagerInterface -i ../bin/patchmanager-daemon/patchmanager_include.h
DBUS_INTERFACES += pm_dbus_interface
