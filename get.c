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

int
may_get_ui (unsigned long *val, may_t x)
{
  if (MAY_TYPE (x) == MAY_INT_T) {
    if (mpz_fits_ulong_p (MAY_INT (x))) {
      *val = mpz_get_ui (MAY_INT (x));
      return 0;
    } else return -2;   /* Overflow */
  } else if (MAY_TYPE (x) == MAY_FLOAT_T) {
    if (mpfr_fits_ulong_p (MAY_FLOAT (x), GMP_RNDD)) {
      *val = mpfr_get_ui (MAY_FLOAT (x), GMP_RNDD);
      return 1;  /* Ok but a rounding occurs */
    }
    else return -2;  /* Overflow */
  } else if (MAY_TYPE (x) == MAY_RAT_T) {
    mpfr_t f;
    mpfr_init2 (f, CHAR_BIT*sizeof (unsigned long));
    mpfr_set_q (f, MAY_RAT(x), GMP_RNDD);
    if (mpfr_fits_ulong_p (f, GMP_RNDD)) {
      *val = mpfr_get_ui (f, GMP_RNDD);
      return 1;  /* Ok but a rounding occurs */
    }
    else return -2;  /* Overflow */
  } else return -1;       /* Can't convert */
}

int
may_get_si (long *val, may_t x)
{
  if (MAY_TYPE(x) == MAY_INT_T) {
    if (mpz_fits_slong_p (MAY_INT (x))) {
      *val = mpz_get_si (MAY_INT (x));
      return 0;           /* Ok */
    } else return -2;     /* Overflow */
  } else if (MAY_TYPE (x) == MAY_FLOAT_T) {
    if (mpfr_fits_slong_p (MAY_FLOAT(x), GMP_RNDD)) {
      *val = mpfr_get_si (MAY_FLOAT(x), GMP_RNDD);
      return 1;           /* Ok but a rounding occurs */
    } else return -2;     /* Overflow */
  } else if (MAY_TYPE (x) == MAY_RAT_T) {
      mpfr_t f;
      mpfr_init2 (f, CHAR_BIT*sizeof (long));
      mpfr_set_q (f, MAY_RAT(x), GMP_RNDD);
      if (mpfr_fits_slong_p (f, GMP_RNDD)) {
        *val = mpfr_get_si (f, GMP_RNDD);
        return 1;         /* Ok but a rounding occurs */
      } else return -2;   /* Overflow */
  } else return -1;       /* Can't convert */
}

int
may_get_str (const char **str, may_t x)
{
  if (MAY_TYPE (x) != MAY_STRING_T)
    return -1;            /* Invalid type */
  *str = MAY_NAME (x);
  return 0;
}


int
may_get_d (double *d, may_t x)
{
  if (MAY_TYPE(x) == MAY_INT_T)
    {
      *d = mpz_get_d (MAY_INT (x));
      return 0;
    }
  else if (MAY_TYPE(x) == MAY_FLOAT_T)
    {
      *d = mpfr_get_d (MAY_FLOAT(x), MAY_RND);
      return 0;
    }
  else if (MAY_TYPE(x) == MAY_RAT_T)
    {
      mpfr_t f;
      mpfr_init2 (f, CHAR_BIT*sizeof (double));
      mpfr_set_q (f, MAY_RAT(x), MAY_RND);
      *d = mpfr_get_d (f, MAY_RND);
      return 0;
    }
  else
    return -1;       /* Can't convert */
}

int
may_get_z (mpz_t z, may_t x)
{
  if (MAY_LIKELY (MAY_TYPE(x) == MAY_INT_T)) {
    mpz_set (z, MAY_INT (x));
    return 0;
  } else if (MAY_TYPE (x) == MAY_FLOAT_T
	     && !mpfr_nan_p (MAY_FLOAT (x))
	     && !mpfr_inf_p (MAY_FLOAT (x))) {
    mpfr_get_z (z, MAY_FLOAT(x), GMP_RNDD);
    return 1;           /* Ok but a rounding occurs */
  } else if (MAY_TYPE (x) == MAY_RAT_T
	     && mpz_sgn (mpq_denref (MAY_RAT (x))) != 0) {
    mpz_fdiv_q (z, mpq_numref (MAY_RAT (x)), mpq_denref (MAY_RAT (x)));
    return 1;         /* Ok but a rounding occurs */
  } else
    return -1;       /* Can't convert */
}

int
may_get_q (mpq_t q, may_t x)
{
  if (MAY_TYPE (x) == MAY_RAT_T) {
    mpq_set (q, MAY_RAT (x));
    return 0;
  }
  else if (MAY_TYPE (x) == MAY_INT_T) {
    mpq_set_z (q, MAY_INT(x));
    return 0;
  }
  return -1;
}

int
may_get_fr (mpfr_t f, may_t x)
{
  if (MAY_TYPE (x) == MAY_FLOAT_T) {
    mpfr_set (f, MAY_FLOAT (x), MAY_RND);
    return 0;
  } else if (MAY_TYPE (x) == MAY_INT_T) {
    mpfr_set_z (f, MAY_INT (x), MAY_RND);
    return 1;
  } else if (MAY_TYPE (x) == MAY_RAT_T) {
    mpfr_set_q (f, MAY_RAT (x), MAY_RND);
    return 1;
  } else
    return -1;
}

int
may_get_cx (may_t *r, may_t *i, may_t x)
{
  if (MAY_UNLIKELY (MAY_TYPE (x) >= MAY_NUM_LIMIT))
    return -1; /* Invalid type */
  else if (MAY_UNLIKELY (MAY_TYPE (x) != MAY_COMPLEX_T)) {
    *r = x;
    *i = MAY_ZERO;
    return 0;
  }
  *r = MAY_RE (x);
  *i = MAY_IM (x);
  return 0;
}

/* return non evaluated arg */
may_t
may_map_c (may_t x, may_t (*func)(may_t))
{
  may_size_t i, n;
  may_t y;

  if (MAY_UNLIKELY (MAY_ATOMIC_P (x)))
    return (*func) (x);

  n = MAY_NODE_SIZE(x);
  y = MAY_NODE_C (MAY_TYPE (x), n);
  for (i = 0 ; MAY_LIKELY (i < n); i++)
    MAY_SET_AT (y, i, (*func) (MAY_AT (x, i)));
  return y;
}

may_t
may_map (may_t x, may_t (*func)(may_t))
{
  return may_eval (may_map_c (x, func));
}

/* return non evaluated arg */
may_t
may_map2_c (may_t x, may_t (*func)(may_t,void*), void *data)
{
  may_size_t i, n;
  may_t y;

  if (MAY_UNLIKELY (MAY_ATOMIC_P (x)))
    return (*func) (x,data);

  n = MAY_NODE_SIZE(x);
  y = MAY_NODE_C (MAY_TYPE (x), n);
  for (i = 0 ; MAY_LIKELY (i < n); i++)
    MAY_SET_AT (y, i, (*func) (MAY_AT (x, i), data));
  return y;
}

may_t
may_map2 (may_t x, may_t (*func)(may_t, void*), void *data)
{
  return may_eval (may_map2_c (x, func, data));
}
