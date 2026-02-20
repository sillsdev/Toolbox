# Building the Project

These steps are for **Visual Studio 2022** on Windows.
They were needed on my system for the recent `.sln` file.

## Visual Studio Installer
- Add C++ MFC component
- **ATL** is automatically included when adding MFC

## Project Configuration
- Change build to `Debug Unicode` rather than MacWine
- Set *Conformance Mode* to `No (/permissive)`
- *Enforce Type Conversion Rules* = `No`
- Add `/wd4596` to command line options
