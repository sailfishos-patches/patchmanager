## Example patch.json in legacy format

This describes the format of the `patch.json` file in the 'legacy' format.
While this is still supported in Patchmanager 3, if possible switch to [the new format](./example_patch.json.md) as it supports more useful information.

    {
        "name": "Example Patch Name",
        "description": "This is a long description of what this Patch does.\nIt will be displayed on the Patch page.",
        "category": "email",
        "infos": {
            "maintainer": "username"
        },
    }

### Description of options:

 - name: The "display name" of the patch, which will be shown on the Patch list.
 - description: Explanation of what the Patch does.
 - category: This is used to structure the list of Patches in categories, but also certain categories will cause Patchmanager to restart affected system services or applications.  For example, Patches in the category "homescreen" will prompt the user to restart Lipstick after their activation.
 - maintainer: The name of the principal Patch author or maintainer.  Either use your real name, as displayed on Github or Twitter, or your usual nickname.

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
