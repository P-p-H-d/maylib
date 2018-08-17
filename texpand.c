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

static may_t cos_expand  (may_t x);
static may_t sin_expand  (may_t x);
static may_t tan_expand  (may_t x);
static may_t cosh_expand (may_t x);
static may_t sinh_expand (may_t x);
static may_t tanh_expand (may_t x);

/* Construct the tchebycheff polynomial */
static void
tchebycheff (long n, long b[n+1], int first_kind)
{
  long a[n+1], c[n+1];
  long i, k, t;

  MAY_ASSERT (n >= 1);

  memset (a, 0, sizeof a);
  memset (b, 0, sizeof a);
  memset (c, 0, sizeof a);

  a[0] = 1;
  if (first_kind) {
    /* T0 = 1   T1=X */
    b[1] = 1;
  } else {
    /* U0 = 1   U1 = 2*X */
    b[1] = 2;
  }

  t = 2;
  for (k = 1; k < n; k++) {
    /* Un+1 = 2*X*Un -Un-1 */
    c[0] = -a[0];
    for (i = 0; i < t; i++)
      c[i+1] = 2*b[i] - a[i+1];
    t++;
    MAY_ASSERT (t <= n+1);
    memcpy (a, b, t*sizeof (long));
    memcpy (b, c, t*sizeof (long));
  }

  return;
}

/* Front ent to tchebycheff */
static may_t
tchebycheff_var (long n, int first_kind, int askabs, may_t arg)
{
  MAY_ASSERT (n >= 1);
  long coeff[n+1];

  tchebycheff (n, coeff, first_kind);
  may_t r = MAY_NODE_C (MAY_SUM_T, n+1);
  long i;
  for (i = 0; i <= n; i++)
    MAY_SET_AT (r, i, may_mul_c (may_set_si (askabs ? labs (coeff[i]) : coeff[i] ),
                                 may_pow_si_c (arg, i)));
  return may_eval (r);
}


/* Expand sin:
   - sin(A+B)=sin(A)*cos(B) + cos(A)*sin(B) */
static may_t
sin_expand (may_t x)
{
  may_t a, b;
  if (may_sum_extract(&a, &b, x)) {
    return may_add_c (may_mul_c (sin_expand (a), cos_expand (b)),
                      may_mul_c (cos_expand (a), sin_expand (b)));
  }
  if (may_product_extract(&a, &b, x)) {
    if (MAY_TYPE (a) == MAY_INT_T
        && mpz_fits_sshort_p (MAY_INT (a))) {
      long n = mpz_get_si (MAY_INT (a)), m;
      may_t arg = may_sin (b);
      int first_kind;
      int sign = 0;
      MAY_ASSERT (n != 0);
      if (n < 0) {
        sign = 1;
        n = -n;
      }
      if (MAY_UNLIKELY (n == 1))
        return may_neg (arg);
      MAY_ASSERT (n >= 2);
      if ((n & 1) == 0) {
        m = n - 1;
        first_kind = 0;
      } else {
        m = n;
        first_kind = 1;
      }
      arg = tchebycheff_var (m, first_kind, 0, arg);
      if ( (((n+1)&3) <= 1 && sign == 0)
         || (((n+1)&3) >= 2 && sign == 1))
        arg = may_neg_c (arg);
      if ( (n&1) == 0)
        arg = may_mul_c (arg, may_cos_c (MAY_AT (x, 1)));
      return may_eval (arg);
    }
  }
  return may_sin_c (x);
}

