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

/* Alloc size for the 'RESERVE' type */
MAY_INLINE may_size_t allocsize (may_size_t s) { return MAX (s + (s>>2), 8); }

may_t
may_addinc_c (may_t x, may_t y)
{
  may_size_t sy, sx;
  may_type_t tx = MAY_TYPE (x), ty = MAY_TYPE (y);

  if (MAY_UNLIKELY (y == MAY_ZERO))
    /* Even if x is a reserve, everything is ok since we can reuse its memory */
    return x;

  /* If x is numerical */
  else if (MAY_UNLIKELY (tx < MAY_NUM_LIMIT)) {
    /* Check if x is zero */
    if (MAY_UNLIKELY (x == MAY_ZERO)) {
      /* If y is a reserve, we have to make a copy of it, since
         we can't use it (and we may reuse the return value).
         We can only reuse x. For example (0+RE_P)*y */
      if (MAY_UNLIKELY (ty == MAY_SUM_RESERVE_T || ty == MAY_PRODUCT_RESERVE_T)) {
        sy = MAY_NODE_SIZE(y);
        may_t z = MAY_NODE_C (MAY_TYPE (y), sy);
        memcpy (MAY_AT_PTR (z, 0), MAY_AT_PTR (y, 0), sy * sizeof (may_t));
        return z;
      } else
        return y;
    } else if (ty < MAY_NUM_LIMIT) {
      /* This way is to check when assertion are on that 'x' is really a garbage */
#if defined(MAY_WANT_ASSERT)
      may_t z = may_num_add (MAY_DUMMY, x, y);
      if (!MAY_EVAL_P (x))
        MAY_OPEN_C (x, MAY_INT_T-1);
      return z;
#else
      return may_num_add (MAY_EVAL_P(x)?MAY_DUMMY:x, x, y);
#endif
        }
    /* Otherwise fall down to default code */
  }

  /* Check if there is reserve in x for new allocated items */
  else if (MAY_UNLIKELY (tx == MAY_SUM_RESERVE_T)) {
    /* op(x,0) = 0 / op (x,1) = numerical / op(x,i)=terms */
    MAY_ASSERT (!MAY_EVAL_P (x) && MAY_NODE_SIZE(x) >= 3 && may_num_zero_p (MAY_AT (x, 0)) && MAY_PURENUM_P (MAY_AT (x, 1)));
    /* Read #x and alloc size of x (which is the alloc size of the first op - which is alwayrs 0 - */
    sx = MAY_NODE_SIZE(x);
    sy = MAY_UNLIKELY (ty == MAY_SUM_T || ty == MAY_SUM_RESERVE_T) ? MAY_NODE_SIZE(y) : 1;
    may_size_t allocx = MAY_INT (MAY_AT (x, 0))->_mp_alloc;
    /* Test if we have allocated enought elements for adding the new ones */
    if (MAY_UNLIKELY (sx + sy > allocx)) {
      may_size_t newallocsize = allocsize (sx+sy);
      x = may_realloc (x, MAY_NODE_ALLOC_SIZE (allocx), MAY_NODE_ALLOC_SIZE (newallocsize));
      MAY_INT (MAY_AT (x, 0))->_mp_alloc = newallocsize;
    }
    /* Add the elements of y in the big enough x:
        + if y was a sum
        + if y was a numerical
        + other cases */
  add_y_in_reserve:
    if (MAY_UNLIKELY (sy != 1)) {
      memcpy (MAY_AT_PTR (x, sx), MAY_AT_PTR (y, 0), sy*sizeof (may_t));
      MAY_NODE_SIZE(x) = sx + sy;
    } else if (ty < MAY_NUM_LIMIT) {
      MAY_SET_AT (x, 1, may_num_add (MAY_AT (x, 1), MAY_AT (x, 1), y));
    } else {
      MAY_SET_AT (x, sx, y);
      MAY_NODE_SIZE(x) = sx + 1;
    }
    return x;
  }

  else if (MAY_UNLIKELY (tx == MAY_SUM_T)) {
    /* Promote x to a sum reserve */
  promote_x:
    sx = MAY_NODE_SIZE(x);
    sy = MAY_UNLIKELY (ty == MAY_SUM_T || ty == MAY_SUM_RESERVE_T) ? MAY_NODE_SIZE(y) : 1;
    may_size_t newallocsize = allocsize (2+sx+sy);
    may_t z = MAY_NODE_C (MAY_SUM_RESERVE_T, newallocsize);
    /* alloc a special 'integer' number set to the nul element to store the number of alloc nodes */
    may_t spe = MAY_ALLOC(MAY_INT_SIZE);
    memcpy (spe, MAY_ZERO, MAY_INT_SIZE);
    MAY_INT (spe)->_mp_alloc = newallocsize;
    MAY_SET_AT (z, 0, spe);
    MAY_SET_AT (z, 1, may_num_set (MAY_DUMMY, MAY_ZERO));
    memcpy(MAY_AT_PTR (z, 2), MAY_AT_PTR (x, 0), sx*sizeof(may_t));
    MAY_NODE_SIZE(z) = sx = sx+2;
    /* x becomes z */
    x = z;
    MAY_ASSERT (!MAY_EVAL_P (x) && MAY_NODE_SIZE(x) >= 3 && may_num_zero_p (MAY_AT (x, 0)) && MAY_PURENUM_P (MAY_AT (x, 1)));
    /* Add the elements of y in the big enough x */
    goto add_y_in_reserve;
  }

  /* x is not a sum, but y is (No else !!) */
  if (MAY_UNLIKELY (ty == MAY_SUM_T || ty == MAY_SUM_RESERVE_T)) {
    /* Consider y as a sum, and promote it */
    swap (tx,ty);
    swap (x,y);
    goto promote_x;
  }

  /* Neither x or y are sum. Create a new one. No need for reserve */
  may_t z = MAY_NODE_C (MAY_SUM_T, 2);
  MAY_SET_AT (z, 0, x);
  MAY_SET_AT (z, 1, y);
  return z;
}

