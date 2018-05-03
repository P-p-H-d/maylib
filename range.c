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

#define LEFT(x)  (MAY_FLOAT(MAY_AT(x, 0)))
#define RIGHT(x) (MAY_FLOAT(MAY_AT(x, 1)))

#undef CHK
#undef CHK1
#undef CHK2
#undef CHK3

#define CHK1(x)   MAY_ASSERT (MAY_TYPE(x) == MAY_RANGE_T)
#define CHK2(x,y) MAY_ASSERT (MAY_TYPE(x) == MAY_RANGE_T && MAY_TYPE (y) == MAY_RANGE_T && x != y)
#define CHK3(x,y,z) MAY_ASSERT (MAY_TYPE(x) == MAY_RANGE_T && MAY_TYPE (y) == MAY_RANGE_T && MAY_TYPE(z) == MAY_RANGE_T && x != y && x != z)

#define NAN_P(x) (mpfr_nan_p (LEFT(x)) || mpfr_nan_p (RIGHT (x)))
#define ZERO_P(x) (MPFR_SIGN (LEFT(x)) <= 0 && MPFR_SIGN (RIGHT(x)) >= 0)
#define POS_P(x) (MPFR_SIGN (LEFT(x)) >= 0)
#define NEG_P(x) (MPFR_SIGN (RIGHT(x)) <= 0)

static may_t
may_range_init (void)
{
  may_t y = MAY_NODE_C (MAY_RANGE_T, 2);
  may_t f1 = MAY_FLOAT_INIT_C ();
  may_t f2 = MAY_FLOAT_INIT_C ();
  MAY_SET_AT (y, 0, f1);
  MAY_SET_AT (y, 1, f2);
  return y;
}

static void
may_range_swap (may_t x, may_t y)
{
  CHK2 (x, y);
  may_t tmp;
  tmp = MAY_AT (x, 0);
  MAY_SET_AT (x, 0, MAY_AT (y, 0));
  MAY_SET_AT (y, 0, tmp);
  tmp = MAY_AT (x, 1);
  MAY_SET_AT (x, 1, MAY_AT (y, 1));
  MAY_SET_AT (y, 1, tmp);
}

static void
may_range_set_z (may_t y, mpz_t z)
{
  CHK1 (y);
  mpfr_set_z (LEFT(y), z, GMP_RNDD);
  mpfr_set_z (RIGHT(y), z, GMP_RNDU);
}

static void
may_range_set_q (may_t y, mpq_t q)
{
  CHK1 (y);
  mpfr_set_q (LEFT(y), q, GMP_RNDD);
  mpfr_set_q (RIGHT(y), q, GMP_RNDU);
}

static void
may_range_set_fr (may_t y, mpfr_t f)
{
  CHK1 (y);
  mpfr_set (LEFT(y), f, GMP_RNDD);
  mpfr_set (RIGHT(y), f, GMP_RNDU);
}

static void
may_range_set_str (may_t y, may_t x)
{
  const char *string = MAY_NAME (x);

  CHK1 (y);
  if (strcmp (string, may_pi_name) == 0) {
    /* Eval PI */
    mpfr_const_pi (LEFT (y), GMP_RNDD);
    mpfr_const_pi (RIGHT(y), GMP_RNDU);
  } else if (string[0] == '#') {
    /* Eval a float string */
    mpfr_set_str (LEFT (y), &string[1], 0, GMP_RNDD);
    mpfr_set_str (RIGHT(y), &string[1], 0, GMP_RNDU);
  } else if (may_real_p (x)) {
    /* A real */
    if (may_nonneg_p (x))
      mpfr_set_ui (LEFT  (y), 0, GMP_RNDN);
    else
      mpfr_set_inf (LEFT (y), -1);
    if (may_nonpos_p (x))
      mpfr_set_ui (RIGHT (y), 0, GMP_RNDN);
    else
      mpfr_set_inf (RIGHT(y), 1);
  } else {
    mpfr_set_nan (LEFT  (y));
    mpfr_set_nan (RIGHT (y));
  }
}

