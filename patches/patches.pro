TEMPLATE = aux

BETTER_VIDEO = gallery-better-video/patch.json \
    gallery-better-video/unified_diff.patch

VOICECALL_VIBRATE = voicecall-vibrate/patch.json \
    voicecall-vibrate/unified_diff.patch

OTHER_FILES = $${BETTER_VIDEO} \
    $${VOICECALL_VIBRATE}

betterVideo.files = $${BETTER_VIDEO}
betterVideo.path = /usr/share/patchmanager/patches/gallery-better-video

voicecallVibrate.files = $${VOICECALL_VIBRATE}
voicecallVibrate.path = /usr/share/patchmanager/patches/voicecall-vibrate

INSTALLS += betterVideo voicecallVibrate
