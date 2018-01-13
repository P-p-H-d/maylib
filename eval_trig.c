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


/* Extract the PI term of a sum */
static may_t
may_extract_PI_factor (may_t *factor, may_t x)
{
  MAY_ASSERT (MAY_TYPE (x) == MAY_SUM_T);
  may_size_t i, n = MAY_NODE_SIZE(x);

  for (i = 0; MAY_LIKELY (i < n); i++) {
    may_t z = MAY_AT (x, i);
    if (MAY_PI_P (z))	{
      *factor = MAY_ONE;
      goto move;
    } else if (MAY_TYPE (z) == MAY_FACTOR_T
               && MAY_PI_P (MAY_AT (z,1))
               && MAY_TYPE (MAY_AT (z,0)) == MAY_INT_T) {
      *factor = MAY_AT (z, 0);
    move:
      n--;
      {
        may_t res = MAY_NODE_C (MAY_SUM_T, n);
        may_size_t j;
        for (j = 0; MAY_LIKELY (j < i); j++)
          MAY_SET_AT (res, j, MAY_AT (x, j));
        for ( ; i < n ; i++)
          MAY_SET_AT (res, i, MAY_AT (x, i+1));
        return n == 1 ? MAY_AT (res, 0) : res;
      }
    }
  }
  return (may_t) 0;
}

