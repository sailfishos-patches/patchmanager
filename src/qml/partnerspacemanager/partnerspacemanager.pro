TARGET = partnerspacemanager
PLUGIN_IMPORT_PATH = org/SfietKonstantin/partnerspacemanager

TEMPLATE = lib
QT = core qml
CONFIG += qt plugin hide_symbols
CONFIG += link_pkgconfig
PKGCONFIG += mlite5

HEADERS += \
    partnerspaceinformation.h \
    partnerspacemodel.h

SOURCES += \
    plugin.cpp \
    partnerspaceinformation.cpp \
    partnerspacemodel.cpp

OTHER_FILES += qmldir \
    PartnerSpaceManagerPage.qml \
#    AboutPage.qml \
#    DevelopersPage.qml \
    PartnerSpacePage.qml

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH

qmlfiles.files += $${OTHER_FILES}
qmlfiles.path +=  $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += target qmlfiles

