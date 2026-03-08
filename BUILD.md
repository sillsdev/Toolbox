# Building the Project

These notes are for **Visual Studio 2022** on Windows. They were needed on my system for the recent `.sln` file.

## Visual Studio Installer
- Add C++ MFC component
- **ATL** is automatically included when adding MFC

## Project Configuration
- Change build to `Debug Unicode` rather than MacWine
- Set *Conformance Mode* to `No (/permissive)`
- *Enforce Type Conversion Rules* = `No`
- Add `/wd4596` to command line options to prevent warnings.

## When Moving or Sending
- Ensure that cc32.dll is in the same folder as Toolbox.exe.

## Tests
- Either run on command line or by right-clicking **Tests** project and going to *Debug > Start New Instance*.
