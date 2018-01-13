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

/* All the functions accept unevaluated args.
   Only simplify functions return evaluated args */

/* Support NON evaluated arg (May be called with MAY_DUMMY) */
MAY_REGPARM int
may_num_zero_p (may_t x)
{
  MAY_ASSERT (MAY_PURENUM_P (x));

  switch (MAY_TYPE (x))
    {
    case MAY_INT_T:
      return mpz_sgn (MAY_INT (x)) == 0;
    case MAY_RAT_T:
      return mpq_sgn (MAY_RAT (x)) == 0;
    case MAY_FLOAT_T:
      return !mpfr_nan_p (MAY_FLOAT (x)) && mpfr_cmp_ui (MAY_FLOAT(x), 0) == 0;
    default:
      MAY_ASSERT (MAY_TYPE (x) == MAY_COMPLEX_T);
      return may_num_zero_p (MAY_IM (x)) && may_num_zero_p (MAY_RE (x));
    }
}

/* Support NON evaluated arg but must be NUMERICAL */
MAY_REGPARM int
may_num_pos_p (may_t x)
{
  MAY_ASSERT (MAY_PURENUM_P (x));

   switch (MAY_TYPE (x))
    {
    case MAY_INT_T:
      return mpz_sgn (MAY_INT (x)) > 0;
    case MAY_RAT_T:
      return mpq_sgn (MAY_RAT (x)) > 0;
    case MAY_FLOAT_T:
      return !mpfr_nan_p (MAY_FLOAT (x)) && mpfr_sgn (MAY_FLOAT(x)) > 0;
    default:
      return 0;
    }
}

/* Support NON evaluated arg but must be NUMERICAL */
MAY_REGPARM int
may_num_poszero_p (may_t x)
{
  MAY_ASSERT (MAY_PURENUM_P (x));

   switch (MAY_TYPE (x))
    {
    case MAY_INT_T:
      return mpz_sgn (MAY_INT (x)) >= 0;
    case MAY_RAT_T:
      return mpq_sgn (MAY_RAT (x)) >= 0;
    case MAY_FLOAT_T:
      return !mpfr_nan_p (MAY_FLOAT (x)) && mpfr_sgn (MAY_FLOAT(x)) >= 0;
    default:
      return 0;
    }
}

/* Support NON evaluated arg but must be NUMERICAL */
MAY_REGPARM int
may_num_neg_p (may_t x)
{
  MAY_ASSERT (MAY_PURENUM_P (x));

   switch (MAY_TYPE (x))
    {
    case MAY_INT_T:
      return mpz_sgn (MAY_INT (x)) < 0;
    case MAY_RAT_T:
      return mpq_sgn (MAY_RAT (x)) < 0;
    case MAY_FLOAT_T:
      return !mpfr_nan_p (MAY_FLOAT (x)) && mpfr_sgn (MAY_FLOAT(x)) < 0;
    default:
      return 0;
    }
}

/* Support NON evaluated arg but must be NUMERICAL */
MAY_REGPARM int
may_num_negzero_p (may_t x)
{
  MAY_ASSERT (MAY_PURENUM_P (x));

   switch (MAY_TYPE (x))
    {
    case MAY_INT_T:
      return mpz_sgn (MAY_INT (x)) <= 0;
    case MAY_RAT_T:
      return mpq_sgn (MAY_RAT (x)) <= 0;
    case MAY_FLOAT_T:
      return !mpfr_nan_p (MAY_FLOAT (x)) && mpfr_sgn (MAY_FLOAT(x)) <= 0;
    default:
      return 0;
    }
}

/* Support NON evaluated arg but must be NUMERICAL */
MAY_REGPARM int
may_num_one_p (may_t x)
{
  MAY_ASSERT (MAY_PURENUM_P (x));

   switch (MAY_TYPE (x))
    {
    case MAY_INT_T:
      return mpz_cmp_ui (MAY_INT (x), 1) == 0;
    case MAY_RAT_T:
      return mpq_cmp_ui (MAY_RAT (x), 1, 1) == 0;
    case MAY_FLOAT_T:
      return !mpfr_nan_p (MAY_FLOAT (x)) && mpfr_cmp_ui (MAY_FLOAT(x), 1) == 0;
    default:
      MAY_ASSERT (MAY_TYPE (x) == MAY_COMPLEX_T);
      return may_num_zero_p (MAY_IM (x)) && may_num_one_p (MAY_RE (x));
    }
}

/* Compare two num args */
MAY_REGPARM int
may_num_cmp (may_t x, may_t y)
{
  mpq_t q;
  int i, sign;

  /* FIXME: If y is float and is NAN, what to do? */
  MAY_ASSERT (MAY_PURENUM_P (x));
  MAY_ASSERT (MAY_PURENUM_P (y));
  if (MAY_UNLIKELY (MAY_TYPE (x) > MAY_TYPE (y)))
    swap (x, y), sign =-1;
  else sign = 1;

  /* Lexicographic order for complex */
  if (MAY_UNLIKELY (MAY_TYPE (y) == MAY_COMPLEX_T)) {
    i = may_num_cmp (x, MAY_RE (y));
    if (i == 0)
      i = may_num_cmp (MAY_ZERO, MAY_IM (y));
  } else /* We compare real */
    switch (MAY_TYPE (x)) {
    case MAY_INT_T:
      switch (MAY_TYPE (y)) {
      case MAY_INT_T:
        i = mpz_cmp (MAY_INT (x), MAY_INT (y));
        break;
      case MAY_RAT_T:
        {
          MAY_RECORD ();
          mpq_init (q);
          mpq_set_z (q, MAY_INT (x));
          i = mpq_cmp (q, MAY_RAT (y));
          MAY_CLEANUP ();
        }
        break;
      default:
        MAY_ASSERT (MAY_TYPE (y) == MAY_FLOAT_T);
        i = -mpfr_cmp_z (MAY_FLOAT (y), MAY_INT (x));
        break;
      }
      break;
    case MAY_RAT_T:
      switch (MAY_TYPE (y)) {
      case MAY_RAT_T:
        i = mpq_cmp (MAY_RAT (x), MAY_RAT (y));
        break;
      default:
        MAY_ASSERT (MAY_TYPE (y) == MAY_FLOAT_T);
        i = -mpfr_cmp_q (MAY_FLOAT (y), MAY_RAT (x));
        break;
      }
      break;
    default:
      MAY_ASSERT (MAY_TYPE (x) == MAY_FLOAT_T);
      MAY_ASSERT (MAY_TYPE (y) == MAY_FLOAT_T);
      i = mpfr_cmp (MAY_FLOAT (x), MAY_FLOAT (y));
      break;
    }
  return i*sign;
}

/* Simplify and return evaluated forms */
MAY_REGPARM may_t
may_mpz_simplify (may_t x)
{
  MAY_ASSERT (MAY_TYPE (x) == MAY_INT_T);
#if defined(MAY_WANT_STAT_ON_INT)
  static unsigned long count1[256];
  static unsigned long count2[256];
  static unsigned long count3[256];
  static unsigned long count_bigger;
  static unsigned long count_really_bigger;
  long u;
  if (mpz_fits_slong_p (MAY_INT (x))) {
    u = mpz_get_si(MAY_INT(x));
    if (u >= (1L<<24) || u <= -(1L<<24))
      count_bigger++;
    else {
      /* abs doesn't work on LONG_MIN */
      u = abs(u);
      if (u >= (1L<<16))
        count3 [ u >> 16 ] ++;
      else if (u >= (1L<<8) )
        count2 [ u >> 8 ] ++;
      else
        count1 [ u ] ++;
    }
  } else
    count_really_bigger++;
  auto void __attribute__((destructor)) __may_log_destruct (void);
  auto void __attribute__((destructor)) __may_log_destruct (void) {
    printf ("[MAYLIB]: Statistic usage of integers:\n");
    for(int i = 0 ; i < 256; i++) {
      if (count1[i] != 0)
        printf (" %d is used %lu times.\n", i, count1[i]);
    }
    for(int i = 1 ; i < 256; i++) {
      if (count2[i] != 0)
        printf (" [%d-%d[ is used %lu times.\n", i<<8, (i+1)<<8, count2[i]);
    }
    for(int i = 1 ; i < 256; i++) {
      if (count3[i] != 0)
        printf (" [%d-%d[ is used %lu times.\n", i<<16, (i+1)<<16, count3[i]);
    }
    if (count_bigger != 0)
      printf (" others (< long) are used %ld times.\n", count_bigger);
    if (count_bigger != 0)
      printf (" others (> long) are used %ld times.\n", count_really_bigger);
  }
#endif
  /* Arithemic in Z/nZ. Simplify number according to modulo */
  if (MAY_UNLIKELY (may_g.frame.intmod != NULL)) {
    mpz_t r;
    mpz_init (r);
    mpz_fdiv_r (r, MAY_INT (x), MAY_INT (may_g.frame.intmod));
    x = MAY_MPZ_NOCOPY_C (r);
  }
  mp_limb_t t;
  /* Simplify number: Don't use mpz_cmpabs_ui, but mpz_size and mpz_getlimbn which are inlined  */
  if (MAY_LIKELY (mpz_size (MAY_INT(x)) <= 1 && ( (t = mpz_getlimbn (MAY_INT (x), 0)) < MAY_MAX_MPZ_CONSTANT))) {
    int sign = mpz_sgn (MAY_INT (x));
    x = may_c.mpz_constant[t+MAY_MAX_MPZ_CONSTANT*(sign<0)];
  }
  else {
    MAY_CLOSE_C (x, MAY_NUM_F|MAY_EVAL_F, may_mpz_hash (MAY_INT (x)));
    MAY_SET_FLAG (x, MAY_EXPAND_F);
  }
  MAY_ASSERT (MAY_EVAL_P (x));
  return x;
}