may_t
may_mulinc_c (may_t x, may_t y)
{
  may_size_t sy, sx;
  may_type_t tx = MAY_TYPE (x), ty = MAY_TYPE (y);

  if (MAY_UNLIKELY (y == MAY_ONE))
    /* Even if x is a reserve, everything is ok since we can reuse its memory */
    return x;

  /* If x is numerical */
  else if (MAY_UNLIKELY (tx < MAY_NUM_LIMIT)) {
    /* Check if x is zero */
    if (MAY_UNLIKELY (x == MAY_ONE)) {
      /* If y is a reserve, we have to make a copy of it, since
         we can't use it (and we may reuse the return value).
         We can only reuse x. For example (1*RE_P)+2 */
      if (MAY_UNLIKELY (ty == MAY_SUM_RESERVE_T || ty == MAY_PRODUCT_RESERVE_T)) {
        sy = MAY_NODE_SIZE(y);
        may_t z = MAY_NODE_C (MAY_TYPE (y), sy);
        memcpy (MAY_AT_PTR (z, 0), MAY_AT_PTR (y, 0), sy * sizeof (may_t));
        return z;
      } else
        return y;
    } else if (ty < MAY_NUM_LIMIT) {
      /* This way is to check when assertion are on that 'x' is really a garbage */
#if defined(MAY_WANT_ASSERT)
      may_t z = may_num_mul (MAY_DUMMY, x, y);
      if (!MAY_EVAL_P (x))
        MAY_OPEN_C (x, MAY_INT_T-1);
      return z;
#else
      return may_num_mul (MAY_EVAL_P(x)?MAY_DUMMY:x, x, y);
#endif
        }
    /* Otherwise fall down to default code */
  }

  /* Check if there is reserve in x for new allocated items */
  else if (MAY_UNLIKELY (tx == MAY_PRODUCT_RESERVE_T)) {
    /* op(x,0) = 1 / op (x,1) = numerical / op(x,i)=terms */
    MAY_ASSERT (!MAY_EVAL_P (x) && MAY_NODE_SIZE(x) >= 3 && may_num_one_p (MAY_AT (x, 0)) && MAY_PURENUM_P (MAY_AT (x, 1)));
    /* Read #x and alloc size of x (which is the alloc size of the first op - which is always 1 - */
    sx = MAY_NODE_SIZE(x);
    sy = MAY_UNLIKELY (ty == MAY_PRODUCT_T || ty == MAY_PRODUCT_RESERVE_T) ? MAY_NODE_SIZE(y) : 1;
    may_size_t allocx = MAY_INT (MAY_AT (x, 0))->_mp_alloc;
    /* Test if we have allocated enough elements for adding the new ones */
    if (MAY_UNLIKELY (sx + sy > allocx)) {
      may_size_t newallocsize = allocsize (sx+sy);
      x = may_realloc (x, MAY_NODE_ALLOC_SIZE (allocx), MAY_NODE_ALLOC_SIZE (newallocsize));
      MAY_INT (MAY_AT (x, 0))->_mp_alloc = newallocsize;
    }
    /* Add the elements of y in the big enough x:
        + if y was a product
        + if y was a factor
        + if y was a numerical
        + other cases */
  add_y_in_reserve:
    if (ty == MAY_FACTOR_T) {
      MAY_SET_AT (x, 1, may_num_mul (MAY_AT (x, 1), MAY_AT (x, 1), MAY_AT (y, 0)));
      MAY_SET_AT (x, sx, MAY_AT (y, 1));
      MAY_NODE_SIZE(x) = sx + 1;
    } else  if (MAY_UNLIKELY (sy != 1)) {
      memcpy (MAY_AT_PTR (x, sx), MAY_AT_PTR (y, 0), sy*sizeof (may_t));
      MAY_NODE_SIZE(x) = sx + sy;
    } else if (ty < MAY_NUM_LIMIT) {
      MAY_SET_AT (x, 1, may_num_mul (MAY_AT (x, 1), MAY_AT (x, 1), y));
    } else {
      MAY_SET_AT (x, sx, y);
      MAY_NODE_SIZE(x) = sx + 1;
    }
    return x;
  }
  
  else if (MAY_UNLIKELY (tx == MAY_PRODUCT_T || tx == MAY_FACTOR_T)) {
    /* Promote x to a product reserve */
  promote_x:
    sx = MAY_NODE_SIZE(x);
    sy = MAY_UNLIKELY (ty == MAY_PRODUCT_T || ty == MAY_PRODUCT_RESERVE_T) ? MAY_NODE_SIZE(y) : 1;
    may_size_t newallocsize = allocsize (2+sx+sy);
    may_t z = MAY_NODE_C (MAY_PRODUCT_RESERVE_T, newallocsize);
    /* alloc a special 'integer' number set to the nul element to store the number of alloc nodes */
    may_t spe = MAY_ALLOC(MAY_INT_SIZE);
    memcpy (spe, MAY_ONE, MAY_INT_SIZE);
    MAY_INT (spe)->_mp_alloc = newallocsize;
    MAY_SET_AT (z, 0, spe);
    MAY_SET_AT (z, 1, may_num_set (MAY_DUMMY, MAY_ONE));
    memcpy(MAY_AT_PTR (z, 2), MAY_AT_PTR (x, 0), sx*sizeof(may_t));
    MAY_NODE_SIZE(z) = sx = sx+2;
    /* x becomes z */
    x = z;
    MAY_ASSERT (!MAY_EVAL_P (x) && MAY_NODE_SIZE(x) >= 3 && may_num_one_p (MAY_AT (x, 0)) && MAY_PURENUM_P (MAY_AT (x, 1)));
    /* Add the elements of y in the big enough x */
    goto add_y_in_reserve;
  }
  
   /* x is not a product, but y is (No else !!) */
  if (MAY_UNLIKELY (ty == MAY_PRODUCT_T || ty == MAY_FACTOR_T || ty == MAY_PRODUCT_RESERVE_T)) {
    /* Consider y as a product, and promote it */
    swap (tx,ty);
    swap (x,y);
    goto promote_x;
  }

  /* Neither x or y are sum. Create a new one. No need for reservation */
  may_t z = MAY_NODE_C (MAY_PRODUCT_T, 2);
  MAY_SET_AT (z, 0, x);
  MAY_SET_AT (z, 1, y);
  return z;
}

