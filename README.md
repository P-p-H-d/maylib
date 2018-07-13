# MAYLIB

MAYLIB is a C library for doing symbolic mathematical calculations.
It is not a CAS (Computer Algebra System): it doesn't have any
internal programming language and it is very limited in
functionalities. It doesn't support global variables, neither global
functions: this is let to the user of the library.

MAYLIB is a tool for programmer, which means that he must be able
to understand completely and fully what the tool can do and can not. This
implies that MAYLIB specifications should be precise and exhaustive.

MAYLIB is not designed to be efficient on pure numerical computations.
If you plan to do heavy computations involving integer polynomial or
float matrix (for example), you should use another tool or library,
and/or write the code yourself.

# Install

Here are the steps needed to install the library on Unix systems:

To build MAYLIB, you first have to install GNU MP
(version 4.2 or higher) on your computer and MPFR (version 2.1.0 or higher).
You need GCC (or a compatible compiler) and a make program.
You have to get [GMP](http://www.gmplib.org/), [MPFR](http://www.mpfr.org/) and [GCC](http://gcc.gnu.org/).
Please see their respective documentation to see how to install them.

In the MAYLIB source directory, type:

       make

Or if you have not installed GMP and/or MPFR in non-system directory, type:

      make GMP=$GMP_INSTALL_DIR MPFR=$MPFR_INSTALL_DIR

This will compile MAYLIB, and create a library archive file libmay.a.

By default, MAYLIB is built with the assertions turned on and no optimization,
which slows down a lot the library, but is a lot safer.
To have an optimized version, type:

        make fast [GMP=...]


To check the built is correct, run:

         make check [GMP=...]

To install run:

         make install [PREFIX=/target/directory]

See manual for further details.

