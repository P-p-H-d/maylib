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

void
may_comdenom (may_t *num, may_t *denom, may_t x)
{
  may_t n, d, n1, d1, n2, d2;
  may_size_t i, size;

  MAY_LOG_FUNC (("%Y", x));

  MAY_ASSERT (MAY_EVAL_P (x));

  MAY_RECORD ();
  switch (MAY_TYPE (x)) {
  case MAY_RAT_T:
    *num   = may_num_simplify (MAY_MPZ_NOCOPY_C (mpq_numref (MAY_RAT (x))));
    *denom = may_num_simplify (MAY_MPZ_NOCOPY_C (mpq_denref (MAY_RAT (x))));
    return;
  case MAY_COMPLEX_T:
    may_comdenom (&n1, &d1, MAY_RE (x));
    may_comdenom (&n2, &d2, MAY_IM (x));
    if (d1 == MAY_ONE && d2 == MAY_ONE)
      goto nothing_to_do;
    d  = may_num_mul (MAY_DUMMY, d1, d2);
    n1 = may_num_simplify (may_num_mul (MAY_DUMMY, n1, d2));
    n2 = may_num_simplify (may_num_mul (MAY_DUMMY, n2, d1));
    n  = may_set_cx (n1, n2);
    break;
  case MAY_SUM_T:
    size = MAY_NODE_SIZE(x);
    n = MAY_NODE_C (MAY_SUM_T, size);
    d1 = MAY_NODE_C (MAY_PRODUCT_T, size);
    for (i = 0; i<size; i++)
      may_comdenom (MAY_AT_PTR (n, i), MAY_AT_PTR (d1, i), MAY_AT (x, i));
    d = may_naive_lcm (size, MAY_AT_PTR (d1, 0));
    if (d == MAY_ONE)
      goto nothing_to_do;
    for (i = 0; i<size; i++) {
      n1 = may_divexact (d, MAY_AT (d1, i));
      MAY_ASSERT (n1 != NULL);
      n2 = may_mul_c (MAY_AT (n, i), n1);
      MAY_SET_AT (n, i, n2);
    }
    break;
  case MAY_FACTOR_T:
  case MAY_PRODUCT_T:
    size = MAY_NODE_SIZE(x);
    n = MAY_NODE_C (MAY_PRODUCT_T, size);
    d = MAY_NODE_C (MAY_PRODUCT_T, size);
    for (i = 0; i<size; i++)
      may_comdenom (MAY_AT_PTR (n, i), MAY_AT_PTR (d, i), MAY_AT (x, i));
    break;
  case MAY_POW_T:
    /* We don't do anything if it is not an integer power */
    if (MAY_TYPE (MAY_AT (x, 1)) == MAY_INT_T) {
      may_comdenom (&n1, &d1, MAY_AT (x, 0));
      if (mpz_sgn (MAY_INT (MAY_AT (x, 1))) < 0)
        swap (n1, d1);
      n = may_pow_c (n1, may_abs_c (MAY_AT (x, 1)));
      d = may_pow_c (d1, may_abs_c (MAY_AT (x, 1)));
      break;
    } else
      goto nothing_to_do;
  default:
  nothing_to_do:
    MAY_CLEANUP();
    *num = x;
    *denom = MAY_ONE;
    return;
  }
  *num = may_eval (n);
  *denom = may_eval (d);

  /* Simplification to avoid too much increase in size.
     Without it:        18481ms
     Use may_naive_gcd:  5996ms
     Use may_gcd:        1680ms  */
  may_t temp[2] = { *num, *denom };
  may_t gcd = may_gcd (2, temp);
  if (!may_one_p (gcd)) {
    *num = may_expand (may_divexact (*num, gcd));
    *denom = may_expand (may_divexact (*denom, gcd));
    MAY_ASSERT (*num != NULL);
    MAY_ASSERT (*denom != NULL);
  }
  MAY_COMPACT_2 (*num, *denom);
}
