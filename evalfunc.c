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

may_t may_exp (may_t z) {
  return may_eval (may_exp_c (z));
}

may_t may_log (may_t z) {
  return may_eval (may_log_c (z));
}

may_t may_abs (may_t z) {
  return may_eval (may_abs_c (z));
}

may_t may_conj (may_t z) {
  return may_eval (may_conj_c (z));
}

may_t may_real (may_t z) {
  return may_eval (may_real_c (z));
}

may_t may_imag (may_t z) {
  return may_eval (may_imag_c (z));
}

may_t may_argument (may_t z) {
  return may_eval (may_argument_c (z));
}

may_t may_sign (may_t z) {
  return may_eval (may_sign_c (z));
}

may_t may_floor (may_t z) {
  return may_eval (may_floor_c (z));
}

may_t may_ceil (may_t z) {
  return may_eval (may_ceil_c (z));
}

may_t  may_cos (may_t z) {
  return may_eval (may_cos_c (z));
}

may_t may_sin (may_t z) {
  return may_eval (may_sin_c (z));
}

may_t may_tan (may_t z) {
  return may_eval (may_tan_c (z));
}

may_t may_asin (may_t z) {
  return may_eval (may_asin_c (z));
}

may_t may_acos (may_t z) {
  return may_eval (may_acos_c (z));
}

may_t may_atan (may_t z) {
  return may_eval (may_atan_c (z));
}

may_t  may_cosh (may_t z) {
  return may_eval (may_cosh_c (z));
}

may_t may_sinh (may_t z) {
  return may_eval (may_sinh_c (z));
}

may_t may_tanh (may_t z) {
  return may_eval (may_tanh_c (z));
}

may_t may_asinh (may_t z) {
  return may_eval (may_asinh_c (z));
}

may_t may_acosh (may_t z) {
  return may_eval (may_acosh_c (z));
}

may_t may_atanh (may_t z) {
  return may_eval (may_atanh_c (z));
}

MAY_REGPARM may_t
may_eval_range (may_t x)
{
  may_t y;
  mp_rnd_t old_rnd;
  may_t left, right;

  MAY_ASSERT (MAY_NODE_SIZE(x) == 2);

  /* Eval the both expression to float */
  old_rnd = may_kernel_rnd (GMP_RNDD);
  left = may_evalf (MAY_AT (x, 0));
  may_kernel_rnd (GMP_RNDU);
  right = may_evalf (MAY_AT (x, 1));
  may_kernel_rnd (old_rnd);

  /* Trivial check if it is ok */
  if (MAY_UNLIKELY (MAY_TYPE (left) != MAY_FLOAT_T
                    || MAY_TYPE (right) != MAY_FLOAT_T))
    MAY_THROW (MAY_INVALID_TOKEN_ERR);
  else if (MAY_UNLIKELY (mpfr_nan_p (MAY_FLOAT (left))
                         || mpfr_nan_p (MAY_FLOAT (right))))
    left = right = MAY_NAN;
  else if (MAY_UNLIKELY (mpfr_cmp (MAY_FLOAT(left),
                                   MAY_FLOAT(right)) > 0))
    MAY_THROW (MAY_DIMENSION_ERR);

  /* Rebuild the evaluated RANGE */
  y = x;
  MAY_SET_AT (y, 0, left);
  MAY_SET_AT (y, 1, right);
  MAY_CLOSE_C (y, MAY_NUM_F|MAY_EVAL_F,
               MAY_NEW_HASH2 (left, right));

  return y;
}

