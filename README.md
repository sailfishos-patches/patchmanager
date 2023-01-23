# Patchmanager for SailfishOS

Patchmanager is a tool for transparently modifying installed files by the patch utility and for managing the special patch files ("Patches") for doing so.
Since version 3.0, Patchmanager does not modify original files on mass storage, it merely alters their content when they are loaded into RAM to be executed.

## Information for users

Note that Patchmanager does not install an application icon on the launcher, but creates a new entry in SailfishOS' Settings app.

To install Patchmanager you should use Storeman, which can be installed via the [Storeman-Installer](https://openrepos.net/content/osetr/storeman-installer), but you also may download the [recent Patchmanager RPM from OpenRepos](https://openrepos.net/content/patchmanager/patchmanager) and install it manually.
The modern alternative to using OpenRepos is the SailfishOS:Chum community repository, which can be easily accessed with the SailfishOS:Chum GUI application or with the ususal command line tools (`pkcon`, `zypper`), after downloading either the SailfishOS:Chum GUI application RPM or the SailfishOS:Chum repo helper RPM from [chumrpm.netlify.app](https://chumrpm.netlify.app/) and installing it.  Both RPMs are also offered at OpenRepos (but the GUI app only for the recent SailfishOS release) and hence are installable by Storeman.

Modern Patches for Patchmanager are provided via [the Web Catalog](https://coderus.openrepos.net/pm2/projects/), while older Patches were provided as [RPMs at OpenRepos](https://openrepos.net/category/patches) and some of them are still maintained.
Mind that many Patches were first released at OpenRepos but later migrated to the Web Catalog: For these you will find outdated RPMs, hence always search in the Web Catalog first.

If you want to translate ("localise") Patchmanager to a language you know well or enhance an existing translation, please use [Patchmanager's Transifex page](https://www.transifex.com/coderus/patchmanager3/).

## Information for Patch developers

To create a Patch for Patchmanager, you must at least provide a patch file.
If you package your Patch as an RPM or archive file, you must also provide the JSON metadata: see below.
If you use the Web Catalog to distribute your Patch, you must not provide a JSON file, but input the corresponding data when submitting the patch file to the Web Catalog.

### patch files

A patch file must be a diff of all the files to be patched in the filesystem. 
It will be applied on the root of the filesystem, with the `-p1` flag. 
It must be named `unified_diff.patch`.

Usually, you can generate such patch file using the following command, with the directories `original` and `patched` containing the original and modified files:
`diff -ur original/ patched/ > unified_diff.patch`

### The JSON metadata file

The metadata file contains information about a Patch.  It is a simple JSON file, that must be named `patch.json`.
This file contains the title of the Patch, a short description of the Patch, a category, and additional information.  For the documentation of this JSON file format see:
 - for the [modern format](./doc/example_patch.json.md)
 - for the much simpler [legacy format](./doc/example_legacy_patch.json.md)

Either format is supported, but the modern one provides more useful features and is recommended.

### Additional files

Patchmanager starting from version 2.0 can utilise additional files to provide an enhanced user experience. 
All these files must be placed in the same folder.

- **main.qml** - A QML page with some additional information about a patch and / or its configuration.  It will be shown when user taps on the patch entry inside Patchmanager.
- **main.png** or **main.svg** - An icon for the patch, which will be displayed on the list of patches, on the right side after the patch name.
- **translation_\<LANG_CODE\>.qm** - Translation files for the QML page, to enable patch developers to [translate texts to various languages](https://github.com/sailfishos-patches/patchmanager/wiki/Translations-(%22i18n%22,-%22l10n%22)).
- Any **.qml**, .**js**, **.png** files used by the QML page may be added.

### Patch distribution using Patchmanager's Web Catalog

Patchmanager supports installing Patches from its Web Catalog. 
Patch developers can upload Patches as archive files (`.zip`, `.tar.gz`, `.tar.bz2` or `.tar.xz`) at https://coderus.openrepos.net/pm2 to enlist them for the Web Catalog. 
When uploading a Patch there, the Patch developer should not provide a `patch.json` file with metadata and must fill the necessary fields on the web-page before uploading: The Web Catalog will encode this metadata automatically and add it to the downloadable file.
Side note: If a JSON file is provided in the upload at the Web Catalog, it does not replace or mangle it; pay attention not to provide inconsistent data this way!

For more information about the requirements for uploading Patches to the Web Catalog, see https://coderus.openrepos.net/pm2/usage/

### Patch distribution at OpenRepos.net and other RPM repositories

Patches can be too complex for Patchmanager's Web Catalog.  In this case the developer may package the Patch as an RPM and upload it at https://openrepos.net or another user-accessible package repository.
OpenRepos provides a [category "Patches"](https://openrepos.net/category/patches) and a [tag "Patch"](https://openrepos.net/tags/patch) for this, which both should be used.

For RPM Patches, the patch file (`unified_diff.patch`) shall be installed in a directory `/usr/share/patchmanager/patches/<patch-name>/`; its [additional files](#additional-files) for this patch shall also be placed there.

Additionaly, if a Patch developer decides to package a Patch as an RPM, the compatibility of a Patch (to SailfishOS release versions, the software infrastructure it uses etc.) shall be properly defined by RPM spec file dependencies ("Requires:", "Conflicts:" etc.).

For an RPM Patch example, see https://github.com/CODeRUS/sailfishos-disable-glass-avatar

### Developing, testing, debugging and translating Patches

Additional, in depth information for developing, testing, debugging and translating Patches is available in [the Wiki](https://github.com/sailfishos-patches/patchmanager/wiki).  Exemplary Patches to depict the aforementioned formats are provided [in the `doc` directory](https://github.com/sailfishos-patches/patchmanager/tree/master/doc).

Sepcifically the tool [sailfish-patch by ichthyosaurus](https://github.com/ichthyosaurus/sailfish-patch#readme) might be helpful to support implementing, debugging and testing Patches.

## Information for Patchmanager developers

Some in depth information for developing, testing, debugging and translating Patchmanager is available in [the Wiki](https://github.com/sailfishos-patches/patchmanager/wiki).

The [Patchmanager Testcases](https://github.com/sailfishos-patches/patchmanager/tree/master/tests) (for details see its [README](https://github.com/sailfishos-patches/patchmanager/blob/master/tests/README_patching.md), RPMs are available at [OpenRepos](https://openrepos.net/content/patchmanager/patchmanager-testcases) and SailfishOS:Chum) might be helpful to support implementing, debugging and testing Patchmanager.
