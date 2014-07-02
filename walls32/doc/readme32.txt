WALLS v2.0b1 (1998-03-31) -- Tools for Cave Survey Data Management

If you're upgrading an earlier version of WALLS, please review the
RECENT CHANGES section below.
 
GENERAL
=======
The 32-bit version of Walls (version 2.x and above) requires that you be
running Windows 95, or Windows NT. All recent development and testing
has been on a Windows 95 system. For more info, please read the
WALLS32.HLP file while running Windows.
 
Although this is experimental software that's being revised almost
continuously, by current standards it's a complete cave surveying
package. It has been used heavily with several large projects during the
last several years, an example being the Purificacion project in Mexico
with more than 20K survey shots.
 
You may redistribute the archive WALLS2B1.ZIP as long as its contents
remain unmodified and as long as you believe it's the current released
version. If it isn't, please discard it and get the latest version
before giving it to anyone else.
 
Thanks for taking the time to look the program over. Send me some
feedback, and if you're still interested let me know if you would like
to be notified of the next update.
 
David Mckenzie davidmck@bga.com

QUICK SETUP
===========
1) Using a ZIP-compatible archive utility (e.g., UNZIP.EXE), extract the
contents of WALLS2B1.ZIP to a directory of your choice (e.g., C:\WALLS).
Users of earlier versions of Walls (i.e., the 16-bit version) can
use their old WALLS directory since there are no file name conflicts.
The two versions *can* coexist, although there's no reason to keep the
old one around unless you need it for 16-bit Windows.

The main program, WALLS32.EXE and its associated DLLs, is
self-contained; there's no need to move files to other directories.
After the program is run, it creates a WALLS32.INI file in its own
directory to store user preferences and other settings. My own
preferences (font choices, etc.) are built in as the initial default.

2) If you don't already have WALLS3D (which hasn't changed recently),
extract the contents of WALLS3D.ZIP, a separate download, to your WALLS
directory. Also, if you don't already have Microsoft's OpenGL libraries
installed, extract the contents of OPENGL95.ZIP (another separate
download) and copy the two files, OPENGL32.DLL and GLU32.DLL, to your
C:\WINDOWS\SYSTEM directory.

3) Create some named directories under C:\WALLS (or anywhere else) to
which you can extract any ZIP archives containing data for projects. A
project archive normally has one or more PRJ files and a bunch of
associated SRV files. At least one archived sample project, TREE.ZIP, is
included in WALLS2B1.ZIP. Also, WALLSPRJ.ZIP can be downloaded from the
Web site. It contains some individual project archives that you can
extract to assigned locations on your drive.

4) Make a link to WALLS32.EXE and place it somewhere on your desktop. You
might also tell Windows to associate files of type .PRJ with this
application.
 
5) Check out the program by scanning the online help and opening an
example project.
 
RECENT CHANGES
==============

More fixes applied to Ver 2, B1 Pre-release (1998-03-31)

A metafile export will now correctly use the frame size specifified for
exported maps (not printed maps). The preview map would sometimes mark a
non-fixed station as a #FIXed point when multiple #FIXed points were
present. After testing on Windows NT 4.0, some dialogs and controls were
resized for appearances sake. Revised help file.


Fixes applied to Ver 2, B1 Pre-release (1998-02-25)

Fixed a font sizing problem. On some systems the data columns in the
Traverse page would not line up with their headers. Fixed anomaly with
the calculator's keyboard interface that caused the window to vanish
if the ENTER key is pressed during data entry. Also fixed a problem
in WALLSRV.DLL (16-bit version as well) that affected prefix
processing in rare situations.

Pre-release of Version 2, B1 (1997-12-10)

The first 32-bit version of Walls represents a major overhaul of the
program and includes many new features, such as metafile export and
various geographical functions. A built-in geographical/magnetic field
calculator is provided. For details, see the help file topic "32-bit and
16-bit Version Differences". There you will find links to the new
sections in the help file.

Release of Version 1, B7 (1997-12-10)

This upgrade of the 16-bit version consists mostly of a few fixes,
including some changes to insure data file compatibility with the 32-bit
version. Also, the help file is replaced with a 16-bit version of the
one distributed with the 32-bit version of Walls.

Release B6 (1997-06-17)

1) The program is now supplied with a VRML viewer, WALLS3D, which can be
launched from WALLS (via a toolbar button) and used to visualize the 3D
files you generate. It will display multiple, multi-colored views of
your surveys as wireframes. You need to be running Win95 or WinNT to use
it. Please review the help file topic, Exporting VRML (WRL) Files.

2) The capacity of the program regarding sizes of project and segment
trees has been increased so that available "workspace" is no longer an
issue. You can now work with multiple large project trees, each with
hundreds of surveys (possibly in different drive locations), and copy or
move branches back and forth between them.

3) The data format (SRV files) has been extended to support most of the
measurement types and numeric formats used by cave surveyors. New
features include support for backsights along with the requisite
back-instrument properties and tolerances, quadrant-style bearings,
deg:min:sec, instrument/target heights in conjunction with multiple
taping methods, and a number of new directives: #Date, #Save, #Restore,
#[ , and #] (block comments). Please review the Format of Survey Data
section in the help file -- especially the reorganized section on
#Units.

4) The SEF import module has been completely overhauled so that more
information regarding measurement types is preserved. Also, a bug
affecting backsight processing was fixed. Check out the revised topic,
Importing SMAPS 5.2 (SEF) Files.

5) The VRML export feature has additional options: density of gridlines,
vertical exaggeration, and the presence of an elevation grid.

6) A number of user interface improvements (mostly minor) were made. For
example, the dialog that optionally pops up when duplicate name pairs
are found can now open two edit windows (if required), and highlight
both of the conflicting vectors. Additional toolar buttons were added.
(To see what they do, just click on them.) File and directory handling
is improved (e.g., transferring tree branches between projects in
different drive locations causes data files to be moved/copied as well).

<EOF>

