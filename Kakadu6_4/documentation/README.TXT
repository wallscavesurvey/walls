HYPERDOC
--------

If "index.html" does not appear in this directory,
be sure to compile the "kdu_hyperdoc" utility
(see "Compiling_Instructions.txt").  Both the
Unix Makefile and the Visual C++ and .NET workspace methods
for building "kdu_hyperdoc" include post-build
steps which will automatically build the
documentation, as well as Java native interfaces (plus
C# and Visual Basic interfaces for .NET builds).
However, if this does not work, run the command found in
"hyperdoc.bat_saved" (generic) or "hypedoc_windows.bat_saved"
(includes a few Windows-specific components).

The "kdu_hyperdoc" utility constructs an integrated
HTML documentation system, as well as an extensive set of
Java native interfaces, plus C# and Visual Basic language
bindings, from the structure and comments found in the
public header files shipped with Kakadu.  You should find
this EXTREMELY USEFUL.