MAY_REGPARM may_t
may_eval_exp (may_t z, may_t x)
{
  may_t y;

  /* exp(FLOAT) = FLOAT */
  if (MAY_TYPE (z) == MAY_FLOAT_T) {
    mpfr_t f;
    mpfr_init (f);
    mpfr_exp (f, MAY_FLOAT (z), MAY_RND);
    y = may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
  }
  /* exp(COMPLEX) = exp(Re(C))*(cos(Im(C))+I*sin(Im(C))) */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
           && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
           && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_exp (z));
  /* exp(0) = 1 */
  else if (MAY_ZERO_P (z))
    y = MAY_ONE;
  /*  exp(n/2*I*PI) -> {+1|+I|-1|-I} for n%4 = {0,1,2,3} */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
           && MAY_PI_P (MAY_AT (z, 1))
           && MAY_TYPE (MAY_AT (z, 0)) == MAY_COMPLEX_T
           && MAY_ZERO_P (MAY_RE (MAY_AT (z, 0)))
           && (MAY_TYPE (MAY_IM (MAY_AT (z, 0))) == MAY_INT_T
               ||(MAY_TYPE (MAY_IM (MAY_AT (z, 0))) == MAY_RAT_T
                  && mpz_cmp_ui (mpq_denref (MAY_RAT (MAY_IM (MAY_AT (z, 0)))), 2)==0))) {
    may_t im = MAY_IM (MAY_AT (z, 0));
    if (MAY_TYPE (im) == MAY_INT_T)
      y = mpz_even_p (MAY_INT (im)) ? MAY_ONE : MAY_N_ONE;
    else {
      y = MAY_I;
      if (mpz_tstbit (mpq_numref (MAY_RAT (im)), 1) != 0)
        y = may_neg (y);
    }
  }
  /* exp(log(x)) = x */
  else if (MAY_TYPE (z) == MAY_LOG_T)
    y = MAY_AT(z, 0);
  /* exp(INT.log(x)) = x^INT && exp(FRAC.log(x)) = x^FRAC */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
           && MAY_TYPE (MAY_AT (z, 1)) == MAY_LOG_T
           && (MAY_TYPE (MAY_AT(z, 0)) == MAY_INT_T
               || MAY_TYPE (MAY_AT(z, 0)) == MAY_RAT_T)) {
    /* exp(k.ln(x)) --> x^k (?) */
    y = may_pow_c (MAY_AT (MAY_AT (z, 1), 0), MAY_AT (z, 0));
    y = may_eval (y);
  }
  /* exp(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->exp)
    y = (*MAY_EXT_GETX (z)->exp) (z);
  /* Rebuild it */
  else {
    y = x;
    MAY_SET_AT (y, 0, z);
    MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
  }

  return y;
}

MAY_REGPARM may_t
may_eval_log (may_t z, may_t x)
{
  may_t y;

  /* log(FLOAT) = FLOAT, but it may degenerate in a complex float */
  if (MAY_TYPE (z) == MAY_FLOAT_T)
    y =  may_num_mpfr_cx_eval (z, mpfr_log, may_cxfr_log);
  /* log (1) = 0 */
  else if (MAY_ONE_P (z))
    y = MAY_ZERO;
  /* log(0) = -Inf if not complex Mode, undefined otherwise */
  else if (MAY_ZERO_P (z))
    y = MAY_NAN;
  /* log(I) = Pi*I/2 log(-I) = -PI/2 */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
           && MAY_ZERO_P (MAY_RE (z))
           && (may_identical (MAY_IM (z), MAY_ONE) == 0
               || may_identical (MAY_IM (z), MAY_N_ONE) == 0)) {
    y = may_mul_vac (MAY_I, MAY_PI,MAY_HALF, NULL);
    if (may_identical (MAY_IM (z), MAY_N_ONE) == 0)
      y = may_neg_c (y);
    y = may_eval (y);
  }
  /* log(complex(real,real) = evaluation */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
           && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
           && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_log (z));
  /* log (-INT) = log(INT) + I * PI */
  else if (MAY_TYPE (z) == MAY_INT_T && MAY_NEG_P (z)) {
    mpz_t ztmp;
    mpz_init_set (ztmp, MAY_INT (z));
    mpz_abs (ztmp, ztmp);
    y = MAY_MPZ_NOCOPY_C (ztmp);
    y = may_add_c (may_log_c (y), may_mul_c (MAY_I, MAY_PI));
    y = may_eval (y);
  }
  /* log(exp(x)) = x if x is real */
  else if (MAY_TYPE (z) == MAY_EXP_T && may_real_p (MAY_AT (z,0)))
    y = MAY_AT (z,0);
  /* log(abs(exp(x))) = real(x) */
  else if (MAY_TYPE (z) == MAY_ABS_T
           && MAY_TYPE (MAY_AT (z, 0)) == MAY_EXP_T)
    y = may_eval (may_real_c (MAY_AT (MAY_AT (z, 0), 0)));
  /* log(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->log)
    y = (*MAY_EXT_GETX (z)->log) (z);
  /* Rebuild log */
  else {
    y = x;
    MAY_SET_AT (y, 0, z);
    MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
  }