MAY_REGPARM may_t
may_eval_cos (may_t z, may_t x)
{
  may_t fact;
  may_t y;

  MAY_ASSERT (MAY_EVAL_P (z));

  /* cos(0) = 1 && cos(Pi) == -1 */
  if (MAY_ZERO_P (z))
    y = MAY_ONE;
  else if (MAY_PI_P (z))
    y = MAY_N_ONE;
  /* cos(FLOAT) = FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    {
      mpfr_t f;
      mpfr_init (f);
      mpfr_cos (f, MAY_FLOAT (z), MAY_RND);
      y = may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
    }
  /* cos(COMPLEX(FLOAT)) = */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T &&
	   MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T &&
	   MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_cos (z));
  /* cos(arccos(x)) = x */
  else if (MAY_TYPE (z) == MAY_ACOS_T)
    y = MAY_AT (z, 0);
  /* cos(arcsin(x)) = sqrt(1-x^2) */
  else if (MAY_TYPE (z) == MAY_ASIN_T)
    {
      y = may_sqrt_c (may_sub_c (MAY_ONE, may_sqr_c (MAY_AT (z, 0))));
      y = (may_eval) (y);
    }
  /* sin(arctan(x)) = 1/Sqrt(1+x^2) */
  else if (MAY_TYPE (z) == MAY_ATAN_T)
    {
      y = may_sqrt_c (may_add_c (MAY_ONE, may_sqr_c (MAY_AT (z, 0))));
      y = may_pow_c (y, MAY_N_ONE);
      y = (may_eval) (y);
    }
  /* cos(FRAC*Pi) = ? */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_PI_P (MAY_AT (z, 1)))
    {
      may_t op = MAY_AT (z, 0);
      may_type_t t = MAY_TYPE (op);
      switch (t)
	{
	case MAY_INT_T:	/* cos(N*Pi) = (-1)^N */
	  y = MAY_EVEN_P (op) ? MAY_ONE : MAY_N_ONE;
	  break;
	case MAY_FLOAT_T:
	  {
	    mpfr_t f;
	    mpfr_init (f);
	    mpfr_const_pi (f, MAY_RND);
	    mpfr_mul (f, f, MAY_FLOAT (op), MAY_RND);
	    mpfr_cos (f, f, MAY_RND);
	    y = may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
	    break;
	  }
	case MAY_RAT_T:
	  /* -A/B --> A/B */
	  {
	    int neg = 0;
	    mpz_t q, r;
	    mpz_init (q), mpz_init (r);
	    mpz_fdiv_qr (q, r, mpq_numref (MAY_RAT (op)),
			 mpq_denref (MAY_RAT (op)));
	    /* A/B = I + C/B with 0<= C < B */
	    /* cos(2*Pi/5) = (Sqrt(5)-1)/4 */
	    if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 2) == 0)
	      y = MAY_ZERO;
	    else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 3) == 0) {
              y = MAY_HALF;
              neg = (mpz_cmp_ui (r, 1) != 0);
            } else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 4) == 0) {
              y = may_mul_c (may_sqrt_c (MAY_TWO), MAY_HALF);
              neg = (mpz_cmp_ui (r, 1) != 0);
            } else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 6) == 0) {
              y = may_mul_c (may_sqrt_c (MAY_THREE), MAY_HALF);
              neg = (mpz_cmp_ui (r, 1) != 0);
            } else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 12) == 0) {
              may_t (*f) (may_t, may_t);
              if (mpz_cmp_ui (r, 1) == 0)
                f = may_add_c, neg = 0;
              else if (mpz_cmp_ui (r, 5) == 0)
                f = may_sub_c, neg = 0;
              else if (mpz_cmp_ui (r, 7) == 0)
                f = may_sub_c, neg = 1;
              else {
                MAY_ASSERT (mpz_cmp_ui (r, 11) == 0);
                f = may_add_c, neg = 1;
              }
              y = may_mul_c (may_div_c (may_sqrt_c (may_set_ui (6)),
                                        may_set_ui (4)),
                             (*f) (MAY_ONE, may_div_c (may_sqrt_c (MAY_THREE),
                                                       MAY_THREE)));
            } else {
              mpq_t w;
              mpz_t pika;
              /* First if r > b/2, r = b-r, neg = 1 */
              mpz_init (pika);
              mpz_fdiv_q_2exp (pika, mpq_denref (MAY_RAT (op)), 1);
              if (mpz_cmp (pika, r) < 0) {
		  mpz_sub (r, mpq_denref (MAY_RAT (op)), r);
		  neg = 1;
              }
              /* Compute cos (r/b*PI) */
              mpq_numref (w)[0] = r[0];
              mpq_denref (w)[0] = mpq_denref (MAY_RAT (op))[0];
              may_t q = MAY_MPQ_NOCOPY_C (w);
              q = may_mpq_simplify (q, MAY_RAT (q));
              z = MAY_NODE_C (MAY_FACTOR_T, 2);
              MAY_ASSERT(MAY_PURENUM_P(q));
              MAY_SET_AT (z, 0, q);
              MAY_SET_AT (z, 1, MAY_PI);
              MAY_CLOSE_C (z, MAY_EVAL_F | MAY_NUM_F,
                           MAY_NEW_HASH2 (q, MAY_PI));
              y = MAY_NODE_C (MAY_COS_T, 1);
              MAY_SET_AT (y, 0, z);
              MAY_CLOSE_C (y, MAY_EVAL_F | MAY_NUM_F, MAY_HASH (z));
            }
	    if (mpz_odd_p (q))
	      neg = !neg;
	    if (neg)
	      y = may_neg_c (y);
	    y = may_eval (y);
	    break;
	  }
	default: /* Can't be done. Rebuild it (Use x if possible) */
	  y = x;
	  MAY_SET_AT (y, 0, z);
	  MAY_CLOSE_C (y, MAY_EVAL_F|MAY_NUM_F, MAY_HASH (z) );
	}
    }
  /* cos(INT*x) -> cos(-INT*x) if x < 0 */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_REAL_P (MAY_AT (z, 0))
	   && MAY_NEG_P (MAY_AT (z, 0)))
    {
      may_t t          = MAY_AT (z, 1);
      may_t absfact = may_num_simplify (may_num_abs (MAY_DUMMY, MAY_AT (z, 0)));
      /* Check if simplify: |-1| = 1 */
      if (!MAY_ONE_P (absfact)) {
	may_t tmp = MAY_NODE_C (MAY_FACTOR_T, 2);
        MAY_ASSERT(MAY_PURENUM_P(absfact));
	MAY_SET_AT (tmp, 0, absfact);
	MAY_SET_AT (tmp, 1, t);
	MAY_CLOSE_C (tmp, MAY_FLAGS (MAY_AT (z, 1)),
		     MAY_NEW_HASH2 (absfact, t));
	t = tmp;
      }
      y = x;
      MAY_SET_AT (y, 0, t);
      MAY_CLOSE_C (y, MAY_FLAGS (MAY_AT (z, 0)), MAY_HASH (t));
    }
  /* cos(x+INT*PI) --> (-1)^even(INT)*cos(x) same for sin */
  else if (MAY_TYPE (z) == MAY_SUM_T
	   && (y = may_extract_PI_factor (&fact, z)) != NULL)
    {
      y = may_cos_c (y);
      if (MAY_ODD_P (fact))
	y = may_neg_c (y);
      y = (may_eval) (y);
    }
  /* cos(x+FRAC*PI) --> transforms frac=INT+a/B with a < B */

  /* cos(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->cos)
    y = (*MAY_EXT_GETX (z)->cos) (z);

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
may_eval_sin (may_t z, may_t x)
{
  may_t fact, y;

  MAY_ASSERT (MAY_EVAL_P (z));
  /* sin(0) == sin(Pi) == 0 */
  if (MAY_ZERO_P (z) || MAY_PI_P (z))
    y = MAY_ZERO;
  /* sin(FLOAT) = FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    {
      mpfr_t f;
      mpfr_init (f);
      mpfr_sin (f, MAY_FLOAT (z), MAY_RND);
      y = may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
    }
  /* sin(COMPLEX(FLOAT)) = */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T &&
	   MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T &&
	   MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_sin (z));
  /* sin(arcsin(x)) = x */
  else if (MAY_TYPE (z) == MAY_ASIN_T)
    y = MAY_AT (z, 0);
  /* sin(arccos(x)) = sqrt(1-x^2) */
  else if (MAY_TYPE (z) == MAY_ACOS_T)
    {
      y = may_sqrt_c (may_sub_c (MAY_ONE, may_sqr_c (MAY_AT (z, 0))));
      y = (may_eval) (y);
    }
  /* sin(arctan(x)) = x/Sqrt(1+x^2) */
  else if (MAY_TYPE (z) == MAY_ATAN_T)
    {
      y = may_sqrt_c (may_add_c (MAY_ONE, may_sqr_c (MAY_AT (z, 0))));
      y = may_div_c (MAY_AT (z, 0), y);
      y = (may_eval) (y);
    }
  /* sin(FRAC*Pi) = ? */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_PI_P (MAY_AT (z, 1)))
    {
      may_t op = MAY_AT (z, 0);
      may_type_t t = MAY_TYPE (op);
      switch (t)
	{
	case MAY_INT_T:	/* sin(N*Pi) = 0 */
	  y = MAY_ZERO;
	  break;
	case MAY_FLOAT_T:
	  {
	    mpfr_t f;
	    mpfr_init (f);
	    mpfr_const_pi (f, MAY_RND);
	    mpfr_mul (f, f, MAY_FLOAT (op), MAY_RND);
	    mpfr_sin (f, f, MAY_RND);
	    y = may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
	    break;
	  }
	case MAY_RAT_T:
	  {
	    mpz_t q, r;
	    mpz_init (q), mpz_init (r);
	    mpz_tdiv_qr (q, r, mpq_numref (MAY_RAT (op)),
			 mpq_denref (MAY_RAT (op)));
	    /* A/B = I + C/B with 0<= C < B
	       sin(Pi-x) = sin(x)
	       sin(PI/10) = (sqrt(5)-1)/4 */
	    if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 2) == 0)
	      y = MAY_ONE;
	    else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 3) == 0)
	      y = may_mul_c (may_sqrt_c (MAY_THREE), MAY_HALF);
	    else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 4) == 0)
	      y = may_mul_c (may_sqrt_c (MAY_TWO), MAY_HALF);
	    else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 6) == 0)
	      y = MAY_HALF;
            else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 12) == 0) {
              may_t (*f) (may_t, may_t);
              if (mpz_cmp_ui (r, 1) == 0 || mpz_cmp_ui (r, 11) == 0)
                f = may_sub_c;
              else
                f = may_add_c;
              y = may_mul_c (may_div_c (may_sqrt_c (may_set_ui (6)), may_set_ui (4)),
                             (*f) (MAY_ONE, may_div_c (may_sqrt_c (MAY_THREE), MAY_THREE)));
            } else {
              mpq_t w;
              mpz_t pika;
              /* First if r > b/2, r = b-r */
              mpz_init (pika);
              mpz_fdiv_q_2exp (pika, mpq_denref (MAY_RAT (op)), 1);
              if (mpz_cmp (pika, r) < 0)
                mpz_sub (r, mpq_denref (MAY_RAT (op)), r);
              /* Compute sin (r/b*PI) */
              mpq_numref (w)[0] = r[0];
              mpq_denref (w)[0] = mpq_denref (MAY_RAT (op))[0];
              may_t q = MAY_MPQ_NOCOPY_C (w);
              q = may_mpq_simplify (q, MAY_RAT (q));
              z = MAY_NODE_C (MAY_FACTOR_T, 2);
              MAY_ASSERT(MAY_PURENUM_P(q));
              MAY_SET_AT (z, 0, q);
              MAY_SET_AT (z, 1, MAY_PI);
              MAY_CLOSE_C (z, MAY_EVAL_F | MAY_NUM_F,
                           MAY_NEW_HASH2 (q, MAY_PI));
              y = MAY_NODE_C (MAY_SIN_T, 1);
              MAY_SET_AT (y, 0, z);
              MAY_CLOSE_C (y, MAY_EVAL_F | MAY_NUM_F, MAY_HASH (z));
            }
	    if (((mpz_odd_p (q) != 0) ^ (mpz_sgn (r) < 0)) != 0)
	      y = may_neg_c (y);
	    y = (may_eval) (y);
	    break;
	  }
	default:
	  y = x;
	  MAY_SET_AT (y, 0, z);
	  MAY_CLOSE_C (y, MAY_EVAL_F | MAY_NUM_F, MAY_HASH (z));
	}
    }
  /* SIN(INT*x) -> -sin(-INT*x) if x < 0 */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_REAL_P (MAY_AT (z, 0))
	   && MAY_NEG_P (MAY_AT (z, 0)))
    {
      may_t n = may_num_simplify (may_num_abs (MAY_DUMMY, MAY_AT (z, 0)));
      may_t t;
      if (MAY_ONE_P (n))	  /* |-x| --> x */
	t = MAY_AT (z, 1);
      else
	{
	  t = MAY_NODE_C (MAY_FACTOR_T, 2);
          MAY_ASSERT(MAY_PURENUM_P(n));
	  MAY_SET_AT (t, 0, n);
	  MAY_SET_AT (t, 1, MAY_AT (z, 1));
	  MAY_CLOSE_C (t, MAY_FLAGS (MAY_AT (z, 1)),
		       MAY_NEW_HASH2 (n, MAY_AT (z, 1)));
	}
      may_t sz = MAY_NODE_C (MAY_SIN_T, 1); /* can't use 'x' */
      MAY_SET_AT (sz, 0, t);
      MAY_CLOSE_C (sz, MAY_FLAGS (t), MAY_HASH (t));
      y = MAY_NODE_C (MAY_FACTOR_T, 2);
      MAY_SET_AT (y, 0, MAY_N_ONE);
      MAY_SET_AT (y, 1, sz);
      MAY_CLOSE_C (y, MAY_FLAGS (sz),
		   MAY_NEW_HASH2 (MAY_N_ONE, sz));
    }
  /* sin(x+INT*PI) --> (-1)^even(INT)*sin(x) */
  else if (MAY_TYPE (z) == MAY_SUM_T
	   && (y = may_extract_PI_factor (&fact, z)) != NULL)
    {
      y = may_sin_c (y);
      if (MAY_ODD_P (fact))
	y = may_neg_c (y);
      y = (may_eval) (y);
    }
  /* sin(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->sin)
    y = (*MAY_EXT_GETX (z)->sin) (z);
  /* Rebuild it */
  else
    {
      y = x;
      MAY_SET_AT (y, 0, z);
      MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
    }
  return y;
}