may_t
may_add_c (may_t x, may_t y)
{
  may_t z;

  if (MAY_UNLIKELY(x == MAY_ZERO))
    return y;
  else if (MAY_UNLIKELY(y == MAY_ZERO))
    return x;
  else if (MAY_TYPE (x) != MAY_SUM_T) {
    if (MAY_TYPE (y) != MAY_SUM_T) {
      z = MAY_NODE_C (MAY_SUM_T, 2);
      MAY_SET_AT (z, 0, x);
      MAY_SET_AT (z, 1, y);
    } else {
      z = MAY_NODE_C (MAY_SUM_T, 1+MAY_NODE_SIZE(y));
      MAY_SET_AT (z, 0, x);
      memcpy (MAY_AT_PTR (z, 1), MAY_AT_PTR (y, 0),
	      MAY_NODE_SIZE(y)*sizeof (may_t));
    }
  } else {
    if (MAY_TYPE (y) != MAY_SUM_T) {
      z = MAY_NODE_C (MAY_SUM_T, 1+MAY_NODE_SIZE(x));
      MAY_SET_AT (z, 0, y);
      memcpy (MAY_AT_PTR (z, 1), MAY_AT_PTR (x, 0),
	      MAY_NODE_SIZE(x)*sizeof (may_t));
    } else {
      z = MAY_NODE_C (MAY_SUM_T, MAY_NODE_SIZE(y)+MAY_NODE_SIZE(x) );
      memcpy (MAY_AT_PTR (z, 0), MAY_AT_PTR (x, 0),
	      MAY_NODE_SIZE(x)*sizeof (may_t));
      memcpy (MAY_AT_PTR (z, MAY_NODE_SIZE(x)), MAY_AT_PTR (y, 0),
	      MAY_NODE_SIZE(y)*sizeof (may_t));
    }
  }
  return z;
}

