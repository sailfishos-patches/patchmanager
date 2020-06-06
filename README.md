# Patchmanager

Patchmanager is a tool that can be used to modify the Sailfish OS user experience.
It is based on AUSMT (Auto-Update System Modification Technology), a set of scripts that enables system file patching.

Patchmanager does not have application icon inside launcher. After installation Patchmanager can be found inside Settings.

## For users

To install Patchmanager you first need to install Storeman (https://openrepos.net/content/osetr/storeman), then install Patchmanager 3 from OpenRepos using Storeman.

RPM patches can be installed from OpenRepos per Storeman or manually downloaded from https://openrepos.net/category/patches

Patches available via Web Catalog can be found here: https://coderus.openrepos.net/pm2/projects/

## For developers

To write a patch, you need to provide at least two files. A patch file, and the JSON metadata.

### The patch file

The patch file must be a diff of all the files to be patched in the filesystem. It will
be applied on the root of the filesystem, with the `-p1` flag. It **must** be named 
`unified_diff.patch`.

Usually, you can generate one of these patch files using the following command

```bash
diff -ur original/ patched/ > unified_diff.patch
```

where `original` and `patched` contains the original and modified files.

### The JSON metadata file

The metadata file contains information about the patch. It is a simple JSON file, that **must**
be named `patch.json`.

This file contains the title of the patch, a quick description of the patch, a category,
and other informations. Here is a sample of a metadata file.

```json
{
    "name": "My super patch",
    "description": "Some description.",
    "category": "other",
    "infos": {
        "maintainer": "Foo Bar"
    }
}
```

#### Categories

The category entry must be in the following list
- browser
- camera
- calendar
- clock
- contacts
- email
- gallery
- homescreen
- media
- messages
- phone
- settings
- silica
- others

Some of these entries allow specific actions to be triggered, such as relaunching the homescreen
or prestarted services.

#### Maintainers

A maintainer can be registered inside the JSON metadata file to claim maintainership of the patch. 
Either use your real name, as displayed on Github or Twitter, or use your usual nickname.

#### Additional files

Patchmanager starting from version 2.0 can utilize additional files to provide better users experience. All files should be placed in the same folder

- **main.qml** - QML page with some additional info about patch and/or configuration. Will be showed when user tap on patch entry inside Patchmanager
- **main.png** or **main.svg** - icon for patch, will be displayed inside list of patches, at right corner after patch name
- **translation_\<LANG_CODE\>.qm** - translation files for QML page, allow patch developers to translate texts to various languages
- any additional **.qml**, .**js**, **.png** files used by QML page is allowed

## Web catalog for patches

Patchmanager is supporting installing patches from web catalog. Patch developers can upload patches to https://coderus.openrepos.net/pm2 to make them visible inside Patchmanager Web Catalog list. When uploading patch to web catalog, patch developer should not provide patch.json file with metadata, but he should fill necessary fields on webpage before uploading instead.

## Patches distribution at OpenRepos.net

Some patches can be too complex for Web Catalog. In this case developer can package patch to RPM file and upload to https://openrepos.net

In this case patch content should be installed to `/usr/share/patchmanager/patches/<patch-name>/` and patch developer should follow same rules for including additional files.

Additionaly, when patch developer decide to use RPM packages, he should manually maintain compatibility of patch by using RPM spec file fields.

Sample RPM patch project: https://github.com/CODeRUS/sailfishos-disable-glass-avatar
