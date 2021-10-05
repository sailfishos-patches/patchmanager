## Example patch.json

This describes the format of the `patch.json`.

This is automatically generated and added to your patch distribution if you choose to upload your patch to the Web Catalog.
So do NOT include one with your patch if you do that.

If you distribute your patch some other way, e.g. as an RPM package, you will have to provide a valid `patch.json` file.

    {
        "author": "Jim Example",
        "name": "the-internal-patch-name",
        "display_name": "Human-facing Patch Name"
        "version": "1.0.0"
        "category": "calendar",
        "description": "This is a longish description of what the patch does.\nIt will be displayed in the patch page.",
        "discussion": "https://example.org/forum/thead?id=9999",
        "sources": "https://git.example.org/patch.git",
        "donations": "https://example.org/donate",
        "compatible": [ "3.2.1.20", "3.3.0.14", "3.3.0.16" ],
        "id": NNN,
        "last_updated": "2020-04-28T06:56:21.931",
    }

### Description of options:

Required/Recommended:

 - display\_name: The name of the patch, as it will show in the patch list
 - version: a version string. It MUST be in the format X.Y.Z
 - author: name of the principle patch author or maintainer. Either use your real name, as displayed on Github or Twitter, or use your usual nickname.
 - compatible: a list of SailfishOS releases the patch works on. Versions MUST be given in full,  e.g. "3.3.0.16", using just "3.3" is invalid.

Optional:
 - description: explains a bit more what the patch actually does
 - category: this is used on the one hand to sort the list of patches, on the other hand certain categories will cause patchmanager to restart affected system services or applications. the category "homescreen" for example will prompt the user to restart Lipstick after applying.  
 - discussion, sources, donations: optional links to websites
 - last\_updated": an ISO date string. Web Catalog sets this. It is used in sorting and update detection.
 - name: an internal name for the patch. As this is used as a directory name it should only contain alphanumeric characters, underscores and hyphens. Notably NO spaces, dollar signs or other special characters please!

Reserved:
 - id: a numeric ID used by Web Catalog. This MUST NOT be included in patches not distributed via Web Catalog.

### List of supported categories:

Supplying a category not on this list will cause the patch to show up as "other".

 - "homescreen" - will restart Lipstick
 - "silica" - will restart Lipstick
 - "keyboard" - will restart the keyboard service

These will stop/restart the corresponding system applications (e.g. jolla-browser) after applying the patch.
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