/* Expand cos */
static may_t
cos_expand (may_t x)
{
  if (MAY_TYPE (x) == MAY_SUM_T) {
    may_t first  = MAY_AT (x, 0);
    may_t last   = may_eval (may_add_vc (MAY_NODE_SIZE(x)-1, MAY_AT_PTR (x, 1)));
    return may_sub_c (may_mul_c (cos_expand (first), cos_expand (last)),
                      may_mul_c (sin_expand (first), sin_expand (last)));
  } else if (MAY_TYPE (x) == MAY_FACTOR_T
             && MAY_TYPE (MAY_AT (x, 0)) == MAY_INT_T
             && mpz_fits_sshort_p (MAY_INT (MAY_AT (x, 0)))) {
    long n = labs (mpz_get_si (MAY_INT (MAY_AT (x, 0))));
    may_t arg = may_cos (MAY_AT (x, 1));
    if (MAY_UNLIKELY (n == 1))
      return arg;
    MAY_ASSERT (n >= 2);
    arg = tchebycheff_var (n, 1, 0, arg);
    return may_eval (arg);
  } else
    return may_cos_c (x);
}

/* Hyperbolic functions are nearly identical */

/* sinh_expand is nearly identical to sin_expand.
   We don't inverse the sign for (n+1)%3 <= 1. */
static may_t
sinh_expand (may_t x)
{
  if (MAY_TYPE (x) == MAY_SUM_T) {
    may_t first  = MAY_AT (x, 0);
    may_t last   = may_eval (may_add_vc (MAY_NODE_SIZE(x)-1, MAY_AT_PTR (x, 1)));
    return may_add_c (may_mul_c (sinh_expand (first), cosh_expand (last)),
                      may_mul_c (cosh_expand (first), sinh_expand (last)));
  } else if (MAY_TYPE (x) == MAY_FACTOR_T
             && MAY_TYPE (MAY_AT (x, 0)) == MAY_INT_T
             && mpz_fits_sshort_p (MAY_INT (MAY_AT (x, 0)))) {
    long n = mpz_get_si (MAY_INT (MAY_AT (x, 0))), m;
    may_t arg = may_sinh (MAY_AT (x, 1));
    int first_kind;
    int sign = 0;
    MAY_ASSERT (n != 0);
    if (n < 0) {
      sign = 1;
      n = -n;
    }
    if (MAY_UNLIKELY (n == 1))
      return may_neg (arg);
    MAY_ASSERT (n >= 2);
    if ((n & 1) == 0) {
      m = n - 1;
      first_kind = 0;
    } else {
      m = n;
      first_kind = 1;
    }
    arg = tchebycheff_var (m, first_kind, 1, arg);
    if (sign == 1)
      arg = may_neg_c (arg);
    if ( (n&1) == 0)
      arg = may_mul_c (arg, may_cosh_c (MAY_AT (x, 1)));
    return may_eval (arg);
  } else
    return may_sinh_c (x);
}

/* It is identical to cos_expand except that
   cosh(a+b) = cosh(a)*cosh(b)+sinh(a)+sinh(b) */
static may_t
cosh_expand (may_t x)
{
  if (MAY_TYPE (x) == MAY_SUM_T) {
    may_t first  = MAY_AT (x, 0);
    may_t last   = may_eval (may_add_vc (MAY_NODE_SIZE(x)-1, MAY_AT_PTR (x, 1)));
    return may_add_c (may_mul_c (cosh_expand (first), cosh_expand (last)),
                      may_mul_c (sinh_expand (first), sinh_expand (last)));
  } else if (MAY_TYPE (x) == MAY_FACTOR_T
             && MAY_TYPE (MAY_AT (x, 0)) == MAY_INT_T
             && mpz_fits_sshort_p (MAY_INT (MAY_AT (x, 0)))) {
    long n = labs (mpz_get_si (MAY_INT (MAY_AT (x, 0))));
    may_t arg = may_cosh (MAY_AT (x, 1));
    if (MAY_UNLIKELY (n == 1))
      return arg;
    MAY_ASSERT (n >= 2);
    arg = tchebycheff_var (n, 1, 0, arg);
    return may_eval (arg);
  } else
    return may_cosh_c (x);
}

