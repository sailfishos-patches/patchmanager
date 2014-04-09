TEMPLATE = app
TARGET = patchmanager-ui
QT += quick qml

CONFIG += link_pkgconfig
PKGCONFIG += sailfishapp
INCLUDEPATH += /usr/include/sailfishapp

SOURCES += main.cpp

OTHER_FILES += qml/main.qml \
    qml/pages/MainPage.qml \
    qml/pages/LipstickPandoraPage.qml \
    patchmanager.desktop \
    patchmanager.png


target.path = /usr/bin

qml.files = qml
qml.path = /usr/share/patchmanager

desktop.files = patchmanager.desktop
desktop.path = /usr/share/applications

icon.files = patchmanager.png
icon.path = /usr/share/icons/hicolor/86x86/apps

INSTALLS += target qml desktop icon
