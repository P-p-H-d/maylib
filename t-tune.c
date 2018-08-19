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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/resource.h>

#include "may-impl.h"

static may_size_t may_sort_threshold = MAY_SORT_THRESHOLD;
#undef MAY_SORT_THRESHOLD
#define MAY_SORT_THRESHOLD may_sort_threshold

#include "eval.c"

static int
cputime (void)
{
  struct rusage rus;

  getrusage (0, &rus);
  return rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_usec / 1000;
}

static int
my_log2(int n)
{
  return sizeof(int)*8 - __builtin_clz(n);
}


static double
measure_sort_pair (may_size_t size)
{
  may_mark ();

  may_pair_t *tab = may_alloc (size * sizeof *tab );
  for(may_size_t i = 0; i < size ;i ++) {
    char buffer[100];
    tab[i].first = may_set_ui ((unsigned long) i*i);
    sprintf (buffer, "x%d", (int) i);
    tab[i].second = may_set_str (buffer);
  }

  int t0, m0;
  t0 = 0;
  m0 = 5;
  while (t0 < 1000) {
    m0 *= 2;
    t0 = cputime();
    for(int m = 0; m < m0 ; m++)
      sort_pair (tab, size);
    t0 = cputime() - t0;
  }
  may_keep (NULL);
  return (double) t0 / m0;
}

static void
best_sort_pair (void)
{
  int count = 0;
  for(may_size_t size = 4 ; size < MAY_HASH_MAX * 2; size+=my_log2(size)) {
    /* count sort */
    may_sort_threshold = 1;
    double d1 = measure_sort_pair (size);
    /* merge sort */
    may_sort_threshold = 1000000000;
    double d2 = measure_sort_pair (size);
    printf ("%d %e %e\n", size, d1, d2);
    count += (d1 < d2);
  }
  /* Profile
     sort2 > sort small
     sort2 < sort medium 7 <= ???
     sort2 > sort around hash_max
     sort2 < sort above 2*hash_max
  */
}

int main()
{
  printf ("%s\n", may_get_version ());
  may_kernel_start (0, 0);
  best_sort_pair();
  may_kernel_end();
}
