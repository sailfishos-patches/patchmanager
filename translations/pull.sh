#!/bin/bash

for lang in {bg,bn_IN,cs,da,de,el,en_GB,en_US,es,es_ES,et,fi,fr,gu,hi,hu,it,ja,kn,ko,ml,mr,nb,nl,pa,pl,pt,pt_BR,ru,sl,sv,ta,te,tr,tt,zh_CN,zh_HK,zh_TW}
do
    echo "Checking $lang..."
    COMPLETED=$(curl -s --user "api:${txapikey}" -X GET "https://www.transifex.com/api/2/project/patchmanager3/resource/settings-patchmanagerts/stats/${lang}" | grep "completed" | sed "s/.*\"\([0-9]\+\)%\".*/\1/")
    if [ "$COMPLETED" = "100" ]
    then
        echo "Downloading $lang..."
        curl -s --user "api:${txapikey}" -o "settings-patchmanager-${lang}.ts" -X GET "https://www.transifex.com/api/2/project/patchmanager3/resource/settings-patchmanagerts/translation/${lang}/?file"
    fi
done