return y;
}

MAY_REGPARM may_t
may_eval_floor (may_t z, may_t x)
{
  /* Complex: Floor(x+I*y) = Floor(x)+I*Float(y) +
     if (a= x-floor(x) + b= y-floor(y) < 1) 0
     if (a+b >= 1 && a >= b) 1
     if (a+b >= 1 && a < b) I */
  may_t y = NULL;

  /* Floor integer */
  if (may_integer_p (z))
    y = z;
  /* Floor numerical approximation */
  else if (may_num_p (z))
    y = may_compute_num_floor (z);
  /* floor(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->floor)
    y = (*MAY_EXT_GETX (z)->floor) (z);

  /* If not be able to compute it, hold form */
  if (y == NULL) {
    y = x;
    MAY_SET_AT (y, 0, z);
    MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
  }
  return y;
}

MAY_REGPARM may_t
may_eval_sign (may_t z, may_t x)
{
  may_t y;

  /* If Num(x)  ==> Try to eval the sign if real
     FIXME: Even if it not numerical, we may eval the sign (X^2+1) */

  int s;
  if (MAY_TYPE (z) == MAY_COMPLEX_T) {
    /* sign z = 1 if Re(z) > 0 or Re(z) = 0 and Im(z) > 0. -1 instead (can't be 0) */
    s = may_num_pos_p (MAY_RE (z)) || (may_num_zero_p (MAY_RE (z)) && may_num_pos_p (MAY_IM (z)));
    y = s ? MAY_ONE : MAY_N_ONE;
  } else if (MAY_NUM_P (z) && (s = may_compute_sign (z)) != 0 && ((s-1)&s) == 0)
    y = (s&2) ? MAY_ONE : (s&4) ? MAY_N_ONE : MAY_ZERO;
  else if (may_nonneg_p (z))
    y = MAY_ONE;
  else if (may_nonpos_p (z))
    y = MAY_N_ONE;
  /* sign(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->sign)
    y = (*MAY_EXT_GETX (z)->sign) (z);
  else {
    y = x;
    MAY_SET_AT (y, 0, z);
    MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
  }
  return y;
}

MAY_REGPARM may_t
may_eval_gamma (may_t z, may_t x)
{
  may_t y = NULL;

  /* gamma(INTEGER) = factorial */
  if (MAY_TYPE (z) == MAY_INT_T) {
    if (mpz_cmp_ui (MAY_INT (z), 0) <= 0)
      y = MAY_P_INF;
    /* TODO: Find a better upper limit ! */
    else if (mpz_fits_ushort_p (MAY_INT (z))) {
      mpz_t f;
      mpz_init (f);
      /* TODO: Add support for intmod */
      mpz_fac_ui (f, mpz_get_ui (MAY_INT (z))-1);
      y = may_mpz_simplify (MAY_MPZ_NOCOPY_C (f));
    }
  }
  /* GAMMA(RATIONAL) = if N/2
     gamma(1/2) = sqrt(PI)  [gamma(x+1) = x*gamma(x) ]
     gamma(RAT) with denom(RAT)=2,
     Num/2 > 1 :
     gamma(RAT) = sqrt(PI) * PRODUCT( (2*i-1)/2, i=1..n);
     with n = (num-1)/2
     = PI((2*i-1),i=1..n) / [PI(2,i=1..n) = 2^n) ]
     = PI(i,i=1..(2*n-1)) / (PI(2*i,i=1..n-1) * 2^n)
     = (2*n-1)! / (2^(2*n-1) * (n-1)!) pour num>1.
     For num < 1,
     = (2^(2*n-1) * (n-1)!) / (2*n-1)! * (-1)^n with n = abs((num-1)/2)
  */
  else if (MAY_TYPE (z) == MAY_RAT_T
           && mpz_cmp_ui (mpq_denref (MAY_RAT (z)), 2) == 0
           && mpz_fits_sshort_p (mpq_numref (MAY_RAT (z)))) {
    mpq_t q;
    int n, num = mpz_get_si (mpq_numref (MAY_RAT (z)));
    mpq_init (q);
    mpq_set_ui (q, 1, 1);
    if (num > 1) {
      n = (num-1)/2;
      mpz_fac_ui (mpq_numref (q), 2*n-1);
      mpz_fac_ui (mpq_denref (q), n-1);
      mpz_mul_2exp (mpq_denref (q), mpq_denref (q), 2*n-1);
    } else if (num < 1) {
      n = -(num-1)/2;
      mpz_fac_ui (mpq_denref (q), 2*n-1);
      mpz_fac_ui (mpq_numref (q), n-1);
      mpz_mul_2exp (mpq_numref (q), mpq_numref (q), 2*n-1);
      if ((n % 2) != 0)
        mpq_neg (q, q);
    }
    mpq_canonicalize (q);
    y = may_mul_c (may_sqrt_c (MAY_PI), may_mpq_simplify (NULL, q));
    y = may_eval (y);
  }
  /* GAMMA(float) -> MPFR */
  else if (MAY_TYPE (z) == MAY_FLOAT_T) {
    mpfr_t f;
    mpfr_init (f);
    mpfr_gamma (f, MAY_FLOAT (z), MAY_RND);
    y = may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
  }
  /* GAMMA(Complex float) = ? -- FIXME: How to compute this? */
  /* gamma(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->gamma)
    y = (*MAY_EXT_GETX (z)->gamma) (z);

  if (y == NULL) {
    y = x;
    MAY_SET_AT (y, 0, z);
    MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
  }

  return y;
}

MAY_REGPARM may_t
may_eval_conj (may_t z, may_t x)
{
  may_t y = NULL;

  /* PURENUM ? */
  if (MAY_PURENUM_P (z))
    y = MAY_TYPE (z) == MAY_COMPLEX_T
      ? may_num_simplify (may_num_conj (MAY_DUMMY, z)) : z;
  /* conj(Sum/product) = sum/product(conj) */
  else if (MAY_TYPE (z) == MAY_SUM_T
           || MAY_TYPE (z) == MAY_PRODUCT_T
           || MAY_TYPE (z) == MAY_FACTOR_T)
    y = may_eval (may_map_c (z, may_conj_c));
  else if (MAY_TYPE (z) == MAY_POW_T
           && MAY_TYPE (MAY_AT (z, 1)) == MAY_INT_T) {
    y = may_pow_c (may_conj_c (MAY_AT (z, 0)), MAY_AT (z, 1));
    y = may_eval (y);
  /* If numerical, check if what we can compute the sign */
  } else if (MAY_NUM_P (z) && may_compute_sign (z) != 0)
    y = z;
  /* Support of hypothesis (Let the predicate do the job) */
  else if (may_real_p (z))
    y = z;
  /* conj(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->conj)
    y = (*MAY_EXT_GETX (z)->conj) (z);

  if (y == NULL) {
    y = x;
    MAY_SET_AT (y, 0, z);
    MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
  }

  return y;
}

MAY_REGPARM may_t
may_eval_real (may_t z, may_t x)
{
  may_t y = NULL;

  /* PURENUM ? */
  if (MAY_PURENUM_P (z))
    y = MAY_TYPE (z) == MAY_COMPLEX_T ? MAY_RE (z) : z;
  /* real(Sum) = sum(real) */
  else if (MAY_TYPE (z) == MAY_SUM_T)
    y = may_eval (may_map_c (z, may_real_c));
  /* Factor: Get out if real */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
           && MAY_TYPE (MAY_AT (z, 0)) != MAY_COMPLEX_T)
    y = may_eval (may_mul_c (MAY_AT (z, 0), may_real_c (MAY_AT (z, 1))));
  /* If numerical, check if what we can compute the sign */
  else if (MAY_NUM_P (z) && may_compute_sign (z) != 0)
    y = z;
  /* Support of hypothesis (Let the predicate do the job) */
  else if (may_real_p (z))
    y = z;
  /* real(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->real)
    y = (*MAY_EXT_GETX (z)->real) (z);

  if (y == NULL) {
    y = x;
    MAY_SET_AT (y, 0, z);
    MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
  }

  return y;
}

MAY_REGPARM may_t
may_eval_imag (may_t z, may_t x)
{
  may_t y = NULL;

  /* PURENUM ? */
  if (MAY_PURENUM_P (z))
    y = MAY_TYPE (z) == MAY_COMPLEX_T ? MAY_IM (z) : MAY_ZERO;
  /* imag(Sum) = sum(imag) */
  else if (MAY_TYPE (z) == MAY_SUM_T)
    y = may_eval (may_map_c (z, may_imag_c));
  /* Factor: Get out if it is real */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
           && MAY_TYPE (MAY_AT (z, 0)) != MAY_COMPLEX_T)
    y = may_eval (may_mul_c (MAY_AT (z, 0), may_imag_c (MAY_AT (z, 1))));
  /* If numerical, check if what we can compute the sign */
  else if (MAY_NUM_P (z) && may_compute_sign (z) != 0)
    y = MAY_ZERO;
  /* Support of hypothesis (Let the predicate do the job) */
  else if (may_real_p (z))
    y = MAY_ZERO;
  /* imag(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->imag)
    y = (*MAY_EXT_GETX (z)->imag) (z);

  if (y == NULL) {
    y = x;
    MAY_SET_AT (y, 0, z);
    MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
  }

  return y;
}

MAY_REGPARM may_t
may_eval_argument (may_t z, may_t x)
{
  /* Complex x+Iy or RectForm ==> Pi/2*sign(y)-atan(x/y) */
  may_t y = NULL;

  /* PURENUM ? */
  if (MAY_PURENUM_P (z)) {
    if (may_num_zero_p (z))
      y = MAY_NAN;
    else if (may_num_pos_p (z))
      y = MAY_ZERO; /* Pos Real */
    else if (may_num_neg_p (z))
      y = MAY_PI; /* Neg Real */
    else /* Complex */ {
      may_t p1 = may_mul_c (MAY_PI, MAY_HALF);
      may_t p2 = may_atan_c (may_div_c (MAY_RE (z), MAY_IM (z)));
      if (may_num_neg_p (MAY_IM (z)))
        p1 = may_neg_c (p1);
      y = may_sub_c (p1, p2);
      y = may_eval (y);
    }
  }
  /* Factor: Get out if it is real */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
           && MAY_TYPE (MAY_AT (z, 0)) != MAY_COMPLEX_T)
    y = may_eval (may_argument_c (MAY_AT (z, 1)));
  /* Product and factor: make a sum of arg ? */

  /* Support of Hypothesis */
  else if (may_real_p (z)) {
    if (may_pos_p (z))
      y = MAY_ZERO;
    else if (may_neg_p (z))
      y = MAY_PI;
  }
  /* argument(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->argument)
    y = (*MAY_EXT_GETX (z)->argument) (z);

  if (y == NULL) {
    y = x;
    MAY_SET_AT (y, 0, may_eval (MAY_AT(x, 0)));
    MAY_CLOSE_C (y, MAY_FLAGS (MAY_AT (y, 0)), MAY_HASH (MAY_AT (y, 0)));
  }

  return y;
}
