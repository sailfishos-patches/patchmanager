TEMPLATE = aux

qmlpages.path = /usr/share/jolla-settings/pages/patchmanager
qmlpages.files = *.qml *.js

plugin_entry.path = /usr/share/jolla-settings/entries
plugin_entry.files = patchmanager.json

icon.path = /usr/share/patchmanager/icons/
icon.files = icon-m-patchmanager.png

INSTALLS += plugin_entry qmlpages icon