may_t
may_mul_c (may_t x, may_t y)
{
  may_t z;

  if (MAY_UNLIKELY(x == MAY_ONE))
    return y;
  else if (MAY_UNLIKELY(y == MAY_ONE))
    return x;
  else if (MAY_TYPE (x) != MAY_PRODUCT_T && MAY_TYPE (x) != MAY_FACTOR_T && MAY_TYPE (x) != MAY_PRODUCT_RESERVE_T) {
    if (MAY_TYPE (y) != MAY_PRODUCT_T && MAY_TYPE (y) != MAY_FACTOR_T && MAY_TYPE (y) != MAY_PRODUCT_RESERVE_T) {
      z = MAY_NODE_C (MAY_PRODUCT_T, 2);
      MAY_SET_AT (z, 0, x);
      MAY_SET_AT (z, 1, y);
    } else {
      z = MAY_NODE_C (MAY_PRODUCT_T, 1+MAY_NODE_SIZE(y));
      MAY_SET_AT (z, 0, x);
      memcpy (MAY_AT_PTR (z, 1), MAY_AT_PTR (y, 0),
	      MAY_NODE_SIZE(y)*sizeof (may_t));
    }
  } else {
    if (MAY_TYPE (y) != MAY_PRODUCT_T && MAY_TYPE (y) != MAY_FACTOR_T && MAY_TYPE (y) != MAY_PRODUCT_RESERVE_T) {
      z = MAY_NODE_C (MAY_PRODUCT_T, 1+MAY_NODE_SIZE(x));
      MAY_SET_AT (z, 0, y);
      memcpy (MAY_AT_PTR (z, 1), MAY_AT_PTR (x, 0),
	      MAY_NODE_SIZE(x)*sizeof (may_t));
    } else {
      z = MAY_NODE_C (MAY_PRODUCT_T, MAY_NODE_SIZE(y) + MAY_NODE_SIZE(x) );
      memcpy (MAY_AT_PTR (z, 0), MAY_AT_PTR (x, 0),
	      MAY_NODE_SIZE(x)*sizeof (may_t));
      memcpy (MAY_AT_PTR (z, MAY_NODE_SIZE(x)), MAY_AT_PTR (y, 0),
	      MAY_NODE_SIZE(y)*sizeof (may_t));
    }
  }
  return z;
}

