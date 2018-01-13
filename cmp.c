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

/* FAST Sorting:
   Return 0 if x, y are structurarly identical
   -1 if x < y and 1 if y > x (structuraly)  */
MAY_REGPARM int
may_identical (may_t x, may_t y)
{
  int diff;
  may_type_t t;
  may_size_t n;

  MAY_ASSERT (MAY_EVAL_P (x) && MAY_EVAL_P (y));
  MAY_ASSERT (MAY_TYPE (x) != MAY_INDIRECT_T && MAY_TYPE(y) != MAY_INDIRECT_T);

  /* Fast compare (20%+43%~60% of the cases) */
  diff = (int) MAY_HASH (x) - (int) MAY_HASH (y);
  if (MAY_LIKELY (diff || x == y)) {
    /* If x == y, diff = 0 */
    MAY_ASSERT (x == y || may_recompute_hash (x) != may_recompute_hash (y));
    return diff;
  }

  /* Same Hash: must compare recursevely but x & y are likely equals
     (~83% of the remaining cases)*/
  t = MAY_TYPE(x);
  diff = t - MAY_TYPE(y);
  if (MAY_UNLIKELY (diff))
    return diff;
  switch (t)
    {
    case MAY_INT_T: /* 25 % */
      return mpz_cmp (MAY_INT(x), MAY_INT(y));
    case MAY_RAT_T:
      return mpq_cmp (MAY_RAT(x), MAY_RAT(y));
    case MAY_FLOAT_T:
      return MAY_UNLIKELY (mpfr_nan_p (MAY_FLOAT(x))) ?
	(mpfr_nan_p (MAY_FLOAT(y)) ? 0 : -1) :
	(MAY_UNLIKELY (mpfr_nan_p (MAY_FLOAT(y))) ? 1 :
	 mpfr_cmp (MAY_FLOAT(x), MAY_FLOAT(y)));
    case MAY_COMPLEX_T:
      diff = may_identical (MAY_RE (x), MAY_RE (y));
      if (MAY_UNLIKELY (diff))
        return diff;
      return may_identical (MAY_IM (x), MAY_IM (y));
    case MAY_STRING_T: /* 21% */
      /* The cast from a size_t which defines the size of a symbol (a string)
         to an int is unlikely to failed (it would mean that the string size
         is bigger than an int */
      n = MAY_SYMBOL_SIZE(x);
      diff = n - MAY_SYMBOL_SIZE(y);
      if (MAY_UNLIKELY (diff))
        return diff;
      /* The strings were expanded to an aligned size so that we can do a
         long compare (faster) */
      MAY_ASSERT (n % sizeof (long) == 0);
      n /= sizeof (long);
      do {
        n--;
        if (MAY_UNLIKELY (((long*)MAY_NAME(x))[n] != ((long*)MAY_NAME(y))[n]))
          return (((long*)MAY_NAME(x))[n] > ((long*)MAY_NAME(y))[n]) ? 1 : -1;
      } while (MAY_UNLIKELY (n));
      break;
      /* return memcmp (MAY_NAME(x), MAY_NAME(y), MAY_SYMBOL_SIZE(x)); */
    case MAY_DATA_T:
      if (MAY_UNLIKELY (MAY_DATA (x).size != MAY_DATA (y).size))
        return MAY_DATA (x).size > MAY_DATA (y).size ? 1 : -1 ;
      return memcmp (MAY_DATA (x).data, MAY_DATA (y).data, MAY_DATA (x).size);
    default: /* 54% */
      n = MAY_NODE_SIZE(x);
      if (MAY_UNLIKELY (n != MAY_NODE_SIZE(y)))
	return n > MAY_NODE_SIZE(y) ? 1 : -1;
      MAY_ASSERT (n >= 1);
      do {
        n--;
        diff = may_identical (MAY_AT(x,n), MAY_AT(y,n));
        if (MAY_UNLIKELY (diff))
          return diff;
      } while (n);
    }
  return 0;
}

/* If we have overloaded may_identical for internal calls to use
   REGPARM, define a proper standard parm version */
#ifdef may_identical
#undef may_identical
int
may_identical (may_t x, may_t y)
{
  return may_identical_internal (x, y);
}
#endif

