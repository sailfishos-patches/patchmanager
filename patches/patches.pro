TEMPLATE = aux

OTHER_FILES = jolla-gallery-extensions/patch.json \
    jolla-gallery-extensions/patchimageview.patch \
    jolla-gallery-extensions/unpatchimageview.patch \
    jolla-gallery-extensions/patchvideoposter.patch \
    jolla-gallery-extensions/unpatchvideoposter.patch

galleryExtensions.files = $${OTHER_FILES}
galleryExtensions.path = /usr/share/patchmanager/patches/jolla-gallery-extensions

INSTALLS += galleryExtensions
