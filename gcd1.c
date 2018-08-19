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

/* Return the greatest commun sub expression (sum).
   For example for x=a+b and y=a+c, it return a. */
MAY_REGPARM may_t
may_naive_gce (may_t x, may_t y)
{
  may_t z;
  may_size_t sz;

  MAY_LOG_FUNC (("x='%Y' y='%Y'", x, y));

  if (MAY_LIKELY (MAY_PURENUM_P (x) && MAY_PURENUM_P (y)))
    return may_num_min (x, y);

  MAY_RECORD ();

  /* Extract iterators for x and y */
  may_iterator_t itx;
  may_t numx = may_sum_iterator_init (itx, x);
  may_iterator_t ity;
  may_t numy = may_sum_iterator_init (ity, y);

  /* Alloc long enough and start the stuff */
  z = MAY_NODE_C (MAY_SUM_T, may_nops (x) + 1);
  sz = 0;

  /* First term may be numerical */
  if (numx != MAY_ZERO && numy != MAY_ZERO)
    MAY_SET_AT (z, sz++, may_num_min (numx, numy));

  /* Main loop */
  may_t bx, by, cx, cy;
  while (may_sum_iterator_end (&cx, &bx, itx)
         && may_sum_iterator_end (&cy, &by, ity)) {
    int i= may_identical (bx, by);
    if (i == 0) {
      may_t tmp = may_num_cmp (cx, cy) < 0 ? may_sum_iterator_ref (itx)
        : may_sum_iterator_ref (ity);
      MAY_SET_AT (z, sz++, tmp);
      may_sum_iterator_next (itx);
      may_sum_iterator_next (ity);
    } else if (i < 0)
      may_sum_iterator_next (itx);
    else
      may_sum_iterator_next (ity);
  }
  if (sz == 0)
    z = MAY_ZERO;
  else
    MAY_NODE_SIZE(z) = sz;
  MAY_RET_EVAL (z);
}

/* Return the naive GCD (naive means quick and dirty) */
may_t
may_naive_gcd (may_size_t n, const may_t *tab)
{
  if (MAY_UNLIKELY (n== 1))
    return tab[0];
  MAY_ASSERT (n >= 2);

  MAY_LOG_FUNC (("n=%lu tab[0]='%Y' tab[1]=='%Y'",n,tab[0],tab[1]));

  MAY_RECORD ();
  may_size_t i;
  may_t el, cel, bel, extranum = NULL;
  /* Skip all 0 (They don't change the GCD) */
  for (i = 0 ; i < n && MAY_ZERO_P(tab[i]) ; i++);

  /* If everything was 0, return 0 */
  if (i == n)
    return MAY_ZERO;

  /* Set the first element as the GCD */
  el = tab[i];
  if (may_sum_p (el)) {
    /* Try to get the naive_gcd of its elements to be a little bit more efficient
       And provide better naive GCD */
    el = may_naive_gcd (MAY_NODE_SIZE(el), MAY_AT_PTR (el, 0));
    if (el != MAY_ONE) {
      /* We have to handle differently if it is a pure number due to an eval rule.
       We want to do mul(el, divexact ()) but since el is a purenumber, it will
       be expanded and we don't want it to. */
      if (MAY_PURENUM_P (el)) {
        extranum = el;
        el  = may_div (tab[0], el);
      } else {
        el = may_mul (el, may_divexact (tab[0], el));
      }
    }
  }
  may_iterator_t it;
  may_t gcdnum = may_product_iterator_init (it, el);
  if (MAY_UNLIKELY (extranum != NULL))
    gcdnum = extranum;
  /* Alloc big enough tables...*/
  may_size_t ngcd = MAY_TYPE (el) == MAY_FACTOR_T ? may_nops (MAY_AT (el, 1)) : may_nops (el);
  may_t *gcdtab = may_alloc (2 * (ngcd+1) * sizeof (may_t));
  may_t *gcdpowtab = gcdtab + (ngcd+1);
  ngcd = 0;
  while (may_product_iterator_end2 (&cel, &bel, it)) {
    gcdtab[ngcd] = bel;
    gcdpowtab[ngcd] = cel;
    ngcd++;
    may_product_iterator_next (it);
  }

  /* Scan the rest */
  for (i = i+1  ; i < n && (gcdnum != MAY_ONE || ngcd > 0); i++) {
    /* Skip 0 */
    if (MAY_ZERO_P(tab[i]))
      continue;
    may_t el = tab[i], extranumel = NULL;
    if (may_sum_p (el)) {
      /* Try to get the naive_gcd of its elements to be a little bit more efficient*/
      el = may_naive_gcd (MAY_NODE_SIZE(el), MAY_AT_PTR (el, 0));
      if (el != MAY_ONE) {
        /* We have to handle differently if there is a pure number due to a eval rule */
        if (MAY_PURENUM_P (el)) {
          extranumel = el;
          el  = may_div (tab[i], el);
        } else {
          el = may_mul (el, may_divexact (tab[i], el));
        }
      }
    }
    may_t numel = may_product_iterator_init (it, el);
    /* Overwrite the numerical part with the one from the factorisation of the sum if any */
    if (MAY_UNLIKELY (extranumel != NULL))
      numel = extranumel;
    /* Compute the numerical GCD */
    if (gcdnum != MAY_ONE)
      gcdnum = may_num_gcd (gcdnum, numel);
    /* Compute the symbolic naive GCD by keeping the minimum exponents of every
       base */
    may_size_t igcd = 0, wgcd = 0;
    while (igcd < ngcd && may_product_iterator_end2 (&cel, &bel, it)) {
      int j = may_identical (gcdtab[igcd], bel);
      if (j == 0) {
        gcdpowtab[wgcd] = may_naive_gce (gcdpowtab[igcd], cel);
        if (gcdpowtab[wgcd] != MAY_ZERO) {
          gcdtab[wgcd] = gcdtab[igcd];
          wgcd ++;
        }
        igcd++;
        MAY_ASSERT (wgcd <= igcd);
        may_product_iterator_next (it);
      } else if (j < 0)
        igcd++;
      else
        may_product_iterator_next (it);
    }
    ngcd = wgcd;
  }

  /* Convert num*gcdtab[]^powtab[] to an expression */
  may_t gcd;
  if (MAY_UNLIKELY (ngcd > 0)) {
    gcd = MAY_NODE_C (MAY_PRODUCT_T, ngcd+1);
    for (may_size_t i = 0; MAY_LIKELY (i<ngcd); i++) {
      MAY_SET_AT (gcd, i, may_pow_c (gcdtab[i], gcdpowtab[i]));
    }
    MAY_SET_AT(gcd, ngcd, gcdnum);
  } else
    gcd = gcdnum;
  MAY_RET_EVAL (gcd);
}

