#!/bin/sh

# Do not interfere with ourselves
export NO_PM_PRELOAD=1

set +e
if [ "$MIC_RUN" != "" ]; then
  echo "patchmanager-setup-preload - returning FAIL to postpone oneshot to first boot"
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
fi

/sbin/ldconfig
if ! grep -qsF 'include whitelist-common-patchmanager.local' /etc/firejail/whitelist-common.local
then
  echo 'include whitelist-common-patchmanager.local' >> /etc/firejail/whitelist-common.local
fi

