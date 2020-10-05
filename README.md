# Walls Cave Survey software

RIP David McKenzie

## Looking for developers!

I (Andy Edwards) have volunteered to help maintain Walls, but I am by no means a C++ or Visual Studio expert, let alone
familiar with the MFC and probably a number of other old libraries it uses.  If you're a caver and C++ developer, please
let me know if you'd be willing to help, even if you only have time to give me tips whenever I hit a snag.

## Roadmap

Right now I need to figure out what else is missing from here; David had all of his projects stored in one giant folder
and I've tried to pare it down to only what's necessary for Walls.  It will probably make the most sense to put WallsMap
and possibly other projects in this repo as well.

## Quick Start for Developers

* Install [Visual Studio 2017](https://www.visualstudio.com/downloads/) (community edition is fine.  Newer versions of Visual Studio may work, but I'm not sure)
* Install the [`nasm` assembler](http://www.nasm.us/) and make sure it's on your `PATH` 
* Install 32-bit CMake 3.3, afterward there should be files in C:\Program Files (x86)\CMake\share\cmake-3.3
* Install the latest [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk)
* Install the [OpenGL extension headers](https://www.khronos.org/registry/khronos_headers.zip).  These go in the same
directory as gl.h, which is something like `C:\Program Files (x86)\Windows Kits\8.1\Include\um\gl` (depending on the Windows SDK version)
* Install the [Visual Studio Installer Projects Extension](https://marketplace.visualstudio.com/items?itemName=VisualStudioClient.MicrosoftVisualStudio2015InstallerProjects)
* Install [Help+Manual](https://www.helpandmanual.com/index.html) version >= 6 and a license (it's very expensive... contact Andy Edwards if you need a license)
* Make sure the `Help+Manual` directory is on your `PATH` (e.g. `C:\Program Files (x86)\EC Software\HelpAndManual6`)
* Open `Walls.sln`
* Select a build configuration: `Debug_XP` if you're on Windows XP, or `Debug` otherwise.
* Cross your fingers
* Hit F7 to build the solution
* Run the project!

## Building the Installer
* Build the `Walls_Help` project first.  I would make the installer project depend on `Walls_Help` if I could, but I can't...because Visual Studio/Installer Projects kinda suck.
* Build the `WallsInstaller` project if you want to make an installer for distribution.