/* Sort according according to LEXICOGRAPHICAL order */
int
may_cmp (may_t x, may_t y)
{
  int diff, doit;
  may_type_t tx, ty;
  may_size_t i, n, sx, sy;
  may_t *px, *py;

  tx = MAY_TYPE (x);
  ty = MAY_TYPE (y);

  /* First, we don't care about wrong sorting 2.x and 3.x */
  if (tx == MAY_FACTOR_T) {
    x = MAY_AT (x, 1);
    tx = MAY_TYPE (x);
  }
  if (ty == MAY_FACTOR_T) {
    y = MAY_AT (y, 1);
    ty = MAY_TYPE (y);
  }

  /* Check for MONOMIAL */
  doit = 0; sx = 1; px = &x; sy = 1; py = &y;
  if (tx == MAY_POW_T && MAY_TYPE(MAY_AT(x, 1)) == MAY_INT_T)
    doit = 1;
  else if (tx == MAY_PRODUCT_T) {
    sx = MAY_NODE_SIZE(x);
    px = MAY_AT_PTR (x, 0);
    doit = 1;
  }
  if (ty == MAY_POW_T && MAY_TYPE(MAY_AT(y, 1)) == MAY_INT_T)
    doit = 1;
  else if (ty == MAY_PRODUCT_T) {
    sy = MAY_NODE_SIZE(y);
    py = MAY_AT_PTR (y, 0);
    doit = 1;
  }

  /* Check if we have to sort monomial? */
  if (doit == 0) {
    /* Classic way of sorting */
    diff = tx - ty;
    if (diff)
      return diff;
    switch (tx)
      {
      case MAY_INT_T:
        return mpz_cmp (MAY_INT(x), MAY_INT(y));
      case MAY_RAT_T:
        return mpq_cmp (MAY_RAT(x), MAY_RAT(y));
      case MAY_FLOAT_T:
        return mpfr_nan_p (MAY_FLOAT(x)) ?
          (mpfr_nan_p (MAY_FLOAT(y)) ? 0 : -1) :
          (mpfr_nan_p (MAY_FLOAT(y)) ? 1 :
           mpfr_cmp (MAY_FLOAT(x), MAY_FLOAT(y)));
      case MAY_COMPLEX_T:
        diff = may_identical (MAY_RE(x), MAY_RE(y));
        return diff ? diff : may_identical (MAY_IM(x), MAY_IM(y));
      case MAY_STRING_T:
        return strcmp (MAY_NAME(x), MAY_NAME(y));
      case MAY_DATA_T:
        diff = MAY_DATA (x).size - MAY_DATA (y).size;
        if (MAY_UNLIKELY (diff))
          return diff;
        return memcmp (MAY_DATA (x).data, MAY_DATA (y).data, MAY_DATA(x).size);
      default:
        n = MAY_NODE_SIZE(x);
        diff = n - MAY_NODE_SIZE(y);
        if (diff)
          return diff;
        for(i = 0 ; MAY_LIKELY (i < n); i++) {
          diff = may_cmp (MAY_AT(x,i), MAY_AT(y,i));
          if (diff)
            return diff;
        }
      }
  } else {
    /* BUG: The variables may not be in the right order, ie
       from x to z... in the product node */
    /* Sort Monomials in Lexicographic order */
    for ( ; sx > 0 && sy > 0 ; sx--, sy--, px++, py++) {
      may_t xbase, ybase, xpow, ypow;
      /* Tansform X and Y in xbase^xpow and ybase^ypow */
      x = *px;
      y = *py;
      if (MAY_TYPE (x) == MAY_POW_T
          && MAY_TYPE (MAY_AT(x,1)) == MAY_INT_T) {
        xpow = MAY_AT(x, 1);
        xbase = MAY_AT(x, 0);
      } else {
        xpow  = MAY_ONE;
        xbase = x;
      }
      if (MAY_TYPE (y) == MAY_POW_T
          && MAY_TYPE (MAY_AT (y,1)) == MAY_INT_T) {
        ypow = MAY_AT(y, 1);
        ybase = MAY_AT(y, 0);
      } else {
        ypow  = MAY_ONE;
        ybase = y;
      }
      /* Then compare base, and after pow */
      diff = may_cmp (xbase, ybase);
      if (diff)
        return diff;
      diff = may_cmp (ypow, xpow); /* X^2 < X !!! */
      if (diff)
        return diff;
    }
    if (sx > 0) /* X^2*Y < X^2 */
      return -1;
    if (sy > 0) /* X^2   > X^2*Y */
      return 1;
  }
  return 0;
}
