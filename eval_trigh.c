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

static int
is_a_sum_of_pure_imaginary (may_t x)
{
  MAY_ASSERT (MAY_TYPE(x) == MAY_SUM_T);
  may_t num, c, t;
  may_iterator_t it;
  for (num = may_sum_iterator_init (it, x) ;
       may_sum_iterator_end (&c, &t, it) ;
       may_sum_iterator_next (it) )
    if (MAY_TYPE (c) != MAY_COMPLEX_T || MAY_RE(c) != MAY_ZERO)
      return 0;
  return num == MAY_ZERO
    || (MAY_TYPE (num) == MAY_COMPLEX_T && MAY_RE(num) == MAY_ZERO);
}


MAY_REGPARM may_t
may_eval_cosh (may_t z, may_t x)
{
  may_t y;

  MAY_ASSERT (MAY_EVAL_P (z));

  /* cosh(0) = 1 */
  if (MAY_ZERO_P (z))
    y = MAY_ONE;
  /* cosh(FLOAT) = FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    {
      mpfr_t f;
      mpfr_init (f);
      mpfr_cosh (f, MAY_FLOAT (z), MAY_RND);
      y = may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
    }
  /* cosh(COMPLEX(FLOAT)) = */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
	   && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
	   && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_cosh (z));
  /* cosh(acosh(x)) = x */
  else if (MAY_TYPE (z) == MAY_ACOSH_T)
    y = MAY_AT (z, 0);
  /* cosh(asinh(x)) = sqrt(1+x^2) */
  else if (MAY_TYPE (z) == MAY_ASINH_T)
    y = (may_eval) (may_sqrt_c (may_add_c (MAY_ONE, may_sqr_c (MAY_AT (z, 0)))));
  /* cosh(atanh(x)) = 1/sqrt(1-x^2) */
  else if (MAY_TYPE (z) == MAY_ATANH_T)
    y = (may_eval) (may_div_c (MAY_ONE,
                may_sqrt_c (may_sub_c (MAY_ONE, may_sqr_c (MAY_AT (z, 0))))));
  /* cosh(INT*x) -> cosh(-INT*x) if x < 0 */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_REAL_P (MAY_AT (z, 0))
	   && MAY_NEG_P (MAY_AT (z, 0)))
    {
      y = may_cosh_c (may_neg_c (z));
      y = (may_eval) (y);
    }
  /* cosh (I*x) = cos (x) */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_TYPE (MAY_AT (z, 0)) == MAY_COMPLEX_T
           && MAY_ZERO_P (MAY_RE (MAY_AT (z, 0))))
    {
      may_t t = MAY_AT (z, 1);
      may_t f = may_num_simplify (may_num_abs (MAY_DUMMY,
                                               MAY_IM (MAY_AT (z, 0))));
      y = may_cos_c (may_mul_c (t, f));
      y = (may_eval) (y);
    }
  else if (MAY_TYPE (z) == MAY_SUM_T && is_a_sum_of_pure_imaginary(z))
    {
      /* cosh(I*(x1+x2+..)) = cos(-I*I*(x1+x2) */
      y = may_cos_c (may_mul_c (may_mul_c (z, MAY_I), MAY_N_ONE));
      y = may_eval (y);
    }
  /* cosh(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->cosh)
    y = (*MAY_EXT_GETX (z)->cosh) (z);
  /* Rebuild it */
  else
    {
      y = x;
      MAY_SET_AT (y, 0, z);
      MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z) );
    }
  return y;
}

