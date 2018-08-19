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

/* Algorithm from:
   M. Bronstein Symbolic Integration I - Transcendental Functions */

static int
PartialFraction (may_t *a, size_t n, may_t d[n], may_t var)
{
  may_t a0, a1, r, t;
  may_t prod;

  prod = may_expand (may_eval (may_mul_vc (n-1, d+1)));
  if (!may_div_qr (&a0, &r, *a, may_mul (d[0], prod), var))
    return 0;
  if (n == 1) {
    *a = a0;
    d[0] = r;
    return 1;
  }
  if (!may_gcdex (&a1, &t, NULL, prod, d[0], r, var))
    return 0;
  if (!PartialFraction (&t, n-1, d+1, var))
    return 0;
  *a = may_add (a0, t);
  d[0] = a1;
  return 1;
}

may_t
may_partfrac (may_t f, may_t var, may_t (*func) (may_t, may_t))
{
  may_t num, denom;

  MAY_ASSERT (MAY_TYPE (var) == MAY_STRING_T);

  may_mark ();

  /* Transform f into a rational function and factorize the denominator */
  may_comdenom (&num, &denom, f);
  if (may_one_p (denom))
    /* Technically we can return f, but since comdenom has simplified
       the fraction num/denom, we prefere returning the simplified fraction. */
    return may_keep (num);
  if (func == NULL)
    func = may_sqrfree;
  denom = (*func) (denom, var);

  /* Count the number of terms in the factored denominator */
  may_iterator_t it;
  size_t i, n = 1;
  may_t p, b;
  for (may_product_iterator_init (it, denom) ;
       may_product_iterator_end (&p, &b, it) ;
       may_product_iterator_next (it))
    n++;
  if (n == 1)
    return may_keep (may_div (num, denom));

  /* Declare tables and extract product in it */
  may_t d[n], de[n];
  unsigned long e[n];
  i = 0;
  for (b = may_product_iterator_init (it, denom) ;
       may_product_iterator_end (&p, &(d[i]), it) ;
       may_product_iterator_next (it), i++ ) {
    de[i] = may_product_iterator_ref (it);
    if (may_get_ui (&(e[i]), p) < 0)
      return may_keep (NULL);
  }
  if (!may_one_p (b)) {
    d[i] = de[i] = b;
    e[i] = 1;
  } else
    n = n-1;

  /* Start the completer partial fraction decompisition */
  may_t a = num;
  if (!PartialFraction (&a, n, de, var))
    return may_keep (NULL);

  may_t partial = may_set_ui (0);
  for (i = 0; i < n ; i++) {
    for (unsigned long j = e[i] ; j >= 1; j--) {
      if (!may_div_qr (&(de[i]), &b, de[i], d[i], var))
        return may_keep (NULL);
      partial = may_addinc_c (partial, may_mul_c (b, may_pow_si_c (d[i], -j)));
    }
    a = may_addinc_c (a, de[i]);
  }
  partial = may_addinc_c (partial, a);

  return may_keep (may_eval (partial));
}