/* Simplify q and return evaluated form ('org' may be the 'may_t' container of 'q' if any,
   or NULL if none ) */
MAY_REGPARM may_t
may_mpq_simplify (may_t org, mpq_t q)
{
  MAY_ASSERT (org == NULL || MAY_TYPE (org) == MAY_RAT_T);
  /* If modulo, converts n/d to an integer modulo */
  if (MAY_UNLIKELY (may_g.frame.intmod != NULL))
    return may_mpz_simplify (may_num_div (MAY_DUMMY,
                                          MAY_MPZ_NOCOPY_C (mpq_numref (q)),
                                          MAY_MPZ_NOCOPY_C (mpq_denref (q))));

  if (MAY_UNLIKELY (mpz_sgn (mpq_denref (q)) == 0))
    return (mpq_sgn (q) == 0 ? MAY_NAN
	    : mpq_sgn (q) > 0 ? MAY_P_INF : MAY_N_INF);
  else if (MAY_UNLIKELY (mpz_cmp_ui (mpq_denref (q), 1) == 0))
    return may_mpz_simplify (MAY_MPZ_NOCOPY_C (mpq_numref(q)));
  else if (org == NULL)
    org = MAY_MPQ_NOCOPY_C (q);
  MAY_CLOSE_C (org, MAY_NUM_F|MAY_EVAL_F, may_mpq_hash (MAY_RAT (org)));
  MAY_SET_FLAG (org, MAY_EXPAND_F);
  MAY_ASSERT (MAY_EVAL_P (org));
  return org;
}

/* Simplify and return evaluated form */
MAY_REGPARM may_t
may_mpfr_simplify (may_t x)
{
  MAY_ASSERT (MAY_TYPE (x) == MAY_FLOAT_T);
  MAY_CLOSE_C  (x, MAY_NUM_F|MAY_EVAL_F, may_mpfr_hash (MAY_FLOAT (x)));
  MAY_SET_FLAG (x, MAY_EXPAND_F);
  MAY_ASSERT   (MAY_EVAL_P (x));
  return x;
}

/* Simplify and return evaluated form */
MAY_REGPARM may_t
may_cx_simplify (may_t x)
{
  MAY_ASSERT (MAY_TYPE (x) == MAY_COMPLEX_T);
  MAY_ASSERT (MAY_PURENUM_P (MAY_RE (x)));
  MAY_ASSERT (MAY_PURENUM_P (MAY_IM (x)));

  /* Simplify sub-args */
  if (MAY_UNLIKELY (!MAY_EVAL_P (MAY_IM (x))))
    MAY_SET_IM (x, may_num_simplify (MAY_IM (x)));
  if (MAY_UNLIKELY (!MAY_EVAL_P (MAY_RE (x))))
    MAY_SET_RE (x, may_num_simplify (MAY_RE (x)));

  /* Check if Img = 0 */
  if (MAY_UNLIKELY (may_num_zero_p (MAY_IM (x))))
    return MAY_RE (x);

  MAY_CLOSE_C (x, MAY_NUM_F|MAY_EVAL_F,
	       MAY_NEW_HASH2 (MAY_RE (x), MAY_IM (x)));
  MAY_SET_FLAG (x, MAY_EXPAND_F);
  MAY_ASSERT (MAY_EVAL_P (x));
  return x;
}

MAY_REGPARM may_t
may_num_simplify (may_t x)
{
  MAY_ASSERT (MAY_PURENUM_P (x));

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
    return may_mpz_simplify (x);
  case MAY_RAT_T:
    return may_mpq_simplify (x, MAY_RAT(x));
  case MAY_FLOAT_T:
    return may_mpfr_simplify (x);
  default:
    MAY_ASSERT (MAY_TYPE (x) == MAY_COMPLEX_T);
    return may_cx_simplify (x);
  }
}


/* Functions providing a wrapper to real functions.
   dest may be MAY_DUMMY: it means create the destination
   (It really allocates a new area of memory), otherwise
   'dest' is a pool of memory the function tries to reuse */
MAY_REGPARM may_t
may_num_set (may_t dest, may_t op)
{
  MAY_ASSERT (MAY_PURENUM_P (op));
  MAY_ASSERT (dest == MAY_DUMMY
              || (!MAY_EVAL_P (dest) && MAY_PURENUM_P (dest)));

  switch (MAY_TYPE (op)) {
  case MAY_INT_T:
    if (MAY_LIKELY (MAY_TYPE (dest) != MAY_INT_T))
      dest = MAY_INT_INIT_C ();
    mpz_set (MAY_INT (dest), MAY_INT(op));
    break;
  case MAY_RAT_T:
    if (MAY_LIKELY (MAY_TYPE (dest) != MAY_RAT_T))
      dest = MAY_RAT_INIT_C ();
    mpq_set (MAY_RAT(dest), MAY_RAT(op));
    break;
  case MAY_FLOAT_T:
    if (MAY_LIKELY (MAY_TYPE (dest) != MAY_FLOAT_T))
      dest = MAY_FLOAT_INIT_C ();
    mpfr_set (MAY_FLOAT(dest), MAY_FLOAT(op), MAY_RND);
    break;
  default:
    MAY_ASSERT (MAY_TYPE (op) == MAY_COMPLEX_T);
    if (MAY_LIKELY (MAY_TYPE (dest) != MAY_COMPLEX_T))
      dest = MAY_COMPLEX_INIT_C ();
    may_t r = may_num_set (MAY_RE (dest), MAY_RE (op));
    may_t i = may_num_set (MAY_IM (dest), MAY_IM (op));
    MAY_SET_RE (dest, r);
    MAY_SET_IM (dest, i);
    break;
  }
  MAY_ASSERT (dest != MAY_DUMMY && !MAY_EVAL_P (dest) && MAY_PURENUM_P (dest));
  return dest;
}

/* Functions providing a wrapper to real functions.
   dest may be MAY_DUMMY: it means create the destination
   (It really allocates a new area of memory), otherwise
   'dest' is a pool of memory the function tries to reuse */
/* Warning: this function may return evaluated arg for complex number and
   may return not a purenum for integer/rationnal complex number */
MAY_REGPARM may_t
may_num_abs (may_t dest, may_t x)
{
  may_t r = may_num_set (dest, x);
  MAY_ASSERT (MAY_PURENUM_P (r));
  switch (MAY_TYPE(x))
    {
    case MAY_INT_T:
      mpz_abs (MAY_INT (r), MAY_INT (r));
      break;
    case MAY_RAT_T:
      mpq_abs (MAY_RAT (r), MAY_RAT (r));
      break;
    case MAY_FLOAT_T:
      mpfr_abs (MAY_FLOAT (r), MAY_FLOAT (r), MAY_RND);
      break;
    default:
      MAY_ASSERT (MAY_TYPE (x) == MAY_COMPLEX_T);
      /* abs(A+I*B) = sqrt(A^2+B^2) */
      {
	may_t AA = may_num_mul (MAY_DUMMY, MAY_RE (r), MAY_RE (r) );
	may_t BB = may_num_mul (MAY_DUMMY, MAY_IM (r), MAY_IM (r) );
	r = may_num_add (r, AA, BB);
        /* It sucks: may_num_pow needs evaluated arg */
	r = may_num_pow (may_num_simplify (r), MAY_HALF);
	/* It may not be a pure num */
        return r;
      }
    }
  MAY_ASSERT (MAY_PURENUM_P (r));
  return r;
}

/* Functions providing a wrapper to real functions.
   dest may be MAY_DUMMY: it means create the destination
   (It really allocates a new area of memory), otherwise
   'dest' is a pool of memory the function tries to reuse */
MAY_REGPARM may_t
may_num_conj (may_t dest, may_t x)
{
  may_t r = may_num_set (dest, x);
  MAY_ASSERT (MAY_PURENUM_P (r));
  if (MAY_TYPE (r) == MAY_COMPLEX_T) {
    may_t i = may_num_neg (MAY_IM (r), MAY_IM (r));
    UNUSED (i);
    MAY_ASSERT (i == MAY_IM (r));
  }
  MAY_ASSERT (MAY_PURENUM_P (r));
  return r;
}

/* Functions providing a wrapper to real functions.
   dest may be MAY_DUMMY: it means create the destination
   (It really allocates a new area of memory), otherwise
   'dest' is a pool of memory the function tries to reuse */
