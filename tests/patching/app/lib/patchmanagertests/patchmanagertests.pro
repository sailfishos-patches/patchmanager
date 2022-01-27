TARGET = patchmanagertests
PLUGIN_IMPORT_PATH = org/SfietKonstantin/patchmanagertests

TEMPLATE = lib
QT = core qml gui
CONFIG += qt plugin hide_symbols

DISTFILES += qmldir \
    TestCase1Item.qml TestCase2Item.qml

qmlfiles.files += $${DISTFILES}
qmlfiles.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH

INSTALLS += qmlfiles
