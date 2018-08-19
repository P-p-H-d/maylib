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

/* Divide y by x exactly in Q[X].
   Returns NULL if y doesn't divide x */
may_t
may_divexact (may_t x, may_t y)
{
  may_t *px, *py;
  may_t numx, numy, z;
  may_size_t sx, sy, sz;
  may_t tab[2];
  may_mark_t org_mark;

  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (MAY_EVAL_P (y));

  MAY_LOG_FUNC (("x='%Y' y='%Y'",x,y));

  may_mark (org_mark);

  /* Special case */
  if (MAY_UNLIKELY (MAY_PURENUM_P (y))) {
    /* Catch other commun special cases */
    if (MAY_ONE_P (y))
      return x;
    if (MAY_PURENUM_P (x)) {
      numx = may_num_div (MAY_DUMMY, x, y);
      numx = may_num_simplify (numx);
      return numx;
    }
    if (MAY_ZERO_P (y))
      return MAY_NAN; /* FIXME: Or NULL ? */
    return may_eval (may_div_c (x, y));
  }
  else if (MAY_UNLIKELY (MAY_PURENUM_P (x))) {
    return MAY_ZERO_P (x) ? x : NULL; /* FAILED since y is not a pure numerical (except for 0) */
  } else if (MAY_UNLIKELY (may_identical (x, y) == 0))
    return MAY_ONE;

  /* Fill in tab before destroying x or y */
  tab[0] = x;
  tab[1] = y;

  /* If we divide by a sum, it is likely that
     it becomes too complicated to be handled in a simple way.
     Fall back to the generic division (which introduces a circular
     definition between may_divexact and may_divqr) */
  if (MAY_UNLIKELY (MAY_TYPE (y) == MAY_SUM_T && MAY_TYPE (x) == MAY_SUM_T)) {
    may_t quo, rem, var;
  use_divqr:
    MAY_LOG_MSG (("fall back to may_div_qr\n"));
    /* Restore original state of the memory */
    may_compact (org_mark, NULL);
    var = may_find_one_polvar (2, tab);
    if (MAY_UNLIKELY (var == NULL))
      return NULL;

    /* We may get something which is not a var!
       And we may get something which can't make the
       original expression view as a univariate polynomial
       ==> Always replace with a temp variable */
    may_t local = may_set_str_local (MAY_COMPLEX_D);
    may_t newx  = may_replace_upol (tab[0], var, local);
    may_t newy  = may_replace_upol (tab[1], var, local);
    /* Process the division */
    int b = may_div_qr (&quo, &rem, newx, newy, local);
    /* Check the remainder is zero */
    if (MAY_UNLIKELY (!b || rem != MAY_ZERO))
      return NULL;
    /* Replace back the local var by its original value */
    quo = may_replace_upol (quo, local, var);
    return quo;
  }

  /* If x is a sum, and y is not, just tries to divide
     each term of x by y, otherwise fall back to CPU
     hungry code */
  else if (MAY_UNLIKELY (MAY_TYPE (x) == MAY_SUM_T)) {
    sx = MAY_NODE_SIZE(x);
    z = MAY_NODE_C (MAY_SUM_T, sx);
    MAY_ASSERT (sx > 0);
    /* TO PARALELIZE */
    for (sz = 0; sz<sx; sz++) {
      may_t alpha = may_divexact (MAY_AT (x, sz), y);
      if (MAY_UNLIKELY (alpha == NULL))
        goto use_divqr;
      MAY_SET_AT (z, sz, alpha);
    }
    return may_eval (z);
  }


  /* Extract the monomial from x */
  numx = MAY_ONE;
  if (MAY_LIKELY (MAY_TYPE (x) == MAY_FACTOR_T)) {
    numx = MAY_AT (x, 0);
    x = MAY_AT (x, 1);
  }
  if (MAY_LIKELY (MAY_TYPE (x) == MAY_PRODUCT_T)) {
    sx = MAY_NODE_SIZE(x);
    px = MAY_AT_PTR (x, 0);
  } else {
    sx = 1;
    px = &x;
  }

  /* Extract the monomial from y */
  numy = MAY_ONE;
  if (MAY_UNLIKELY (MAY_TYPE (y) == MAY_FACTOR_T)) {
    numy = MAY_AT (y, 0);
    y = MAY_AT (y, 1);
  }
  if (MAY_LIKELY (MAY_TYPE (y) == MAY_PRODUCT_T)) {
    sy = MAY_NODE_SIZE(y);
    py = MAY_AT_PTR (y, 0);
  } else {
    sy = 1;
    py = &y;
  }

  /* Optimize commun case where only the numerical term differs */
  if (may_identical (x, y) == 0) {
    if (numy != MAY_ONE) {
      numx = may_num_div (MAY_DUMMY, numx, numy);
      numx = may_num_simplify (numx);
    }
    return numx;
  }

  /* Divide the monomial X^B / Y^A */
  z = MAY_NODE_C (MAY_PRODUCT_T, sx+1);
  MAY_ASSERT (sx > 0 && sy > 0);
  for (sz = 0; sx > 0 && sy > 0; px++, sx--) {
    may_t bx, by, cx, cy, tmp;
    /* Extract the power and get the base */
    if (MAY_LIKELY (MAY_TYPE (*px) == MAY_POW_T)) {
      bx = MAY_AT (*px, 0);
      cx = MAY_AT (*px, 1);
    } else {
      bx = *px;
      cx = MAY_ONE;
    }
    if (MAY_LIKELY (MAY_TYPE (*py) == MAY_POW_T)) {
      by = MAY_AT (*py, 0);
      cy = MAY_AT (*py, 1);
    } else {
      by = *py;
      cy = MAY_ONE;
    }
    /* Check if the terms divide */
    tmp = *px;
    if (MAY_LIKELY (may_identical (bx, by) == 0)) {
      py++, sy--;
      if (MAY_LIKELY (may_identical (cx, cy) == 0))
        continue; /* Optimize bx^0 to 1 */
      tmp = may_pow_c (bx, may_sub_c (cx, cy));
    }
    MAY_SET_AT (z, sz++, tmp);
  }

  /* y must divise x, otherwise there is an error  */
  if (MAY_UNLIKELY (sy != 0)) {
    goto use_divqr;
  }
  /* Copy the remaining terms */
  for ( ; sx > 0; sx--, px++)
    MAY_SET_AT (z, sz++, *px);

  /* Compute the num part */
  if (MAY_UNLIKELY (numy != MAY_ONE)) {
    numx = may_num_div (MAY_DUMMY, numx, numy);
    numx = may_num_simplify (numx);
  }
  if (MAY_LIKELY (numx != MAY_ONE))
    MAY_SET_AT (z, sz++, numx);
  /* Size of the product */
  MAY_NODE_SIZE(z) = sz;

  /* Return the result */
  MAY_ASSERT (sz != 0);
  return may_eval (z);
}
