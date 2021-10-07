## Example patch.json

This describes the format of the `patch.json`.

This is automatically generated and added to your patch distribution if you choose to upload your patch to the Web Catalog.
Hence you should NOT include one with your patch if you do that.

If you distribute your patch some other way, e.g. as an RPM package, you must provide a valid `patch.json` file.

    {
        "author": "Jim Example",
        "name": "the-internal-patch-name",
        "display_name": "Human-facing Patch Name",
        "version": "1.0.0",
        "category": "calendar",
        "description": "This is a long description of what the patch does.\nIt will be displayed in the patch page.",
        "discussion": "https://example.org/forum/thead?id=9999",
        "sources": "https://git.example.org/patch.git",
        "donations": "https://example.org/donate",
        "compatible": [ "3.2.1.20", "3.3.0.14", "3.3.0.16" ],
        "id": NNN,
        "last_updated": "2020-04-28T06:56:21.931",
    }

### Description of options:

Required / Recommended:

 - display\_name: The name of the patch, which will be shown on the patch list.
 - version: A version string.  It MUST be in the format X.Y.Z
 - author: The name of the principal patch author or maintainer.  Either use your real name, as displayed on Github or Twitter, or your usual nickname.
 - compatible: A list of SailfishOS releases the patch works on.  Versions MUST be given in full with all four fields populated, e.g. "3.3.0.16" (using just "3.3" is invalid).

Optional:
 - description: Explanation of what the patch does.
 - category: This is used to structure the list of patches in categories, but also certain categories will cause patchmanager to restart affected system services or applications.  For example, patches in the category "homescreen" will prompt the user to restart Lipstick after applying.
 - discussion, sources, donations: Optional links to websites.
 - last\_updated": An ISO date string, the Web Catalog sets this.  It is used for sorting and update detection.
 - name: An internal name for the patch.  As this is used as a directory name, it should only contain alphanumeric characters, underscores and hyphens.  Notably NO spaces, dollar signs or other special characters should be used!

Reserved:
 - id: A numeric ID used by Web Catalog.  This MUST NOT be included in patches not distributed via Web Catalog.

### List of supported categories:

Supplying a category not on this list will cause the patch to be shown in the category "other".

 - "homescreen" - will restart Lipstick
 - "silica" - will restart Lipstick
 - "keyboard" - will restart the keyboard service

These will stop or restart the corresponding system applications (e.g. the jolla-browser) after applying the patch.

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