MAY_REGPARM may_t
may_eval_tan (may_t z, may_t x)
{
  may_t fact, y;

  MAY_ASSERT (MAY_EVAL_P (z));

  /* tan(0) == tan(Pi) == 0 */
  if (MAY_ZERO_P (z) || MAY_PI_P (z))
    y = MAY_ZERO;
  /* tan(FLOAT) = FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    {
      mpfr_t f;
      mpfr_init (f);
      mpfr_tan (f, MAY_FLOAT (z), MAY_RND);
      y = may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
    }
  /* tan(COMPLEX(FLOAT)) = */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
	   && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
	   && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_tan (z));
  /* tan(arctan(x)) = x */
  else if (MAY_TYPE (z) == MAY_ATAN_T)
    y = MAY_AT (z, 0);
  /* tan(arcsin(x)) = x / sqrt(1-x^2) */
  else if (MAY_TYPE (z) == MAY_ASIN_T)
    {
      y = may_sqrt_c (may_sub_c (MAY_ONE, may_sqr_c (MAY_AT (z, 0))));
      y = (may_eval) (may_div_c (MAY_AT (z, 0), y));
    }
  /* tan(arccos(x)) = sqrt(1-x^2)/x */
  else if (MAY_TYPE (z) == MAY_ACOS_T)
    {
      y = may_sqrt_c (may_sub_c (MAY_ONE, may_sqr_c (MAY_AT (z, 0))));
      y = (may_eval) (may_div_c (y, MAY_AT (z, 0)));
    }
  /* tan(FRAC*Pi) = ? */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_PI_P (MAY_AT (z, 1)))
    {
      may_t op = MAY_AT (z, 0);
      may_type_t t = MAY_TYPE (op);
      switch (t)
	{
	case MAY_INT_T:	/* tan(N*Pi) = 0 */
	  y = MAY_ZERO;;
	  break;
	case MAY_FLOAT_T:
	  {
	    mpfr_t f;
	    mpfr_init (f);
	    mpfr_const_pi (f, MAY_RND);
	    mpfr_mul (f, f, MAY_FLOAT (op), MAY_RND);
	    mpfr_tan (f, f, MAY_RND);
	    y = may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
	    break;
	  }
	case MAY_RAT_T:
	  {
	    int neg = 0;
	    mpz_t q, r, pika;
	    mpz_init (q), mpz_init (r), mpz_init (pika);
	    mpz_fdiv_qr (q, r, mpq_numref (MAY_RAT (op)),
			 mpq_denref (MAY_RAT (op)));
	    /* A/B = I + C/B with 0<= C < B
	       First if r > b/2, r = b-r since tan(Pi-x) = tan(x) */
	    mpz_fdiv_q_2exp (pika, mpq_denref (MAY_RAT (op)), 1);
	    if (mpz_cmp (pika, r) < 0)
	      {
		mpz_sub (r, mpq_denref (MAY_RAT (op)), r);
		neg = 1;
	      }
	    /* Check special values
	       Don't need to check the numerator since it should be 1
	       if the denominator is 2, 3, 4 or 6 (not for 12) */
	    if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 2) == 0)
	      y = MAY_NAN;
	    else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 3) == 0)
	      y = may_sqrt_c (MAY_THREE);
	    else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 4) == 0)
	      y = MAY_ONE;
	    else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 6) == 0)
	      y = may_div_c (may_sqrt_c (MAY_THREE), MAY_THREE);
            /* SQRT(5-2*SQRT(5)) -> tan(pi/5) */
            else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 5) == 0
                     && mpz_cmp_ui (r, 1) == 0) {
              y = may_set_ui (5);
              y = may_sqrt_c (may_sub_c (y, may_mul_c (MAY_TWO, may_sqrt_c (y))));
            }
            /* SQRT(25-10*SQRT(5))/5 -> tan(pi/10) */
            else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 10) == 0
                     && mpz_cmp_ui (r, 1) == 0) {
              y = may_set_ui (5);
              y = may_div_c (may_sqrt_c (may_sub_c (may_set_ui (25), may_mul_c (may_set_ui (10), may_sqrt_c (y)))), y);
            }
            else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 12) == 0
                     && mpz_cmp_ui (r, 1) == 0)
              y = may_sub_c (MAY_TWO, may_sqrt_c (MAY_THREE));
            else if (mpz_cmp_ui (mpq_denref (MAY_RAT (op)), 12) == 0
                     && mpz_cmp_ui (r, 5) == 0)
              y = may_add_c (MAY_TWO, may_sqrt_c (MAY_THREE));
	    else {
              mpq_t w;
              /* Compute tan (r/b*PI) */
              mpq_numref (w)[0] = r[0];
              mpq_denref (w)[0] = mpq_denref (MAY_RAT (op))[0];
              may_t q = MAY_MPQ_NOCOPY_C (w);
              q = may_mpq_simplify (q, MAY_RAT (q));
              z = MAY_NODE_C (MAY_FACTOR_T, 2);
              MAY_ASSERT(MAY_PURENUM_P(q));
              MAY_SET_AT (z, 0, q);
              MAY_SET_AT (z, 1, MAY_PI);
              MAY_CLOSE_C (z, MAY_EVAL_F | MAY_NUM_F,
                           MAY_NEW_HASH2 (q, MAY_PI));
              y = MAY_NODE_C (MAY_TAN_T, 1);
              MAY_SET_AT (y, 0, z);
              MAY_CLOSE_C (y, MAY_EVAL_F | MAY_NUM_F, MAY_HASH (z));
            }
	    if (neg)
	      y = may_neg_c (y);
	    y = (may_eval) (y);
	    break;
	  }
	default:
	  y = x;
	  MAY_SET_AT (y, 0, z);
	  MAY_CLOSE_C (y, MAY_EVAL_F | MAY_NUM_F, MAY_HASH (z) );
	}
    }
  /* TAN(INT*x) -> -tan(-INT*x) if x < 0 */
  else if (MAY_TYPE (z) == MAY_FACTOR_T &&
	   MAY_REAL_P (MAY_AT (z, 0)) && MAY_NEG_P (MAY_AT (z, 0)))
    {
      may_t n = may_num_simplify (may_num_abs (MAY_DUMMY, MAY_AT (z, 0)));
      may_t t;
      if (MAY_ONE_P (n))	/* |-x| = x --> Simplify */
	t = MAY_AT (z, 1);
      else
	{
	  t = MAY_NODE_C (MAY_FACTOR_T, 2);
          MAY_ASSERT(MAY_PURENUM_P(n));
	  MAY_SET_AT (t, 0, n);
	  MAY_SET_AT (t, 1, MAY_AT (z, 1));
	  MAY_CLOSE_C (t, MAY_FLAGS (MAY_AT (z, 1)),
		       MAY_NEW_HASH2 (n, MAY_AT (z, 1)));
	}
      may_t sz = MAY_NODE_C (MAY_TAN_T, 1); /* Can't reuse 'x' */
      MAY_SET_AT (sz, 0, t);
      MAY_CLOSE_C (sz, MAY_FLAGS (t), MAY_HASH (t));
      y = MAY_NODE_C (MAY_FACTOR_T, 2);
      MAY_SET_AT (y, 0, MAY_N_ONE);
      MAY_SET_AT (y, 1, sz);
      MAY_CLOSE_C (y, MAY_FLAGS (sz), MAY_NEW_HASH2 (MAY_N_ONE, sz));
    }
  /* tan(x+INT*PI) --> tan(x) */
  else if (MAY_TYPE (z) == MAY_SUM_T
	   && (y = may_extract_PI_factor (&fact, z)) != NULL)
    y = (may_eval) (may_tan_c (y));
  /* tan(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->tan)
    y = (*MAY_EXT_GETX (z)->tan) (z);
  /* Rebuild it */
  else
    {
      y = x;
      MAY_SET_AT (y, 0, z);
      MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
    }
  return y;
}

