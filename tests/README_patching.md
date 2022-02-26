## Patching Test Cases

The patching test infrastructure consists of:

 - The QML module under LIBDIR/qt5/qml/..., which is some QML component files and a qmldir file so they can be used as a module
 - The little app as a QML file and .desktop file under /usr/share/, which imports the above module and thus shows changes to the module's components.
 - a set of patches which act on those files in various ways

#### Why did you invent org.SfietKonstantin.patchmanagertests?

In order to have something that lives under LIBDIR so the 32/64 things can be tested.

#### How do I use these?

Basically, you install the package, open the Patchmanager, look for the patches named "Test Case".
Read their descriptions for "detailed" information on what they do.

Activate/apply the patches.

Open the "Test Cases" companion app. See whether the patches changes show up as expected.

Note that some patches may be supposed to "fail" applying, that may be the test case specifically.

#### What do the patches do?

I'll cop out of this one and refer to "read the source Luke" here.

Also, unless the test case patch author was to lazy, the patch description should have that information.

Not all of them have a visible effect on the app, mos of the time main goal is to test the
patching process itself, not so much changing the test app.

#### How do I add a Test Case/patch?

 1. Create a patch as usual, and store it under `/tests/patching/<testcase patch name>/patch/`. You should have at least `patch.json` and a
`unified_diff.patch` file in that directory.  
Please describe what your test case is about and how to check success or failure in the `description` of the `patch.json` file.

 1. Optionally (but this is encouraged) also add the "source files" used to generate the patch under `/tests/patching/<testcase patch name>/patch-source/`.

 1. Create a `.pro` file `/tests/patching/<testcase patch name>/patch/<testcase patch name>.pro` which installs your patch.

 1. Add your test case directory to the `SUBDIRS` of the `/tests/patching/patching.pro` file.

