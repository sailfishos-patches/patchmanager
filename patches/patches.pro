TEMPLATE = aux

OTHER_FILES = gallery-better-video/patch.json \
    gallery-better-video/unified_diff.patch

patches.files = $${OTHER_FILES}
patches.path = /usr/share/patchmanager/patches/gallery-better-video

INSTALLS += patches
