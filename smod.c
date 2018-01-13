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

static may_t
smod_recur_c (may_t e, may_t b)
{
  if (may_imminteger_p (e) )
    return may_num_smod (e, b);
  if (may_sum_p (e) || may_product_p (e))
    return may_map2_c (e, (may_t (*)(may_t, void*))smod_recur_c, b);
  return e;
}

/* Return the Modulus (in symmetric representation).
   return a mod b in the range [-iquo(abs(b)-1,2), iquo(abs(b),2)] */
may_t
may_smod (may_t a, may_t b)
{
  MAY_ASSERT (MAY_TYPE (b) == MAY_INT_T);
  MAY_ASSERT (MAY_EVAL_P (a) && MAY_EVAL_P(b));

  MAY_LOG_FUNC (("a='%Y' b='%Y'", a, b));

  if (MAY_UNLIKELY (b == MAY_ZERO || b == MAY_ONE))
    return MAY_ZERO;

  may_mark();
  may_t y = smod_recur_c (a, b);
  return may_keep(may_eval (y));

}
