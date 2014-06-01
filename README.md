# Patchmanager

Patchmanager is a tool that can be used to modify the Sailfish OS user experience.
It is based on AUSMT, a set of scripts that emables system file patching.

## For developers

To write a patch, you need to provide two files. A patch file, and the JSON metadata.
Both files should be installed inside `/usr/share/patchmanager/patches/<patch-subfolder>`.

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
