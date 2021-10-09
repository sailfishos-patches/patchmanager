## Example patch.json in legacy format

This describes the format of the `patch.json` file in the 'legacy' format.
While this is still supported in Patchmanager 3, if possible switch to [the new format](./example_patch.json.md) as it supports more useful information.

    {
        "name": "Example Patch Name",
        "description": "This is a long description of what the patch does.\nIt will be displayed on the patch page.",
        "category": "email",
        "infos": {
            "maintainer": "username"
        },
    }

### Description of options:

 - name: The "display name" of the patch, which will be shown on the patch list.
 - description: Explanation of what the patch does.
 - category: This is used to structure the list of patches in categories, but also certain categories will cause patchmanager to restart affected system services or applications.  For example, patches in the category "homescreen" will prompt the user to restart Lipstick after applying.
 - maintainer: The name of the principal patch author or maintainer.  Either use your real name, as displayed on Github or Twitter, or your usual nickname.

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
