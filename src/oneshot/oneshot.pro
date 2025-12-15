TEMPLATE = aux

# oneshot.d is Sailish OS specific. See https://github.com/sailfishos/oneshot
# While the path could be configurable though the %{_oneshotdir} RPM macro,
# this location seems stable enough.
oneshot.path = /usr/lib/oneshot.d/
oneshot.files = patchmanager-setup-preload.sh

INSTALLS += oneshot
