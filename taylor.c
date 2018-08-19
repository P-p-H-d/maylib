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

/* Returns sum(diff(f,v,n)/n!*(x-a)^n,n=0,m) */

may_t
may_taylor (may_t f, may_t x, may_t a, unsigned long m)
{
  mpz_t z;
  unsigned long n;
  may_t xa, ref, fn;
  may_t result[m+1];

  MAY_ASSERT (MAY_EVAL_P (f) && MAY_EVAL_P (x) && MAY_EVAL_P (a));
  MAY_ASSERT (MAY_TYPE (x) == MAY_STRING_T);

  MAY_LOG_FUNC (("f='%Y' x='%Y' a='%Y' m=%lu", f, x, a, m));

  may_mark ();

  mpz_init_set_ui (z, 1);
  xa = may_set_ui (1);
  ref = may_sub (x, a);
  fn = f;

  result[0] = may_replace_c (f, x, a);
  for (n = 1; n<=m; n++) {
    xa = may_mul (xa, ref);       /* xa = (x-a)^n */
    mpz_mul_ui (z, z, n);         /* z = n! */
    fn = may_diff (fn, x);        /* fn = diff(f,x,n) */
    if (MAY_UNLIKELY (may_zero_p (fn))) {
      m = n-1;
      break;
    }
    /* Add new term */
    result[n] = may_div_c (may_mul_c (may_replace_c (fn, x, a),
                                      xa),
                           may_set_z (z));
  }
  ref = may_add_vc (m+1, result);
  return may_keep (may_eval (ref));
}
