cppmv
=====

Simple utility for moving files in a CPP source folder without breaking includes/header guards

Usage: cppmv [--git --cmake --sort] src dst

Can support moving directories or files. 
Destination directories will be merged, however end file locations cannot already exist.
Does not currently abitrary amount of pairs of src, dst arguments (can be done with work on the bash wrapper).

Does not at this moment support directories with spaces in them due to the bash wrapper implementation.

--git: use git mv so files remain tracked nicely
--cmake: update CMakeLists.txt if present, makes a rather naive assumption that all source files are somewhere in file, one line per file.
The CMakeLists.txt file must be in the current working folder.
--sort: Sort includes alphabetically and from least-nested to most-nested. Does not attempt to group them nicely at this time, simply swapping their locations.

The tool is implemented as a bash script over a small C++-programmed utility.

Building
=====

g++ -o cppmv-update cppmv-update.cpp


Running:
===== 

cppmv [--git --cmake --sort] src dst
cppmv must be in the same directory as an executable called cppmv-update (created from cppmv-update.cpp)

Examples:

cppmv src/a.h src/b.h
Effect: rename the file and update all includes that reference a.h

cppmv src/a.cpp src/foo/a.cpp
Effect: move the file and update all includes in a.cpp to work with the new location

cppmv works only with full paths (you cannot do src/a.h src/foo/ in the previous example).


Assumptions:
=====

cppmv assumes includes of the form #include <myfile.h> are from external libraries and will not update them.
cppmv assumes includes of the form #include "myfile.h" are relative paths and will update them globally.

cppmv will read in its file in its entirety and then write back the file, and it may take a bit of time. I advise only using this tool if your source is under version control, and using the source controls diff tool to ensure that the changes are correct.
