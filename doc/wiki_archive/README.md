This is an archive of the GitHub Wiki pages (https://github.com/sailfishos-patches/patchmanager/wiki)
See https://github.com/sailfishos-patches/patchmanager/issues/100 for the discussion that lead to the creation of this archive.

`wiki.bundle` is a git bundle which can be used to restore the wiki repository if necessary.
To do that, follow the `git bundle` documentation, but basically:

    git init new-repo
    git verify /path/to/wiki.bundle
    git pull /path/to/wiki.bundle
    git log --oneline


`root` contains the newest versions of the wiki strure as a copy.


