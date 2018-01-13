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

static void
rectform (may_t *r, may_t *i, may_t x)
{
  may_t y, t, x2, y2;
  may_size_t j, n;

  MAY_ASSERT (MAY_EVAL_P (x));

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
  case MAY_RAT_T:
  case MAY_FLOAT_T:
  case MAY_REAL_T:
  case MAY_IMAG_T:
  case MAY_ABS_T:
  case MAY_ARGUMENT_T:
    *r = x;
    *i = MAY_ZERO;
    return; /* No need to eval the args */
    break;
  case MAY_COMPLEX_T:
    *r = MAY_RE (x);
    *i = MAY_IM (x);
    return; /* No need to eval the args */
  case MAY_SUM_T:
    n = MAY_NODE_SIZE(x);
    *r = MAY_NODE_C (MAY_SUM_T, n);
    *i = MAY_NODE_C (MAY_SUM_T, n);
    MAY_ASSERT (n > 1);
    for (j = 0; MAY_LIKELY (j<n); j++)
      may_rectform (MAY_AT_PTR (*r, j), MAY_AT_PTR (*i, j), MAY_AT (x, j));
    break;
  case MAY_FACTOR_T:
    y = MAY_AT (x, 0);
    may_rectform (r, i, MAY_AT (x, 1));
    if (MAY_TYPE (MAY_AT (x, 0)) != MAY_COMPLEX_T) {
      *r = may_mul_c (y, *r);
      *i = may_mul_c (y, *i);
    } else { /* Complex * complex */
      t  = may_sub_c (may_mul_c (MAY_RE (y), *r), may_mul_c (MAY_IM (y), *i));
      *i = may_add_c (may_mul_c (MAY_RE (y), *i), may_mul_c (MAY_IM (y), *r));
      *r = t;
    }
    break;
  case MAY_PRODUCT_T:
    n = MAY_NODE_SIZE(x);
    MAY_ASSERT (n > 1);
    may_rectform (r, i, MAY_AT (x, 0));
    for (j = 1; j < n; j++) {
      may_t rr, ii;
      may_rectform (&rr, &ii, MAY_AT (x, j));
      t  = may_sub_c (may_mul_c (rr, *r), may_mul_c (ii, *i));
      *i = may_add_c (may_mul_c (rr, *i), may_mul_c (ii, *r));
      *r = t;
    }
    break;
  case MAY_EXP_T:
    /* exp(a+Ib) = exp(a)cos(b)+Iexp(a)sin(b) */
    may_rectform (r, i, MAY_AT (x, 0));
    y  = may_exp_c (*r);
    *r = may_mul_c (y, may_cos_c (*i));
    *i = may_mul_c (y, may_sin_c (*i));
    break;
  case MAY_LOG_T:
    /* log(a+Ib) = log(abs(x)) + I*argument(x) */
    *r = may_log_c (may_abs_c (MAY_AT (x, 0)));
    *i = may_argument_c (MAY_AT (x, 0));
    break;
  case MAY_COS_T:
    /* cos(a+Ib) = cos(a)cosh(b)-Isin(a)sinh(b) */
    may_rectform (r, i, MAY_AT (x, 0));
    t  = may_mul_c (may_cos_c (*r), may_cosh_c (*i));
    *i = may_mul_vac (MAY_N_ONE, may_sin_c (*r), may_sinh_c (*i), NULL);
    *r = t;
    break;
  case MAY_SIN_T:
    /* sin(a+Ib) = sin(a)cosh(b)+Icos(a)sinh(b) */
    may_rectform (r, i, MAY_AT (x, 0));
    t  = may_mul_c (may_sin_c (*r), may_cosh_c (*i));
    *i = may_mul_c (may_cos_c (*r), may_sinh_c (*i));
    *r = t;
    break;
  case MAY_TAN_T:
    /* tan(x+Iy) = (sin(2x) +I*sinh(2*y))/(cos(2*x)+cosh(2*y)) */
    may_rectform (&x2, &y2, MAY_AT (x, 0));
    x2 = may_mul (MAY_TWO, x2);
    y2 = may_mul (MAY_TWO, y2);
    t = may_add_c (may_cos_c (x2), may_cosh_c (y2));
    *r = may_div_c (may_sin_c (x2), t);
    *i = may_div_c (may_sinh_c (y2), t);
    break;
  case MAY_SINH_T:
    /* sinh(x+I*y) = sinh(x) cos(y) + cosh(x) sin(y) I */
    may_rectform (r, i, MAY_AT (x, 0));
    t  = may_mul_c (may_sinh_c (*r), may_cos_c (*i));
    *i = may_mul_c (may_cosh_c (*r), may_sin_c (*i));
    *r = t;
    break;
  case MAY_COSH_T:
    /* cosh(x+I*Y) = cosh(x) cos(y) + sinh(x) sin(y) I */
    may_rectform (r, i, MAY_AT (x, 0));
    t  = may_mul_c (may_cosh_c (*r), may_cos_c (*i));
    *i = may_mul_c (may_sinh_c (*r), may_sin_c (*i));
    *r = t;
    break;
  case MAY_TANH_T:
    /* tanh(x+I*y) = (sinh(2*X) + I* sin(2*y)) / (cosh(2*x)+cos(2*y)) */
    may_rectform (&x2, &y2, MAY_AT (x, 0));
    x2 = may_mul (MAY_TWO, x2);
    y2 = may_mul (MAY_TWO, y2);
    t = may_add_c (may_cosh_c (x2), may_cos_c (y2));
    *r = may_div_c (may_sinh_c (x2), t);
    *i = may_div_c (may_sin_c (y2), t);
    break;
    /* TODO: More func atrig */
  case MAY_POW_T:
    y = MAY_AT (x, 1);
    if (MAY_TYPE (y) == MAY_INT_T && mpz_fits_sshort_p (MAY_INT (y))) {
      may_t rr, ii;
      long j, m, n = mpz_get_si (MAY_INT (y));
      /* (rr,ii) = rectform (base) */
      may_rectform (&rr, &ii, MAY_AT (x, 0));
      if (n < 0) {
        /* Inverse base: 1/(C+I*D) = (C-I*D)/(C^2+D^2) */
        t  = may_add_c (may_sqr_c (rr), may_sqr_c (ii));
        rr = may_div_c (rr, t);
        ii = may_neg_c (may_div_c (ii, t));
        n = -n;
      }
      for (j = 0, m = n; m != 0; j++)
        m >>= 1;
      *r = rr, *i = ii;
      for (j -= 2; j >= 0; j--) {
        /* Squaring (r,i) */
        t  = may_sub_c (may_sqr_c (*r), may_sqr_c (*i));
        *i = may_mul_vac (MAY_TWO, *r, *i, NULL);
        *r = t;
        if (n & (1UL << j)) {
          /* (r,i) = (r,i) * (rr,ii) */
          t  = may_sub_c (may_mul_c (rr, *r), may_mul_c (ii, *i));
          *i = may_add_c (may_mul_c (rr, *i), may_mul_c (ii, *r));
          *r = t;
        }
      }
      break;
    }
    /* Else fall down */
  case MAY_STRING_T:
  default:
    if (MAY_UNLIKELY (may_real_p (x))) {
      *r = x;
      *i = MAY_ZERO;
      return; /* No need to eval the args */
    }
    *r = may_real_c (x);
    *i = may_imag_c (x);
    break;
  }

  *r = may_eval (*r);
  *i = may_eval (*i);
}

void
may_rectform (may_t *r, may_t *i, may_t x)
{
  may_t temp[2];

  MAY_ASSERT (MAY_EVAL_P (x));
  may_mark ();
  /* Compute and ... */
  rectform(&temp[0], &temp[1], x);
  /* compact */
  may_compact_v (2, temp);
  *r = temp[0];
  *i = temp[1];

  return;
}
