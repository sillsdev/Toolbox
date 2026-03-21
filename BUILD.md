# Building the Project

These notes are for **Visual Studio 2022** on Windows.

## Visual Studio Installer

- Add C++ MFC component
- **ATL** is automatically included when adding MFC

## Project Configuration

The following changes are required to avoid compiler warnings, although they should already be set if you use the files from the repo.

- Select the **Toolbox** project, open *Project → Properties*, and choose *All Configurations*.
- Set *Conformance Mode* to `No (/permissive)`
- *Enforce Type Conversion Rules* = `No`
- Add `/wd4596` to command line options to prevent warnings.

Then build the `Debug` solution configuration.

## When Moving or Sending

Ensure that `cc32.dll` (Consistent Changes) is in the same folder as `Toolbox.exe`.

## Tests

1. Build the `Tests` solution config. This builds the main project as a full library (not the small stub that accompanies the .exe), then links the testing executable to it.

2. Right-click **Tests** project and choose *Set as Startup Project*, then press F5. The test suite is powered by *doctest*.

3. To run a specific test case, go to *Project → Properties → Configuration Properties → Debugging → Command Arguments* and enter, for example: `--test-case="CRecord Hierarchy,CRecord Ball Example" --no-breaks --success`
