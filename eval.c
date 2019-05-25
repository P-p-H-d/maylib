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

static MAY_REGPARM may_t may_eval_sum (may_t x);
static MAY_REGPARM may_t may_eval_product (may_t x);

int
may_eval_p (may_t x)
{
  return MAY_EVAL_P(x);
}

may_t
may_reeval (may_t x)
{
  may_mark();
  return may_keep (may_eval (may_copy_c (x, 0)));
}

static may_t
evalf (may_t x)
{
  mpfr_t f;
  may_t y;

  switch (MAY_TYPE(x)) {
  case MAY_INT_T:
    mpfr_init_set_z (f, MAY_INT (x), MAY_RND);
  return_mpfr_no_copy:
    return MAY_MPFR_NOCOPY_C (f);
  case MAY_RAT_T:
    mpfr_init_set_q (f, MAY_RAT(x), MAY_RND);
    goto return_mpfr_no_copy;
  case MAY_FLOAT_T:
    return x;
  case MAY_COMPLEX_T:
    return MAY_COMPLEX_C (evalf (MAY_RE (x)),
                          evalf (MAY_IM (x)));
  case MAY_STRING_T:
    /* Check if PI string */
    if (MAY_PI_P (x)) {
      mpfr_init (f);
      mpfr_const_pi (f, MAY_RND);
      goto return_mpfr_no_copy;
    }
    /* Check if 10 base float string */
    else if (MAY_NAME (x)[0] == '#') {
      mpfr_init_set_str (f, &MAY_NAME (x)[1], 0, MAY_RND);
      goto return_mpfr_no_copy;
    } else
      return x;
  case MAY_POW_T:
    y = MAY_NODE_C (MAY_POW_T, 2);
    MAY_SET_AT (y, 0, evalf(MAY_AT(x,0)));
    MAY_SET_AT (y, 1, MAY_AT(x, 1));
    return y;
  default:
    if (MAY_UNLIKELY (MAY_ATOMIC_P (x)))
      return x;
    may_size_t i, n = MAY_NODE_SIZE(x);
    y = MAY_NODE_C (MAY_TYPE (x), n);
    for (i = 0; i < n; i++)
      MAY_SET_AT (y, i, evalf(MAY_AT(x,i)));
    return y;
  }
}
may_t
may_evalf (may_t x)
{
  may_mark();
  return may_keep (may_eval (evalf(x)));
}

/* Eval 'x' in Z not inside Z/pZ (if it was set)
   It is useful for evaluating power */
MAY_INLINE may_t
eval_outside_intmod (may_t x)
{
  may_t saved_intmod = may_g.frame.intmod;
  may_g.frame.intmod = NULL;
  x = may_eval (x);
  may_g.frame.intmod = saved_intmod;
  return x;
}

/* Return TRUE if x is "positive" for extract_constant.
   It is used for even odd functions to choose a represent between -f(x) and f(-x) */
MAY_INLINE int
pos_p (may_t x)
{
  /* Get numerical coeeficient if any */
  if (MAY_TYPE (x) == MAY_FACTOR_T)
    x = MAY_AT (x, 0);
  /* Special treatement for complex numbers */
  if (MAY_UNLIKELY (MAY_TYPE (x) == MAY_COMPLEX_T))
    return !may_num_negzero_p (MAY_RE (x)) || !may_num_neg_p (MAY_IM (x));
  else if (MAY_LIKELY (MAY_PURENUM_P (x)))
    return !may_num_neg_p (x);
  return 1;
}

/* From a sum x+y, extract the overall coefficient (the integer content):
   For 2*x+4*y, extract 2 as a overall coeff, and x+2*y.
   It also extracts the sign, so that we can have (a-b)/(b-a)
   evals to -1 */
static MAY_REGPARM may_t
extract_constant_coefficient_from_sum (may_t x)
{
  may_t z, retone;
  may_size_t i,n;
  mpz_t gcd;

  MAY_ASSERT (MAY_TYPE (x) == MAY_SUM_T);

  n = MAY_NODE_SIZE(x);
  MAY_ASSERT (n >= 2);
  retone = pos_p (MAY_AT (x, 0)) ? MAY_ONE : MAY_N_ONE;

  /* First: a fast pass to detect if we can do something */
  /* Deal with 1st term which may be a numerical */
  for (i = MAY_TYPE (MAY_AT (x, 0)) == MAY_INT_T; i < n; i++) {
    z = MAY_AT (x, i);
    if (MAY_TYPE (z) != MAY_FACTOR_T
        || MAY_TYPE (MAY_AT (z, 0)) != MAY_INT_T)
      return retone;
  }

  /* Compute the GCD of all integer coefficients of the sum */
  mpz_init (gcd);
  z = MAY_AT (x, 0);
  mpz_set (gcd, MAY_INT (MAY_TYPE (z) == MAY_INT_T ? z : MAY_AT (z, 0)));
  for (i = 1; i < n; i++) {
    z = MAY_AT (x, i);
    MAY_ASSERT (MAY_TYPE (z) == MAY_FACTOR_T);
    mpz_gcd (gcd, gcd, MAY_INT (MAY_AT (z, 0)));
    if (mpz_cmp_ui (gcd, 1) == 0)
      return retone;
  }

  /* Get the sign of the leading term */
  if (retone == MAY_N_ONE)
    mpz_neg (gcd, gcd);
  return may_mpz_simplify (MAY_MPZ_NOCOPY_C (gcd));
}

static MAY_REGPARM may_t
fix_sum_after_extracting_coefficient (may_t sum, may_t gcd)
{
  may_t z;
  may_size_t i,n;

  MAY_ASSERT (MAY_TYPE (sum) == MAY_SUM_T);
  MAY_ASSERT (MAY_TYPE (gcd) == MAY_INT_T);
  MAY_ASSERT (mpz_cmp_ui (MAY_INT (gcd), 1) != 0);

  n = MAY_NODE_SIZE(sum);
  z = MAY_NODE_C (MAY_SUM_T, n);
  MAY_ASSERT (n >= 2);

  /* Special case for -1 */
  if (MAY_UNLIKELY (gcd == MAY_N_ONE)) {
    if (MAY_PURENUM_P (MAY_AT (sum, 0))) {
      i = 1;
      MAY_SET_AT (z, 0, may_num_neg (MAY_DUMMY, MAY_AT (sum, 0)));
    } else
      i = 0;
    for ( /* void */; i < n; i++) {
      may_t old = MAY_AT (sum, i);
      may_t new;
      if (MAY_LIKELY (MAY_TYPE (old) == MAY_FACTOR_T)) {
        new = may_num_neg (MAY_DUMMY, MAY_AT (old, 0));
        old = MAY_AT (old, 1);
      } else
        new = MAY_N_ONE;
      new = may_mul_c (new, old);
      MAY_SET_AT (z, i, new);
    }
    z = may_eval (z);
    return z;
  }

  /* Deal with 1st term which may be a numerical */
  if (MAY_LIKELY (MAY_TYPE (MAY_AT (sum, 0)) == MAY_INT_T)) {
    i = 1;
    MAY_SET_AT (z, 0, may_div_c (MAY_AT (sum, 0), gcd));
  } else
    i = 0;
  for ( /*void*/; i < n; i++) {
    may_t old = MAY_AT (sum, i);
    may_t num;
    may_t term;
    MAY_ASSERT (MAY_TYPE (old) == MAY_FACTOR_T);
    num = may_div_c (MAY_AT (old, 0), gcd);
    term = may_mul_c (num, MAY_AT (old, 1));
    MAY_SET_AT (z, i, term);
  }
  z = may_eval (z);
  return z;
}

/* Inline since it is called only once, and it is quite often used,
   we want to inline this function.
   Eval the power of x which must be of type MAY_POW_T. */