may_t
may_sub_c (may_t x, may_t y)
{
  may_t z, s;

  if (MAY_UNLIKELY(y == MAY_ZERO))
    return x;

  /* If both are purenum, it is faster to compute them
     directly as sub, instead of performing a multipliation
     and then an addition */
  if (MAY_PURENUM_P (x) && MAY_PURENUM_P (y))
    return may_num_sub (MAY_DUMMY, x, y);

  s = MAY_NODE_C (MAY_PRODUCT_T, 2);
  MAY_SET_AT(s, 0, MAY_MPZ_NOCOPY_C (MAY_INT (MAY_N_ONE)));
  MAY_SET_AT(s, 1, y);
  if (MAY_UNLIKELY(x == MAY_ZERO))
    return s;
  z = MAY_NODE_C (MAY_SUM_T, 2);
  MAY_SET_AT(z, 0, x);
  MAY_SET_AT(z, 1, s);
  return z;
}

may_t
may_neg_c (may_t x)
{
  may_t s;

  s = MAY_NODE_C (MAY_PRODUCT_T, 2);
  MAY_SET_AT(s, 0, MAY_MPZ_NOCOPY_C (MAY_INT (MAY_N_ONE)));
  MAY_SET_AT(s, 1, x);
  return s;
}

may_t
may_div_c (may_t x, may_t y)
{
  may_t s;

  /* Handle easy cases */
  if (MAY_UNLIKELY (y == MAY_ONE))
    return x;

  /* If both are purenum, it is faster to compute them
     directly as div, instead of performing an exponentation
     and then a multiplication */
  if (MAY_PURENUM_P (x) && MAY_PURENUM_P (y))
    return may_num_div (MAY_DUMMY, x, y);

  s = MAY_NODE_C (MAY_POW_T, 2);
  MAY_SET_AT(s, 0, y);
  MAY_SET_AT(s, 1, MAY_N_ONE);
  if (MAY_UNLIKELY(x == MAY_ONE))
    return s;
  return may_mul_c (x, s);
}

static MAY_REGPARM may_t
may_binary_ifunc_c (may_t x, may_t y, may_type_t type)
{
  may_t z;
  z = MAY_NODE_C (type, 2);
  MAY_SET_AT (z, 0, x);
  MAY_SET_AT (z, 1, y);
  return z;
}

may_t
may_pow_c (may_t x,  may_t y)
{
  if (MAY_UNLIKELY (y == MAY_ONE))
    return x;
  return may_binary_ifunc_c (x, y, MAY_POW_T);
}

may_t
may_pow_si_c (may_t x,  long y)
{
  if (MAY_UNLIKELY (y == 1))
    return x;
  return may_binary_ifunc_c (x, MAY_SLONG_C (y), MAY_POW_T);
}

