# Patchmanager

Patchmanager is a tool that can be used to modify the Sailfish OS user experience.
It is based on AUSMT (Auto-Update System Modification Technology), a set of scripts that enables system file patching.

Patchmanager does not have application icon on the launcher.  After installation Patchmanager can be found inside Settings.

## For users

To install Patchmanager you first need to install Storeman (https://openrepos.net/content/osetr/storeman-installer), then install Patchmanager 3 from OpenRepos using Storeman. 
Alternatively you may download the recent Patchmanager RPM from OpenRepos (https://openrepos.net/content/coderus/patchmanager-30) and install it manually.

RPM patches can be installed from OpenRepos per Storeman or manually downloaded from https://openrepos.net/category/patches

Patches available via Web Catalog are listed and detailed here: https://coderus.openrepos.net/pm2/projects/

## For developers

To write a patch, you need to provide at least a patch file.
If you package your patch as an RPM or archive file, must also provide the JSON metadata: see below.

If you use the Web Catalog to distribute your patch, you need not and should not provide a JSON file.

### The patch file

The patch file must be a diff of all the files to be patched in the filesystem. 
It will be applied on the root of the filesystem, with the `-p1` flag. 
It **must** be named `unified_diff.patch`.

Usually, you can generate one of these patch files using the following command

```bash
diff -ur original/ patched/ > unified_diff.patch
```

where `original` and `patched` contains the original and modified files.

### The JSON metadata file

The metadata file contains information about the patch.  It is a simple JSON file, that **must** be named `patch.json`.

This file contains the title of the patch, a short description of the patch, a category, and other information. 
For documentation of the json file format see:

 - example [patch.json](./doc/example_patch.json.md)
 - the much simpler [legacy format](./doc/example_legacy_patch.json.md)

Either format is supported, but the modern one provides more useful features and is recommended.

#### Additional files

Patchmanager starting from version 2.0 can utilize additional files to provide better user experience. 
All files shall be placed in the same folder.

- **main.qml** - QML page with some additional info about patch and/or configuration.  Will be shown when user taps on the patch entry inside Patchmanager.
- **main.png** or **main.svg** - Icon for the patch, which will be displayed at the list of patches, at right corner after patch name.
- **translation_\<LANG_CODE\>.qm** - Translation files for QML page, to allow patch developers to translate texts to various languages.
- Any additional **.qml**, .**js**, **.png** files used by QML page are allowed.

## Patch distribution using Patchmanager's Web Catalog

Patchmanager supports installing patches from its Web Catalog. 
Patch developers can upload patches to https://coderus.openrepos.net/pm2 to enlist them on the Catalog. 
When uploading a patch, the patch developer should not provide a `patch.json` file with metadata, but shall fill the necessary fields on the webpage before uploading, instead.

Web Catalog will encode the metadata automatically in the downloadable file.

For more information about requirements for using Web Catalog, see https://coderus.openrepos.net/pm2/usage/

## Patch distribution at OpenRepos.net and other repositories

Patches can be too complex for Patchmanager's Web Catalog. 
In this case the developer can package the patch as an RPM and upload it at https://openrepos.net or another user-accessible package repository. OpenRepos provides a "Applications/Patches" category for this.

For RPM patches, the patch content shall be installed in a directory `/usr/share/patchmanager/patches/<patch-name>/`; [additional files (see above)](#additional-files) for this patch shall also be placed there.

Additionaly, when a patch developer decides to package a patch as an RPM, the compatibility of a patch (to Sailfish OS release versions, infrastructure it uses etc.) shall be properly defined by RPM spec file dependencies ("Requires:", "Conflicts:" etc.).

Example RPM patch project: https://github.com/CODeRUS/sailfishos-disable-glass-avatar