MAY_REGPARM may_t
may_eval_sinh (may_t z, may_t x)
{
  may_t y;

  MAY_ASSERT (MAY_EVAL_P (z));

  /* sinh(0) = 0 */
  if (MAY_ZERO_P (z))
    y = MAY_ZERO;
  /* sinh(FLOAT) = FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    {
      mpfr_t f;
      mpfr_init (f);
      mpfr_sinh (f, MAY_FLOAT (z), MAY_RND);
      y = may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
    }
  /* sinh(COMPLEX(FLOAT)) = */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
	   && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
	   && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_sinh (z));
  /* sinh(asinh(x)) = x */
  else if (MAY_TYPE (z) == MAY_ASINH_T)
    y = MAY_AT (z, 0);
  /* sinh(acosh(x)) = sqrt(x-1)*sqrt(x+1) */
  else if (MAY_TYPE (z) == MAY_ACOSH_T)
    y = (may_eval) (may_mul_c (may_sqrt_c (may_add_c (MAY_AT (z, 0), MAY_N_ONE)),
                               may_sqrt_c (may_add_c (MAY_AT (z, 0), MAY_ONE))));
  /* sinh(atanh(x)) = x/sqrt(1-x^2) */
  else if (MAY_TYPE (z) == MAY_ATANH_T)
    y = (may_eval) (may_div_c (MAY_AT (z, 0),
                               may_sqrt_c (may_sub_c (MAY_ONE, may_sqr_c (MAY_AT (z, 0))))));
  /* sinh (INT*x) -> -sinh(-INT*x) if x < 0 */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_REAL_P (MAY_AT (z, 0))
	   && MAY_NEG_P (MAY_AT (z, 0)))
    {
      y = may_neg_c (may_sinh_c (may_neg_c (z)));
      y = (may_eval) (y);
    }
  /* sinh (I*x) = I*sin (x) */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_TYPE (MAY_AT (z, 0)) == MAY_COMPLEX_T
           && MAY_ZERO_P (MAY_RE (MAY_AT (z, 0))))
    {
      may_t t = MAY_AT (z, 1);
      may_t f = may_num_simplify (may_num_abs (MAY_DUMMY,
                                               MAY_IM (MAY_AT (z, 0))));
      may_t i_or_n_i = may_num_pos_p (MAY_IM (MAY_AT (z, 0)))
        ? MAY_I : may_num_neg (MAY_DUMMY, MAY_I);
      y = may_mul_c (may_sin_c (may_mul_c (t, f)), i_or_n_i);
      y = (may_eval) (y);
    }
  else if (MAY_TYPE (z) == MAY_SUM_T && is_a_sum_of_pure_imaginary(z))
    {
      /* sinh(I*(x1+x2+..)) = I*sin(-I*I*(x1+x2) */
      y = may_mul_c (may_sin_c (may_mul_c (may_mul_c (z, MAY_I), MAY_N_ONE)), MAY_I);
      y = may_eval (y);
    }
  /* sinh(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->sinh)
    y = (*MAY_EXT_GETX (z)->sinh) (z);
  /* Rebuild it */
  else
    {
      y = x;
      MAY_SET_AT (y, 0, z);
      MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z) );
    }
  return y;
}

MAY_REGPARM may_t
may_eval_tanh (may_t z, may_t x)
{
  may_t y;

  MAY_ASSERT (MAY_EVAL_P (z));

  /* tanh(0) = 0 */
  if (MAY_ZERO_P (z))
    y = MAY_ZERO;
  /* tanh(FLOAT) = FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    {
      mpfr_t f;
      mpfr_init (f);
      mpfr_tanh (f, MAY_FLOAT (z), MAY_RND);
      y = may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
    }
  /* tanh(COMPLEX(FLOAT)) = */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
	   && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
	   && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_tanh (z));
  /* tanh(atanh(x)) = x */
  else if (MAY_TYPE (z) == MAY_ATANH_T)
    y = MAY_AT (z, 0);
  /* tanh(acosh(x)) = sqrt(x-1)*sqrt(x+1)/x */
  else if (MAY_TYPE (z) == MAY_ACOSH_T)
    y = (may_eval) (may_div_c (may_mul_c (may_sqrt_c (may_add_c (MAY_AT (z, 0),
                                                                 MAY_N_ONE)),
                                          may_sqrt_c (may_add_c (MAY_AT (z, 0),
                                                                 MAY_ONE))),
                               MAY_AT (z, 0)));
  /* tanh(asinh(x)) = x/sqrt(1+x^2) */
  else if (MAY_TYPE (z) == MAY_ASINH_T)
    y = (may_eval) (may_div_c (MAY_AT (z, 0),
                               may_sqrt_c (may_add_c (MAY_ONE, may_sqr_c (MAY_AT (z, 0))))));
  /* tanh (INT*x) -> -tanh(-INT*x) if x < 0 */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_REAL_P (MAY_AT (z, 0))
	   && MAY_NEG_P (MAY_AT (z, 0)))
    {
      y = may_neg_c (may_tanh_c (may_neg_c (z)));
      y = (may_eval) (y);
    }
  /* tanh (I*x) = I*tan (x) */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_TYPE (MAY_AT (z, 0)) == MAY_COMPLEX_T
           && MAY_ZERO_P (MAY_RE (MAY_AT (z, 0))))
    {
      may_t t = MAY_AT (z, 1);
      may_t f = may_num_simplify (may_num_abs (MAY_DUMMY,
                                               MAY_IM (MAY_AT (z, 0))));
      y = may_mul_c (MAY_I, may_tan_c (may_mul_c (t, f)));
      y = (may_eval) (y);
    }
  /* tanh(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->tanh)
    y = (*MAY_EXT_GETX (z)->tanh) (z);
  /* Rebuild it */
  else
    {
      y = x;
      MAY_SET_AT (y, 0, z);
      MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z) );
    }
  return y;
}