may_t
may_addmul_vc (size_t size, const may_pair_t *tab)
{
  may_size_t i;
  may_t y;

  if (MAY_UNLIKELY (size == 0))
    return MAY_ZERO;
  y = MAY_NODE_C (MAY_SUM_T, size);
  for (i = 0; MAY_LIKELY (i < size); i++)
    MAY_SET_AT (y, i, may_mul_c (tab[i].first, tab[i].second));
  return y;
}
may_t
may_mulpow_vc (size_t size, const may_pair_t *tab)
{
  may_size_t i;
  may_t y;

  if (MAY_UNLIKELY (size == 0))
    return MAY_ONE;
  y = MAY_NODE_C (MAY_PRODUCT_T, size);
  for (i = 0; MAY_LIKELY (i < size); i++)
    MAY_SET_AT (y, i, may_pow_c (tab[i].second, tab[i].first));
  return y;
}

may_t
may_sqr_c (may_t x)
{
  return may_binary_ifunc_c (x, MAY_TWO, MAY_POW_T);
}

may_t
may_sqrt_c (may_t x)
{
  return may_binary_ifunc_c (x, MAY_HALF, MAY_POW_T);
}

may_t
may_mod_c (may_t x, may_t y)
{
  return may_binary_ifunc_c (x, y, MAY_MOD_T);
}

may_t
may_gcd_c (may_t x, may_t y)
{
  return may_binary_ifunc_c (x, y, MAY_GCD_T);
}

static MAY_REGPARM may_t
may_unary_ifunc_c (may_t x, may_type_t type)
{
  may_t z;
  z = MAY_NODE_C (type, 1);
  MAY_SET_AT(z, 0, x);
  return z;
}

may_t
may_exp_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_EXP_T);
}

may_t
may_log_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_LOG_T);
}

may_t
may_sin_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_SIN_T);
}

may_t
may_cos_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_COS_T);
}

may_t
may_tan_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_TAN_T);
}

may_t
may_asin_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_ASIN_T);
}

may_t
may_acos_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_ACOS_T);
}

may_t
may_atan_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_ATAN_T);
}

may_t
may_sinh_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_SINH_T);
}

may_t
may_cosh_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_COSH_T);
}

may_t
may_tanh_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_TANH_T);
}

may_t
may_asinh_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_ASINH_T);
}

may_t
may_acosh_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_ACOSH_T);
}

may_t
may_atanh_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_ATANH_T);
}

may_t
may_abs_c (may_t x)
{
  /* Trivial check to verify if the input is already a positive real */
  if (MAY_PURENUM_P (x) && may_num_poszero_p (x))
    return x;
  return may_unary_ifunc_c (x, MAY_ABS_T);
}

may_t
may_sign_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_SIGN_T);
}

may_t
may_floor_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_FLOOR_T);
}

may_t
may_conj_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_CONJ_T);
}

may_t
may_real_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_REAL_T);
}

may_t
may_imag_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_IMAG_T);
}

may_t
may_argument_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_ARGUMENT_T);
}

may_t
may_range_c (may_t l, may_t r)
{
  return may_binary_ifunc_c (l, r, MAY_RANGE_T);
}

may_t
may_gamma_c (may_t x)
{
  return may_unary_ifunc_c (x, MAY_GAMMA_T);
}

may_t
may_fact_c (may_t x)
{
  return may_gamma_c (may_add_c (x, MAY_ONE));
}

may_t
may_ceil_c (may_t x)
{
  return may_neg_c (may_floor_c (may_neg_c (x)));
}

static may_t
may_diff3_c (may_t x)
{
  may_size_t n;
  if (MAY_UNLIKELY (MAY_TYPE (x) != MAY_LIST_T
                    || (n = MAY_NODE_SIZE(x)) < 2 || n > 4))
    MAY_THROW (MAY_INVALID_TOKEN_ERR);
  return may_diff_c (MAY_AT (x, 0),
                     MAY_AT (x, 1),
                     (n > 2) ? MAY_AT (x, 2) : MAY_ONE,
                     (n > 3) ? MAY_AT (x, 3) : MAY_AT (x, 1));
}