MAY_REGPARM may_t
may_num_add (may_t dest, may_t op1, may_t op2)
{
  MAY_ASSERT (MAY_PURENUM_P (op1));
  MAY_ASSERT (MAY_PURENUM_P (op2));
  MAY_ASSERT (dest == MAY_DUMMY
              || (!MAY_EVAL_P (dest) && MAY_PURENUM_P (dest)));

  /* Commun case where both types are equal */
  if (MAY_LIKELY (MAY_TYPE (op1) == MAY_TYPE (op2))) {
    switch (MAY_TYPE(op1)) {
    case MAY_INT_T:
      if (MAY_TYPE(dest) != MAY_INT_T)
        dest = MAY_INT_INIT_C ();
      mpz_add (MAY_INT (dest), MAY_INT (op1), MAY_INT (op2));
      break;
    case MAY_RAT_T:
      if (MAY_TYPE(dest) != MAY_RAT_T)
        dest = MAY_RAT_INIT_C ();
      mpq_add ( MAY_RAT(dest), MAY_RAT(op1), MAY_RAT(op2));
      break;
    case MAY_FLOAT_T:
      if (MAY_TYPE(dest) != MAY_FLOAT_T)
        dest = MAY_FLOAT_INIT_C ();
      mpfr_add (MAY_FLOAT(dest), MAY_FLOAT(op1), MAY_FLOAT(op2), MAY_RND);
      break;
    default:
      MAY_ASSERT (MAY_TYPE (op2) == MAY_COMPLEX_T);
      if (MAY_TYPE(dest) != MAY_COMPLEX_T)
        dest = MAY_COMPLEX_INIT_C ();
      MAY_SET_RE (dest, may_num_add (MAY_RE(dest), MAY_RE(op1), MAY_RE(op2)));
      MAY_SET_IM (dest, may_num_add (MAY_IM(dest), MAY_IM(op1), MAY_IM(op2)));
      break;
    }
    MAY_ASSERT (dest != MAY_DUMMY && !MAY_EVAL_P (dest) && MAY_PURENUM_P (dest));
    return dest;
  }

  if (MAY_UNLIKELY (MAY_TYPE(op1) > MAY_TYPE(op2)))
    swap (op1, op2);

  switch (MAY_TYPE(op1))
    {
    case MAY_INT_T:
      switch (MAY_TYPE(op2))
	{
	case MAY_RAT_T:
	  if (MAY_TYPE(dest) != MAY_RAT_T)
	    dest = MAY_RAT_INIT_C ();
	  {
	    mpz_t tmp;
 	    mpz_init (tmp);
	    mpz_mul (tmp, mpq_denref(MAY_RAT(op2)), MAY_INT(op1));
	    mpz_add (mpq_numref(MAY_RAT(dest)), mpq_numref(MAY_RAT(op2)), tmp);
	    mpz_set (mpq_denref(MAY_RAT(dest)), mpq_denref(MAY_RAT(op2)));
	    mpq_canonicalize (MAY_RAT (dest));
	    mpz_clear (tmp);
	  }
	  break;
	case MAY_FLOAT_T:
	  if (MAY_TYPE(dest) != MAY_FLOAT_T)
	    dest = MAY_FLOAT_INIT_C ();
	  mpfr_add_z (MAY_FLOAT(dest), MAY_FLOAT(op2), MAY_INT(op1), MAY_RND);
	  break;
	default:
          MAY_ASSERT (MAY_TYPE (op2) == MAY_COMPLEX_T);
	  if (MAY_TYPE (dest) != MAY_COMPLEX_T)
	    dest = MAY_COMPLEX_INIT_C ();
	  MAY_SET_RE (dest, may_num_add (MAY_RE (dest), op1, MAY_RE (op2)) );
	  MAY_SET_IM (dest, may_num_set (MAY_IM (dest), MAY_IM (op2))      );
	  break;
	}
      break;
    case MAY_RAT_T:
      switch (MAY_TYPE (op2))
	{
	case MAY_FLOAT_T:
	  if (MAY_TYPE(dest) != MAY_FLOAT_T)
	    dest = MAY_FLOAT_INIT_C ();
	  mpfr_add_q (MAY_FLOAT(dest), MAY_FLOAT(op2), MAY_RAT(op1), MAY_RND);
	  break;
	default:
          MAY_ASSERT (MAY_TYPE (op2) == MAY_COMPLEX_T);
	  if (MAY_TYPE(dest) != MAY_COMPLEX_T)
	    dest = MAY_COMPLEX_INIT_C ();
	  MAY_SET_RE (dest, may_num_add (MAY_RE(dest), op1, MAY_RE(op2)) );
	  MAY_SET_IM (dest, may_num_set (MAY_IM(dest), MAY_IM(op2))      );
	  break;
	}
      break;
    default:
      MAY_ASSERT (MAY_TYPE (op1) == MAY_FLOAT_T);
      MAY_ASSERT (MAY_TYPE (op2) == MAY_COMPLEX_T);
      if (MAY_TYPE(dest) != MAY_COMPLEX_T)
        dest = MAY_COMPLEX_INIT_C();
      MAY_SET_RE (dest, may_num_add (MAY_RE(dest), op1, MAY_RE(op2)) );
      MAY_SET_IM (dest, may_num_set (MAY_IM(dest), MAY_IM(op2))      );
      break;
    }
  MAY_ASSERT (dest != MAY_DUMMY && !MAY_EVAL_P (dest) && MAY_PURENUM_P (dest));
  return dest;
}

/* Functions providing a wrapper to real functions.
   dest may be MAY_DUMMY: it means create the destination
   (It really allocates a new area of memory), otherwise
   'dest' is a pool of memory the function tries to reuse */
MAY_REGPARM may_t
may_num_neg (may_t dest, may_t op)
{
  MAY_ASSERT (MAY_TYPE(op) < MAY_NUM_LIMIT);
  MAY_ASSERT (dest == MAY_DUMMY
              || (!MAY_EVAL_P (dest) && MAY_PURENUM_P (dest)));

  switch (MAY_TYPE(op))
    {
    case MAY_INT_T:
      if (MAY_LIKELY (MAY_TYPE(dest) != MAY_INT_T))
	dest = MAY_INT_INIT_C();
      mpz_neg (MAY_INT(dest), MAY_INT(op));
      break;
    case MAY_RAT_T:
      if (MAY_LIKELY (MAY_TYPE(dest) != MAY_RAT_T))
        dest = MAY_RAT_INIT_C();
      mpq_neg (MAY_RAT(dest), MAY_RAT(op));
      break;
    case MAY_FLOAT_T:
      if (MAY_LIKELY (MAY_TYPE(dest) != MAY_FLOAT_T))
        dest = MAY_FLOAT_INIT_C();
      mpfr_neg (MAY_FLOAT(dest), MAY_FLOAT(op), MAY_RND);
      break;
    default:
      MAY_ASSERT (MAY_TYPE (op) == MAY_COMPLEX_T);
      if (MAY_LIKELY (MAY_TYPE(dest) != MAY_COMPLEX_T))
        dest = MAY_COMPLEX_INIT_C ();
      MAY_SET_RE (dest, may_num_neg (MAY_RE (dest), MAY_RE (op)) );
      MAY_SET_IM (dest, may_num_neg (MAY_IM (dest), MAY_IM (op)) );
      break;
    }
  MAY_ASSERT (dest != MAY_DUMMY && !MAY_EVAL_P (dest) && MAY_PURENUM_P (dest));
  return dest;
}

/* Functions providing a wrapper to real functions.
   dest may be MAY_DUMMY: it means create the destination
   (It really allocates a new area of memory), otherwise
   'dest' is a pool of memory the function tries to reuse */
MAY_REGPARM may_t
may_num_sub (may_t dest, may_t op1, may_t op2)
{
  MAY_ASSERT (MAY_PURENUM_P (op1));
  MAY_ASSERT (MAY_PURENUM_P (op2));
  MAY_ASSERT (dest == MAY_DUMMY
              || (!MAY_EVAL_P (dest) && MAY_PURENUM_P (dest)));

  /* Commun case where both types are equal */
  if (MAY_LIKELY (MAY_TYPE (op1) == MAY_TYPE (op2))) {
    switch (MAY_TYPE(op1)) {
    case MAY_INT_T:
      if (MAY_TYPE(dest) != MAY_INT_T)
        dest = MAY_INT_INIT_C ();
      mpz_sub (MAY_INT (dest), MAY_INT (op1), MAY_INT (op2));
      break;
    case MAY_RAT_T:
      if (MAY_TYPE(dest) != MAY_RAT_T)
        dest = MAY_RAT_INIT_C ();
      mpq_sub ( MAY_RAT(dest), MAY_RAT(op1), MAY_RAT(op2));
      break;
    case MAY_FLOAT_T:
      if (MAY_TYPE(dest) != MAY_FLOAT_T)
        dest = MAY_FLOAT_INIT_C ();
      mpfr_sub (MAY_FLOAT(dest), MAY_FLOAT(op1), MAY_FLOAT(op2), MAY_RND);
      break;
    default:
      MAY_ASSERT (MAY_TYPE (op2) == MAY_COMPLEX_T);
      if (MAY_TYPE(dest) != MAY_COMPLEX_T)
        dest = MAY_COMPLEX_INIT_C ();
      MAY_SET_RE (dest, may_num_sub (MAY_RE(dest), MAY_RE(op1), MAY_RE(op2)));
      MAY_SET_IM (dest, may_num_sub (MAY_IM(dest), MAY_IM(op1), MAY_IM(op2)));
      break;
    }
  } else
    /* Type are differents : fall back to generic way */
    dest = may_num_add (dest, op1,
                        may_num_neg (dest != op1 ? dest : MAY_DUMMY, op2));

  MAY_ASSERT (dest != MAY_DUMMY && !MAY_EVAL_P (dest) && MAY_PURENUM_P (dest));
  return dest;
}