MAY_REGPARM may_t
may_eval_asinh (may_t z, may_t x)
{
  may_t y;

  MAY_ASSERT (MAY_EVAL_P (z));

  y = NULL;
  /* asinh(0)=0 */
  if (MAY_ZERO_P (z))
    y = MAY_ZERO;
  /* asinh(FLOAT) = FLOAT or COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    y = may_num_mpfr_cx_eval (z, mpfr_asinh, may_cxfr_asinh);
  /* asinh(COMPLEX_FLOAT) = COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
	   && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
	   && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_asinh (z));
  /* asinh(-x) = -asinh(x) */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_REAL_P (MAY_AT (z, 0))
	   && MAY_NEG_P (MAY_AT (z, 0))) {
    y = may_neg_c (may_asinh_c (may_neg_c (z)));
    y = (may_eval) (y);
  }
  /* asinh(I*x) = I*asin(x) */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_TYPE (MAY_AT (z, 0)) == MAY_COMPLEX_T
	   && may_num_zero_p (MAY_RE (MAY_AT (z, 0)))) {
    y = may_asin_c (may_mul_c (MAY_AT (z, 1), MAY_IM (MAY_AT (z, 0))));
    y = may_mul_c (MAY_I, y);
    y = (may_eval) (y);
  }
  /* asinh(sinh(x)) = x */
  else if (MAY_TYPE (z) == MAY_SINH_T)
    y = MAY_AT (z, 0);
  /* asinh(cosh(x)) = ? */
  /* asinh(tanh(x)) = ? */
  /* asinh(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->asinh)
    y = (*MAY_EXT_GETX (z)->asinh) (z);
  /* Rebuild it if y = NULL */
  if (y == NULL) {
    y = x;
    MAY_SET_AT (y, 0, z);
    MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
  }
  return y;
}

MAY_REGPARM may_t
may_eval_acosh (may_t z, may_t x)
{
  may_t y;

  MAY_ASSERT (MAY_EVAL_P (z));

  y = NULL;
  if (MAY_TYPE (z) == MAY_INT_T) {
    if (MAY_ZERO_P (z))
      y = (may_eval) (may_mul_vac (MAY_I, MAY_HALF, MAY_PI, NULL));
    else if (MAY_ONE_P (z))
      y = MAY_ZERO;
    else if (MAY_TYPE (z) == MAY_INT_T
             && mpz_cmp_si (MAY_INT (z), -1) == 0)
      y = (may_eval) (may_mul_c (MAY_I, MAY_PI));
  }
  /* acosh(FLOAT) = FLOAT or COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    y = may_num_mpfr_cx_eval (z, mpfr_acosh, may_cxfr_acosh);
  /* acosh(COMPLEX_FLOAT) = COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
	   && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
	   && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_acosh (z));
  /* acosh(cosh(x)) = ? */
  /* acosh(sinh(x)) = ? */
  /* acosh(I*x)     = ? */
  /* acosh(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->acosh)
    y = (*MAY_EXT_GETX (z)->acosh) (z);
  /* Rebuild it if y = NULL */
  if (y == NULL) {
    y = x;
    MAY_SET_AT (y, 0, z);
    MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
  }
  return y;
}

MAY_REGPARM may_t
may_eval_atanh (may_t z, may_t x)
{
  may_t y;

  MAY_ASSERT (MAY_EVAL_P (z));

  y = NULL;
  if (MAY_TYPE (z) == MAY_INT_T)
    {
      if (mpz_cmp_ui (MAY_INT (z), 0) == 0)
	y = MAY_ZERO;
      else if (mpz_cmp_ui (MAY_INT (z), 1) == 0)
	y = MAY_P_INF;
      else if (mpz_cmp_si (MAY_INT (z), -1) == 0)
	y = MAY_N_INF;
    }
  /* atanh(FLOAT) = FLOAT or COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    y = may_num_mpfr_cx_eval (z, mpfr_atanh, may_cxfr_atanh);
  /* atanh(COMPLEX_FLOAT) = COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
	   && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
	   && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_atanh (z));
  /* atanh(-2*x) = -atanh(2*x) */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_REAL_P (MAY_AT (z, 0))
	   && MAY_NEG_P (MAY_AT (z, 0))) {
    y = may_neg_c (may_atanh_c (may_neg_c (z)));
    y = (may_eval) (y);
  }
  /* atanh(I*x) = atan(x)*I */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_TYPE (MAY_AT (z, 0)) == MAY_COMPLEX_T
	   && may_num_zero_p (MAY_RE (MAY_AT (z, 0)))) {
    y = may_atan_c (may_mul_c (MAY_AT (z, 1), MAY_IM (MAY_AT (z, 0))));
    y = may_mul_c (MAY_I, y);
    y = (may_eval) (y);
  }
  /* atanh(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->atanh)
    y = (*MAY_EXT_GETX (z)->atanh) (z);
  /* atanh(tanh(x)) = atanh(tanh(x)) */
  /* Rebuild it if y = NULL */
  if (y == NULL)
    {
      y = x;
      MAY_SET_AT (y, 0, z);
      MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
    }
  return y;
}