static may_t
may_range3_c (may_t x)
{
  if (MAY_UNLIKELY (MAY_TYPE (x) != MAY_LIST_T
                    || MAY_NODE_SIZE(x) != 2))
    MAY_THROW (MAY_INVALID_TOKEN_ERR);
  return may_range_c (MAY_AT(x, 0), MAY_AT(x, 1));
}

static may_t
may_gcd3_c (may_t x)
{
  if (MAY_UNLIKELY (MAY_TYPE (x) != MAY_LIST_T
                    || MAY_NODE_SIZE(x) != 2))
    MAY_THROW (MAY_INVALID_TOKEN_ERR);
  return may_gcd_c (MAY_AT(x, 0), MAY_AT(x, 1));
}

static may_t
may_mod3_c (may_t x)
{
  if (MAY_UNLIKELY (MAY_TYPE (x) != MAY_LIST_T
                    || MAY_NODE_SIZE(x) != 2))
    MAY_THROW (MAY_INVALID_TOKEN_ERR);
  return may_mod_c (MAY_AT(x, 0), MAY_AT(x, 1));
}

may_t
may_func_c (const char name[], may_t x)
{
  return may_func_domain_c (name, x, may_g.frame.domain);
}

may_t
may_func_domain_c (const char name[], may_t x, may_domain_e domain)
{
  static const struct {
    const char *name;
    may_t (*func)(may_t);
  } FuncTab[] = {
    {may_range_name, may_range3_c}, /* In upper cases ==> At the top */
    {may_abs_name,   may_abs_c},
    {may_acos_name,  may_acos_c},
    {may_acosh_name, may_acosh_c},
    {may_argument_name, may_argument_c},
    {may_asin_name,  may_asin_c},
    {may_asinh_name, may_asinh_c},
    {may_atan_name,  may_atan_c},
    {may_atanh_name, may_atanh_c},
    {may_ceil_name,  may_ceil_c},
    {may_conj_name,  may_conj_c},
    {may_cos_name,   may_cos_c},
    {may_cosh_name,  may_cosh_c},
    {may_diff_name,  may_diff3_c},
    {may_exp_name,   may_exp_c},
    {may_floor_name, may_floor_c},
    {may_gamma_name, may_gamma_c},
    {may_gcd_name,   may_gcd3_c},
    {may_imag_name,  may_imag_c},
    {may_ln_name,    may_log_c},
    {may_log_name,   may_log_c},
    {may_mod_name,   may_mod3_c},
    {may_real_name,  may_real_c},
    {may_sign_name,  may_sign_c},
    {may_sin_name,   may_sin_c},
    {may_sinh_name,  may_sinh_c},
    {may_sqrt_name,  may_sqrt_c},
    {may_tan_name,   may_tan_c},
    {may_tanh_name,  may_tanh_c} };
  may_t z;
  size_t start, end;
  start = 0;
  end   = numberof (FuncTab);
  while (start < end) {
    size_t pos = (start+end)/2;
    int ret = strcmp (name, FuncTab[pos].name);
    if (ret == 0) {
      return  (*FuncTab[pos].func) (x);
    } else if (ret < 0) {
      end = pos;
    } else {
      start = pos+1;
    }
  }
  MAY_ASSERT (start == end);
  MAY_ASSERT (start >= numberof (FuncTab) || strcmp (name, FuncTab[start].name) != 0);

  z = MAY_NODE_C (MAY_FUNC_T, 2);
  MAY_SET_AT (z, 0, MAY_STRING_C (name, domain));
  MAY_SET_AT (z, 1, x);
  return z;
}

