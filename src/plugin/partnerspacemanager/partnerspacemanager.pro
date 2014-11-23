TEMPLATE = aux

qmlpages.path = /usr/share/jolla-settings/pages/partnerspacemanager
qmlpages.files = *.qml *.js

plugin_entry.path = /usr/share/jolla-settings/entries
plugin_entry.files = partnerspacemanager.json

INSTALLS += plugin_entry qmlpages