static may_t
tan_expand (may_t x)
{
  if (MAY_TYPE (x) == MAY_SUM_T) {
    may_t first  = tan_expand (MAY_AT (x, 0));
    may_t last   = tan_expand (may_eval (may_add_vc (MAY_NODE_SIZE(x)-1, MAY_AT_PTR (x, 1))));
    return may_div_c (may_add_c (first, last), may_sub_c (MAY_ONE,may_mul_c (first, last)));
  } else if (MAY_TYPE (x) == MAY_FACTOR_T
             && MAY_TYPE (MAY_AT (x, 0)) == MAY_INT_T
             && mpz_fits_sshort_p (MAY_INT (MAY_AT (x, 0)))) {
    /* Pure lazyness */
    may_t g = may_eval (may_div_c (sin_expand (x), cos_expand (x)));
    g = may_sin2tancos (g);
    g = may_rewrite (g,
                     may_parse_str ("cos($1)^$2"),
                     may_parse_str ("(1+tan($1)^2)^(-$2/2)"));
    return g;
  } else
    return may_tan (x);

}


static may_t
tanh_expand (may_t x)
{
  if (MAY_TYPE (x) == MAY_SUM_T) {
    may_t first  = tanh_expand (MAY_AT (x, 0));
    may_t last   = tanh_expand (may_eval (may_add_vc (MAY_NODE_SIZE(x)-1, MAY_AT_PTR (x, 1))));
    return may_div_c (may_add_c (first, last), may_add_c (MAY_ONE,may_mul_c (first, last)));
  } else if (MAY_TYPE (x) == MAY_FACTOR_T
             && MAY_TYPE (MAY_AT (x, 0)) == MAY_INT_T
             && mpz_fits_sshort_p (MAY_INT (MAY_AT (x, 0)))) {
    /* Pure lazyness */
    may_t g = may_eval (may_div_c (sinh_expand (x), cosh_expand (x)));
    g = may_sin2tancos (g);
    g = may_rewrite (g,
                     may_parse_str ("cosh($1)^$2"),
                     may_parse_str ("(1-tanh($1)^2)^(-$2/2)"));
    return g;
  } else
    return may_tanh (x);

}

static const char *const texpand_name[] = {
  may_sin_name, may_cos_name, may_tan_name,
  may_sinh_name, may_cosh_name, may_tanh_name
};

static const void *const texpand_func[] = {
  sin_expand, cos_expand, tan_expand,
  sinh_expand, cosh_expand, tanh_expand
};

may_t
may_texpand (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (texpand_name) == numberof (texpand_func));
  MAY_LOG_FUNC (("%Y", x));

  MAY_RECORD ();
  may_t y = may_subs_c (x, 1, numberof (texpand_name),
                        texpand_name, texpand_func);
  MAY_RET_EVAL (y);
}


/* FIXME: Semantic to define */
static may_t
exp_expand (may_t x)
{
  may_t y;
  if (MAY_TYPE (x) == MAY_SUM_T) {
    may_size_t i, n = MAY_NODE_SIZE(x);
    y = MAY_NODE_C (MAY_PRODUCT_T, n);
    for (i = 0 ; i < n; i++)
      MAY_SET_AT (y, i, exp_expand (MAY_AT (x, i)));
    return may_eval (y);
  } else if (MAY_TYPE (x) == MAY_FACTOR_T) {
    y = may_pow_c (exp_expand (MAY_AT (x, 1)), MAY_AT (x, 0));
  } else if (MAY_PURENUM_P (x) && may_num_neg_p (x)) {
    y = may_pow_c (may_exp_c (may_num_abs (MAY_DUMMY, x)), MAY_N_ONE);
  } else
    y = may_exp_c (x);
  return may_eval (y);
}


static const char *const eexpand_name[] = {
  may_exp_name
};

static const void *const eexpand_func[] = {
  exp_expand
};

may_t
may_eexpand (may_t x)
{
  may_t y;

  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (eexpand_name) == numberof (eexpand_func));

  MAY_LOG_FUNC (("%Y", x));

  MAY_RECORD ();
  y = may_subs_c (x, 1, numberof (eexpand_name),
                  eexpand_name, eexpand_func);
  MAY_RET_EVAL (y);
}
