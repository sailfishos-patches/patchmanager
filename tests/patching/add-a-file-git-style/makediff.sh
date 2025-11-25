#!/bin/sh

mkdir repo
pushd repo >/dev/null
git init
mkdir -p usr/lib/qt5/qml/org/SfietKonstantin/patchmanagertests
printf 'this file already exists\n' > usr/lib/qt5/qml/org/SfietKonstantin/patchmanagertests/pre-existing-file
git add --all >/dev/null
git commit --author "Test User <test@example.org>" --all -m "initial commit"  >/dev/null
git commit --allow-empty --author "Test User <test@example.org>" --all -m "empty commit"  >/dev/null
printf 'this file is new\n' > usr/lib/qt5/qml/org/SfietKonstantin/patchmanagertests/post-existing-file
git add --all >/dev/null
git commit --author "Test User <test@example.org>" --all -m "second commit"  >/dev/null
git diff HEAD~1 | tee ../patch/unified_diff.patch
popd >/dev/null

rm -rf repo