/* Return the naive factorisation x (naive means quick and dirty) */
may_t
may_naive_factor (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  switch (MAY_TYPE (x))
    {
    case MAY_SUM_T:
      {
        may_t gcd, y;
        may_size_t n = MAY_NODE_SIZE(x), i;
        MAY_RECORD ();
        y = MAY_NODE_C (MAY_SUM_T, n);
        for (i =0; MAY_LIKELY (i<n); i++) {
          may_t z = may_naive_factor (MAY_AT (x, i));
          MAY_SET_AT (y, i , z);
        }
        gcd = may_naive_gcd (MAY_NODE_SIZE(y), MAY_AT_PTR (y, 0));
        for (i =0; MAY_LIKELY (i<n); i++) {
          may_t z = may_divexact (MAY_AT (y, i), gcd);
          MAY_SET_AT (y, i , z);
        }
        y = may_mul_c (gcd, y);
        MAY_RET_EVAL (y); /* Miss combine */
      }
    case MAY_PRODUCT_T:
      {
        may_size_t n = MAY_NODE_SIZE(x), i;
        may_t y;
        int isnew = 0;
        MAY_RECORD ();
        y = MAY_NODE_C (MAY_PRODUCT_T, n);
        for (i = 0; MAY_LIKELY (i < n); i++) {
          may_t z = may_naive_factor (MAY_AT (x, i));
          MAY_SET_AT (y, i, z);
          isnew |= (z != MAY_AT (x, i));
        }
        if (!isnew) {
          MAY_CLEANUP ();
          return x;
        }
        MAY_RET_EVAL (y);
      }
    case MAY_FACTOR_T:
      {
        may_t base, y;
        MAY_RECORD ();
        base = may_naive_factor (MAY_AT (x, 1));
        if (base == MAY_AT (x, 1)) {
          MAY_CLEANUP ();
          return x;
        }
        y = may_mul_c (base, MAY_AT (x, 0));
        MAY_RET_EVAL (y);
      }
    case MAY_POW_T:
      {
        may_t base, y;
        MAY_RECORD ();
        base = may_naive_factor (MAY_AT (x, 0));
        if (base == MAY_AT (x, 0)) {
          MAY_CLEANUP ();
          return x;
        }
        y = may_pow_c (base, MAY_AT (x, 1));
        MAY_RET_EVAL (y);
      }
    default:
      return x;
    }
}
