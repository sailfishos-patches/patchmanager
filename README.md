# Patchmanager

Patchmanager is a tool for transparently modifying installed files by the patch utility and for managing the patches for doing so.
Since version 3.0 it does not modify original files, but alters their content when they are loaded into RAM to be run.

## Information for users

Note that Patchmanager does not install an application icon on the launcher, but creates a new entry in Sailfish OS' Settings app.

To install Patchmanager you should install Storeman (https://openrepos.net/content/osetr/storeman-installer) to install Patchmanager 3.x from OpenRepos using Storeman.  Alternatively you may download the recent Patchmanager RPM from OpenRepos and install it manually.

Modern patches for Patchmanager are provided via [the Web Catalog](https://coderus.openrepos.net/pm2/projects/), while older patches were provided as [RPMs at Openrepos](https://openrepos.net/category/patches) and some of them are still maintained.
Mind that many patches were first released at Openrepos but later migrated to the Web Catalog: For these you will find outdated RPMs, hence always search in the Web Catalog first.

## Information for developers

To create a patch for Patchmanager, you must at least provide a patch file.
If you package your patch as an RPM or archive file, you must also provide the JSON metadata: see below.
If you use the Web Catalog to distribute your patch, you must not provide a JSON file, but input the corresponding data when submitting the patch to the Web Catalog.

### The patch file

The patch file must be a diff of all the files to be patched in the filesystem. 
It will be applied on the root of the filesystem, with the `-p1` flag. 
It must be named `unified_diff.patch`.

Usually, you can generate such patch file using the following command, with the directories `original` and `patched` containing the original and modified files:
`diff -ur original/ patched/ > unified_diff.patch`

### The JSON metadata file

The metadata file contains information about a patch.  It is a simple JSON file, that must be named `patch.json`.
This file contains the title of the patch, a short description of the patch, a category, and additional information.  For the documentation of this JSON file format see:
 - for the [modern format](./doc/example_patch.json.md)
 - for the much simpler [legacy format](./doc/example_legacy_patch.json.md)

Either format is supported, but the modern one provides more useful features and is recommended.

#### Additional files

Patchmanager starting from version 2.0 can utilize additional files to provide an enhanced user experience. 
All these files must be placed in the same folder.

- **main.qml** - A QML page with some additional information about a patch and / or its configuration.  It will be shown when user taps on the patch entry inside Patchmanager.
- **main.png** or **main.svg** - An icon for the patch, which will be displayed on the list of patches, on the right side after the patch name.
- **translation_\<LANG_CODE\>.qm** - Translation files for the QML page, to enable patch developers to translate texts to various languages.
- Any **.qml**, .**js**, **.png** files used by the QML page may be added.

## Patch distribution using Patchmanager's Web Catalog

Patchmanager supports installing patches from its Web Catalog. 
Patch developers can upload patches to https://coderus.openrepos.net/pm2 to enlist them for the Web Catalog. 
When uploading a patch, the patch developer must not provide a `patch.json` file with metadata, but instead must fill the necessary fields on the webpage before uploading: The Web Catalog will encode this metadata automatically and add it to the downloadable file.

For more information about the requirements for using the Web Catalog, see https://coderus.openrepos.net/pm2/usage/

## Patch distribution at OpenRepos.net and other repositories

Patches can be too complex for Patchmanager's Web Catalog.  In this case the developer may package the patch as an RPM and upload it at https://openrepos.net or another user-accessible package repository.
OpenRepos provides a [category "Patches"](https://openrepos.net/category/patches) and a [tag "Patch"](https://openrepos.net/tags/patch) for this, which both should be used.

For RPM patches, the patch content should be installed in a directory `/usr/share/patchmanager/patches/<patch-name>/`; its [additional files](#additional-files) for this patch should also be placed there.

Additionaly, when a patch developer decides to package a patch as an RPM, the compatibility of a patch (to Sailfish OS release versions, the software infrastructure it uses etc.) should be properly defined by RPM spec file dependencies ("Requires:", "Conflicts:" etc.).

For an RPM patch example, see https://github.com/CODeRUS/sailfishos-disable-glass-avatar