static void
may_range_set (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_set (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_set (RIGHT(dest), RIGHT(x), GMP_RNDU);
}

/*
void
may_range_union (may_t dest, may_t a, may_t b)
{
  CHK3 (dest, a, b);

  if (mpfr_cmp(LEFT(a), LEFT(b)) <= 0)
    mpfr_set (LEFT (dest), LEFT (a), GMP_RNDD);
  else
    mpfr_set (LEFT (dest), LEFT (b), GMP_RNDD);

  if (mpfr_cmp(RIGHT (b), RIGHT (a) ) <= 0)
    mpfr_set (RIGHT (dest), RIGHT (a), GMP_RNDU);
  else
    mpfr_set (RIGHT (dest), RIGHT (b), GMP_RNDU);
} */

static void
may_range_add (may_t dest, may_t a, may_t b)
{
  CHK3 (dest, a, b);
  mpfr_add (LEFT(dest), LEFT(a), LEFT(b), GMP_RNDD);
  mpfr_add (RIGHT(dest), RIGHT(a), RIGHT(b), GMP_RNDU);
}

static void
may_range_mul (may_t dest, may_t a, may_t b)
{
  CHK3 (dest, a, b);

  if (NAN_P (a) || NAN_P (b))
    {
      mpfr_set_nan (LEFT (dest));
      mpfr_set_nan (RIGHT(dest));
    }
  else if (MPFR_SIGN (LEFT(a)) >= 0)
    {
      /* a > 0 */
      if (MPFR_SIGN (LEFT(b)) >= 0)
	{
	  /* b  > 0 */
	  mpfr_mul (LEFT (dest), LEFT (a), LEFT (b), GMP_RNDD);
	  mpfr_mul (RIGHT(dest), RIGHT(a), RIGHT(b), GMP_RNDU);
	}
      else if (MPFR_SIGN (RIGHT(b)) <= 0)
	{
	  /* b < 0 */
	  mpfr_mul (LEFT (dest), RIGHT(a), LEFT (b), GMP_RNDD);
	  mpfr_mul (RIGHT(dest), LEFT (a), RIGHT(b), GMP_RNDU);
	}
      else
	{
	  /* b around 0 */
	  mpfr_mul (LEFT (dest), RIGHT(a), LEFT (b), GMP_RNDD);
	  mpfr_mul (RIGHT(dest), RIGHT(a), RIGHT(b), GMP_RNDU);
	}
    }
  else if (MPFR_SIGN (RIGHT (a)) <= 0)
    {
      /* a < 0 */
      if (MPFR_SIGN (LEFT(b)) >= 0)
	{
	  /* b  > 0 */
	  mpfr_mul (LEFT (dest), LEFT (a), RIGHT(b), GMP_RNDD);
	  mpfr_mul (RIGHT(dest), RIGHT(a), LEFT (b), GMP_RNDU);
	}
      else if (MPFR_SIGN (RIGHT(b)) <= 0)
	{
	  /* b < 0 */
	  mpfr_mul (LEFT (dest), RIGHT(a), RIGHT(b), GMP_RNDD);
	  mpfr_mul (RIGHT(dest), LEFT (a), LEFT (b), GMP_RNDU);
	}
      else
	{
	  /* b around 0 */
	  mpfr_mul (LEFT (dest), LEFT (a), RIGHT(b), GMP_RNDD);
	  mpfr_mul (RIGHT(dest), LEFT (a), LEFT (b), GMP_RNDU);
	}
    }
  else
    {
      /* a around 0 */
      if (MPFR_SIGN (LEFT(b)) >= 0)
	{
	  /* b  > 0 */
	  mpfr_mul (LEFT (dest), LEFT (a), RIGHT(b), GMP_RNDD);
	  mpfr_mul (RIGHT(dest), RIGHT(a), RIGHT(b), GMP_RNDU);
	}
      else if (MPFR_SIGN (RIGHT(b)) <= 0)
	{
	  /* b < 0 */
	  mpfr_mul (LEFT (dest), RIGHT(a), LEFT (b), GMP_RNDD);
	  mpfr_mul (RIGHT(dest), LEFT (a), LEFT (b), GMP_RNDU);
	}
      else
	{
	  /* b around 0: [min(ra*lb,la*rb), max(la*lb,ra*rb)] */
	  mpfr_t tmp;
	  mpfr_init (tmp);

	  mpfr_mul (tmp, RIGHT(a), LEFT(b), GMP_RNDD);
	  mpfr_mul (LEFT(dest), LEFT(a), RIGHT(b), GMP_RNDD);
	  if (mpfr_cmp (tmp, LEFT(dest)) < 0)
	    mpfr_set (LEFT(dest), tmp, GMP_RNDD);

	  mpfr_mul (tmp, LEFT (a), LEFT (b), GMP_RNDU);
	  mpfr_mul (RIGHT(dest), RIGHT (a), RIGHT (b), GMP_RNDU);
	  if (mpfr_cmp (tmp, RIGHT(dest)) > 0)
	    mpfr_set (RIGHT (dest), tmp, GMP_RNDU);

	  mpfr_clear (tmp);
	}
    }
}

static void
may_range_inv (may_t dest, may_t x)
{
  if (NAN_P (x))
    {
      mpfr_set_nan (LEFT (dest));
      mpfr_set_nan (RIGHT(dest));
    }
  else if (MPFR_SIGN (LEFT (x)) <= 0 && MPFR_SIGN (RIGHT (x)) >= 0)
    {
      mpfr_set_inf (LEFT (dest), -1);
      mpfr_set_inf (RIGHT(dest), 1);
    }
  else
    {
      mpfr_ui_div (LEFT (dest), 1, RIGHT(x), GMP_RNDD);
      mpfr_ui_div (RIGHT(dest), 1, LEFT (x), GMP_RNDU); 
    }
}

/* TODO: Simplify the function */
static void
may_range_mul_scal (may_t dest, may_t r, may_t x)
{
  MAY_ASSERT (MAY_TYPE (x) < MAY_NUM_LIMIT);
  if (MAY_UNLIKELY(MAY_TYPE (x) == MAY_COMPLEX_T))
    MAY_THROW (MAY_INVALID_TOKEN_ERR);

  switch (MAY_TYPE (x))
    {
    case MAY_INT_T:
      if (mpz_sgn (MAY_INT (x)) > 0)
	{
	  mpfr_mul_z (LEFT (dest), LEFT (r), MAY_INT (x), GMP_RNDD);
	  mpfr_mul_z (RIGHT(dest), RIGHT(r), MAY_INT (x), GMP_RNDU);
	}
      else
	{
	  mpfr_mul_z (LEFT (dest), RIGHT(r), MAY_INT (x), GMP_RNDD);
	  mpfr_mul_z (RIGHT(dest), LEFT (r), MAY_INT (x), GMP_RNDU);
	}
      break;
    case MAY_RAT_T:
      if (mpq_sgn (MAY_RAT (x)) > 0)
	{
	  mpfr_mul_q (LEFT (dest), LEFT (r), MAY_RAT (x), GMP_RNDD);
	  mpfr_mul_q (RIGHT(dest), RIGHT(r), MAY_RAT (x), GMP_RNDU);
	}
      else
	{
	  mpfr_mul_q (LEFT (dest), RIGHT(r), MAY_RAT (x), GMP_RNDD);
	  mpfr_mul_q (RIGHT(dest), LEFT (r), MAY_RAT (x), GMP_RNDU);
	}
      break;
    case MAY_FLOAT_T:
      if (MPFR_SIGN (MAY_FLOAT (x)) > 0)
	{
	  mpfr_mul (LEFT (dest), LEFT (r), MAY_FLOAT (x), GMP_RNDD);
	  mpfr_mul (RIGHT(dest), RIGHT(r), MAY_FLOAT (x), GMP_RNDU);
	}
      else
	{
	  mpfr_mul (LEFT (dest), RIGHT(r), MAY_FLOAT (x), GMP_RNDD);
	  mpfr_mul (RIGHT(dest), LEFT (r), MAY_FLOAT (x), GMP_RNDU);
	}
      break;
    default:
      MAY_ASSERT(0);
      break;
    }
}

/*
void
may_range_sqrt (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_sqrt (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_sqrt (RIGHT(dest), RIGHT(x), GMP_RNDD);
}*/

static void
may_range_abs (may_t dest, may_t x)
{
  CHK2 (dest, x);
  if (ZERO_P(x))
    {
      mpfr_set_ui (LEFT(dest), 0, GMP_RNDD);
      if (mpfr_cmpabs (LEFT(x), RIGHT (x)) < 0)
	mpfr_set (RIGHT(dest), RIGHT(x), GMP_RNDD);
      else
	mpfr_abs (RIGHT(dest), LEFT (x), GMP_RNDD);
    }
  else if (POS_P(x))
    {
      mpfr_set (LEFT (dest), LEFT (x), GMP_RNDD);
      mpfr_set (RIGHT(dest), RIGHT(x), GMP_RNDU);
    }
  else
    {
      mpfr_abs (LEFT (dest), RIGHT(x), GMP_RNDD);
      mpfr_abs (RIGHT(dest), LEFT (x), GMP_RNDU); 
    }
}

static void
may_range_sign (may_t dest, may_t x)
{
  CHK2 (dest, x);
  if (POS_P (x))
    {
      mpfr_set_ui (LEFT (dest), 1, GMP_RNDD);
      mpfr_set_ui (RIGHT(dest), 1, GMP_RNDU);
    }
  else if (NEG_P (x))
    {
      mpfr_set_si (LEFT (dest), -1, GMP_RNDD);
      mpfr_set_si (RIGHT(dest), -1, GMP_RNDU);
    }
  else
    {
      mpfr_set_si (LEFT (dest), -1, GMP_RNDD);
      mpfr_set_ui (RIGHT(dest), 1, GMP_RNDU);
    }
}

static void
may_range_log (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_log (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_log (RIGHT(dest), RIGHT(x), GMP_RNDU);
}

static void
may_range_exp (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_exp (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_exp (RIGHT(dest), RIGHT(x), GMP_RNDU);
}

static void
may_range_acos (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_acos (LEFT (dest), RIGHT(x), GMP_RNDD);
  mpfr_acos (RIGHT(dest), LEFT (x), GMP_RNDU);
}
 
static void
may_range_asin (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_asin (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_asin (RIGHT(dest), RIGHT(x), GMP_RNDU);
}

static void
may_range_atan (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_atan (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_atan (RIGHT(dest), RIGHT(x), GMP_RNDU);
}

static void
may_range_cosh (may_t dest, may_t x)
{
  CHK2 (dest, x);
  if (NAN_P (x))
    {
      mpfr_set_nan (LEFT (dest));
      mpfr_set_nan (RIGHT(dest));
    }
  else if (POS_P (x))
    {
      mpfr_cosh (LEFT (dest), LEFT (x), GMP_RNDD);
      mpfr_cosh (RIGHT(dest), RIGHT(x), GMP_RNDU);
    }
  else if (NEG_P(x))
    {
      mpfr_cosh (LEFT (dest), RIGHT(x), GMP_RNDD);
      mpfr_cosh (RIGHT(dest), LEFT (x), GMP_RNDU);
    }
  else
    {
      mpfr_cosh (LEFT (dest), LEFT (x), GMP_RNDU);
      mpfr_cosh (RIGHT(dest), RIGHT(x), GMP_RNDU);
      if (mpfr_cmp (LEFT (dest), RIGHT(dest)) > 0)
	mpfr_set (RIGHT(dest), LEFT(dest), GMP_RNDU);
      mpfr_set_ui (LEFT (dest), 1, GMP_RNDD);
    }
}

static void
may_range_sinh (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_sinh (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_sinh (RIGHT(dest), RIGHT(x), GMP_RNDU);
}

static void
may_range_tanh (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_tanh (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_tanh (RIGHT(dest), RIGHT(x), GMP_RNDU);
}

static void
may_range_acosh (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_acosh (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_acosh (RIGHT(dest), RIGHT(x), GMP_RNDU);
}

static void
may_range_asinh (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_asinh (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_asinh (RIGHT(dest), RIGHT(x), GMP_RNDU);
}

static void
may_range_atanh (may_t dest, may_t x)
{
  CHK2 (dest, x);
  mpfr_atanh (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_atanh (RIGHT(dest), RIGHT(x), GMP_RNDU);
}

static void
may_range_pow_ui (may_t dest, may_t x, unsigned long n)
{
  CHK2 (dest, x);
  if ((n & 1) != 0 || POS_P (x)) /* Odd or Pos? */
    {
      mpfr_pow_ui (LEFT (dest), LEFT (x), n, GMP_RNDD);
      mpfr_pow_ui (RIGHT(dest), RIGHT(x), n, GMP_RNDU);
    }
  else if (NEG_P (x))
    {
      mpfr_pow_ui (LEFT (dest), RIGHT(x), n, GMP_RNDD);
      mpfr_pow_ui (RIGHT(dest), LEFT (x), n, GMP_RNDU);
    }
  else
    {
      mpfr_pow_ui (LEFT (dest), LEFT (x), n, GMP_RNDU);
      mpfr_pow_ui (RIGHT(dest), RIGHT(x), n, GMP_RNDU);
      if (mpfr_cmp (LEFT (dest), RIGHT(dest)) > 0)
	mpfr_set (RIGHT (dest), LEFT (dest), GMP_RNDU);
      mpfr_set_ui (LEFT (dest), 0, GMP_RNDD);
    }
}

static void
may_range_pow (may_t dest, may_t x, may_t y)
{
  CHK3 (dest, x, y);
  /* x^y = exp(y*ln(x)) */
  may_t lnx = may_range_init ();
  may_t mul = may_range_init ();
  may_range_log (lnx, x);
  may_range_mul (mul, y, lnx);
  may_range_exp (dest, mul);
}

static void
may_range_floor (may_t dest, may_t x)
{
  CHK2 (dest, x);

  mpfr_rint_floor (LEFT (dest), LEFT (x), GMP_RNDD);
  mpfr_rint_floor (RIGHT(dest), RIGHT(x), GMP_RNDU);
}

static void
may_range_real (may_t dest, may_t x)
{
  if (NAN_P (x)) {
    mpfr_set_inf (LEFT (dest), -1);
    mpfr_set_inf (RIGHT(dest), 1);
  } else {
    mpfr_set (LEFT (dest), LEFT (x), GMP_RNDD);
    mpfr_set (RIGHT(dest), RIGHT(x), GMP_RNDU);
  }
}

static void
may_range_imag (may_t dest, may_t x)
{
  if (NAN_P (x)) {
    mpfr_set_inf (LEFT (dest), -1);
    mpfr_set_inf (RIGHT(dest), 1);
  } else {
    mpfr_set_ui (LEFT (dest), 0, GMP_RNDD);
    mpfr_set_ui (RIGHT(dest), 0, GMP_RNDU);
  }
}

/* TODO: Test && trigo */

/* EvalR function */
static void
may_range_evalr (may_t dest, may_t x)
{
  void (*func)(may_t, may_t);

  switch (MAY_TYPE (x))
    {
    case MAY_INT_T:
      may_range_set_z (dest, MAY_INT (x));
      return;
    case MAY_RAT_T:
      may_range_set_q (dest, MAY_RAT (x));
      return;
    case MAY_FLOAT_T:
      may_range_set_fr (dest, MAY_FLOAT (x));
      return;
    case MAY_STRING_T:
      may_range_set_str (dest, x);
      return;

    case MAY_RANGE_T:
      may_range_set (dest, x);
      return;

    case MAY_SUM_T:
      {
	may_t s1 = may_range_init (), s2 = may_range_init ();
	may_size_t i, n = MAY_NODE_SIZE(x);
	may_range_evalr (dest, MAY_AT (x, 0));
	for (i = 1 ; i < n ; i++)
	  {
	    may_range_evalr (s1, MAY_AT (x, i));
	    may_range_add (s2, dest, s1);
	    may_range_swap (s2, dest);
	  }
	return;
      }
    case MAY_FACTOR_T:
      {
	may_t r1 = may_range_init ();
	may_range_evalr (r1, MAY_AT (x, 1));
	may_range_mul_scal (dest, r1, MAY_AT (x, 0) );
	return ;
      }
    case MAY_PRODUCT_T:
      {
	may_t s1 = may_range_init (), s2 = may_range_init ();
	may_size_t i, n = MAY_NODE_SIZE(x);
	may_range_evalr (dest, MAY_AT (x, 0));
	for (i = 1 ; i < n ; i++)
	  {
	    may_range_evalr (s1, MAY_AT (x, i));
	    may_range_mul (s2, dest, s1);
	    may_range_swap (s2, dest);
	  }
	return;
      }
    case MAY_EXP_T:
      func = may_range_exp;
      goto evalr_func;
    case MAY_LOG_T:
      func = may_range_log;
      goto evalr_func;
    case MAY_ACOS_T:
      func = may_range_acos;
      goto evalr_func;
    case MAY_ASIN_T:
      func = may_range_asin;
      goto evalr_func;
    case MAY_ATAN_T:
      func = may_range_atan;
      goto evalr_func;
    case MAY_COSH_T:
      func = may_range_cosh;
      goto evalr_func;
    case MAY_SINH_T:
      func = may_range_sinh;
      goto evalr_func;
    case MAY_TANH_T:
      func = may_range_tanh;
      goto evalr_func;
    case MAY_ACOSH_T:
      func = may_range_acosh;
      goto evalr_func;
    case MAY_ASINH_T:
      func = may_range_asinh;
      goto evalr_func;
    case MAY_ATANH_T:
      func = may_range_atanh;
      goto evalr_func;
    case MAY_ABS_T:
      func = may_range_abs;
      goto evalr_func;
    case MAY_FLOOR_T:
      func = may_range_floor;
      goto evalr_func;
    case MAY_REAL_T:
      func = may_range_real;
      goto evalr_func;
    case MAY_IMAG_T:
      func = may_range_imag;
      goto evalr_func;
    case MAY_SIGN_T:
      func = may_range_sign;
    evalr_func:
      {
	may_t ex = may_range_init ();
	may_range_evalr (ex, MAY_AT (x, 0));
	(*func) (dest, ex);
	return;
      }

    case MAY_POW_T:
      {
	may_t base, expo;
	base = may_range_init ();
	if (MAY_TYPE (MAY_AT (x, 1)) == MAY_INT_T
	    && mpz_fits_slong_p (MAY_INT(MAY_AT(x, 1))))
	  {
	    long n = mpz_get_si (MAY_INT(MAY_AT(x, 1)));
	    may_range_evalr (base, MAY_AT (x, 0));
	    if (n < 0)
	      {
		expo = may_range_init ();
		may_range_inv (expo, base);
		may_range_pow_ui (dest, expo, -n);
	      }
	    else
	      may_range_pow_ui (dest, base, n);
	  }
	else
	  {
	    expo = may_range_init ();
	    may_range_evalr (base, MAY_AT (x, 0));
	    may_range_evalr (expo, MAY_AT (x, 1));
	    may_range_pow (dest, base, expo);
	  }
	return;
      }
      
    default:
      MAY_THROW (MAY_INVALID_TOKEN_ERR);
      break;
    }
}

may_t
may_evalr (may_t x)
{
  may_t y;

  MAY_LOG_FUNC (("%Y", x));

  may_mark();
  if (MAY_TYPE (x) == MAY_LIST_T) {
    may_size_t i, n = MAY_NODE_SIZE(x);
    y = MAY_NODE_C (MAY_TYPE (x), n);
    for (i = 0 ; i < n ; i++)
      MAY_SET_AT (y, i,  may_evalr (MAY_AT (x, i)));
  } else {
    y = may_range_init ();
    may_range_evalr (y, x);
  }
  return may_keep (may_eval (y));
}

may_t
may_compute_num_floor (may_t z)
{
  volatile may_t y = NULL;
  MAY_ASSERT (may_num_p (z));

  may_mark();

  MAY_TRY {
    may_t r = may_evalr (z), d = may_range_init (), left, right;
    MAY_ASSERT (MAY_TYPE (r) == MAY_RANGE_T);
    may_range_floor (d, r);
    left  = MAY_AT (d, 0);
    right = MAY_AT (d, 1);
    if (mpfr_cmp (MAY_FLOAT (left), MAY_FLOAT (right)) == 0) {
      if (mpfr_nan_p (MAY_FLOAT (left))
          || mpfr_inf_p (MAY_FLOAT (left)))
        y = may_eval (left);
      else {
        mpz_t z;
        mpz_init (z);
        mpfr_get_z (z, MAY_FLOAT (left), MAY_RND);
        y = may_mpz_simplify (MAY_MPZ_NOCOPY_C (z));
      }
    }
  } MAY_CATCH {}
  MAY_ENDTRY;

  return may_keep (y);
}


/* Returns the computed sign of an expression:
    0 unkwown, 1 = 0, 2 >0, 4 <0 */
int
may_compute_sign  (may_t x)
{
  may_t r;
  volatile int ret = 0;
  int max;
  mp_prec_t old = may_kernel_prec (0), prec;

  MAY_LOG_FUNC (("%Y", x));

  if (may_zero_p (x))
    return 1;

  /* MAX: 4 try */
  MAY_TRY
    for (prec = old, max = 3 ; max >= 0 ; max --) {
      MAY_RECORD ();
      r = may_evalr (x);
      if (!NAN_P (r)) {
        if (POS_P (r)) {
          ret = 2 | (mpfr_zero_p (LEFT (r)) != 0);
          MAY_COMPACT_VOID ();
          break;
        } else if (NEG_P (r)) {
          ret = 4 | (mpfr_zero_p (RIGHT (r)) != 0);
          MAY_COMPACT_VOID ();
          break;
        }
      }
      prec += prec/2;
      may_kernel_prec (prec);
      MAY_COMPACT_VOID ();
    }
  MAY_CATCH
    ret = 0;
  MAY_ENDTRY;

  may_kernel_prec (old);
  return ret;
}
