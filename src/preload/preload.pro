TEMPLATE = lib
QT =
CONFIG += plugin
CONFIG += link_pkgconfig
CONFIG += use_c_linker
PKGCONFIG += libshadowutils
INCLUDEPATH += /usr/include/libshadowutils
QMAKE_CFLAGS += -std=c11 -Werror

LIBS = -ldl

SOURCES += \
    src/preloadpatchmanager.c

TARGET = preloadpatchmanager
target.path = $$LIBDIR

INSTALLS = target
