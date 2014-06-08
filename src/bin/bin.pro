TEMPLATE = app
TARGET = patchmanager-ui
QT += quick qml

CONFIG += link_pkgconfig
PKGCONFIG += sailfishapp
INCLUDEPATH += /usr/include/sailfishapp

SOURCES += main.cpp

OTHER_FILES += qml/main.qml \
    qml/pages/MainPage.qml \
    qml/pages/PatchPage.qml \
    qml/pages/LipstickWarningDialog.qml \
    qml/pages/AboutPage.qml \
    qml/pages/DevelopersPage.qml \
    qml/cover/MainCover.qml \
    data/patchmanager.png \
    data/patchmanager-big.png \
    data/sfiet_konstantin.png \
    data/morpog.png \
    data/webosinternals.png \
    patchmanager.desktop \
    patchmanager.png


target.path = /usr/bin

qml.files = qml
qml.path = /usr/share/patchmanager

data.files = data
data.path = /usr/share/patchmanager

desktop.files = patchmanager.desktop
desktop.path = /usr/share/applications

icon.files = patchmanager.png
icon.path = /usr/share/icons/hicolor/86x86/apps

INSTALLS += target qml data desktop icon
