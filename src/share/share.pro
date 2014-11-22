TEMPLATE = aux

OTHER_FILES = patchmanager.png \
    patchmanager-big.png \
    morpog.jpeg \
    sfiet_konstantin.jpg \
    webosinternals.png

data.files = $${OTHER_FILES}
data.path = /usr/share/patchmanager/data

INSTALLS += data
