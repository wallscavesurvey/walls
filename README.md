# Walls Cave Survey software

RIP David McKenzie

## Looking for developers!

I (Andy Edwards) have volunteered to help maintain Walls, but I am by no means a C++ or Visual Studio expert, let alone
familiar with the MFC and probably a number of other old libraries it uses. If you're a caver and C++ developer, please
let me know if you'd be willing to help, even if you only have time to give me tips whenever I hit a snag.

## Progress and Roadmap

I've cleaned up the project structure to make it easier to build, and added an automated CI build and release for Walls.
I've also gotten WallsMap to build, but I'm not very close to making any new releases of WallsMap yet (I've never used
it personally, so it will be harder for me to confirm if any features are broken in these new builds).

It took me many years to get going, but I've finally fixed some bugs, and I have work in progress on SVG Profile export
and roundtripping that I'll release soon.

I hope to meet with the Texas Speleological Survey and Cave Research Foundation soon to discuss their uses and needs,
as well as trying to find out who else is still using Walls.

## Quick Start for Developers

- Install [Visual Studio 2022](https://www.visualstudio.com/downloads/)
- Install the [`nasm` assembler](http://www.nasm.us/) and make sure it's on your `PATH`
- Install 32-bit CMake 3.3, afterward there should be files in C:\Program Files (x86)\CMake\share\cmake-3.3
- Install the latest [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk)
- Install the [Visual Studio Installer Projects Extension](https://marketplace.visualstudio.com/items?itemName=VisualStudioClient.MicrosoftVisualStudio2022InstallerProjects)
- Optional: Install [Help+Manual](https://www.helpandmanual.com/index.html) version >= 6 and a license (it's very expensive... contact Andy Edwards if you need a license)
  - Make sure the `Help+Manual` directory is on your `PATH` (e.g. `C:\Program Files (x86)\EC Software\HelpAndManual6`)
- Open `Walls.sln`
- Cross your fingers
- Hit F7 to build the solution
- Run the project!

## Building the Installer

- Build the `Walls_Help` project first. I would make the installer project depend on `Walls_Help` if I could, but I can't...because Visual Studio/Installer Projects kinda suck.
- Build the `WallsInstaller` project if you want to make an installer for distribution.

## Thanks To

- David McKenzie, of course
- Dariusz Lubomski (@dlubom) for initial work on setting up a CI build