/* Functions providing a wrapper to real functions.
   dest may be MAY_DUMMY: it means create the destination
   (It really allocates a new area of memory), otherwise
   'dest' is a pool of memory the function tries to reuse */
MAY_REGPARM may_t
may_num_mul (may_t dest, may_t op1, may_t op2)
{
  MAY_ASSERT (MAY_PURENUM_P (op1));
  MAY_ASSERT (MAY_PURENUM_P (op2));
  MAY_ASSERT (dest == MAY_DUMMY
              || (!MAY_EVAL_P (dest) && MAY_PURENUM_P (dest)));

  /* Commun case where both types are equals */
  if (MAY_LIKELY (MAY_TYPE (op1) == MAY_TYPE (op2))) {
    switch (MAY_TYPE (op1)) {
    case MAY_INT_T:
      if (MAY_TYPE (dest) != MAY_INT_T)
        dest = MAY_INT_INIT_C ();
      mpz_mul (MAY_INT (dest), MAY_INT (op1), MAY_INT (op2));
      break;
    case MAY_RAT_T:
      if (MAY_TYPE (dest) != MAY_RAT_T)
        dest = MAY_RAT_INIT_C ();
      mpq_mul (MAY_RAT (dest), MAY_RAT (op1), MAY_RAT (op2));
      break;
    case MAY_FLOAT_T:
      if (MAY_TYPE(dest) != MAY_FLOAT_T)
        dest = MAY_FLOAT_INIT_C ();
      mpfr_mul (MAY_FLOAT(dest), MAY_FLOAT(op2), MAY_FLOAT(op1), MAY_RND);
      break;
    case MAY_COMPLEX_T:
      MAY_ASSERT (MAY_TYPE (op2) == MAY_COMPLEX_T);
      if (MAY_TYPE(dest) != MAY_COMPLEX_T)
	dest = MAY_COMPLEX_INIT_C();
      /* (A+B*I)*(C+I*D) = (A*C-B*D)+I*(B*C+A*D)
	 = (A*C-B*D)+I*((A+B)*(C+D)-(A*C+B*D))
         No threeshold: Always Karatsuba ! */
      {
	may_t AxC = may_num_mul (MAY_DUMMY, MAY_RE(op1), MAY_RE(op2) );
	may_t BxD = may_num_mul (MAY_DUMMY, MAY_IM(op1), MAY_IM(op2) );
	may_t AB  = may_num_add (MAY_DUMMY, MAY_RE(op1), MAY_IM(op1) );
	may_t CD  = may_num_add (MAY_DUMMY, MAY_RE(op2), MAY_IM(op2) );
	AB = may_num_mul(AB, AB, CD);
	CD = may_num_add(CD, AxC, BxD);
	MAY_SET_RE (dest, may_num_sub (MAY_RE(dest), AxC, BxD) );
	MAY_SET_IM (dest, may_num_sub (MAY_IM(dest), AB, CD  ) );
      }
      break;
    }
    MAY_ASSERT (dest != MAY_DUMMY && !MAY_EVAL_P (dest) && MAY_PURENUM_P (dest));
    return dest;
  }

  /* Type are differents */
  if (MAY_UNLIKELY (MAY_TYPE (op1) > MAY_TYPE (op2)))
    swap (op1, op2);

  switch (MAY_TYPE (op1))
    {
    case MAY_INT_T:
      switch (MAY_TYPE (op2))
	{
	case MAY_RAT_T:
	  if (MAY_TYPE (dest) != MAY_RAT_T)
	    dest = MAY_RAT_INIT_C ();
	  mpz_mul (mpq_numref (MAY_RAT(dest)),
		   MAY_INT (op1), mpq_numref (MAY_RAT (op2)));
	  mpz_set (mpq_denref (MAY_RAT (dest)), mpq_denref (MAY_RAT (op2)));
	  mpq_canonicalize (MAY_RAT (dest));
	  break;
	case MAY_FLOAT_T:
	  if (MAY_TYPE (dest) != MAY_FLOAT_T)
	    dest = MAY_FLOAT_INIT_C ();
	  mpfr_mul_z (MAY_FLOAT(dest), MAY_FLOAT(op2), MAY_INT(op1), MAY_RND);
	  break;
        default:
          MAY_ASSERT (MAY_TYPE (op2) == MAY_COMPLEX_T);
	  if (MAY_TYPE (dest) != MAY_COMPLEX_T)
	    dest = MAY_COMPLEX_INIT_C ();
          MAY_SET_IM (dest, may_num_mul (MAY_IM (dest), op1, MAY_IM (op2))  );
	  MAY_SET_RE (dest, may_num_mul (MAY_RE (dest), op1, MAY_RE (op2))  );
	  break;
	}
      break;
    case MAY_RAT_T:
      switch (MAY_TYPE (op2))
	{
	case MAY_FLOAT_T:
	  if (MAY_TYPE (dest) != MAY_FLOAT_T)
	    dest = MAY_FLOAT_INIT_C ();
	  mpfr_mul_q (MAY_FLOAT(dest), MAY_FLOAT(op2), MAY_RAT(op1), MAY_RND);
	  break;
	default:
          MAY_ASSERT (MAY_TYPE (op2) == MAY_COMPLEX_T);
	  if (MAY_TYPE (dest) != MAY_COMPLEX_T)
	    dest = MAY_COMPLEX_INIT_C ();
	  /* Before Im part since op1 may be dest[0]! */
          MAY_SET_IM (dest, may_num_mul (MAY_IM(dest), op1, MAY_IM(op2)) );
	  MAY_SET_RE (dest, may_num_mul (MAY_RE(dest), op1, MAY_RE(op2)) );
	  break;
	}
      break;

    default:
      MAY_ASSERT (MAY_TYPE (op1) == MAY_FLOAT_T);
      MAY_ASSERT (MAY_TYPE (op2) == MAY_COMPLEX_T);
      if (MAY_TYPE(dest) != MAY_COMPLEX_T)
        dest = MAY_COMPLEX_INIT_C ();
      /* Before Im part since op1 may be dest[0]! */
      MAY_SET_IM(dest, may_num_mul (MAY_IM(dest), op1, MAY_IM(op2)) );
      MAY_SET_RE(dest, may_num_mul (MAY_RE(dest), op1, MAY_RE(op2)) );
      break;
    }
  MAY_ASSERT (dest != MAY_DUMMY && !MAY_EVAL_P (dest) && MAY_PURENUM_P (dest));
  return dest;
}

/* Functions providing a wrapper to real functions.
   dest may be MAY_DUMMY: it means create the destination
   (It really allocates a new area of memory), otherwise
   'dest' is a pool of memory the function tries to reuse */
MAY_INLINE may_t
may_num_invmod (may_t dest, may_t op)
{
  int i;
  MAY_ASSERT (MAY_TYPE (op) == MAY_INT_T);
  MAY_ASSERT (may_g.frame.intmod != NULL);
  if (MAY_TYPE(dest) != MAY_INT_T)
    dest = MAY_INT_INIT_C ();
  i = mpz_invert (MAY_INT (dest), MAY_INT (op), MAY_INT (may_g.frame.intmod));
  return i == 0 ? MAY_NAN : dest;
}

/* Functions providing a wrapper to real functions.
   dest may be MAY_DUMMY: it means create the destination
   (It really allocates a new area of memory), otherwise
   'dest' is a pool of memory the function tries to reuse */
MAY_REGPARM may_t
may_num_inv (may_t dest, may_t op)
{
  MAY_ASSERT(MAY_TYPE(op) < MAY_NUM_LIMIT);
  MAY_ASSERT (dest == MAY_DUMMY
              || (!MAY_EVAL_P (dest) && MAY_PURENUM_P (dest)));

  switch (MAY_TYPE(op))
    {
    case MAY_INT_T:
      if (MAY_UNLIKELY(mpz_sgn (MAY_INT (op)) == 0))
	return may_num_set (dest, MAY_NAN);
      if (MAY_UNLIKELY(mpz_cmp_ui (MAY_INT (op), 1) == 0))
	return may_num_set (dest, MAY_ONE);
      if (MAY_UNLIKELY (may_g.frame.intmod != NULL))
        return may_num_invmod (dest, op);
      if (MAY_UNLIKELY(mpz_cmp_si (MAY_INT (op), -1) == 0))
	return may_num_set (dest, MAY_N_ONE);
      if (MAY_TYPE(dest) != MAY_RAT_T)
	dest = MAY_RAT_INIT_C ();
      mpz_set (mpq_numref (MAY_RAT (dest)), MAY_INT (MAY_ONE));
      mpz_set (mpq_denref (MAY_RAT (dest)), MAY_INT (op));
      mpq_canonicalize (MAY_RAT (dest));
      break;
    case MAY_RAT_T:
      if (MAY_TYPE (dest) != MAY_RAT_T)
	dest = MAY_RAT_INIT_C ();
      mpq_inv (MAY_RAT (dest), MAY_RAT (op));
      break;
    case MAY_FLOAT_T:
      if (MAY_TYPE(dest) != MAY_FLOAT_T)
	dest = MAY_FLOAT_INIT_C ();
      mpfr_ui_div (MAY_FLOAT (dest), 1, MAY_FLOAT (op), MAY_RND);
      break;
    default:
      MAY_ASSERT (MAY_TYPE (op) == MAY_COMPLEX_T);
      /* 1/(C+I*D) = (C-I*D)/(C^2+D^2) */
      {
	if (MAY_TYPE(dest) != MAY_COMPLEX_T)
          dest = MAY_COMPLEX_INIT_C ();
	may_t n = may_num_add(MAY_DUMMY,
			      may_num_mul (MAY_DUMMY, MAY_RE(op), MAY_RE(op)),
			      may_num_mul (MAY_DUMMY, MAY_IM(op), MAY_IM(op)));
	MAY_SET_RE (dest, may_num_div (MAY_RE (dest), MAY_RE (op), n));
	MAY_SET_IM (dest, may_num_div (MAY_IM (dest), MAY_IM (op), n));
	MAY_SET_IM (dest, may_num_neg (MAY_IM (dest), MAY_IM (dest)) );
	break;
      }
    }
  MAY_ASSERT (dest != MAY_DUMMY && !MAY_EVAL_P (dest) && MAY_PURENUM_P (dest));
  return dest;
}

