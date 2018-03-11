echo "Performing DOXYGEN on NCSJP2 source code"

set DOXYGEN_EXE="P:/Programs/doxygen-1.1.0/bin/doxygen.exe"

touch dummy.txt

%DOXYGEN_EXE% ../NCSJP2/doxyfile