MAY_INLINE may_t
may_eval_pow (may_t x)
{
  may_t y;

  MAY_ASSERT (MAY_TYPE (x) == MAY_POW_T);
  MAY_ASSERT (MAY_NODE_SIZE(x) == 2);

  /* 1. Eval both sub-args. They are likely already evaluated */
  may_t base = MAY_AT (x, 0);
  if (MAY_UNLIKELY (!MAY_EVAL_P (base)))
    base = may_eval (base);
  /* Compute exponent outside the intmod */
  may_t expo = MAY_AT (x, 1);
  if (MAY_UNLIKELY (!MAY_EVAL_P (expo)))
    expo = eval_outside_intmod (expo);
  MAY_ASSERT (MAY_EVAL_P (base) && MAY_EVAL_P (expo));

  may_t gcd, num, new;
  may_size_t n, i;

  /* Commun case is the case where expo is an integer.
     Optimize it. */
  if (MAY_LIKELY (MAY_TYPE (expo) == MAY_INT_T)) {
    /* Special case: expo is an integer */
    mpz_srcptr zexpo = MAY_INT (expo);
    /* 2. If expo == 1 or 0 */
    if (MAY_UNLIKELY (expo == MAY_ZERO))
      y = MAY_ONE;
    else if (MAY_UNLIKELY (expo == MAY_ONE))
      y = base;

    else
      switch (MAY_TYPE (base)) {
	/* 3. If both are pure numerical types, eval them. */
      case MAY_INT_T:
      case MAY_FLOAT_T:
      case MAY_RAT_T:
      case MAY_COMPLEX_T:
	y = may_num_pow (base, expo);
	MAY_ASSERT (MAY_EVAL_P (y));
	break;

	/* 4. If base is Product, and expo is int, returns product of pow.*/
      case MAY_FACTOR_T:
	/* (A.X) ^ INT --> A^INT.X^INT WARNING: X^INT may be num or a factor */
	num = may_num_pow (MAY_AT (base, 0), expo);
	new = may_eval (may_pow_c (MAY_AT (base, 1), expo) );
	MAY_ASSERT (MAY_EVAL_P (num) && MAY_EVAL_P (new));
        if (MAY_UNLIKELY (!MAY_PURENUM_P (num))) {
          /* Case if A^INT refuses to be computed because it is too big */
          y = may_mul(num, new);
        } else if (MAY_UNLIKELY (MAY_ONE_P (num)))
	  y = new; /* (-x)^2 -->x^2 */
	else if (MAY_UNLIKELY (MAY_PURENUM_P (new)))
	  y = may_num_simplify (may_num_mul (MAY_DUMMY, num, new));
	else {
	  if (MAY_UNLIKELY (MAY_TYPE (new) == MAY_FACTOR_T)) {
	    num = may_num_simplify (may_num_mul (MAY_DUMMY, num,
						 MAY_AT (new, 0)));
	    new = MAY_AT (new, 1);
	  }
          y = x;
          MAY_OPEN_C (y, MAY_FACTOR_T);
          MAY_NODE_SIZE(y) = 2;
          MAY_ASSERT(MAY_PURENUM_P(num));
	  MAY_SET_AT (y, 0, num);
	  MAY_SET_AT (y, 1, new);
	  MAY_CLOSE_C (y, MAY_FLAGS (new), MAY_NEW_HASH2 (num, new));
	}
	break;

      case MAY_PRODUCT_T:
	n = MAY_NODE_SIZE(base);
	MAY_ASSERT (n >= 2);
	if (n > 2)
	  y = MAY_NODE_C (MAY_PRODUCT_T, n);
	else {
          MAY_ASSERT (n == 2);
	  y = x;
	  MAY_OPEN_C (y, MAY_PRODUCT_T);
	  MAY_NODE_SIZE(y) = 2;
	}
	for (i = 0; i < n; i++)
          MAY_SET_AT (y, i, may_pow_c (MAY_AT (base, i), expo) );
	y = may_eval (y);
	break;

        /* 4'. Extract the common GCD for all terms of a sum
           (a*g+b*g+c*g)^^N --> g^N*(a+b+c)^^N */
      case MAY_SUM_T:
	gcd = extract_constant_coefficient_from_sum (base);
	if (gcd != MAY_ONE) {
	  y = fix_sum_after_extracting_coefficient (base, gcd);
	  y = may_mul_c (may_pow_c (gcd, expo), may_pow_c (y, expo));
	  y = may_eval (y);
	} else
	  goto rebuild_pow;
	break;

        /* 5. If Leader is Pow, combine the exponents  */
      case MAY_POW_T:
	if (MAY_UNLIKELY (MAY_PURENUM_P (MAY_AT (base, 1)))) {
	  may_t newexpo = may_mul_c (expo, MAY_AT (base, 1));
	  y = x;
	  MAY_SET_AT (y, 0, MAY_AT (base, 0));
	  MAY_SET_AT (y, 1, newexpo);
	  y = may_eval (y);
	} else
	  goto rebuild_pow;
	break;

	/* 7. If base=abs and expo even integer and base real, remove abs */
      case MAY_ABS_T:
	if (mpz_even_p (zexpo) && may_real_p (MAY_AT (base, 0))) {
	  y = x;
	  MAY_SET_AT (y, 0, MAY_AT (base, 0));
	  MAY_SET_AT (y, 1, expo);
	  y = may_eval (y);
	} else
	  goto rebuild_pow;
	break;

	/* 8. Else generic */
      case MAY_STRING_T:
      case MAY_FUNC_T:
      case MAY_EXP_T:
      case MAY_LOG_T:
      case MAY_SIGN_T: /* FIXME: sign(x)^2 might be simplified */
      case MAY_FLOOR_T:
      case MAY_SIN_T:
      case MAY_COS_T:
      case MAY_TAN_T:
      case MAY_ASIN_T:
      case MAY_ACOS_T:
      case MAY_ATAN_T:
      case MAY_SINH_T:
      case MAY_COSH_T:
      case MAY_TANH_T:
      case MAY_ASINH_T:
      case MAY_ACOSH_T:
      case MAY_ATANH_T:
      case MAY_CONJ_T:
      case MAY_REAL_T:
      case MAY_IMAG_T:
      case MAY_ARGUMENT_T:
      case MAY_GAMMA_T:
      case MAY_DIFF_T:
      case MAY_MOD_T:
      case MAY_GCD_T:
      rebuild_pow:
        y = x;
        MAY_SET_AT (y, 0, base);
        MAY_SET_AT (y, 1, expo);
        MAY_CLOSE_C (y, MAY_FLAGS(base),
                     MAY_NEW_HASH2 (base, expo) );
        break;

      default:
        if (MAY_UNLIKELY (MAY_EXT_P (base))) {
          y = may_eval_extension_pow (base, expo);
        } else
          goto rebuild_pow;
	break;
      } /* end switch on the type of base */

  } else {
    /* General case (EXPO is not an integer) */
    /* 2. If base==1, or exp == 1.0 or 0.0 */
    if (MAY_ONE_P (base) || MAY_ZERO_P (expo))
      y = MAY_ONE;
    else if (MAY_ONE_P (expo))
      y = base;
    else if (MAY_NAN_P (expo) || MAY_NAN_P (base))
      y = MAY_NAN;
    /* 3. If both are num types, eval them. */
    else if (MAY_PURENUM_P (base) && MAY_PURENUM_P (expo)) {
      y = may_num_pow (base, expo);
      MAY_ASSERT (MAY_EVAL_P (y));
    }

    /* 5. If Leader is Pow, combine the exponents if:
           * both powers are RAT and if the first expo looks like 1/N
	   * base is pos and both power are real
	   * base is real and both pow are real and second one looks like
	   1/N, with N odd
	   * base is real and both pow are real and second one looks like
	   1/N, with N even => abs (x) ^(mul(expo1,expo2)) (TODO)
    */
    else if (MAY_TYPE (base) == MAY_POW_T
	     && MAY_PURENUM_P (MAY_AT (base, 1))
	     && ((MAY_TYPE (expo) == MAY_RAT_T
		  && MAY_TYPE (MAY_AT (base, 1)) == MAY_RAT_T
		  && mpz_cmp_ui (mpq_numref(MAY_RAT(MAY_AT(base,1))), 1) == 0)
		 || (may_nonneg_p (MAY_AT (base, 0))
		     && may_real_p (expo)
		     && may_real_p (MAY_AT (base, 1)))
		 || (may_real_p (MAY_AT (base, 0))
		     && may_real_p (MAY_AT (base, 1))
		     && MAY_TYPE (expo) == MAY_RAT_T
		     && mpz_cmp_ui (mpq_numref (MAY_RAT (expo)), 1) == 0
		     && mpz_odd_p (mpq_denref (MAY_RAT (expo))))
		 )) {
      may_t newexpo = may_mul_c (expo, MAY_AT (base, 1));
      y = x;
      MAY_SET_AT (y, 0, MAY_AT (base, 0));
      MAY_SET_AT (y, 1, newexpo);
      y = may_eval (y);
    }

    /* 6. Check if expo = "X^(f.x) --> (X^x)^f" */
    else if (MAY_TYPE (expo) == MAY_FACTOR_T
	     && MAY_TYPE (MAY_AT (expo, 0)) == MAY_INT_T) {
      may_t newbase = may_pow_c (base, MAY_AT (expo, 1));
      y = x;
      MAY_SET_AT (y, 0, newbase);
      MAY_SET_AT (y, 1, MAY_AT (expo, 0) );
      y = may_eval (y);
      MAY_ASSERT (MAY_TYPE (MAY_AT (y, 1)) != MAY_FACTOR_T);
    }
    /* 7. If base=abs and expo even integer and base real, remove abs */
    else if (MAY_TYPE (base) == MAY_ABS_T
	     && may_even_p (expo) && may_real_p (MAY_AT (base, 0))) {
      y = x;
      MAY_SET_AT (y, 0, MAY_AT (base, 0));
      MAY_SET_AT (y, 1, expo);
      y = may_eval (y);
    }

    /* 8. Else generic */
    else {
      if (MAY_UNLIKELY (MAY_EXT_P (base) || MAY_EXT_P (expo))) {
        y = may_eval_extension_pow (base, expo);
      } else {
        y = x;
        MAY_SET_AT (y, 0, base);
        MAY_SET_AT (y, 1, expo);
        MAY_CLOSE_C (y, MAY_FLAGS(base) & MAY_FLAGS(expo),
		   MAY_NEW_HASH2 (base, expo) );
      }
    }
  }

  return y;
}

