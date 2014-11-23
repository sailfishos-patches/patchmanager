TARGET = patchmanager
PLUGIN_IMPORT_PATH = org/SfietKonstantin/patchmanager

TEMPLATE = lib
QT = core qml
CONFIG += qt plugin hide_symbols

HEADERS += \
    patchmanager.h

SOURCES += \
    plugin.cpp \
    patchmanager.cpp
 
OTHER_FILES += qmldir \
    PatchManagerPage.qml \
    AboutPage.qml \
    DevelopersPage.qml \
    LipstickWarningDialog.qml \
    PatchPage.qml \
    RestartServicesDialog.qml

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH

qmlfiles.files += $${OTHER_FILES}
qmlfiles.path +=  $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += target qmlfiles
