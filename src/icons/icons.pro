TEMPLATE = aux

# Configures svg to png
THEMENAME=sailfish-default

CONFIG += sailfish-svg2png

# also install SVG:
svg.path = /usr/share/icons/hicolor/scalable/apps
svg.files = icon-m-patchmanager2.svg
INSTALLS += svg