MAY_REGPARM may_t
may_eval_asin (may_t z, may_t x)
{
  may_t y;

  MAY_ASSERT (MAY_EVAL_P (z));

  y = NULL;
  /* asin(0)=0 asin(1)=Pi/2 asin(-1)=-Pi/2 */
  if (MAY_TYPE (z) == MAY_INT_T)
    {
      if (mpz_cmp_ui (MAY_INT (z), 0) == 0)
	y = MAY_ZERO;
      else if (mpz_cmp_ui (MAY_INT (z), 1) == 0)
	y = may_mul (MAY_HALF, MAY_PI);
      else if (mpz_cmp_si (MAY_INT (z), -1) == 0)
	y = may_mul (MAY_N_HALF, MAY_PI);
    }
  /* asin(1/2)=Pi/6 asin(-1/2)=-Pi/6 */
  else if (MAY_TYPE (z) == MAY_RAT_T)
    {
      if (mpq_cmp_ui (MAY_RAT (z), 1, 2) == 0)
	y = (may_eval) (may_div_c (MAY_PI, may_set_ui (6)));
      else if (mpq_cmp_si (MAY_RAT (z), -1, 2) == 0)
	y = (may_eval) (may_div_c (MAY_PI, may_set_si (-6)));
    }
  /* asin(FLOAT) = FLOAT or COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    y = may_num_mpfr_cx_eval (z, mpfr_asin, may_cxfr_asin);
  /* asin(COMPLEX_FLOAT) = COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
	   && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
	   && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_asin (z));
  /* asin(-x) = -asin(x) */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_REAL_P (MAY_AT (z, 0))
	   && MAY_NEG_P (MAY_AT (z, 0)))
    {
      y = may_neg_c (may_asin_c (may_neg_c (z)));
      y = (may_eval) (y);
    }
  /* asin(sqrt(2)/2) = Pi/4 asin(sqrt(3)/3)=Pi/3 */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_TYPE (MAY_AT (z, 1)) == MAY_POW_T
	   && MAY_TYPE (MAY_AT (MAY_AT (z, 1), 0)) == MAY_INT_T
	   && may_identical (MAY_AT (MAY_AT (z, 1), 1), MAY_HALF) == 0)
    {
      if (mpz_cmp_ui (MAY_INT (MAY_AT (MAY_AT (z, 1), 0)), 2) == 0
          && may_identical (MAY_HALF, MAY_AT (z, 0)) == 0)
	y = (may_eval) (may_div_c (MAY_PI, may_set_ui (4)));
      else if (mpz_cmp_ui (MAY_INT (MAY_AT (MAY_AT (z, 1), 0)), 3) == 0
               && may_identical (may_set_si_ui (1, 3), MAY_AT (z, 0)) == 0)
	y = (may_eval) (may_div_c (MAY_PI, MAY_THREE));
    }
  /* asin(I*x) = I*arcsinh(x) */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_TYPE (MAY_AT (z, 0)) == MAY_COMPLEX_T
	   && may_num_zero_p (MAY_RE (MAY_AT (z, 0))))
    {
      y = may_asinh_c (may_mul_c (MAY_AT (z, 1), MAY_IM (MAY_AT (z, 0))));
      y = may_mul_c (MAY_I, y);
      y = (may_eval) (y);
    }
  /* asin(sin(x)) = ? */
  else if (MAY_TYPE (z) == MAY_SIN_T && may_num_p (MAY_AT (z,0)))
    {
      /* ASin(Sin(x)): Try to compute floor((x+Pi/2)/Pi)
	 If Returns an int, if int is even returns "x-N*PI", else
	 if int is odd, returns "N*PI-x" */
      may_t tmp;
      MAY_RECORD ();
      tmp = may_mul_c (MAY_HALF, MAY_PI);
      tmp = may_add_c (MAY_AT (z, 0), tmp);
      tmp = may_div_c (tmp, MAY_PI);
      tmp = may_floor_c (tmp);
      tmp = may_eval (tmp);
      if (MAY_TYPE (tmp) == MAY_INT_T)
	{
	  if (mpz_even_p (MAY_INT (tmp)))
	    y = may_sub_c (MAY_AT (z, 0), may_mul_c (tmp, MAY_PI));
	  else
	    y = may_sub_c (may_mul_c (tmp, MAY_PI), MAY_AT (z, 0));
	  y = may_eval (y);
	}
      MAY_COMPACT (y);
    }
  /* asin(cos(x)) = PI/2-acos(cos(x)) */
  else if (MAY_TYPE (z) == MAY_COS_T)
    {
      y = may_sub_c (may_mul_c (MAY_HALF, MAY_PI),
		   may_acos_c (may_cos_c (MAY_AT (z, 0))));
      y = (may_eval) (y);
    }
  /* asin(cosh(x)) = ? */
  /* asin(I*x) = arcsinh(-I*x)*I */
  /* asin(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->asin)
    y = (*MAY_EXT_GETX (z)->asin) (z);
  /* Rebuild it if y = NULL */
  if (y == NULL)
    {
      y = x;
      MAY_SET_AT (y, 0, z);
      MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
    }
  return y;
}

