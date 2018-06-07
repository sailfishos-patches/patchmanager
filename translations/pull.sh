#!/bin/bash

for lang in {ca,zh_CN,nl_BE,en_FI,fi,fi_FI,fr_FR,de,de_AT,de_DE,hu,it,ja,pl,pt_BR,ru,sl,sl_SI,es,sv}
do
    echo "Checking $lang..."
    COMPLETED=$(curl -s --user "api:${txapikey}" -X GET "https://www.transifex.com/api/2/project/patchmanager3/resource/settings-patchmanagerts/stats/${lang}" | grep "completed" | sed "s/.*\"\([0-9]\+\)%\".*/\1/")
    if [ "$COMPLETED" = "100" ]
    then
        echo "Downloading $lang..."
        curl -s --user "api:${txapikey}" -o "settings-patchmanager-${lang}.ts" -X GET "https://www.transifex.com/api/2/project/patchmanager3/resource/settings-patchmanagerts/translation/${lang}/?file"
    fi
done
