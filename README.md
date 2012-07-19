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
--sort: Sort includes. Does not attempt to group them nicely at this time, simply swapping their locations.