MAY_REGPARM may_t
may_eval_acos (may_t z, may_t x)
{
  may_t y;

  MAY_ASSERT (MAY_EVAL_P (z));

  y = NULL;
  if (MAY_TYPE (z) == MAY_INT_T)
    {
      /* acos(0)=PI/2    acos(1)=0  acos(-1)=PI */
      if (mpz_cmp_ui (MAY_INT (z), 0) == 0)
	y = may_mul (MAY_HALF, MAY_PI);
      else if (mpz_cmp_ui (MAY_INT (z), 1) == 0)
	y = MAY_ZERO;
      else if (mpz_cmp_si (MAY_INT (z), -1) == 0)
	y = MAY_PI;
    }
  else if (MAY_TYPE (z) == MAY_RAT_T)
    {
      /* acos(1/2)=PI/3   acos(-1/2)=2/3*PI */
      if (mpq_cmp_ui (MAY_RAT (z), 1, 2) == 0)
	y = may_div (MAY_PI, MAY_THREE);
      else if (mpq_cmp_si (MAY_RAT (z), -1, 2) == 0)
	y = may_mul (MAY_PI, may_set_si_ui (2, 3) );
    }
  /* acos(FLOAT) = FLOAT or COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    y = may_num_mpfr_cx_eval (z, mpfr_acos, may_cxfr_acos);
  /* acos(COMPLEX_FLOAT) = COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
	   && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
	   && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_acos (z));
  /* acos(-2*x) = PI-acos(2*x) */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_REAL_P (MAY_AT (z, 0))
	   && MAY_NEG_P (MAY_AT (z, 0)))
    {
      y = may_sub_c (MAY_PI, may_acos_c (may_neg_c (z)));
      y = (may_eval) (y);
    }
  /* acos(sqrt(2)/2)=PI/4 acos(sqrt(3)/2)=PI/6 */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_TYPE (MAY_AT (z, 1)) == MAY_POW_T
	   && MAY_TYPE (MAY_AT (MAY_AT (z, 1), 0)) == MAY_INT_T
	   && may_identical (MAY_AT (MAY_AT (z, 1), 1), MAY_HALF) == 0)
    {
      if (mpz_cmp_ui (MAY_INT (MAY_AT (MAY_AT (z, 1), 0)), 2) == 0
          && may_identical (MAY_HALF, MAY_AT (z, 0)) == 0)
	y = (may_eval) (may_div_c (MAY_PI, may_set_ui (4)));
      else if (mpz_cmp_ui (MAY_INT (MAY_AT (MAY_AT (z, 1), 0)), 3) == 0
               && may_identical (may_set_si_ui (1,3), MAY_AT (z, 0)) == 0)
	y = (may_eval) (may_div_c (MAY_PI, may_set_ui (6)));
    }
  else if (MAY_TYPE (z) == MAY_COS_T && may_num_p (MAY_AT (z,0)))
    {
      /* acos(cos(x)): Try to compute floor(x/Pi)
	 If Returns an int, if int is odd returns "(N+1)*PI-x", else
	 if int is even, returns "x-N*PI" */
      may_t tmp;
      MAY_RECORD ();
      tmp = may_div_c (MAY_AT (z, 0), MAY_PI);
      tmp = may_floor_c (tmp);
      tmp = (may_eval) (tmp);
      if (MAY_TYPE (tmp) == MAY_INT_T)
	{
	  if (mpz_odd_p (MAY_INT (tmp)))
	    y = may_sub_c (may_mul_c (may_add_c (tmp, MAY_ONE), MAY_PI), MAY_AT (z, 0));
	  else
	    y = may_sub_c (MAY_AT (z, 0), may_mul_c (tmp, MAY_PI));
	  y = (may_eval) (y);
	}
      MAY_COMPACT (y);
    }
  /* acos(sin(x)) = PI/2-asin(sin(x)) */
  else if (MAY_TYPE (z) == MAY_SIN_T)
    {
      y = may_sub_c (may_mul_c (MAY_HALF, MAY_PI),
		   may_asin_c (may_sin_c (MAY_AT (z, 0))));
      y = (may_eval) (y);
    }
  /* acos(I*x)=Pi/2-I*asinh(x); */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_TYPE (MAY_AT (z, 0)) == MAY_COMPLEX_T
	   && may_num_zero_p (MAY_RE (MAY_AT (z, 0))))
    {
      y = may_asinh_c (may_mul_c (MAY_AT (z, 1), MAY_IM (MAY_AT (z, 0))));
      y = may_sub_c (may_mul_c (MAY_HALF, MAY_PI), may_mul_c (MAY_I, y));
      y = (may_eval) (y);
    }
  /* acos(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->acos)
    y = (*MAY_EXT_GETX (z)->acos) (z);
  /* Rebuild it if y = NULL */
  if (y == NULL)
    {
      y = x;
      MAY_SET_AT (y, 0, z);
      MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
    }
  return y;
}

