TEMPLATE = aux

manglelist.files = manglelist.conf
manglelist.path = /etc/patchmanager
firejail.files = whitelist-common-patchmanager.local
firejail.path = /etc/firejail
INSTALLS += firejail manglelist

