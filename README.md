# Walls Cave Survey software

RIP David McKenzie

## Looking for developers!

I (Andy Edwards) have volunteered to help maintain Walls, but I am by no means a C++ or Visual Studio expert, let alone
familiar with the MFC and probably a number of other old libraries it uses.  If you're a caver and C++ developer, please
let me know if you'd be willing to help, even if you only have time to give me tips whenever I hit a snag.

## Roadmap

Right now I need to figure out what else is missing from here; David had all of his projects stored in one giant folder
and I've tried to pare it down to only what's necessary for Walls.  But some things are definitely missing (for example,
Walls2D is needed for SVG export).  It may also make sense to store WallsMap and other programs in this repo/codebase.

## Quick Start for Developers

* Install the latest version of [Visual Studio](https://www.visualstudio.com/downloads/) (community edition is fine)
* Install the [`nasm` assembler](http://www.nasm.us/) and make sure it's on your `PATH` 
* Open `Walls.sln`
* Select a build configuration: `Debug_XP` if you're on Windows XP, or `Debug` otherwise.
* Cross your fingers
* Hit F7 to build the solution