MAY_REGPARM may_t
may_eval_atan (may_t z, may_t x)
{
  may_t y;

  MAY_ASSERT (MAY_EVAL_P (z));

  y = NULL;
  if (MAY_TYPE (z) == MAY_INT_T)
    {
      /* atan(0)=0  acos(1)=Pi/4  acos(-1)=-PI/4 */
      if (mpz_cmp_ui (MAY_INT (z), 0) == 0)
	y = MAY_ZERO;
      else if (mpz_cmp_ui (MAY_INT (z), 1) == 0)
	y = may_mul (MAY_PI, may_set_si_ui (1, 4));
      else if (mpz_cmp_si (MAY_INT (z), -1) == 0)
	y = may_mul (MAY_PI, may_set_si_ui (-1, 4));
    }
  /* atan(FLOAT) = FLOAT or COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_FLOAT_T)
    {
      if (mpfr_inf_p (MAY_FLOAT (z)))
	y = may_mul (MAY_PI, mpfr_sgn (MAY_FLOAT (z)) > 0 ? MAY_HALF : MAY_N_HALF);
      else if (mpfr_nan_p (MAY_FLOAT (z)))
	y = MAY_NAN;
      else
	y = may_num_mpfr_cx_eval (z, mpfr_atan, may_cxfr_atan);
    }
  /* acos(COMPLEX_FLOAT) = COMPLEX_FLOAT */
  else if (MAY_TYPE (z) == MAY_COMPLEX_T
	   && MAY_TYPE (MAY_RE (z)) == MAY_FLOAT_T
	   && MAY_TYPE (MAY_IM (z)) == MAY_FLOAT_T)
    y = may_cx_simplify (may_cxfr_atan (z));
  /* atan(-2*x) = -atan(2*x) */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_REAL_P (MAY_AT (z, 0))
	   && MAY_NEG_P (MAY_AT (z, 0)))
    {
      y = may_neg_c (may_atan_c (may_neg_c (z)));
      y = (may_eval) (y);
    }
  /* atan(sqrt(3)/3)=PI/6 */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_TYPE (MAY_AT (z, 0)) == MAY_RAT_T
	   && mpq_cmp_ui (MAY_RAT (MAY_AT (z, 0)), 1, 3) == 0
	   && MAY_TYPE (MAY_AT (z, 1)) == MAY_POW_T
	   && MAY_TYPE (MAY_AT (MAY_AT (z, 1), 0)) == MAY_INT_T
	   && may_identical (MAY_AT (MAY_AT (z, 1), 1), MAY_HALF) == 0
	   && mpz_cmp_ui (MAY_INT (MAY_AT (MAY_AT (z, 1), 0)), 3) == 0)
    y = may_div (MAY_PI, may_set_ui (6));
  /* atan(sqrt(3)) = Pi/3 */
  else if (MAY_TYPE (z) == MAY_POW_T
	   && MAY_TYPE (MAY_AT (z, 0)) == MAY_INT_T
	   && MAY_TYPE (MAY_AT (z, 1)) == MAY_RAT_T
	   && mpz_cmp_ui (MAY_INT (MAY_AT (z, 0)), 3) == 0
	   && mpq_cmp_ui (MAY_RAT (MAY_AT (z, 1)), 1, 2) == 0)
    y = may_div (MAY_PI, MAY_THREE);
  /* atan(SQRT(5-2*SQRT(5))) -> pi/5 */
  /* atan(SQRT(25-10*SQRT(5))/5) -> pi/10 */
  /* atan(I*x) = arctanh(y)*I */
  else if (MAY_TYPE (z) == MAY_FACTOR_T
	   && MAY_TYPE (MAY_AT (z, 0)) == MAY_COMPLEX_T
	   && may_num_zero_p (MAY_RE (MAY_AT (z, 0))))
    {
      y = may_atanh_c (may_mul_c (MAY_AT (z, 1), MAY_IM (MAY_AT (z, 0))));
      y = may_mul_c (MAY_I, y);
      y = (may_eval) (y);
    }
  /* atan(tan(x)) = ? */
  else if (MAY_TYPE (z) == MAY_TAN_T && may_num_p (MAY_AT (z,0)))
    {
      /* atan(tan(x)): Try to compute floor(x/Pi+(1/2))
	 If Returns an int, returns "x-N*PI" */
      may_t tmp;
      MAY_RECORD ();
      tmp = may_div_c (MAY_AT (z, 0), MAY_PI);
      tmp = may_add_c (tmp, MAY_HALF);
      tmp = may_floor_c (tmp);
      tmp = (may_eval) (tmp);
      if (MAY_TYPE (tmp) == MAY_INT_T) {
        y = may_sub_c (MAY_AT (z, 0), may_mul_c (tmp, MAY_PI));
        y = (may_eval) (y);
      }
      MAY_COMPACT (y);
    }
  /* atan(EXTENSION) */
  else if (MAY_EXT_P (z) && MAY_EXT_GETX (z)->atan)
    y = (*MAY_EXT_GETX (z)->atan) (z);

  /* Rebuild it if y = NULL */
  if (y == NULL)
    {
      y = x;
      MAY_SET_AT (y, 0, z);
      MAY_CLOSE_C (y, MAY_FLAGS (z), MAY_HASH (z));
    }
  return y;
}