/* Functions providing a wrapper to real functions.
   dest may be MAY_DUMMY: it means create the destination
   (It really allocates a new area of memory), otherwise
   'dest' is a pool of memory the function tries to reuse */
MAY_REGPARM may_t
may_num_div (may_t dest, may_t op1, may_t op2)
{
  MAY_ASSERT (MAY_PURENUM_P (op1));
  MAY_ASSERT (MAY_PURENUM_P (op2));
  MAY_ASSERT (dest == MAY_DUMMY
              || (!MAY_EVAL_P (dest) && MAY_PURENUM_P (dest)));

  /* If the types are differents, it is quite uncommun, and quite
     complicated to handle. Let's call only mul and inv */
  if (MAY_TYPE(op1) != MAY_TYPE(op2)) {
  div_by_mul_inverse:
    dest = may_num_mul (dest, op1,
                        may_num_inv (dest != op1 ? dest : MAY_DUMMY, op2));
  } else
    switch (MAY_TYPE(op1))
      {
      case MAY_INT_T:
	if (MAY_UNLIKELY(mpz_sgn (MAY_INT (op2)) == 0))
	  return may_num_set (dest, MAY_NAN);
	if (MAY_UNLIKELY(mpz_cmp_ui (MAY_INT (op2), 1) == 0))
	  return may_num_set (dest, op1);
	if (MAY_UNLIKELY (may_g.frame.intmod != NULL))
          goto div_by_mul_inverse;
        if (MAY_UNLIKELY(mpz_cmp_si (MAY_INT (op2), -1) == 0))
	  return may_num_neg (dest, op1);
        if (MAY_TYPE (dest) != MAY_RAT_T)
	  dest = MAY_RAT_INIT_C ();
	mpz_set (mpq_numref (MAY_RAT (dest)), MAY_INT (op1));
	mpz_set (mpq_denref (MAY_RAT (dest)), MAY_INT (op2));
	mpq_canonicalize (MAY_RAT(dest));
	break;
      case MAY_RAT_T:
	if (MAY_TYPE(dest) != MAY_RAT_T)
	  dest = MAY_RAT_INIT_C ();
	mpq_div ( MAY_RAT (dest), MAY_RAT (op1), MAY_RAT (op2));
	break;
      case MAY_FLOAT_T:
	if (MAY_TYPE(dest) != MAY_FLOAT_T)
	  dest = MAY_FLOAT_INIT_C ();
	mpfr_div (MAY_FLOAT (dest), MAY_FLOAT (op1), MAY_FLOAT (op2), MAY_RND);
	break;
      default:
        MAY_ASSERT (MAY_TYPE (op2) == MAY_COMPLEX_T);
	dest = may_num_mul (dest, op1,
			    may_num_inv (dest != op1 ? dest : MAY_DUMMY, op2));
      }
  MAY_ASSERT (dest != MAY_DUMMY && !MAY_EVAL_P (dest) && MAY_PURENUM_P (dest));
  return dest;
}

/* Eval a complex 'x' to an integer power 'n' using fast binary exponentiation */
static MAY_REGPARM may_t
complex_pow (may_t x, long n)
{
  may_t r;
  long i, m;

  MAY_ASSERT (MAY_TYPE (x) == MAY_COMPLEX_T);
  MAY_ASSERT (n != 0 && n != 1);

  MAY_RECORD ();

  if (n < 0) {
    x = may_num_inv (MAY_DUMMY, x);
    n = -n;
    if (n == 1)
      MAY_RET (may_cx_simplify (x));
  }

  for (i = 0, m = n; m != 0; i++)
    m >>= 1;
  r = may_num_set (MAY_DUMMY, x);
  for (i -= 2; i >= 0; i--)
    {
      r = may_num_mul (r, r, r);
      if (n & (1UL << i))
        r = may_num_mul (r, r, x);
    }
  r = may_cx_simplify (r);
  MAY_RET (r);
}

/* Try to eval INT^(1/N), in the INTEGER domain if INT > 0
   if INT < 0 and N is even, it tries the INTEGER COMPLEX domain */
/* base = INT / expo = RAT = 1/N */
static MAY_REGPARM may_t
may_mpz_root (may_t base, may_t expo)
{
  mpz_t z;
  may_t y;
  int i;
  unsigned long n;

  MAY_ASSERT (MAY_TYPE (base) == MAY_INT_T);

  n = mpz_get_ui (mpq_denref (MAY_RAT (expo)));
  if (MAY_UNLIKELY (n < 2))
    return n == 0 ? MAY_NAN : base;
  else if (MAY_UNLIKELY (base == MAY_N_ONE))
    return NULL;

  /* TODO: intmod. FIXME: what to do? */

  MAY_RECORD ();
  mpz_init (z);
  mpz_abs (z, MAY_INT (base));
  i = mpz_root (z, z, n);
  if (i == 0)
    y = NULL;
  else {
    y = may_num_pow (may_mpz_simplify (MAY_MPZ_NOCOPY_C (z)),
                     may_mpz_simplify (MAY_MPZ_NOCOPY_C (mpq_numref (MAY_RAT (expo)))));
    /* If base < 0, returns (-1)^(1/N)*root */
    if (mpz_sgn (MAY_INT (base)) < 0)
      y = may_eval (may_mul_c (may_pow_c (MAY_N_ONE, expo), y));
  }
  MAY_RET (y);
}

/* This function tests if a^b fits as an integer.
   It returns b as an unsigned long if successfull */
MAY_REGPARM unsigned long
may_test_overflow_pow_ui (mpz_srcptr a, mpz_srcptr b)
{
  unsigned long c, sa;
  /* If b doesn't fit in an unsigned long, fail */
  if (!mpz_fits_ulong_p (b))
    return 0;
  c = mpz_get_ui (b);
  /* If a == 0 or 1, it is ok . Or if c is 1 */
  if (c <= 1 || mpz_cmpabs_ui (a, 1) <= 0)
    return c;
  /* Get log2(a) */
  sa = mpz_sizeinbase (a, 2);
  /* If a is a power of 2, we need to remove one bit.
     In fact, the number of bits we want is for 'a-1', not 'a'.
     This away avoids copying 'a' to remove 1 */
  if (mpz_scan1 (a, 0) == (sa-1))
    sa = sa -1;
  sa = sa * c;
  /* If there is an overflow in the product of sa is greater than intmaxsize */
  if (sa < c || sa >= may_g.frame.intmaxsize) {
    return 0;
  } else
    return c;
}

/* Return base^expo with base and expo evaluated pure numbers */
/* This function returns an evaluated form.
   Expect evaluated form too. */
