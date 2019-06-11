/* This file is part of the MAYLIB libray.
   Copyright 2014 Patrick Pelissier

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

#if defined(_WIN32)
# include <windows.h>
#elif (defined(__APPLE__) && defined(__MACH__)) \
  || defined(__DragonFly__) || defined(__FreeBSD__) \
  || defined(__NetBSD__) || defined(__OpenBSD__)
# include <sys/param.h>
# include <sys/sysctl.h>
# define USE_SYSCTL
#else
# include <unistd.h>
#endif

/* Return the number of CPU of the system */
int may_get_cpu_count(void)
{
#if defined(_WIN32)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
#elif defined(USE_SYSCTL)
  int nm[2];
  int count = 0;
  size_t len = sizeof (count);
  nm[0] = CTL_HW;
  nm[1] = HW_NCPU;
  sysctl(nm, 2, &count, &len, NULL, 0);
  return MAX(1, count);
#elif defined (_SC_NPROCESSORS_ONLN)
  return sysconf(_SC_NPROCESSORS_ONLN);
#else
  return 1;
#endif
}


