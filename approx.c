/* This file is part of the MAYLIB libray.
   Copyright 2007-2009 Patrick Pelissier

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

/* Number of times we reperform an increased computation
   until we assume that it is an exact result.
   (Heuristic value) */
#define MAX_NUMBER_OF_IT_BEFORE_ASSUMING_EXACT_RESULT 5
/* Number of times we reperform an increased computation
   until we assume that it is an exact zero.
   (Heuristic value) */
#define MAX_NUMBER_OF_IT_BEFORE_ASSUMING_ZERO 50

static const unsigned short Log2Table[][2] = {
  {1, 1},
  {53, 84},
  {1, 2},
  {4004, 9297},
  {53, 137},
  {2393, 6718},
  {1, 3},
  {665, 2108},
  {4004, 13301},
  {949, 3283},
  {53, 190},
  {5231, 19357},
  {2393, 9111},
  {247, 965},
  {1, 4},
  {4036, 16497},
  {665, 2773},
  {5187, 22034},
  {4004, 17305},
  {51, 224},
  {949, 4232},
  {3077, 13919},
  {53, 243},
  {73, 339},
  {5231, 24588},
  {665, 3162},
  {2393, 11504},
  {4943, 24013},
  {247, 1212},
  {3515, 17414},
  {1, 5},
  {4415, 22271},
  {4036, 20533},
  {263, 1349},
  {665, 3438},
  {1079, 5621},
  {5187, 27221},
  {2288, 12093},
  {4004, 21309},
  {179, 959},
  {51, 275},
  {495, 2686},
  {949, 5181},
  {3621, 19886},
  {3077, 16996},
  {229, 1272},
  {53, 296},
  {109, 612},
  {73, 412},
  {1505, 8537},
  {5231, 29819},
  {283, 1621},
  {665, 3827},
  {32, 185},
  {2393, 13897},
  {1879, 10960},
  {4943, 28956},
  {409, 2406},
  {247, 1459},
  {231, 1370},
  {3515, 20929} };

/* WARNING: may_num_presimplify must be off so that float
   are not presimplified at parsed time */
may_t
may_approx (may_t x, unsigned int base, unsigned long n, mp_rnd_t rnd)
{
  may_t y;
  mp_prec_t p, pold;
  char *s1, *s2;
  mp_exp_t e1, e2, eold = 0;
  int i;
  int count2zero;

  MAY_ASSERT (base >= 2);
  MAY_ASSERT (base <= numberof (Log2Table) + 1);

  may_mark ();
  pold = may_kernel_prec (0);
  p = Log2Table[base-2][1] * (n+1) / Log2Table[base-2][0];
  p = MAX (pold, p) + 10;
  count2zero = 0;
  /* Use range arithmetic to compute a range of the exact
     value until we are able to round properly the value
     to the required precision in the required base. */
  for (i = 0; ;i++) {
    may_kernel_prec (p);
    y = may_evalr (x);
    MAY_ASSERT (MAY_TYPE (y) == MAY_RANGE_T);
    s1 = mpfr_get_str (NULL, &e1, base, n, MAY_FLOAT (MAY_AT (y,0)), rnd);
    s2 = mpfr_get_str (NULL, &e2, base, n, MAY_FLOAT (MAY_AT (y,1)), rnd);
    if (e1 == e2 && strcmp (s1, s2) >= 0)
      break;
    if (i >= MAX_NUMBER_OF_IT_BEFORE_ASSUMING_EXACT_RESULT) {
      y = may_compact (y);
      s1 = mpfr_get_str (NULL, &e1, base, n, MAY_FLOAT(MAY_AT (y,0)),GMP_RNDN);
      s2 = mpfr_get_str (NULL, &e2, base, n, MAY_FLOAT(MAY_AT (y,1)),GMP_RNDN);
      if (e1 == e2 && strcmp (s1, s2) >= 0)
        break;
      /* Try to detect perfect 0 like sqrt(3)*sqrt(2)-sqrt(6)
         Not perfect, but better than an infinite loop */
      e1 = MIN (e1, e2);
      count2zero += (e1 < eold);
      eold = e1;
      if (count2zero >= MAX_NUMBER_OF_IT_BEFORE_ASSUMING_ZERO) {
        s1 = (char *) "0";
        e1 = 1;
        break;
      }
    }
    p += 64;
    may_compact (NULL);
  }
  {
    /* Optionnal sign + first digit + '.' + mantissa + 'E' + optionnal sign
       + exponent + zero */
    char Buffer[3+strlen(s1)+2+4*sizeof (mp_exp_t)+1];
    const char *negate = *s1 == '-' ? s1++, "-" : "";
    if (strcmp (s1, "0") != 0) {
      sprintf (Buffer, "#%s%c.%sE%ld", negate, *s1, s1+1, e1-1);
      y = may_set_str (Buffer);
    } else
      y = MAY_ZERO;
  }
  may_kernel_prec (pold);
  return may_keep (y);
}