MAY_REGPARM may_t
may_num_pow (may_t base, may_t expo)
{
  mpfr_t dest_fr;
  mpq_t  dest_q;
  mpz_t  dest_z;
  may_t  y;
  unsigned long e;

  MAY_ASSERT (MAY_PURENUM_P (base) && MAY_EVAL_P (base));
  MAY_ASSERT (MAY_PURENUM_P (expo) && MAY_EVAL_P (expo));

  /* Deal with 0^x and x^1 */
  if (MAY_UNLIKELY (may_num_zero_p (base))) {
    may_t re = MAY_TYPE (expo) == MAY_COMPLEX_T ? MAY_RE (expo) : expo;
    return may_num_negzero_p (re) ? MAY_NAN : MAY_ZERO;
  } else if (MAY_UNLIKELY (may_num_one_p (expo)))
    return base;

  /************* BASE = INTEGER **************/
  if (MAY_TYPE (base) == MAY_INT_T) {
    if (MAY_TYPE (expo) == MAY_INT_T) {
      /* INT ^ INT */
      if (mpz_sgn (MAY_INT (expo)) > 0) {
	/* INT^+INT */
        if (MAY_UNLIKELY (may_g.frame.intmod != NULL)) {
          /* (base raised to exp) % MOD */
          mpz_init (dest_z);
          mpz_powm (dest_z, MAY_INT (base), MAY_INT (expo),
                    MAY_INT (may_g.frame.intmod));
          return may_mpz_simplify (MAY_MPZ_NOCOPY_C (dest_z));
        }
        /* Check for overflow */
        e = may_test_overflow_pow_ui (MAY_INT (base), MAY_INT (expo));
	if (MAY_UNLIKELY (e == 0))
	  goto build;
	/* We can evalute base^expo */
	mpz_init (dest_z);
	mpz_pow_ui (dest_z, MAY_INT (base), e);
	return may_mpz_simplify (MAY_MPZ_NOCOPY_C (dest_z));
      } else {
	/* INT^-INT */
	/* Check for 0 ^ (-INT) */
	if (MAY_UNLIKELY (mpz_sgn (MAY_INT (base)) == 0))
	  return MAY_NAN;
        if (MAY_UNLIKELY (may_g.frame.intmod != NULL)) {
          /* (base raised to -expo) % MOD */
          mpz_t absexp;
          mpz_init (dest_z);
          /* Invert by ourself to avoid the potential "divide by 0" signal
             returned by GMP */
          if (mpz_invert (dest_z, MAY_INT (base),
                          MAY_INT (may_g.frame.intmod)) ==0)
            return MAY_NAN;
          absexp[0] = MAY_INT (expo)[0];
          mpz_abs (absexp, absexp);
          mpz_powm (dest_z, dest_z, absexp, MAY_INT (may_g.frame.intmod));
          return may_mpz_simplify (MAY_MPZ_NOCOPY_C (dest_z));
        }
        /* Check for overflow */
	mpq_init (dest_q);
        mpz_abs (mpq_denref (dest_q), MAY_INT (expo));
        e = may_test_overflow_pow_ui (MAY_INT (base), mpq_denref (dest_q));
        if (MAY_UNLIKELY (e == 0))
	  goto build;
	/* We can evalute base^(-expo) */
	mpq_set_ui (dest_q, 1, 1);
	mpz_pow_ui (mpq_denref (dest_q), MAY_INT (base), e);
	mpq_canonicalize (dest_q); /* FIXME: Usefull? */
	return may_mpq_simplify (NULL, dest_q);
      }
    }
    /* base = INT but expo isn't a int */
    else if (MAY_TYPE (expo) == MAY_RAT_T) {
      /* INT ^ (A/B) */
      /* base=int and expo=a/b. Transform it to q+r/b */
      if (mpz_cmpabs (mpq_numref (MAY_RAT (expo)),
		      mpq_denref (MAY_RAT(expo))) >= 0) {
	mpq_t r;
	mpz_t q;
      handle_rat_decomposition:
	mpz_init (q);
	mpq_init (r);
	mpq_set_z (r, mpq_denref(MAY_RAT(expo)));
	mpz_fdiv_qr (q, mpq_denref(r), mpq_numref(MAY_RAT(expo)),
		     mpq_denref(MAY_RAT(expo)));
	mpq_inv (r, r);
	mpq_canonicalize (r);
	y = may_mul_c (may_pow_c (base, MAY_MPZ_NOCOPY_C (q)),
		       may_pow_c (base, MAY_MPQ_NOCOPY_C (r)));
	return may_eval (y);
      }
      /* base=INT, expo=a/b with a < b */
      else if (mpz_cmp_si (MAY_INT (base), -1) == 0
	       && mpq_cmp_ui (MAY_RAT (expo), 1, 2) == 0)
	/* (-1) ^ (1/2) --> I */
	return MAY_I;
      else if (mpz_cmp_si (MAY_INT (base), -1) == 0
	       && mpq_cmp_si (MAY_RAT (expo), -1, 2) == 0)
	/* (-1) ^ (-1/2) --> -I */
	return may_neg(MAY_I);
      else if (mpz_fits_ushort_p (mpq_denref (MAY_RAT (expo)))
	       && (y = may_mpz_root (base,  expo)) != NULL)
	return y;
      /* base=int expo=rat and fits_slong_p(denom(rat)) and num=1 ??
	 : factorize base and simplify it
	 list = ifactor (denom)
	 Recherche facteur de count > denom
	 Division, simplify
      */
      goto build;
    } else if (MAY_TYPE (expo) == MAY_FLOAT_T) {
      /* base=int and expo=float */
      mpfr_init (dest_fr);
      mpfr_set_z (dest_fr, MAY_INT (base), MAY_RND);
      mpfr_pow (dest_fr, dest_fr, MAY_FLOAT (expo), MAY_RND);
      if (mpfr_nan_p (dest_fr))
	goto toexpln;
      return may_mpfr_simplify (MAY_MPFR_NOCOPY_C (dest_fr));
    }
    /* base=int && expo=complex */
    else if (MAY_TYPE (MAY_RE (expo)) == MAY_FLOAT_T
	     && MAY_TYPE (MAY_IM (expo)) == MAY_FLOAT_T)
      goto toexpln;
    else
      goto build;
  }

  /************* BASE = RATIONAL **************/
  else if (MAY_TYPE (base) == MAY_RAT_T) {
    /* FIXME: Handle intmod? */
    if (MAY_TYPE (expo) == MAY_INT_T) {
      /* (A/B)^INT */
      if (mpz_sgn (MAY_INT(expo)) > 0) {
	/* (A/B)^+INT */
        /* Check for overflow */
	mpq_init (dest_q);
        /* Get the biggest from num/den of base */
        if (mpz_cmpabs (mpq_numref(MAY_RAT(base)), mpq_denref(MAY_RAT(base))) > 0)
          mpz_abs (mpq_numref (dest_q), mpq_numref(MAY_RAT(base)));
        else
          mpz_abs (mpq_numref (dest_q), mpq_denref(MAY_RAT(base)));
	e = may_test_overflow_pow_ui (mpq_numref (dest_q), MAY_INT(expo));
	if (MAY_UNLIKELY (e == 0))
	  goto build;
	/* We can evalute base^expo */
	mpz_pow_ui (mpq_numref(dest_q), mpq_numref(MAY_RAT(base)),e);
	mpz_pow_ui (mpq_denref(dest_q), mpq_denref(MAY_RAT(base)),e);
	mpq_canonicalize (dest_q);
	return may_mpq_simplify (NULL, dest_q);
      } else {
	/* (A/B)^-INT */
        /* Check for overflow */
	mpq_init (dest_q);
        /* Get the biggest from num/den of base */
        if (mpz_cmpabs (mpq_numref(MAY_RAT(base)), mpq_denref(MAY_RAT(base))) > 0)
          mpz_abs (mpq_numref (dest_q), mpq_numref(MAY_RAT(base)));
        else
          mpz_abs (mpq_numref (dest_q), mpq_denref(MAY_RAT(base)));
        mpz_abs (mpq_denref (dest_q), MAY_INT (expo));
        e = may_test_overflow_pow_ui (mpq_numref (dest_q), mpq_denref (dest_q));
	if (MAY_UNLIKELY (e == 0))
	  goto build;

	/* We can evalute base^(-expo) */
	mpz_pow_ui (mpq_numref(dest_q), mpq_denref(MAY_RAT(base)),e);
	mpz_pow_ui (mpq_denref(dest_q), mpq_numref(MAY_RAT(base)),e);
	mpq_canonicalize (dest_q);
	return may_mpq_simplify (NULL, dest_q);
      }
    } /* expo == int */
    else if (MAY_TYPE(expo) == MAY_FLOAT_T) {
      /* (A/B)^FLOAT: transform to float */
      mpfr_init (dest_fr);
      mpfr_set_q (dest_fr, MAY_RAT (base), MAY_RND);
      mpfr_pow (dest_fr, dest_fr, MAY_FLOAT (expo), MAY_RND);
      if (mpfr_nan_p (dest_fr))
	goto toexpln;
      return may_mpfr_simplify (MAY_MPFR_NOCOPY_C (dest_fr));
    } else if (MAY_TYPE (expo) == MAY_RAT_T) {
      if (mpz_cmpabs (mpq_numref (MAY_RAT (expo)),
		      mpq_denref (MAY_RAT(expo))) >= 0)
        goto handle_rat_decomposition;
      may_t fnum   = MAY_MPZ_NOCOPY_C (mpq_numref (MAY_RAT (base)));
      may_t fdenom = MAY_MPZ_NOCOPY_C (mpq_denref (MAY_RAT (base)));
      fnum = may_mpz_simplify (fnum);
      fdenom = may_mpz_simplify (fdenom);
      fnum = may_num_pow (fnum, expo);
      fdenom = may_num_pow (fdenom, expo);
      if (!MAY_PURENUM_P (fnum) && !MAY_PURENUM_P (fdenom))
        goto build;
      return may_div (fnum, fdenom);
    } else
      goto build;
    }

  /************* BASE = FLOAT **************/
  else if (MAY_TYPE (base) == MAY_FLOAT_T) {
    /* base = float */
    if (mpfr_nan_p (MAY_FLOAT (base)))
      return MAY_NAN;
    mpfr_init (dest_fr);
    if (MAY_TYPE (expo) == MAY_INT_T)
      mpfr_set_z (dest_fr, MAY_INT(expo), MAY_RND);
    else if (MAY_TYPE (expo) == MAY_RAT_T)
      mpfr_set_q (dest_fr, MAY_RAT(expo), MAY_RND);
    else if (MAY_TYPE (expo) == MAY_FLOAT_T)
      mpfr_set (dest_fr, MAY_FLOAT (expo), MAY_RND);
    else
      goto toexpln; /* FLOAT^(COMPLEX) */
    mpfr_pow (dest_fr, MAY_FLOAT (base), dest_fr, MAY_RND);
    /* If still NAN --> exp(expo*log(base)) */
    if (mpfr_nan_p (dest_fr))
      goto toexpln;
    return may_mpfr_simplify (MAY_MPFR_NOCOPY_C (dest_fr));
  }

  /************* BASE = COMPLEX **************/
  else if (MAY_TYPE (base) == MAY_COMPLEX_T) {
    if (MAY_TYPE (expo) == MAY_INT_T && mpz_fits_slong_p (MAY_INT (expo))) {
      e = mpz_get_si (MAY_INT(expo));
      y = complex_pow (base, e);
      MAY_ASSERT (MAY_EVAL_P (y));
      return y;
    }  else if (MAY_TYPE(expo) == MAY_FLOAT_T
		|| (MAY_TYPE(MAY_RE(base)) == MAY_FLOAT_T
		    && MAY_TYPE(MAY_IM(base)) == MAY_FLOAT_T)
		|| (MAY_TYPE(expo) == MAY_COMPLEX_T
		    && MAY_TYPE(MAY_RE(expo)) == MAY_FLOAT_T
		    && MAY_TYPE(MAY_IM(expo)) == MAY_FLOAT_T ) )
      goto toexpln;
    else
      goto build;
    }

 build:
  /* Can't eval it, rebuild it */
  y = MAY_NODE_C (MAY_POW_T, 2);
  MAY_SET_AT (y, 0, base);
  MAY_SET_AT (y, 1, expo);
  MAY_CLOSE_C (y, MAY_NUM_F|MAY_EVAL_F, MAY_NEW_HASH2 (base, expo));
  MAY_ASSERT (MAY_EVAL_P (y));
  return y;

 toexpln:
  /* Evaluate it in the form exp(expo*log(base)) IN float mode */
  MAY_ASSERT (MAY_EVAL_P (expo));
  y = may_exp_c (may_mul_c (expo, may_log_c (base)));
  y = may_evalf (y);
  MAY_ASSERT (MAY_EVAL_P (y));
  return y;
}

