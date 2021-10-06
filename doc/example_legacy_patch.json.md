## Example patch.json in legacy format

This describes the format of the `patch.json` file in the 'legacy' format.
While this is still supported in Patchmanager 3, if possible switch to [the new format](./example_patch.json.md) as it supports more useful information.

    {
        "name": "Example Patch Name"
        "description": "This is a longish description of what the patch does.\nIt will be displayed in the patch page.",
        "category": "email",
        "infos": {
            "maintainer": "username"
        },
    }

### Description of options:

 - name: The "display name" of the patch, as it will show in the patch list
 - description: explains a bit more what the patch actually does
 - category: this is used on the one hand to sort the list of patches, on the other hand certain categories will cause patchmanager to restart affected system services or applications. the category "homescreen" for example will prompt the user to restart Lipstick after applying.  
 - maintainer: name of the principal patch author or maintainer. Either use your real name, as displayed on Github or Twitter, or use your usual nickname.

### List of supported categories:

 - "homescreen" - will restart Lipstick
 - "silica" - will restart Lipstick
 - "keyboard" - will restart the keyboard service
 - "browser"
 - "calendar"
 - "camera"
 - "clock"
 - "contacts"
 - "email"
 - "gallery"
 - "media"
 - "messages"
 - "phone"
 - "settings"
