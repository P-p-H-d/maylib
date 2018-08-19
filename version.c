/* This file is part of the MAYLIB libray.
   Copyright 2007-2018 Patrick Pelissier

This Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

This Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with th Library; see the file COPYING.LESSER.txt.
If not, write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston,
MA 02110-1301, USA. */

#include "may-impl.h"

#define STRINGIFY1(x) #x
#define STRINGIFY2(x) STRINGIFY1(x)

#define VERSION(M,m,p) STRINGIFY2(M) "." STRINGIFY2(m) "." STRINGIFY2(p)

const char *
may_get_version (void)
{
  return "MAY V" VERSION(MAY_MAJOR_VERSION, MAY_MINOR_VERSION, MAY_PATCHLEVEL_VERSION)
    " (GMP V" VERSION(__GNU_MP_VERSION, __GNU_MP_VERSION_MINOR, __GNU_MP_VERSION_PATCHLEVEL)
    " MPFR V" VERSION(MPFR_VERSION_MAJOR,MPFR_VERSION_MINOR,MPFR_VERSION_PATCHLEVEL)
#ifdef CC
    " CC=" STRINGIFY2 (CC)
#endif
#ifdef CFLAGS
    " CFLAGS=" STRINGIFY2 (CFLAGS)
#endif
    ")"
  ;
}