/* Return the Modulus (in symmetric representation).
   return a mod b in the range [-iquo(abs(b)-1,2), iquo(abs(b),2)] */
MAY_REGPARM may_t
may_num_smod (may_t a, may_t b)
{
  MAY_ASSERT (MAY_TYPE (a) == MAY_INT_T && MAY_TYPE (b) == MAY_INT_T);
  mpz_t m, bd2;
  MAY_RECORD ();
  /* bd2 = (b / 2) -1 */
  mpz_init_set (bd2, MAY_INT (b));
  mpz_cdiv_q_2exp (bd2, bd2, 1);
  mpz_sub_ui (bd2, bd2, 1);
  /* m = ((a+bd2) % b) - bd2 */
  mpz_init_set (m, MAY_INT (a));
  mpz_add (m, m, bd2);
  mpz_mod (m, m, MAY_INT (b));
  mpz_sub (m, m, bd2);
  /* Return the evaluated argument */
  MAY_RET (may_num_simplify (MAY_MPZ_NOCOPY_C (m)));
}

/* Compute the numerical GCD of two numbers. */
/* Accept evaluated or non-evaluated arguments */
MAY_REGPARM may_t
may_num_gcd (may_t x, may_t y)
{
  mpz_t z;
  may_type_t t;

  MAY_LOG_FUNC (("x='%Y' y=='%Y'",x,y));

  MAY_ASSERT (MAY_PURENUM_P (x) && MAY_PURENUM_P (y));

  /* case where x==y (Not unlikely) */
  if (may_num_cmp (x, y) == 0)
    return x;

  /* Definitions for RAT:
     1. GCD(RAT,RAT)=1 ?
     2. GCD(P1/Q1,P2/Q2)= GCD(P1,P2)/LCM(Q1,Q2) ? */

  if (MAY_TYPE (x) > MAY_TYPE (y))
    swap (x, y);
  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
    switch (MAY_TYPE (y)) {
    case MAY_INT_T:
      mpz_init (z);
      mpz_gcd (z, MAY_INT (x), MAY_INT (y));
      return mpz_cmp_ui (z, 1) == 0 ? MAY_ONE : MAY_MPZ_NOCOPY_C (z);
    case MAY_RAT_T:
    case MAY_FLOAT_T: /* TODO: To define... */
      return MAY_ONE;
    default:
      MAY_ASSERT (MAY_TYPE (y) == MAY_COMPLEX_T);
      if (!(MAY_TYPE (MAY_RE (y)) == MAY_INT_T
            && MAY_TYPE (MAY_IM (y)) == MAY_INT_T))
        return MAY_ONE;
      mpz_init (z);
      mpz_gcd (z, MAY_INT (MAY_RE (y)), MAY_INT (MAY_IM (y)));
      mpz_gcd (z, z, MAY_INT (x));
      return mpz_cmp_ui (z, 1) == 0 ? MAY_ONE : MAY_MPZ_NOCOPY_C (z);
    }
  case MAY_RAT_T:
    switch (MAY_TYPE (y)) {
    case MAY_RAT_T:
    case MAY_FLOAT_T:
      return MAY_ONE; /* TODO: To define */
    default:
      MAY_ASSERT (MAY_TYPE (y) == MAY_COMPLEX_T);
      if (!(MAY_TYPE (MAY_RE (y)) == MAY_RAT_T
            && MAY_TYPE (MAY_IM (y)) == MAY_RAT_T))
        return MAY_ONE;
      if (mpq_cmp (MAY_RAT (MAY_RE (y)), MAY_RAT (MAY_IM (y))) == 0
          && mpq_cmp (MAY_RAT (x), MAY_RAT (MAY_RE (y))) == 0)
        return x;
      return MAY_ONE;
    }
  case MAY_FLOAT_T:
    switch (MAY_TYPE (y)) {
    case MAY_FLOAT_T:
      return MAY_ONE; /* TODO: To define */
    default:
      MAY_ASSERT (MAY_TYPE (y) == MAY_COMPLEX_T);
      if (!(MAY_TYPE (MAY_RE (y)) == MAY_FLOAT_T
            && MAY_TYPE (MAY_IM (y)) == MAY_FLOAT_T))
        return MAY_ONE;
      if (mpfr_equal_p (MAY_FLOAT (MAY_RE (y)), MAY_FLOAT (MAY_IM (y)))
          && mpfr_equal_p (MAY_FLOAT (x), MAY_FLOAT (MAY_RE (y))))
        return x;
      return MAY_ONE;
    }
  default:
    MAY_ASSERT (MAY_TYPE (x) == MAY_COMPLEX_T);
    MAY_ASSERT (MAY_TYPE (y) == MAY_COMPLEX_T);
    t = MAY_TYPE (MAY_RE (x));
    if (MAY_TYPE (MAY_IM (x)) != t
        || MAY_TYPE (MAY_RE (y)) != t
        || MAY_TYPE (MAY_IM (y)) != t)
      return MAY_ONE;
    if (t == MAY_INT_T) {
      mpz_init (z);
      /* To improve: (2+2*I) */
      mpz_gcd (z, MAY_INT (MAY_RE (x)), MAY_INT (MAY_IM (x)));
      mpz_gcd (z, z, MAY_INT (MAY_RE (y)));
      mpz_gcd (z, z, MAY_INT (MAY_IM (y)));
      if (MAY_RE (x) == MAY_ZERO && MAY_RE (y) == MAY_ZERO)
        return mpz_cmp_ui (z, 1) == 0 ? MAY_I : MAY_COMPLEX_C (MAY_ZERO, MAY_MPZ_NOCOPY_C (z));
      else
        return mpz_cmp_ui (z, 1) == 0 ? MAY_ONE : MAY_MPZ_NOCOPY_C (z);
    }
    return MAY_ONE;
  }
}

MAY_REGPARM may_t
may_num_lcm (may_t x, may_t y)
{
  may_t z1, z2;

  MAY_ASSERT (MAY_PURENUM_P (x) && MAY_PURENUM_P (y));
  /* case where x==y (Not unlikely) */
  if (may_identical (x, y) == 0)
    return x;

  /* Naive way: Returnx x*y/gcd(x,y) */
  MAY_RECORD ();
  z1 = may_num_mul (MAY_DUMMY, x, y);
  z2 = may_num_gcd (x, y);
  z1 = may_num_div (z1, z1, z2);
  MAY_RET_EVAL (z1);
}

/* Computes with MPFR and if it fails (NAN), computes with the complex
   float function.
   Return evaluated form */
may_t
may_num_mpfr_cx_eval (may_t z,
		      int (*funcmpfr)(mpfr_ptr, mpfr_srcptr, mp_rnd_t),
		      may_t (*funccx)(may_t))
{
  mpfr_t f;

  MAY_ASSERT (MAY_TYPE (z) == MAY_FLOAT_T);

  mpfr_init (f);
  (*funcmpfr) (f, MAY_FLOAT (z), MAY_RND);
  if (MAY_LIKELY (!mpfr_nan_p (f)))
    return may_mpfr_simplify (MAY_MPFR_NOCOPY_C (f));
  mpfr_set_ui (f, 0, MAY_RND);
  z = MAY_COMPLEX_C (z, MAY_MPFR_NOCOPY_C (f) );
  return may_cx_simplify ((*funccx) (z));
}


