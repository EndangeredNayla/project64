Project64 - Building from source
================================

This document describes how to set up a local build environment for Project64 on Windows.
It is meant for helping contributors set up building Project64 on their machines so that they can write code to fix bugs and add new features.
If you just want to run Project64 or its development builds, use the [Readme](https://github.com/smash64-dev/project64/blob/develop/README.md) instead.

## Required software

* Visual Studio 2022 Community Edition (though you can probably retarget to port to earlier MSVC)
* During installation, select the `Desktop development with C++`
* During the same part of installation, go to `Individual components` and select `MSVC c++ v.143 (latest)`

## Clone the repository

You must clone the repository to be able to build many of the projects in the solution.
This can be done in the visual studio welcome screen. The git URL is:

```
https://github.com/smash64-dev/project64.git
```

## Build from source

Open the `Project64.sln` file in Visual Studio. You can now build the solution from the Build menu.

Use the `Git Changes` tab to check out the `project64k` branch. This is our current working branch.

Our testing is currently focusing on the `Release` and `Win32` configurations.

Building a Release build will also generate an installer file alongside the output binary that you can use to test the installation process.

In the current state of Project64k (March 2023) you will get errors from trying to build the Android projects. These error messages don't mean anything for the Windows builds. You can safely unload the offending projects from the Solution View to reduce clutter in the Build Log if you are only planning to contribute to the Windows builds.

## Recommended additional steps

* If you wish to quickly launch the Project64 application with Visual Studio's debugger you should right-click the Project64 project in the Solution View and choose "Set as Startup Project" in the context menu. Pressing F5 or the Local Windows Debugger option should now launch the Project64 application.

* In the `Config` folder in the root of the repository is a `Project64.cfg.development` file. Copying this file over top of the `Project64.cfg` file in the same directory will ensure the builds in the `Bin` subdirectories have the proper directories set for accurate debugging.
