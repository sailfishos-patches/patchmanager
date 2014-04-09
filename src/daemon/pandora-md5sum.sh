#!/bin/sh
rm -f /opt/lipstick-pandora/md5sums > /dev/null 2>&1
for f in $(find /opt/lipstick-pandora/qml -name '*.qml') ; do 
  echo $(sha256sum $f) >> /opt/lipstick-pandora/md5sums
done
