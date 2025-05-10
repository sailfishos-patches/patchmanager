# Patchmanager for SailfishOS

Patchmanager is a tool for transparently modifying installed files by the patch utility and for managing the special patch files ("Patches") for doing so.
Since version 3.0, Patchmanager does not modify original files on mass storage, it merely alters their content when they are loaded into RAM to be executed.

## Information for users

Note that Patchmanager does not install an application icon on the launcher, but creates a new entry in SailfishOS' Settings app.

### Installing Patchmanager

To install a recent Patchmanager the SailfishOS:Chum community repository can be used, which is easily accessed with the SailfishOS:Chum GUI application, which in turn can be installed via the [SailfishOS:Chum GUI Installer](https://openrepos.net/content/olf/sailfishoschum-gui-installer).  Alternatively Patchmanager can be installed from [its OpenRepos repository](https://openrepos.net/user/16848/programs) using Storeman, which can be installed via the [Storeman Installer](https://openrepos.net/content/olf/storeman-installer).  While these two sources can also be used to manually download and install Patchmanager, you may alternatively obtain Patchmanager RPMs from [its releases page at GitHub](https://github.com/sailfishos-patches/patchmanager/releases).  Note that the Patchmanager's OpenRepos repository and GitHub releases page also offer the final releases of Patchmanager 3.0 for SailfishOS < 3.4.0 and Patchmanager 2.3.3 for SailfishOS < 2.2.x.

### Installing Patches
Modern Patches for Patchmanager 3 are provided via [the Web Catalog](https://coderus.openrepos.net/pm2/projects/), while older Patches were provided as [RPMs at OpenRepos](https://openrepos.net/category/patches) and some of them are still maintained.
Mind that many Patches were first released at OpenRepos but later migrated to the Web Catalog: For these you will find outdated RPMs, hence always search in the Web Catalog first.

## Translating Patchmanager
If you want to translate ("localise") Patchmanager to a language you know well or enhance an existing translation, please read Patchmanager's corresponding Wiki page [Translations ("i18n", "l10n")](https://github.com/sailfishos-patches/patchmanager/wiki/Translations-(%22i18n%22,-%22l10n%22)).

## Information for Patch developers

Please see the [Patch Developer README](README-Patch-Developers.md) for information on how to create a Patch for Patchmanager.

## Information for Patchmanager developers

Please see the [Patchmanager Developer README](README-Developers.md) for information on developing Patchmanager itself.
