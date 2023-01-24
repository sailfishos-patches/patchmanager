## Patching Test Cases

The patching test infrastructure consists of:

 - The QML module under `$LIBDIR/qt5/qml/…` (which expands to, e.g., `/usr/lib/qt5/qml/org/SfietKonstantin/patchmanagertests/`), which are some QML component files and a `qmldir` file so they can be used as a module.
 - The `patchmanager-testcase` app as a QML file and `.desktop` file under `/usr/share/`, which imports the above module and thus shows changes to the module's components.
 - A set of patches which act on those files in various ways.

#### Why did you invent `org.SfietKonstantin.patchmanagertests`?

In order to have something that lives under `$LIBDIR` so the 32-/64-bit mangling can be tested.

#### How do I use these?

1. Basically, you install the package, open the Patchmanager, and look for the Patches named "Test Case".
   Read their descriptions for "detailed" information on what they do.

2. Activate / apply these Patches.

3. Open the "Test Cases" companion app via its launcher icon or `sailfish-qml patchmanager-testcase`.  See whether the Patches' changes show up as expected.

4. Note that some Patches may be supposed to "fail" applying, as that may be the specific test case.

#### What do these Patches do?

I'll cop out of this one and refer to "read the source Luke" here.

Also, unless Patch author for a specific test case was too lazy, the Patch's description should provide this information.

Note that not all of them have a visible effect on the app, most of the time the primary goal is to test the patching process itself, not so much changing the test app.

#### How do I add a Test Case / Patch?

 1. Create a patch as usual and store it under `/tests/patching/<testcase patch name>/patch/`.  You should have at least `patch.json` and a `unified_diff.patch` file in that directory.  
    Please describe what your test case is about and how to check success or failure in the `description` of the `patch.json` file.

 1. Optionally (but this is encouraged) also add the "source files" used to generate the patch under `/tests/patching/<testcase patch name>/patch-source/`.

 1. Create a `.pro` file `/tests/patching/<testcase patch name>/patch/<testcase patch name>.pro` which installs your patch.

 1. Add your test case directory to the `SUBDIRS` of the `/tests/patching/patching.pro` file.

