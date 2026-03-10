# Building the Project

These notes are for **Visual Studio 2022** on Windows.

## Visual Studio Installer

- Add C++ MFC component
- **ATL** is automatically included when adding MFC

## Project Configuration

- Select *All Configurations* to modify. These changes are required to avoid compile warnings, although they should already be set if you use the .sln from the repo.
- Set *Conformance Mode* to `No (/permissive)`
- *Enforce Type Conversion Rules* = `No`
- Add `/wd4596` to command line options to prevent warnings.
- When building, use solution config `Debug` rather than MacWine.

## When Moving or Sending

- Ensure that `cc32.dll` (Consistent Changes) is in the same folder as Toolbox.exe.

## Tests

1. Build a full Toolbox.lib (not the stub that is only about 3kb). To do this, select the main project in Configuration Manager and set configuration to **TestLib**, then build the project again. This should be fast if you've already built the .exe.

2. Build the **Tests** project. Make sure it links against Toolbox.lib.

3. Either run Tests.exe on command line, or right-click **Tests** project and go to *Debug > Start New Instance*.