/* Theses functions don't provide the correct rounding behaviour of MPC.
   They return unevaluated form */

/* returns exp(re(x))*cos(Im(x)) + I * exp(re(x))*sin(Im(x)) */
may_t
may_cxfr_exp (may_t x)
{
  mpfr_t expc, cosc, sinc;

  MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
  MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
  MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

  mpfr_init (expc);
  mpfr_init (cosc);
  mpfr_init (sinc);
  mpfr_exp (expc, MAY_FLOAT(MAY_RE(x)), MAY_RND);
  mpfr_sin_cos (sinc, cosc, MAY_FLOAT(MAY_IM(x)), MAY_RND);
  mpfr_mul (sinc, sinc, expc, MAY_RND);
  mpfr_mul (cosc, cosc, expc, MAY_RND);
  x = MAY_COMPLEX_C (MAY_MPFR_NOCOPY_C (cosc), MAY_MPFR_NOCOPY_C (sinc));
  return x;
}

/* returns (exp(I*x)+exp(-I*x))/2 */
may_t
may_cxfr_cos (may_t x)
{
   MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
   MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
   MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

   /* I*(a+I*b) = -b+I*a
      -I*(a+I*b) = b-I*a */
   may_t y = MAY_COMPLEX_C (may_num_neg(MAY_DUMMY,MAY_IM(x)),
                             may_num_set(MAY_DUMMY,MAY_RE(x)));
   may_t p1 = may_cxfr_exp (y);
   y = may_num_neg(y,y);
   may_t p2 = may_cxfr_exp (y);
   y = may_num_add (y, p1, p2);
   y = may_num_mul (y, y, MAY_HALF);
   return y;
}

/* returns (exp(I*x)-exp(-I*x))/(2I) */
may_t
may_cxfr_sin (may_t x)
{
   MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
   MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
   MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

   /* I*(a+I*b) = -b+I*a
      -I*(a+I*b) = b-I*a */
   may_t y = MAY_COMPLEX_C (may_num_neg(MAY_DUMMY,MAY_IM(x)),
                            may_num_set(MAY_DUMMY,MAY_RE(x)));
   MAY_ASSERT (!MAY_EVAL_P (y));
   may_t p1 = may_cxfr_exp (y);
   y = may_num_neg (y, y);
   may_t p2 = may_cxfr_exp (y);
   y = may_num_sub (y, p1, p2);
   y = may_num_mul (y, y, MAY_HALF);
   y = may_num_div (y, y, MAY_I);
   return y;
}

/* tan(x) = sin(x) / cos(x) */
may_t
may_cxfr_tan (may_t x)
{
   MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
   MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
   MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);
   /* Hum... I am sure we can do faster and better. */
   may_t p1 = may_cxfr_sin (x);
   may_t p2 = may_cxfr_cos (x);
   p1= may_num_div (p1, p1, p2);
   return p1;
}

/* returns (exp(x)+exp(-x))/2 */
may_t
may_cxfr_cosh (may_t x)
{
   MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
   MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
   MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

   may_t p1 = may_cxfr_exp (x);
   may_t y  = may_num_neg(MAY_DUMMY, x);
   may_t p2 = may_cxfr_exp (y);
   y = may_num_add (y, p1, p2);
   y = may_num_mul (y, y, MAY_HALF);
   return y;
}

/* returns (exp(x)-exp(-x))/2 */
may_t
may_cxfr_sinh (may_t x)
{
   MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
   MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
   MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

   may_t p1 = may_cxfr_exp (x);
   may_t y  = may_num_neg (MAY_DUMMY, x);
   may_t p2 = may_cxfr_exp (y);
   y = may_num_sub (y, p1, p2);
   y = may_num_mul (y, y, MAY_HALF);
   return y;
}

/* tanh(x) = sinh(x)/cosh(x) */
may_t
may_cxfr_tanh (may_t x)
{
   MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
   MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
   MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);
   /* Hum... I am sure we can do faster and better. */
   may_t p1 = may_cxfr_sinh (x);
   may_t p2 = may_cxfr_cosh (x);
   p1= may_num_div (p1, p1, p2);
   return p1;
}

/* angle(x+I*y) = Pi/2*sign(y)-atan(x/y) */
may_t
may_cxfr_angle (may_t x)
{
  mpfr_t pi, tmp;

  MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
  MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
  MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

  mpfr_init (pi);
  if (MAY_UNLIKELY (may_num_zero_p (MAY_IM (x)))) {
    if (mpfr_sgn (MAY_FLOAT(MAY_RE(x))) >= 0)
      mpfr_set_ui (pi, 0, MAY_RND);
    else
      mpfr_const_pi (pi, MAY_RND);
  } else {
    int s = mpfr_sgn (MAY_FLOAT (MAY_IM (x)));
    mpfr_init (tmp);
    mpfr_div (tmp, MAY_FLOAT (MAY_RE (x)), MAY_FLOAT (MAY_IM (x)), MAY_RND);
    mpfr_atan (tmp, tmp, MAY_RND);
    mpfr_const_pi (pi, MAY_RND);
    mpfr_div_2ui (pi, pi, 1, MAY_RND);
    if (s < 0)
      mpfr_neg (pi, pi, MAY_RND);
    mpfr_sub (pi, pi, tmp, MAY_RND);
  }
  return MAY_MPFR_NOCOPY_C (pi);
}

/* log(x+I*y) = log(|x+I*y|) + I* angle(x+I*y) */
may_t
may_cxfr_log (may_t x)
{
  mpfr_t f;
  may_t norm, angle;

  MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
  MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
  MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

  norm  = may_num_abs (MAY_DUMMY, x);
  angle = may_cxfr_angle (x);

  MAY_ASSERT (MAY_TYPE(norm) == MAY_FLOAT_T && MAY_EVAL_P (norm));
  MAY_ASSERT (MAY_TYPE(angle) == MAY_FLOAT_T);
  mpfr_init (f);
  mpfr_log (f, MAY_FLOAT (norm), MAY_RND);
  return MAY_COMPLEX_C (MAY_MPFR_C (f), angle);
}

/* acos(x) = -I* log(x+(1-x^2)^(1/2)*I) */
may_t
may_cxfr_acos (may_t x)
{
  may_t t;

  MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
  MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
  MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

  t = may_sqrt_c (may_sub_c (MAY_ONE, may_sqr_c (x)));  /* (1-x^2)^(1/2) */
  t = may_log_c (may_add_c (x, may_mul_c (MAY_I, t)));  /* log(x+t*I)    */
  t = may_neg_c (may_mul_c (MAY_I, t));                 /* -T*I          */
  return may_eval (t);
}

/* asin(x) = -I* log(I*x+(1-x^2)^(1/2)) */
may_t
may_cxfr_asin (may_t x)
{
  may_t t;

  MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
  MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
  MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

  t = may_sqrt_c ( may_sub_c (MAY_ONE, may_sqr_c (x)));  /* (1-x^2)^(1/2) */
  t = may_log_c (may_add_c (may_mul_c (MAY_I, x), t));   /* log(I*x+t)    */
  t = may_neg_c (may_mul_c (MAY_I, t));
  return may_eval (t);
}

/* atan(x) = 1/2*I*ln( (1-x*I)/(1+x*I) ) */
may_t
may_cxfr_atan (may_t x)
{
  may_t t;

  MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
  MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
  MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

  t = may_mul_c (MAY_I, x);
  t = may_div_c (may_sub_c (MAY_ONE, t), may_add_c (MAY_ONE, t));
  t = may_mul_c (may_mul_c (MAY_HALF, MAY_I), may_log_c (t) );
  return may_eval (t);
}

/* acosh(x) = ln (x + (x^2-1)^(1/2) ) */
may_t
may_cxfr_acosh (may_t x)
{
  may_t t;

  MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
  MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
  MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

  t = may_sqrt_c (may_sub_c ( may_sqr_c (x), MAY_ONE));
  t = may_log_c (may_add_c (x, t));
  return may_eval (t);
}

/* asinh(x) = ln (x + (x^2+1)^(1/2) ) */
may_t
may_cxfr_asinh (may_t x)
{
  may_t t;

  MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
  MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
  MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

  t = may_sqrt_c (may_add_c ( may_sqr_c (x), MAY_ONE));
  t = may_log_c (may_add_c (x, t));
  return may_eval (t);
}

/* atanh(x) = 1/2*ln((1+x)/(1-x)) */
may_t
may_cxfr_atanh (may_t x)
{
  may_t t;

  MAY_ASSERT (MAY_TYPE(x) == MAY_COMPLEX_T);
  MAY_ASSERT (MAY_TYPE(MAY_RE(x)) == MAY_FLOAT_T);
  MAY_ASSERT (MAY_TYPE(MAY_IM(x)) == MAY_FLOAT_T);

  t = may_div_c (may_add_c (MAY_ONE, x), may_sub_c (MAY_ONE, x));
  t = may_mul_c (MAY_HALF, may_log_c (t));
  return may_eval (t);
}
