TEMPLATE = aux

qmlpages.path = /usr/share/jolla-settings/pages/patchmanager
qmlpages.files = patchmanager.qml

plugin_entry.path = /usr/share/jolla-settings/entries
plugin_entry.files = patchmanager.json

INSTALLS += plugin_entry qmlpages
