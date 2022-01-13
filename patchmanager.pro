TEMPLATE = subdirs
SUBDIRS = src test

OTHER_FILES += rpm/patchmanager.spec

TRANSLATIONS += $$files(translations/settings-$${TARGET}-*.ts)

TS_FILE = $${_PRO_FILE_PWD_}/translations/settings-$${TARGET}.ts
HAVE_TRANSLATIONS = 0

# Translation source directories
TRANSLATION_SOURCE_CANDIDATES = $${_PRO_FILE_PWD_}
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
qm.commands += lupdate -noobsolete $${TRANSLATION_SOURCES} -ts $${TS_FILE} && \
    mkdir -p translations && \
    [ \"$${OUT_PWD}\" != \"$${_PRO_FILE_PWD_}\" -a $$HAVE_TRANSLATIONS -eq 1 ] && \
    cp -af $${TRANSLATIONS_IN} $${OUT_PWD}/translations || :

# create the qm files
qm.commands += ; [ $$HAVE_TRANSLATIONS -eq 1 ] && lrelease -nounfinished $${TRANSLATIONS_OUT} || :

# special case: as TS_FILE serves as both source file as well as
# the English translation source, create the en qm file from it:
qm.files += $$replace(TS_FILE, \.ts, -en.qm)
qm.commands += ; [ $$HAVE_TRANSLATIONS -eq 1 ] && lrelease -nounfinished $$TS_FILE -qm $$replace(TS_FILE, \.ts, -en.qm) || :

INSTALLS += qm

OTHER_FILES += $$TRANSLATIONS
