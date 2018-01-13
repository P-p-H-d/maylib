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

/* Format:
   RootOf(N, POLY, VAR)
   ==> RootOf(N, POLY, VAR)^N = SUBS(POLY,VAR=RootOf) */

#define ROOTOF_N(s) (MAY_ASSERT (may_ext_p (s) == may_rootof_ext), may_op(s,0))
#define ROOTOF_P(s) (MAY_ASSERT (may_ext_p (s) == may_rootof_ext), may_op(s,1))
#define ROOTOF_X(s) (MAY_ASSERT (may_ext_p (s) == may_rootof_ext), may_op(s,2))

static may_ext_t may_rootof_ext;

static may_t
rootof_constructor (may_t list)
{
  /* FIXME: An extension must not use MAY_OPEN_C */
  MAY_OPEN_C (list, may_rootof_ext);
  return list;
}

static may_t
rootof_c (may_t n, may_t poly, may_t x)
{
  MAY_ASSERT (MAY_TYPE (x) == MAY_STRING_T);
  MAY_ASSERT (MAY_TYPE (n) == MAY_INT_T);

  may_t s = may_ext_c (may_rootof_ext, 3);
  may_ext_set_c (s, 0, n);
  may_ext_set_c (s, 1, poly);
  may_ext_set_c (s, 2, x);
  return may_hold (s);
}

static may_t
rootof_eval (may_t l)
{
  /* Check the arguments */
  if (may_nops (l) != 3)
    may_error_throw (MAY_DIMENSION_ERR, __func__);
  may_t n  = may_eval (ROOTOF_N (l));
  may_t p  = may_eval (ROOTOF_P (l));
  may_t x  = may_eval (ROOTOF_X (l));

  if (may_get_name (x) != may_string_name
      || may_get_name (n) != may_integer_name)
    may_error_throw (MAY_DIMENSION_ERR, __func__);

  return rootof_c (n, p, x);
}

static may_t
rootof_pow (may_t base, may_t power)
{
  if (may_ext_p (base) == may_rootof_ext
      && may_get_name (power) == may_integer_name) {
    if (may_num_cmp (power, ROOTOF_N (base)) >= 0) {
      may_t num = may_pow (ROOTOF_X (base), power);
      may_t pol = may_sub(may_pow(ROOTOF_X (base), ROOTOF_N (base)),
                          ROOTOF_P(base));
      may_div_qr(NULL, &pol, num, pol, ROOTOF_X(base));
      pol = may_replace (pol, ROOTOF_X(base), base);
      return pol;
    }
  }
  /* Default: done nothing */
  return may_hold (may_pow_c (base, power));
}


static const may_extdef_t rootof_cb = {
  .name = "ROOTOF",
  .priority = 10,
  .eval = rootof_eval,
  .pow = rootof_pow,
  .constructor = rootof_constructor
};


/*****************************************************************/

void may_rootof_ext_init (void)
{
  /* Register the erootof extension if not already done. */
  if (may_rootof_ext == 0) {
    may_rootof_ext = may_ext_register (&rootof_cb, MAY_EXT_INSTALL);
    if (may_rootof_ext == 0)
      may_error_throw (MAY_DIMENSION_ERR, __func__);
  }
}
