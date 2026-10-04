#!/bin/sh

# Do not interfere with ourselves
export NO_PM_PRELOAD=1

set +e
if [ "$MIC_RUN" != "" ]; then
  echo "patchmanager-setup-preload - returning FAIL to postpone oneshot to first boot"
  exit 1
fi

# While updating, return failure so we get run at next boot
# See https://github.com/sailfishos-patches/patchmanager/issues/39#issuecomment-940607912
if [ -e /tmp/os-update-running ]
then
  echo "patchmanager-setup-preload - returning FAIL to postpone oneshot to next boot"
  exit 1
fi

qual=$(getconf LONG_BIT)
libdir=/usr/lib
if [ x"${qual}" = x"64" ]; then
  libdir=/usr/lib64
fi

if ! grep -qsF 'libpreloadpatchmanager.so' /etc/ld.so.preload
then
  echo "${libdir}/libpreloadpatchmanager.so" >> /etc/ld.so.preload
  if [ $? -ne 0 ]; then
    echo "patchmanager-setup-preload - setting up preload FAILed. Trying again next boot"
    exit 1
  fi
fi

/sbin/ldconfig

if ! grep -qsF 'include whitelist-common-patchmanager.local' /etc/firejail/whitelist-common.local
then
  echo 'include whitelist-common-patchmanager.local' >> /etc/firejail/whitelist-common.local
  if [ $? -ne 0 ]; then
    echo "patchmanager-setup-preload - setting up whitelist FAILed. Trying again next boot"
    exit 1
  fi
fi

echo "patchmanager-setup-preload - system setup SUCCESSFUL."

exit 0