static MAY_REGPARM may_t
handle_va_list (may_type_t type, may_t x, va_list arg)
{
  va_list argc;
  may_size_t n;
  may_t y;

  /* Count # of args */
  va_copy (argc, arg);
  n = 0;
  do {
    y = va_arg (arg, may_t);
    n++;
  } while (y != 0);

  /* Create list with given type */
  y = MAY_NODE_C(type, n);
  n = 0;
  do {
    MAY_SET_AT (y, n, x);
    x = va_arg (argc, may_t);
    n++;
  } while (x != 0);
  return y;
}

may_t
may_list_vac (may_t x, ...)
{
  va_list arg;
  may_t y;

  va_start (arg, x);
  y = handle_va_list (MAY_LIST_T, x, arg);
  va_end (arg);
  return y;
}

may_t
may_add_vac (may_t x, ...)
{
  va_list arg;
  may_t y;

  va_start (arg, x);
  y = handle_va_list (MAY_SUM_T, x, arg);
  va_end (arg);
  return y;
}

may_t
may_mul_vac (may_t x, ...)
{
  va_list arg;
  may_t y;

  va_start (arg, x);
  y = handle_va_list (MAY_PRODUCT_T, x, arg);
  va_end (arg);
  return y;
}

may_t
may_list_vc (size_t size, const may_t *tab)
{
  may_size_t i;
  may_t y;

  y = MAY_NODE_C (MAY_LIST_T, size);
  for (i = 0 ; MAY_LIKELY (i < size); i++)
    MAY_SET_AT (y, i, tab[i]);
  return y;
}

may_t
may_add_vc (size_t size, const may_t *tab)
{
  may_size_t i;
  may_t y;

  if (MAY_UNLIKELY (size == 0))
    return MAY_ZERO;
  y = MAY_NODE_C (MAY_SUM_T, size);
  for (i = 0; MAY_LIKELY (i < size); i++)
    MAY_SET_AT (y, i, tab[i]);
  return y;
}

may_t
may_mul_vc (size_t size, const may_t *tab)
{
  may_size_t i;
  may_t y;

  if (MAY_UNLIKELY (size == 0))
    return MAY_ONE;
  y = MAY_NODE_C (MAY_PRODUCT_T, size);
  for (i = 0 ; MAY_LIKELY (i < size); i++)
    MAY_SET_AT (y, i, tab[i]);
  return y;
}



may_t
may_add (may_t x, may_t y)
{
  MAY_ASSERT (MAY_EVAL_P (x) && MAY_EVAL_P (y));
  MAY_RECORD ();
  x = may_add_c (x, y);
  MAY_RET_EVAL (x);
}

may_t
may_sub (may_t x, may_t y)
{
  MAY_ASSERT (MAY_EVAL_P (x) && MAY_EVAL_P (y));
  MAY_RECORD ();
  x = may_sub_c (x, y);
  MAY_RET_EVAL (x);
}

may_t
may_mul (may_t x, may_t y)
{
  MAY_ASSERT (MAY_EVAL_P (x) && MAY_EVAL_P (y));
  MAY_RECORD ();
  x = may_mul_c (x, y);
  MAY_RET_EVAL (x);
}

may_t
may_div (may_t x, may_t y)
{
  MAY_ASSERT (MAY_EVAL_P (x) && MAY_EVAL_P (y));
  MAY_RECORD ();
  x = may_div_c (x, y);
  MAY_RET_EVAL (x);
}

may_t
may_pow (may_t x, may_t y)
{
  MAY_ASSERT (MAY_EVAL_P (x) && MAY_EVAL_P (y));
  MAY_RECORD ();
  x = may_pow_c (x, y);
  MAY_RET_EVAL (x);
}

may_t
may_neg (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_RECORD ();
  x = may_neg_c (x);
  MAY_RET_EVAL (x);
}

may_t
may_sqr (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_RECORD ();
  x = may_sqr_c (x);
  MAY_RET_EVAL (x);
}

may_t
may_sqrt (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_RECORD ();
  x = may_sqrt_c (x);
  MAY_RET_EVAL (x);
}
