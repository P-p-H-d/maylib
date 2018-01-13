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

/* Create a complete copy of an unevaluated symbolic number so that
   the returned number is the unique reference and can be modified directly without
   copying it first (or the symbol is evaluated and can't be modified).
   If eval_p is 0, it will create a copy even if x or its childreen are evaluated */
may_t may_copy_c (may_t x, int eval_p)
{
  may_t y;

  if (eval_p && MAY_EVAL_P (x))
    return x;

  switch (MAY_TYPE(x)) {
  case MAY_INT_T:
    return MAY_MPZ_C(MAY_INT(x));
  case MAY_RAT_T:
    return MAY_MPQ_C(MAY_RAT(x));
  case MAY_FLOAT_T:
    return MAY_MPFR_C(MAY_FLOAT(x));
  case MAY_COMPLEX_T:
    return MAY_COMPLEX_C(may_copy_c(MAY_RE(x), eval_p),
                         may_copy_c (MAY_IM(x), eval_p));
  case MAY_STRING_T:
    return MAY_STRING_C(MAY_NAME(x),MAY_SYMBOL(x).domain);
  case MAY_INDIRECT_T:
    return may_copy_c (MAY_INDIRECT(x), eval_p);
  case MAY_DATA_T:
    y   =  may_data_c (MAY_DATA(x).size);
    memcpy (MAY_DATA(y).data, MAY_DATA(x).data, MAY_DATA(x).size);
    return y;
  default:
    {
      may_size_t n = MAY_NODE_SIZE(x);
      y = MAY_NODE_C (MAY_TYPE (x), n);
      MAY_ASSERT (n > 0);
      do {
	MAY_SET_AT (y, n-1, may_copy_c (MAY_AT(x,n-1), eval_p));
      } while (--n != 0);
      return y;
    }
  }
}