MAY_REGPARM may_t
may_eval (may_t x)
{
  may_t y;

  MAY_ASSERT (x != NULL);
  MAY_ASSERT (MAY_TYPE (x) != MAY_INDIRECT_T || !MAY_EVAL_P (x));
  MAY_ASSERT (MAY_TYPE (x) <= MAY_END_LIMIT+may_c.extension_size);

  /* If x is already evaluated  */
  if (MAY_LIKELY (MAY_EVAL_P (x))) {
    MAY_ASSERT (may_recompute_hash (x) == MAY_HASH (x));
    return x;
  }

  /***** The big switch ******/
  switch (MAY_TYPE (x)) {
    /* The nums */
  case MAY_INT_T:
    y = may_mpz_simplify (x);
    break;
  case MAY_RAT_T:
    y = may_mpq_simplify (x, MAY_RAT (x));
    break;
  case MAY_FLOAT_T:
    y = may_mpfr_simplify (x);
    break;
  case MAY_COMPLEX_T:
    y = may_cx_simplify (x);
    break;

    /* The other atomics */
  case MAY_STRING_T:
    y = x;
    MAY_CLOSE_C (y, MAY_EVAL_F, may_string_hash (MAY_NAME (x)));
    break;
  case MAY_DATA_T:
    y = x;
    MAY_CLOSE_C (y, MAY_EVAL_F,
                 may_data_hash ((const char*)MAY_DATA (x).data,
                                MAY_DATA (x).size));
    break;
  case MAY_INDIRECT_T:
    MAY_ASSERT (MAY_INDIRECT (x) != x);
    x = MAY_INDIRECT (x);
    MAY_ASSERT (MAY_EVAL_P (x) && MAY_TYPE (x) != MAY_INDIRECT_T);
    return x; /* It is forbidden to go down: return immediately */

    /* The most importants types: eval must be fast for them */
  case MAY_SUM_T:
  case MAY_SUM_RESERVE_T:
    y = may_eval_sum (x);
    break;
  case MAY_FACTOR_T:
  case MAY_PRODUCT_T:
  case MAY_PRODUCT_RESERVE_T:
    y = may_eval_product (x);
    break;
  case MAY_POW_T:
    y = may_eval_pow (x);
    break;

  case MAY_RANGE_T:
    y = may_eval_range (x);
    break;
  case MAY_EXP_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_exp (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_LOG_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_log (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_COS_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_cos (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_SIN_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_sin (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_TAN_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_tan (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_ASIN_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_asin (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_ACOS_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_acos (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_ATAN_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_atan (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_COSH_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_cosh (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_SINH_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_sinh (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_TANH_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_tanh (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_ASINH_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_asinh (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_ACOSH_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_acosh (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_ATANH_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_atanh (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_FLOOR_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_floor (may_eval (MAY_AT (x, 0)), x);
    break;
  case MAY_SIGN_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_sign (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_GAMMA_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_gamma (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_CONJ_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_conj (may_eval (MAY_AT (x, 0)), x);
    break;
  case MAY_REAL_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_real (may_eval (MAY_AT (x, 0)), x);
    break;
  case MAY_IMAG_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_imag (may_eval (MAY_AT (x, 0)), x);
    break;
  case MAY_ARGUMENT_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    y = may_eval_argument (eval_outside_intmod(MAY_AT (x, 0)), x);
    break;
  case MAY_ABS_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
    {
      may_t gcd, z = may_eval (MAY_AT (x, 0));
      int s;
      y = NULL;
      /* If Pure Numerical, do it. For complex, we may go outside purenum,
         so we can't use may_num_simplify. */
      if (MAY_PURENUM_P (z))
	y = may_num_pos_p (z) ? z
          : may_eval (may_num_abs (MAY_DUMMY, z));
      /* If Num(x), try to eval the sign */
      else if (MAY_NUM_P (z) && (s = may_compute_sign (z)) != 0)
	y = (s&4) ? may_neg (z) : z;
      /* if Product, expand it -- FIXME: Good idea? */
      else if (MAY_TYPE (z) == MAY_FACTOR_T
               || MAY_TYPE (z) == MAY_PRODUCT_T)
        y = may_eval (may_map_c (z, may_abs_c));
      else if (MAY_TYPE (z) == MAY_POW_T
               && MAY_TYPE (MAY_AT (z, 1)) == MAY_INT_T) {
        y = may_pow_c (may_abs_c (MAY_AT (z, 0)), MAY_AT (z, 1));
        y = may_eval (y);
      }
      else if (MAY_TYPE (z) == MAY_SUM_T
               && (gcd = extract_constant_coefficient_from_sum (z))
               != MAY_ONE) {
        y = fix_sum_after_extracting_coefficient (z, gcd);
        y = may_mul_c (may_abs_c (gcd), may_abs_c (y));
        y = may_eval (y);
      }
      /* Support of generalised expressions through predicates */
      else if (may_real_p (z)) {
        if (may_nonneg_p (z))
          y = z;
        else if (may_nonpos_p (z))
          y = may_neg (z);
      }
      /* Else Rebuild it */
      if (y == NULL) {
	y = x;
	MAY_SET_AT (y, 0, z);
	MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
      }
      break;
    }
  case MAY_DIFF_T:
    /* Always reeval */
    y = may_eval_diff (x);
    break;
  case MAY_FUNC_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 2);
    {
      may_t func = MAY_AT (x, 0); /*FIXME: Shouldn't we eval it? */
      may_t z = eval_outside_intmod(MAY_AT (x, 1));
      /* Rebuild it */
      y = x;
      MAY_SET_AT (y, 0, func);
      MAY_SET_AT (y, 1, z);
      MAY_CLOSE_C (y, MAY_EVAL_F, MAY_NEW_HASH2 (func, z));
    }
    break;
  case MAY_GCD_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 2);
    {
      may_t temp[2];
      temp[0] = may_eval (MAY_AT (x, 0));
      temp[1] = may_eval (MAY_AT (x, 1));
      y = may_gcd (2, temp);
      /* We return y*gcd(a/y,b/y) except if we know gcd(a/y,b/y) is one */
      if (y != MAY_ONE) {
        temp[0] = may_divexact (temp[0], y);
        temp[1] = may_divexact (temp[1], y);
      }
      if (temp[0] != MAY_ONE && temp[1] != MAY_ONE
          && !(MAY_PURENUM_P (temp[0]) && MAY_PURENUM_P (temp[1])))
        y = may_mul (y, may_hold (may_gcd_c (temp[0], temp[1])));
    }
    break;
  case MAY_MOD_T:
    MAY_ASSERT (MAY_NODE_SIZE(x) == 2);
    {
      may_t z1 = may_eval (MAY_AT (x, 0)),
        z2 = may_eval (MAY_AT (x, 1));
      /* Rebuild it */
      y = x;
      MAY_SET_AT (y, 0, z1);
      MAY_SET_AT (y, 1, z2);
      MAY_CLOSE_C (y, MAY_EVAL_F, MAY_NEW_HASH2 (z1, z2));
    }
    break;
  default:
    /* Check if this is an extension */
    if (MAY_LIKELY (MAY_EXT_P (x)))
      y = may_eval_extension (x);
    else
      /* Not a registered type. Not a fatal error (assert) since
         it may come due to registering an extension, building an expression,
         unregistering the extension, and reevaluating the expression... */
      MAY_THROW (MAY_INVALID_TOKEN_ERR);
  }

  MAY_ASSERT (MAY_EVAL_P (y));
  MAY_ASSERT (may_recompute_hash (y) == MAY_HASH (y));

  /* Cache the evaluation by modifing the input if possible */
  if (MAY_UNLIKELY (x != y))
    MAY_SET_INDIRECT (x, y);

  return y;
}



/* Check the order of a sum or a product */
#ifdef MAY_WANT_ASSERT
static int
check_sum_order (may_t x)
{
  may_size_t i, n;
  may_t a, b;
  MAY_ASSERT (MAY_TYPE (x) == MAY_SUM_T);
  n = MAY_NODE_SIZE(x);
  //printf ("Start %d\n", n);
  for (i=1+MAY_PURENUM_P (MAY_AT (x, 0)); i<n; i++) {
    a = MAY_AT (x, i-1);
    b = MAY_AT (x, i);
    a = MAY_TYPE (a) == MAY_FACTOR_T ? MAY_AT (a, 1) : a;
    b = MAY_TYPE (b) == MAY_FACTOR_T ? MAY_AT (b, 1) : b;
    //printf ("%04X ", (unsigned int) MAY_HASH (a)); may_dump (a);
    if (may_identical (a, b) >= 0) {
      //printf ("%04X ", (unsigned int) MAY_HASH (b)); may_dump (b);
      printf ("!!! Error in sorting !!!\n");
      return 0;
    }
  }
  //printf ("%04X ", (unsigned int) MAY_HASH (MAY_AT (x, n-1))); may_dump (MAY_AT (x, n-1));
  return 1;
}
#else
#define check_sum_order(x) 1
#endif

/* From begin to end, sum all the num values */
static MAY_REGPARM may_t
may_SumNum (may_pair_t *begin, may_pair_t *end)
{
  may_pair_t *it;
  may_t sum;

  /* Compute trivial case */
  it = begin;
  MAY_ASSERT (it < end);
  if (MAY_LIKELY (it+1 == end))
    return it->first;

  sum = may_num_add (MAY_DUMMY, it->first, (it+1)->first);
  for (it += 2; it != end; it++)
    sum = may_num_add (sum, sum, it->first);
  return may_num_simplify (sum);
}

#define CMP(i,j) cmp_pair (&tab[i], &tab[j])
#define SWAP(i, j) do {may_pair_t tmp = tab[i]; tab[i] = tab[j]; tab[j]=tmp;} while (0)
#define ROLL(i,j,k) do {may_pair_t tmp = tab[i]; tab[i] = tab[j]; tab[j] = tab[k]; tab[k] = tmp;} while (0)

MAY_INLINE int
cmp_pair (may_pair_t *x, may_pair_t *y)
{
  int i;
  may_t xs = x->second, ys = y->second;

  if (MAY_UNLIKELY (xs == ys))
    return 0;

  i = may_identical (xs, ys);

  /* If the expressions were identical, then we speed up next computing */
  if (MAY_UNLIKELY (i == 0)) {
    if ( (void*) xs < (void*) ys)
      y->second = xs;
    else
      x->second = ys;
  }
  return i;
}

static void
sort_pair2 (may_pair_t *tab, may_size_t size, may_pair_t *t)
{
  /* If small enought, do an inline sort */
  if (MAY_LIKELY (size <= 4)) {
    if (MAY_UNLIKELY (size < 2))
      return;
    else if (size == 2) {
      if (CMP (0, 1) > 0)
	SWAP (0, 1);
      return;
    } else if (size == 3) {
      if (CMP (0, 1) > 0)
	SWAP (0 ,1);
      if (CMP (1, 2) <= 0)
        return;
      if (CMP (0, 2) > 0)
        ROLL (2, 1, 0);
      else
	SWAP (1, 2);
      return;
    } else {
      MAY_ASSERT (size == 4);
#if 1
      if (CMP (0, 1) > 0)
	SWAP (0 ,1);
      if (CMP (2, 3) > 0)
	SWAP (2, 3);
      if (CMP (1, 2) <= 0)
	return;
      if (CMP (0, 2) <= 0) {
	SWAP (1, 2);
        if (CMP (2, 3) > 0)
          SWAP (2, 3);
      } else if (CMP (0, 3) <= 0) {
        ROLL (2, 1, 0);
        if (CMP (2, 3) > 0)
          SWAP (2, 3);
      } else {
        SWAP (0, 2);
        SWAP (1, 3);
      }
      return;
#else
      if (CMP (0, 1) > 0)
	SWAP (0 ,1);
      if (CMP (2, 3) > 0)
	SWAP (2, 3);
      if (CMP (0, 2) > 0) {
	SWAP (0, 2);
        SWAP (1, 3);
      }
      if (CMP (1, 2) <= 0)
        return;
      if (CMP (1, 3) > 0)
        ROLL (1, 2, 3);
      else
        SWAP (1, 2);
      return;
#endif
    }
    MAY_ASSERT (0);
  }

  /* Do a merge sort */
  may_pair_t *tmp, *tab1, *tab2;
  may_size_t n1, n2;

  n1 = size / 2;
  n2 = size - n1;
  tab1 = tab;
  tab2 = tab + n1;

  sort_pair2 (tab1, n1, t);
  sort_pair2 (tab2, n2, t);

  tmp = t;

  for (;;) {
    if (cmp_pair (tab1, tab2) <= 0) {
      *tmp++ = *tab1++;
      if (MAY_UNLIKELY (-- n1 == 0))
	break;
    } else {
      *tmp++ = *tab2++;
      if (MAY_UNLIKELY (-- n2 == 0)) {
	if (n1 > 0)
	  memcpy (tmp, tab1, n1 * sizeof (may_pair_t));
	break;
      }
    }
  }
  memcpy (tab, t, (size - n2) * sizeof (may_pair_t));
}

#if 0
static void
merge_pairs (may_pair_t *tab, may_size_t n1, may_size_t n2)
{
  may_pair_t *tmp, *t, *tab1, *tab2;
  may_size_t size = n1+n2;

  if (n1 == 0 || n2 == 0)
    return;

  MAY_RECORD ();
  tmp = t = may_alloc (size * sizeof * tmp);
  tab1 = tab;
  tab2 = &tab[n1];

  for (;;) {
    if (cmp_pair (tab1, tab2) <= 0) {
      *tmp++ = *tab1++;
      if (MAY_UNLIKELY (-- n1 == 0))
	break;
    } else {
      *tmp++ = *tab2++;
      if (MAY_UNLIKELY (-- n2 == 0)) {
	if (n1 > 0)
	  memcpy (tmp, tab1, n1 * sizeof (may_pair_t));
	break;
      }
    }
  }

  memcpy (tab, t, (size - n2) * sizeof (may_pair_t));
  MAY_CLEANUP ();
}
#endif

// Note: see how to perform a radix like sort?
static void
sort_pair (may_pair_t *tab, may_size_t size)
{
  may_pair_t *it, *t;
  may_size_t i, m, rank;
  may_hash_t previous;

  /* Alloc the temporary table inside MAY stack to avoid
     a system stack overflow (MAY tries to enlarge it if need is) */
  MAY_RECORD ();
  t = MAY_ALLOC (size * sizeof *t);
  if (MAY_LIKELY (size <= MAY_SORT_THRESHOLD1)
      || (size >= MAY_SORT_THRESHOLD2 && size <= MAY_SORT_THRESHOLD3)) {
    sort_pair2  (tab, size, t);
    MAY_CLEANUP ();
    return;
  }

  MAY_ASSERT ((MAY_HASH_MAX & (MAY_HASH_MAX-1)) == 0);
  /* Do a count sort first */
  unsigned int count_tab[MAY_HASH_MAX];
  size_t size_hash_tab = MAY_SIZE_IN_BITS (size);
  size_hash_tab = MIN ( (2U << size_hash_tab) - 1, MAY_HASH_MAX-1);
  int shift_hash = MAY_SIZE_IN_BITS (MAY_HASH_MAX-1)
    - MAY_SIZE_IN_BITS (size_hash_tab);
  memset (count_tab, 0, (size_hash_tab+1) * sizeof (unsigned int));
  if (MAY_UNLIKELY (shift_hash <= 0)) {
    /* No shift for the HASH table. Huge table ==> inline the code */
    for (it = tab, m = size; m -- > 0; )
      count_tab[(unsigned int) MAY_HASH ((*it++).second)] ++;
    for (i = 1; i <= size_hash_tab; i++)
      count_tab[i] += count_tab[i-1];
    for (it = tab, m = size; m -- > 0; ) {
      rank = --count_tab[(unsigned int)MAY_HASH (it->second)];
      t[rank] = *it++;
    }

    /* Merge the remaining items */
    previous = MAY_HASH (t[0].second);
    for (i = 1; i < size; i++) {
      MAY_ASSERT (previous <= (MAY_HASH (t[i].second)));
      if (previous == MAY_HASH (t[i].second)) {
        may_size_t j;
        for (j = i+1; j < size
               && MAY_HASH (t[j].second) == previous; j++);
        sort_pair2 (&t[i-1], j-i+1, &tab[i-1]); /* Hick... slow down 2x*/
        if (MAY_UNLIKELY (j == size))
          break;
        i = j;
      }
      previous = MAY_HASH (t[i].second);
    }
  } else {
    for (it = tab, m = size; m -- > 0; )
      count_tab[(unsigned int) MAY_HASH ((*it++).second) >> shift_hash] ++;
    for (i = 1; i <= size_hash_tab; i++)
      count_tab[i] += count_tab[i-1];
    for (it = tab, m = size; m -- > 0; ) {
      rank = --count_tab[(unsigned int)MAY_HASH (it->second) >> shift_hash];
      t[rank] = *it++;
    }

    /* Merge the remaining items */
    previous = MAY_HASH (t[0].second) >> shift_hash;
    for (i = 1; i < size; i++) {
      MAY_ASSERT (previous <= (MAY_HASH (t[i].second)>>shift_hash));
      if (previous == (MAY_HASH (t[i].second) >> shift_hash)) {
        may_size_t j;
        for (j = i+1; j < size
               && (MAY_HASH (t[j].second) >> shift_hash) == previous; j++);
        sort_pair2 (&t[i-1], j-i+1, &tab[i-1]); /* Hick... slow down 2x*/
        if (MAY_UNLIKELY (j == size))
          break;
        i = j;
      }
      previous = MAY_HASH (t[i].second) >> shift_hash;
    }
  }

  memcpy (tab, t, size * sizeof *t);
  MAY_CLEANUP ();
  return;
}

static MAY_REGPARM may_t
may_eval_sum (may_t x)
{
  may_t y;
  may_t num = MAY_ZERO;
  may_size_t i, nx, nsymb, nsum, nnum, ntotal, ndest;

  /* Pass1: Eval all the sub arguments and class them
            EXT / term / SUM and extract the num part */
  nx = nsymb = MAY_NODE_SIZE(x);
  if (MAY_UNLIKELY (nx == 1))
    return may_eval (MAY_AT (x, 0));
  MAY_ASSERT (nx >= 2);
  nnum = nsum = ntotal = ndest = 0;
  y = x;
  MAY_ASSERT (nsymb >= 2);
  for (i = 0; i < nsymb; i++) {
    may_t arg = may_eval (MAY_AT (x, i));
    may_type_t targ = MAY_TYPE (arg);

    if (MAY_UNLIKELY (targ < MAY_NUM_LIMIT)) {
      MAY_ASSERT (nnum == 0 || MAY_TYPE (num) < MAY_NUM_LIMIT);
      num = (nnum == 0
             ? arg
             : may_num_add ((nnum == 1 ? MAY_DUMMY : num), num, arg));
      nnum ++;
    } else if (MAY_UNLIKELY (targ == MAY_SUM_T)) {
      may_size_t narg = MAY_NODE_SIZE(arg);
      /* Check numerical part of the sum (1st term) */
      if (MAY_PURENUM_P (MAY_AT (arg, 0))) {
        num = (nnum == 0
               ? MAY_AT (arg, 0)
               : may_num_add ((nnum == 1 ? MAY_DUMMY : num), num,
                              MAY_AT (arg, 0)));
        nnum ++;
        narg --;
        if (narg == 1) {
          arg = MAY_AT (arg, 1);
          goto set_current;
        }
      }
      nsymb --;
      MAY_SET_AT (y, i, MAY_AT (x, nsymb));
      MAY_SET_AT (y, nsymb, arg);
      nsum ++;
      ntotal += narg;
      i --;
    } else {
    set_current:
      MAY_SET_AT (y, ndest++, arg);
      MAY_ASSERT (ndest <= (i+1));
      ntotal ++;
    }
  }

  /* We have sorted the SUM in 2: normal / sum
     and extracted the numerical part in num */
  x = y;

  /* Finish by simplifying the constant term of the sum */
  if (!MAY_EVAL_P (num))
    num = may_num_simplify (num);
  MAY_ASSERT (MAY_EVAL_P (num));

  /* Pass 2: Build a large enough table */
  if (MAY_UNLIKELY (ntotal == 0))
    return num;

  /* If the sum is small enough, alloc it in the stack. */
  may_pair_t tab_temp[MAY_SMALL_ALLOC_SIZE+1];
  may_pair_t *tab = ntotal <= MAY_SMALL_ALLOC_SIZE ? &tab_temp : may_alloc ((ntotal+1)*sizeof*tab);
  MAY_FLAG_DECL (flag);
  int is_extension = 0;

  //printf ("\nSum Ntotal=%d NExtra=%d Ndest=%d NSum=%d\n", ntotal, nextra, ndest, nsum);

  /* Pass 3: Extract the (num, key) and fill the table */
  {
    may_t *it = MAY_AT_PTR (x, 0);
    may_pair_t *dest = tab;
    while (ndest-- > 0) {
      may_t a = *it++;
      if (MAY_LIKELY (MAY_TYPE (a) == MAY_FACTOR_T)) {
	dest->first  = MAY_AT (a, 0);
	dest->second = MAY_AT (a, 1);
      } else {
	dest->first  = MAY_ONE;
	dest->second = a;
      }
      /* If it is an extension which has overloaded the
         add operator */
      if (MAY_UNLIKELY (MAY_EXT_P (dest->second)))
        is_extension = MAY_EXT_GETX(dest->second)->add != 0;
      MAY_ASSERT (MAY_EVAL_P (dest->first));
      MAY_ASSERT (MAY_EVAL_P (dest->second));
      dest++;
    }
    // sort_pair (tab, dest-tab);
    if (MAY_UNLIKELY (nsum != 0)) {
      it = MAY_AT_PTR (x, nsymb);
      while (nsum-- > 0) {
	may_t sum = *it++;
	may_size_t ns = MAY_NODE_SIZE(sum);
	may_t *it2 = MAY_AT_PTR (sum, 0);
        // may_pair_t *dest2 = dest;
	if (MAY_PURENUM_P (*it2))
	  it2++, ns--;
	while (ns -- > 0) {
	  may_t a = *it2++;
	  if (MAY_LIKELY (MAY_TYPE (a) == MAY_FACTOR_T)) {
	    dest->first  = MAY_AT (a, 0);
	    dest->second = MAY_AT (a, 1);
	  } else {
	    dest->first  = MAY_ONE;
	    dest->second = a;
	  }
          /* If it is an extension which has overloaded the
             add operator */
          if (MAY_UNLIKELY (MAY_EXT_P (dest->second)))
            is_extension = MAY_EXT_GETX(dest->second)->add != 0;
	  dest++;
	}
        //merge_pairs (tab, dest2-tab, dest-dest2);
      }
    }
    MAY_ASSERT (dest == &tab[ntotal]);
  }

  /* Pass 4: Sort the tab (FIXME: The sums are already sorted...)  */
  sort_pair (tab, ntotal);
  tab[ntotal].second = MAY_ZERO; /* So that the last term is computed */

  /* Pass 5: Factorize and evaluate flags */
  {
    may_pair_t *begin = tab;
    may_t leader = tab[0].second, factor;
    nsymb = 0;
    MAY_ASSERT (ntotal >= 1);
    for (i = 1 ; MAY_LIKELY (i <= ntotal) ; i++) {
      if (may_identical (leader, tab[i].second)) {
        factor = may_SumNum (begin, &tab[i]);
        if (MAY_LIKELY (!may_num_zero_p(factor))) {
          tab[nsymb].first  = factor;
          tab[nsymb].second = leader;
          nsymb++;
          MAY_FLAG_UP (flag, MAY_FLAGS (leader));
        }
        begin  = &tab[i];
        leader = tab[i].second;
      }
    }
  }

  /* Pass 6: Deals with the extensions (since after we will globered x) */
  if (MAY_UNLIKELY (is_extension != 0)) {
    /* Eval the sum of extensions. This perform a second sort
       to sort the extension by priority order */
    may_eval_extension_sum (num, &nsymb, tab);
    /* Resort the array in the semantic order of the expression */
    /* This is the third sort (So not efficient at all). */
    /* In general, extensions reduce greatly the number of arguments
       so this sort should be very fast... */
    sort_pair (tab, nsymb);
    num = MAY_ZERO;
  }

  /* Pass 7: Rebuild the sum and compute HASH */
  if (MAY_UNLIKELY (nsymb <= 1)) {
    if (MAY_UNLIKELY (nsymb < 1))
      y = num; /* No symbol, return the numerical part */
    /* Only one symbol: return NUM + coeff.leader */
    else if (may_num_zero_p (num)) {
      MAY_ASSERT(MAY_PURENUM_P(tab[0].first));
      /* Return  coeff.leader */
      if (MAY_ONE_P (tab[0].first))
	/* Return leader */
	y = tab[0].second;
      else {
	/* Return: coeff.Leader */
	MAY_ASSERT (!MAY_ONE_P(tab[0].second));
	MAY_ASSERT (nx >= 2);  /* Reuse x since x==y when eval */
	y = x;
        MAY_OPEN_C (x, MAY_FACTOR_T);
        MAY_NODE_SIZE(y) = 2;
	MAY_SET_AT (y, 0, tab[0].first);
	MAY_SET_AT (y, 1, tab[0].second);
	MAY_CLOSE_C (y, MAY_FLAG_GET (flag),
		     MAY_NEW_HASH2 (tab[0].first, tab[0].second));
      }
    } else if (MAY_ONE_P (tab[0].first)) {
      /* Return NUM + Leader */
      MAY_ASSERT (nx >= 2);  /* Reuse x since x==y when eval */
      y = x;
      MAY_OPEN_C (y, MAY_SUM_T);
      MAY_NODE_SIZE(y) = 2;
      MAY_SET_AT (y, 0, num);              /* NUM is term 0 */
      MAY_SET_AT (y, 1, tab[0].second);    /* Leader */
      MAY_CLOSE_C (y, MAY_FLAG_GET (flag),
		   MAY_NEW_HASH2 (num, tab[0].second));
    } else {
      /* Return NUM + Coeff.Leader */
      may_t c_leader = MAY_NODE_C (MAY_FACTOR_T, 2);
      MAY_SET_AT (c_leader, 0, tab[0].first);
      MAY_SET_AT (c_leader, 1, tab[0].second);
      MAY_CLOSE_C (c_leader, MAY_FLAGS (tab[0].second),
		   MAY_NEW_HASH2 (tab[0].first, tab[0].second));
      MAY_ASSERT (nx >= 2);          /* Reuse x */
      y = x;
      MAY_OPEN_C (y, MAY_SUM_T);
      MAY_NODE_SIZE(y) = 2;
      MAY_SET_AT (y, 0, num);              /* NUM is term 0 */
      MAY_SET_AT (y, 1, c_leader);         /* Leader */
      MAY_CLOSE_C (y, MAY_FLAG_GET (flag),
		   MAY_NEW_HASH2 (num, c_leader));
    }
  } else {
    /* General case: Return NUM + SUM(ai*Leader[i]) */
    MAY_HASH_DECL (hash);
    int not_zero_p = may_num_zero_p (num) == 0;
    ntotal = nsymb + not_zero_p;
    if (MAY_LIKELY (ntotal <= nx)) /* Try to reuse x if possible */
      { y = x; MAY_OPEN_C (y, MAY_SUM_T); MAY_NODE_SIZE(y) = ntotal; }
    else
      y = MAY_NODE_C (MAY_SUM_T, ntotal);
    ndest = 0;
    /* Set num term */
    if (MAY_LIKELY (not_zero_p)) {
      MAY_SET_AT (y, ndest++, num);
      MAY_HASH_UP (hash, MAY_HASH (num));
    }
    /* Set general term */
    MAY_ASSERT (nsymb >= 1);
    for (i = 0; MAY_LIKELY (i < nsymb); i++) {
      if (MAY_UNLIKELY (MAY_ONE_P (tab[i].first))) {
        MAY_SET_AT (y, ndest++, tab[i].second);
        MAY_HASH_UP (hash, MAY_HASH (tab[i].second));
      } else {
        MAY_ASSERT (!MAY_ONE_P (tab[i].second));
        may_t z = MAY_NODE_C (MAY_FACTOR_T, 2);
        MAY_ASSERT(MAY_PURENUM_P(tab[i].first));
        MAY_SET_AT (z, 0, tab[i].first);
        MAY_SET_AT (z, 1, tab[i].second);
        MAY_CLOSE_C (z, MAY_FLAGS (tab[i].second),
                     MAY_NEW_HASH2 (tab[i].first, tab[i].second));
        MAY_SET_AT (y, ndest++, z);
        MAY_HASH_UP (hash, MAY_HASH (z));
      }
    }
    MAY_CLOSE_C (y, MAY_FLAG_GET (flag), MAY_HASH_GET (hash));
  }
  MAY_ASSERT (MAY_TYPE (y) != MAY_SUM_T || check_sum_order (y));

  return y;
}


static MAY_REGPARM may_t
may_eval_product (may_t x)
{
  may_t y;
  may_t num = MAY_ONE;
  may_size_t i, nx, nsymb, nsum, nnum, ntotal, ndest;

  /* Pass1: Eval all the sub arguments and class them
            EXTRA / term / SUM and extract the num part */
  nx = nsymb = MAY_NODE_SIZE(x);
  if (MAY_UNLIKELY (nx == 1))
    return may_eval (MAY_AT (x, 0));
  MAY_ASSERT (nx >= 2);
  nnum = nsum = ntotal = ndest = 0;
  y = x;
  MAY_ASSERT (nsymb >= 2);
  for (i = 0; i < nsymb; i++) {
    may_t arg = may_eval (MAY_AT (x, i));
    MAY_ASSERT (MAY_EVAL_P (arg));
    switch (MAY_TYPE (arg)) {
    case MAY_INT_T:
    case MAY_RAT_T:
    case MAY_FLOAT_T:
    case MAY_COMPLEX_T:
      MAY_ASSERT (nnum == 0 || MAY_PURENUM_P (num));
      num = (nnum == 0
             ? arg
             : may_num_mul ((nnum == 1 ? MAY_DUMMY : num), num, arg));
      nnum ++;
      break;
    case MAY_SUM_T:
      (void) 0;
      may_t gcd = extract_constant_coefficient_from_sum (arg);
      if (MAY_UNLIKELY (gcd != MAY_ONE)) {
        MAY_ASSERT (nnum == 0 || MAY_PURENUM_P (num));
        num = (nnum == 0 ? gcd
               : may_num_mul ((nnum == 1 ? MAY_DUMMY : num), num, gcd));
        nnum ++;
        arg = fix_sum_after_extracting_coefficient (arg, gcd);
        MAY_ASSERT (MAY_PURENUM_P (num));
      }
      goto set_current;
      break;
    case MAY_FACTOR_T:
      MAY_ASSERT (MAY_PURENUM_P (MAY_AT (arg, 0)));
      MAY_ASSERT (nnum == 0 || MAY_PURENUM_P (num));
      num = (nnum == 0
             ? MAY_AT (arg, 0)
             : may_num_mul ((nnum == 1 ? MAY_DUMMY : num), num,
                            MAY_AT (arg, 0)));
      nnum ++;
      arg = MAY_AT (arg, 1);
      if (MAY_UNLIKELY (MAY_TYPE (arg) == MAY_PRODUCT_T))
        goto set_product;
      else
        goto set_current;
      break;
    case MAY_PRODUCT_T:
      (void) 0;
      may_size_t narg;
    set_product:
      narg = MAY_NODE_SIZE(arg);
      MAY_ASSERT (!MAY_PURENUM_P (MAY_AT (arg, 0)));
      nsymb --;
      MAY_SET_AT (y, i, MAY_AT (x, nsymb));
      MAY_SET_AT (y, nsymb, arg);
      nsum ++;
      ntotal += narg;
      i --;
      break;
    case MAY_STRING_T:
    case MAY_POW_T:
    default:
    set_current:
      MAY_SET_AT (y, ndest++, arg);
      ntotal ++;
    }
  }

  /* We have sorted the PRODUCT in 2: normal & sum
     and extracted the numerical part in num (sum is in fact product) */
  x = y;

  /* Finish by simplifying the constant term of the produt */
  MAY_ASSERT (MAY_PURENUM_P (num));
  if (!MAY_EVAL_P (num))
    num = may_num_simplify (num);
  MAY_ASSERT (MAY_EVAL_P (num) && MAY_PURENUM_P (num));

  /* TODO: Handle case num=0 faster */

  /* Pass 2: Build a large enought table */
  if (MAY_UNLIKELY (ntotal == 0))
    return num;

  /* If the sum is small enough, alloc it in the stack. */
  may_pair_t tab_temp[MAY_SMALL_ALLOC_SIZE+1];
  may_pair_t *tab = ntotal <= MAY_SMALL_ALLOC_SIZE ? &tab_temp : may_alloc ((ntotal+1)*sizeof*tab);
  MAY_FLAG_DECL (flag);
  int is_extension = 0;

  /* Pass 3: Extra the (num, key) and fill the table */
  {
    may_pair_t *dest = tab;
    may_t *rit = MAY_AT_PTR (x, 0);
    while (ndest -- > 0) {
      may_t a = *rit++;
      if (MAY_TYPE (a) == MAY_POW_T && MAY_PURENUM_P (MAY_AT(a, 1))) {
	dest->first  = MAY_AT (a, 1);
	dest->second = MAY_AT (a, 0);
      } else {
	dest->first  = MAY_ONE;
	dest->second = a;
      }
      /* If it is an extension which has overloaded the
         mul operator */
      if (MAY_UNLIKELY (MAY_EXT_P (dest->second)))
        is_extension = MAY_EXT_GETX(dest->second)->mul != 0;
      MAY_ASSERT (MAY_EVAL_P (dest->first) && MAY_EVAL_P (dest->second));
      dest++;
    }
    if (MAY_UNLIKELY (nsum != 0)) {
      rit = MAY_AT_PTR (x, nsymb);
      while (nsum -- > 0) {
	may_t prod = *rit++, *rit2 = MAY_AT_PTR (prod, 0);
	may_size_t ns = MAY_NODE_SIZE(prod);
	while (ns -- > 0) {
	  may_t a = *rit2++;
	  if (MAY_TYPE (a) == MAY_POW_T && MAY_PURENUM_P (MAY_AT (a, 1))){
	    dest->first  = MAY_AT (a, 1);
	    dest->second = MAY_AT (a, 0);
	  } else {
	    dest->first  = MAY_ONE;
	    dest->second = a;
	  }
          /* If it is an extension which has overloaded the
             mul operator */
          if (MAY_UNLIKELY (MAY_EXT_P (dest->second)))
            is_extension = MAY_EXT_GETX(dest->second)->mul != 0;
	  MAY_ASSERT (MAY_EVAL_P (dest->first) && MAY_EVAL_P (dest->second));
	  dest ++;
	}
      }
    }
    MAY_ASSERT ((may_size_t) (dest-tab) == ntotal);
  }

  /* Save the orginal tab if there was an extension (The order must be kept for
     non-commutative extensions */
  may_pair_t *org_tab;
  if (MAY_UNLIKELY (is_extension != 0)) {
    org_tab = may_alloc ( (ntotal+1)*sizeof*tab);
    memcpy (org_tab, tab, (ntotal+1)*sizeof*tab);
  }

  /* Pass 4: Sort the tab (FIXME: The products are already sorted...)  */
  sort_pair (tab, ntotal);
  /* The last term is zero so that we could handle the last term in the factorizing code */
  tab[ntotal].second = MAY_ZERO;

  /* Pass 5: Factorize and evaluate flags.
     A little bit more complicated than sum due to INTEGER^SOMETHING which may simplify
     to an integer or a product of an integer by a power. */
  {
    may_t oldintmod = may_g.frame.intmod;
    may_pair_t *begin = tab;
    may_t leader = tab[0].second, factor;
    /* Disable Integer modulo when computing the sum of exponent */
    may_g.frame.intmod = NULL;
    nsymb = 0;
    MAY_ASSERT (MAY_EVAL_P (leader));
    MAY_ASSERT (ntotal >= 1);
    for (i = 1;  MAY_LIKELY (i <= ntotal); i++) {
      if (may_identical (leader, tab[i].second)) {
        factor = may_SumNum (begin, &tab[i]);
        if (MAY_LIKELY (!may_num_zero_p (factor))) {
          /* sqrt(5)*(-sqrt(5)) gives 5, which is a num term: deal with it */
          if (MAY_UNLIKELY (MAY_TYPE (leader) == MAY_INT_T
                            && MAY_TYPE (factor) == MAY_INT_T)) {
            may_t val = may_num_pow (leader, factor);
            /* val may be returned as leader^factor
               due to intmaxsize overflow */
            if (MAY_UNLIKELY (!MAY_PURENUM_P (val)))
              goto add_in_table;
            /* HACK: num may be in evaluated form. Undo this. */
            if (nnum == 0)
              num = val;
            else if (nnum == 1 || MAY_EVAL_P (num))
              num = may_num_mul (MAY_DUMMY, num, val);
            else
              num = may_num_mul (num, num, val);
            nnum++;
            /* Case: 5^(1/2)*5^(1/2)*5^(1/2) --> 5*5^(1/2) ! */
          } else if (MAY_UNLIKELY (MAY_TYPE (leader) == MAY_INT_T
                                   && MAY_TYPE (factor) == MAY_RAT_T
                                   && begin+1 != &tab[i]
                                   /* Avoid infinite loop */)) {
            may_t val = may_num_pow (leader, factor);
            /* val can't be an integer since factor is a rationnal.
               It can be  [tmpnum*]leader^tmpfactor (I excepted) */
            MAY_ASSERT (MAY_TYPE (val) != MAY_INT_T);
            /* Deal with potential factor term */
            if (MAY_TYPE (val) == MAY_FACTOR_T) {
              /* HACK: num may be in evaluated form. Undo this. */
              may_t val2 = MAY_AT (val, 0);
              val = MAY_AT (val, 1);
              if (nnum == 0)
                num = val2;
              else if (nnum == 1 || MAY_EVAL_P (num))
                num = may_num_mul (MAY_DUMMY, num, val2);
              else
                num = may_num_mul (num, num, val2);
              nnum++;
            }
            /* Another sub-case: (-1) ^ 1/4 * (-1) ^ 1/4 --> I */
            if (MAY_LIKELY (MAY_TYPE (val) == MAY_POW_T)) {
              tab[nsymb].first  = MAY_AT (val, 1);
              tab[nsymb].second = MAY_AT (val, 0);
              MAY_FLAG_UP (flag, MAY_FLAGS (MAY_AT (val, 0)));
            } else {
              MAY_ASSERT (val == MAY_I);
              tab[nsymb].first  = MAY_ONE;
              tab[nsymb].second = MAY_I;
              MAY_FLAG_UP (flag, MAY_FLAGS (MAY_I));
            }
            nsymb++;
          } else {
          add_in_table:
            tab[nsymb].first  = factor;
            tab[nsymb].second = leader;
            nsymb++;
            MAY_FLAG_UP (flag, MAY_FLAGS (leader));
          }
        }
        begin  = &tab[i];
        leader = tab[i].second;
      }
    }
    /* Restore old integer modulo */
    may_g.frame.intmod = oldintmod;
  }

  /* Finish by simplifying the constant term of the product
     which may have changed again */
  if (MAY_UNLIKELY (!MAY_EVAL_P (num)))
    num = may_num_simplify (num);

  /* Pass 6: Deals with the extensions (since after we will globered x) */
  if (MAY_UNLIKELY (is_extension != 0)) {
    may_eval_extension_product (num, &nsymb, tab, ntotal, org_tab);
    MAY_ASSERT (nsymb > 0);
    /* Bug: num may be returned as tab[0] but it shouldn't be treated
       as a symbol. This is really a very bad and horrible hack to fix this.
       The correct way for solving this is to change the interface of extension_product
       I think */
    if (MAY_PURENUM_P (tab[0].second)) {
      MAY_ASSERT (MAY_ONE_P (tab[0].first));
      num = tab[0].second;
      nsymb--;
      memmove (&tab[0], &tab[1], nsymb*sizeof (tab[0]));
    } else
      num = MAY_ONE;
    sort_pair (tab, nsymb);
  }

  if (MAY_UNLIKELY (may_num_zero_p (num)))
    return num;

  /* Pass 7: Rebuild the product and compute HASH */
  if (MAY_UNLIKELY (nsymb <= 1)) {
    if (MAY_UNLIKELY (nsymb < 1))
      y = num; /* No symbol, return the numerical part */
    /* Only one symbol: return NUM * leader^coeff */
    else if (MAY_ONE_P (num)) {     /* Return leader^coeff */
      if (MAY_ONE_P (tab[0].first))
	/* Return leader */
	y = tab[0].second;
      else {
	/* Return: Leader^coeff */
	MAY_ASSERT (!MAY_ONE_P (tab[0].second));
	MAY_ASSERT (nx >= 2);  /* Reuse x since x==y when eval */
	y = x; MAY_OPEN_C (y, MAY_POW_T); MAY_NODE_SIZE(y) = 2;
	MAY_SET_AT (y, 0, tab[0].second);
	MAY_SET_AT (y, 1, tab[0].first);
	MAY_CLOSE_C (y, MAY_FLAG_GET (flag),
		     MAY_NEW_HASH2 (tab[0].second, tab[0].first));
      }
    } else if (MAY_ONE_P (tab[0].first)) {
      /* Return NUM * Leader */
      if (MAY_TYPE (tab[0].second) == MAY_SUM_T) {
	/* Return 2*(x+y) --> 2*x+2*y */
	may_t z = tab[0].second;
	nx = MAY_NODE_SIZE(z);
	y = MAY_NODE_C (MAY_SUM_T, nx);
	for (i = 0; i < nx; i++) {
          MAY_SET_AT (y, i, may_mul_c (num, MAY_AT (z,i)));
        }
	y = may_eval (y);
      } else {
	MAY_ASSERT (nx >= 2);  /* Reuse x since x==y when eval */
	y = x; MAY_OPEN_C (y, MAY_FACTOR_T); MAY_NODE_SIZE(y) = 2;
	MAY_SET_AT (y, 0, num);              /* NUM is term 0 */
	MAY_SET_AT (y, 1, tab[0].second);    /* Leader */
	MAY_CLOSE_C (y, MAY_FLAG_GET (flag),
		     MAY_NEW_HASH2 (num, tab[0].second));
      }
    } else {
      /* Return NUM * Leader^coeff */
      may_t c_leader = MAY_NODE_C (MAY_POW_T, 2);
      MAY_SET_AT (c_leader, 0, tab[0].second);
      MAY_SET_AT (c_leader, 1, tab[0].first);
      MAY_CLOSE_C (c_leader, MAY_FLAGS (tab[0].second),
		   MAY_NEW_HASH2 (tab[0].second, tab[0].first));
      MAY_ASSERT (nx >= 2);          /* Reuse x */
      y = x; MAY_OPEN_C (y, MAY_FACTOR_T); MAY_NODE_SIZE(y) = 2;
      MAY_SET_AT (y, 0, num);              /* NUM is term 0 */
      MAY_SET_AT (y, 1, c_leader);         /* Leader */
      MAY_CLOSE_C (y, MAY_FLAG_GET (flag),
		   MAY_NEW_HASH2 (num, c_leader));
    }

  } else {
    /* General case: Return NUM . PRODUCT (Leader[i]^ai) */
    int is_one = MAY_ONE_P (num);
    MAY_HASH_DECL (hash);
    /* No bug if reeval since x has been reaffected with a copy in this case */
    if (is_one && nsymb <= nx) /* Try to reuse x if possible */
      { y = x; MAY_OPEN_C (y, MAY_PRODUCT_T); MAY_NODE_SIZE(y) = nsymb; }
    else
      y = MAY_NODE_C (MAY_PRODUCT_T, nsymb);
    /* Set general term. nsymb is usually 2 or 3 */
    for (i = 0; i < nsymb; i++) {
      if (MAY_UNLIKELY (MAY_ONE_P (tab[i].first))) {
        MAY_SET_AT (y, i, tab[i].second);
        MAY_HASH_UP (hash, MAY_HASH (tab[i].second));
      } else {
        MAY_ASSERT (!MAY_ONE_P (tab[i].second));
        may_t z = MAY_NODE_C (MAY_POW_T, 2);
        MAY_SET_AT (z, 0, tab[i].second);
        MAY_SET_AT (z, 1, tab[i].first);
        MAY_CLOSE_C (z, MAY_FLAGS (tab[i].second),
                     MAY_NEW_HASH2 (tab[i].second, tab[i].first));
        MAY_SET_AT (y, i, z);
        MAY_HASH_UP (hash, MAY_HASH (z));
      }
    }
    MAY_CLOSE_C (y, MAY_FLAG_GET (flag), MAY_HASH_GET (hash));
    /* Set num term */
    if (MAY_LIKELY (!is_one)) {
      MAY_ASSERT (x != y);
      MAY_ASSERT (MAY_NODE_SIZE(x) >= 2);
      MAY_OPEN_C (x, MAY_FACTOR_T);
      MAY_NODE_SIZE(x) = 2;
      MAY_SET_AT (x, 0, num);
      MAY_SET_AT (x, 1, y);
      MAY_CLOSE_C (x, MAY_FLAGS (y), MAY_NEW_HASH2 (num, y));
      y = x;
    }
  }

  return y;
}

/* If we have overloaded may_eval for internal calls to use
   REGPARM, define a proper standard parm version */
#ifdef may_eval
#undef may_eval
may_t
may_eval (may_t x)
{
  return may_eval_internal (x);
}
#endif
