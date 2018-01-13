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

/* FIXME: Remove intmod during computation since this algorithm is not working
   in this field? */
may_t
may_sqrfree (may_t expr, may_t v)
{
  MAY_ASSERT (MAY_EVAL_P (expr) && MAY_EVAL_P (v));
  MAY_ASSERT (MAY_TYPE (v) == MAY_STRING_T);
  MAY_LOG_FUNC (("expr='%Y' v='%Y'", expr, v));

  may_mark ();

  /* Does a naive factorisation */
  expr = may_naive_factor (expr);

  /* Process each term of the product */
  may_iterator_t it;
  may_t retvalue, power, p;
  for (retvalue = may_product_iterator_init (it, expr) ;
       may_product_iterator_end (&power, &p, it)       ;
       may_product_iterator_next (it) ) {
    /* Process the YUN algorithm */
    may_t w = p;
    may_t y = may_diff (p, v);
    may_t g = may_gcd2 (w, y);
    may_t list = may_set_ui (1);
    long i = 1;
    goto start;
    do {
      list = may_mulinc_c (list, may_pow_si_c (g, i));
      i++;
    start:
      /* Simplify w and g by their gcd */
      w = may_divexact (w, g);
      y = may_divexact (y, g);
      MAY_ASSERT (w != 0 && y != 0);
      y = may_sub (y, may_diff (w, v));
      g  = may_gcd2 (w, y);                  // New term of degree
    } while (!may_zero_p (y));
    /* Fix the last term  */
    list = may_mulinc_c (list, may_pow_si_c (w, i));
    y = may_eval (list);

    /* The YUN algorithm processes on */
    may_t reste = may_divexact (p, y);
    MAY_ASSERT (reste != NULL);
    y = may_mul_c (y, reste);
    /* Give back the extracter power */
    y = may_eval (may_pow_c (y, power));

    /* Add this result to the factorised list */
    retvalue = may_mulinc_c (retvalue, y);
  }

  return may_keep (may_eval (retvalue));
}
