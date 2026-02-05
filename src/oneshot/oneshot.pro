TEMPLATE = aux

# oneshot.d is SailishOS specific, see https://github.com/sailfishos/oneshot
# While its path is configurable via the %{_oneshotdir} RPM macro,
# this location seems stable.
oneshot.path = /usr/lib/oneshot.d/
oneshot.files = patchmanager-setup-preload.sh

INSTALLS += oneshot
