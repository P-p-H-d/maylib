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

/* If we are included inside may-impl for inline definition,
   define the functions as inline, and use other internal names to avoid collision.
   Otherwise define the real function (undef the inline versions too) */
#ifdef MAY_INLINE_ITERATOR
# define MAY_ITERATOR_PROTO MAY_INLINE
# define may_sum_p may_sum_p_internal
# define may_product_p may_product_p_internal
# define may_sum_iterator_init may_sum_iterator_init_internal
# define may_sum_iterator_next may_sum_iterator_next_internal
# define may_sum_iterator_end  may_sum_iterator_end_internal
# define may_sum_iterator_ref  may_sum_iterator_ref_internal
# define may_sum_iterator_tail may_sum_iterator_tail_internal
# define may_sum_extract       may_sum_extract
# define may_product_iterator_init may_product_iterator_init_internal
# define may_product_iterator_next may_product_iterator_next_internal
# define may_product_iterator_end  may_product_iterator_end_internal
# define may_product_iterator_end2  may_product_iterator_end2_internal
# define may_product_iterator_ref   may_product_iterator_ref_internal
# define may_product_iterator_tail  may_product_iterator_tail_internal
# define may_product_extract        may_product_extract
# define may_nops may_nops_internal
# define may_op may_op_internal
#else
# define MAY_ITERATOR_PROTO /* */
# undef may_sum_p
# undef may_product_p
# undef may_sum_iterator_init
# undef may_sum_iterator_next
# undef may_sum_iterator_end
# undef may_sum_iterator_ref
# undef may_sum_iterator_tail
# undef may_product_iterator_init
# undef may_product_iterator_next
# undef may_product_iterator_end
# undef may_product_iterator_end2
# undef may_product_iterator_ref
# undef may_product_iterator_tail
# undef may_nops
# undef may_op
#endif

MAY_ITERATOR_PROTO size_t
may_nops (may_t x)
{
  /* WARNING: x may be unevaled. x may be an extension */
 return MAY_UNLIKELY(MAY_ATOMIC_P (x)) ? 0 : MAY_NODE_SIZE(x);
}

MAY_ITERATOR_PROTO may_t
may_op (may_t x, size_t i)
{
  /* WARNING: x may be unevaled. x may be an extension */
  if (MAY_UNLIKELY (MAY_ATOMIC_P (x))
      || MAY_UNLIKELY (i >= MAY_NODE_SIZE(x)))
    return NULL;
  return MAY_AT (x, i);
}


/* Define the real items of the iterator structure.
   We used an array of union so that the caller don't really what it is
   inside. Define what it is properly */
#define MAY_IT_N(it) (it[0].l)
#define MAY_IT_P(it) (it[1].pm)
#define MAY_IT_X(it) (it[2].m)

MAY_ITERATOR_PROTO int
may_sum_p (may_t x)
{
  return MAY_TYPE (x) == MAY_SUM_T;
}

MAY_ITERATOR_PROTO may_t
may_sum_iterator_init (may_iterator_t it, may_t x)
{
  may_t first;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_IT_X(it) = x;
  if (MAY_LIKELY (MAY_TYPE (x) == MAY_SUM_T)) {
    MAY_IT_N(it) = MAY_NODE_SIZE(x);
    MAY_IT_P(it) = MAY_AT_PTR (x, 0);
    first = MAY_AT (x, 0);
  } else {
    MAY_IT_N(it) = 1;
    MAY_IT_P(it) = &MAY_IT_X(it);
    first = x;
  }
  if (MAY_PURENUM_P (first)) {
     MAY_IT_N(it) --;
     MAY_IT_P(it) ++;
     return first;
  } else {
    return MAY_ZERO;
  }
}

MAY_ITERATOR_PROTO void
may_sum_iterator_next (may_iterator_t it)
{
  MAY_IT_N(it) --;
  MAY_IT_P(it) ++;
}

MAY_ITERATOR_PROTO int
may_sum_iterator_end  (may_t *num, may_t *term, may_iterator_t it)
{
  if (MAY_UNLIKELY (MAY_IT_N(it) <= 0))
    return 0;
  may_t x = *MAY_IT_P(it);
  if (MAY_LIKELY (MAY_TYPE (x) == MAY_FACTOR_T)) {
    *num  = MAY_AT (x, 0);
    *term = MAY_AT (x, 1);
  } else {
    *num = MAY_ONE;
    *term = x;
  }

  return 1;
}

MAY_ITERATOR_PROTO may_t
may_sum_iterator_ref (may_iterator_t it)
{
  MAY_ASSERT (MAY_IT_N(it) > 0);
  return *MAY_IT_P(it);
}

MAY_ITERATOR_PROTO may_t
may_sum_iterator_tail (may_iterator_t it)
{
  MAY_ASSERT (MAY_IT_N(it) > 0);
  if (MAY_IT_N(it) == 1)
    return *MAY_IT_P (it);
  else
    return may_add_vc (MAY_IT_N(it), MAY_IT_P (it));
}

MAY_ITERATOR_PROTO bool
may_sum_extract (may_t *a, may_t *b, may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x) && a != NULL && b != NULL);
  if (MAY_LIKELY (MAY_TYPE (x) == MAY_SUM_T)) {
    *a = MAY_AT (x, 0);
    if (MAY_NODE_SIZE (x) == 2) {
      *b = MAY_AT (x, 1);
    } else {
      *b = may_add_vc (MAY_NODE_SIZE (x)-1, MAY_AT_PTR(x, 1));
    }
    return true;
  }
  return false;
}

MAY_ITERATOR_PROTO int
may_product_p (may_t x)
{
  return MAY_TYPE (x) == MAY_FACTOR_T || MAY_TYPE (x) == MAY_PRODUCT_T;
}

MAY_ITERATOR_PROTO may_t
may_product_iterator_init (may_iterator_t it, may_t x)
{
  may_t first;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_IT_X(it) = x;
  if (MAY_TYPE (x) == MAY_FACTOR_T) {
    first = MAY_AT (x, 0);
    x = MAY_IT_X(it) = MAY_AT( x,1);
  } else
    first = MAY_ONE;

  if (MAY_LIKELY (MAY_TYPE (x) == MAY_PRODUCT_T)) {
    MAY_IT_N(it) = MAY_NODE_SIZE(x);
    MAY_IT_P(it) = MAY_AT_PTR (x, 0);
  } else if (MAY_PURENUM_P (x)) {
    MAY_IT_N(it) = 0;
    MAY_IT_P(it) = 0;
    first = x;
  } else {
    MAY_IT_N(it) = 1;
    MAY_IT_P(it) = &MAY_IT_X(it);
  }

  return first;
}

MAY_ITERATOR_PROTO void
may_product_iterator_next (may_iterator_t it)
{
  MAY_IT_N(it) --;
  MAY_IT_P(it) ++;
}

MAY_ITERATOR_PROTO int
may_product_iterator_end  (may_t *power, may_t *base, may_iterator_t it)
{
  if (MAY_UNLIKELY (MAY_IT_N(it) <= 0))
    return 0;
  may_t x = *MAY_IT_P(it);
  if (MAY_LIKELY (MAY_TYPE (x) == MAY_POW_T && MAY_TYPE (MAY_AT (x, 1)) == MAY_INT_T)) {
    *base  = MAY_AT (x, 0);
    *power = MAY_AT (x, 1);
  } else {
    *base = x;
    *power = MAY_ONE;
  }

  return 1;
}

MAY_ITERATOR_PROTO int
may_product_iterator_end2  (may_t *power, may_t *base, may_iterator_t it)
{
  if (MAY_UNLIKELY (MAY_IT_N(it) <= 0))
    return 0;
  may_t x = *MAY_IT_P(it);
  if (MAY_LIKELY (MAY_TYPE (x) == MAY_POW_T)) {
    *base  = MAY_AT (x, 0);
    *power = MAY_AT (x, 1);
  } else {
    *base = x;
    *power = MAY_ONE;
  }

  return 1;
}

MAY_ITERATOR_PROTO may_t
may_product_iterator_ref (may_iterator_t it)
{
  MAY_ASSERT (MAY_IT_N(it) > 0);
  return *MAY_IT_P(it);
}

MAY_ITERATOR_PROTO may_t
may_product_iterator_tail (may_iterator_t it)
{
  MAY_ASSERT (MAY_IT_N(it) > 0);
  if (MAY_IT_N(it) == 1)
    return *MAY_IT_P (it);
  else
    return may_mul_vc (MAY_IT_N(it), MAY_IT_P(it));
}

MAY_ITERATOR_PROTO bool
may_product_extract (may_t *a, may_t *b, may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x) && a != NULL && b != NULL);
  if (MAY_LIKELY (MAY_TYPE (x) == MAY_FACTOR_T)) {
    *a = MAY_AT (x, 0);
    *b = MAY_AT (x, 1);
    return true;
  } else if (MAY_TYPE (x) == MAY_PRODUCT_T) {
    *a = MAY_AT (x, 0);
    if (MAY_NODE_SIZE (x) == 2) {
      *b = MAY_AT (x, 1);
    } else {
      // TBC: Evaluated or not?
      *b = may_mul_vc (MAY_NODE_SIZE (x)-1, MAY_AT_PTR(x, 1));
    }
    return true;
  }
  return false;
}

/* Undef everything except the overloaded functions */
#undef MAY_IT_N
#undef MAY_IT_P
#undef MAY_INLINE_ITERATOR
#undef MAY_ITERATOR_PROTO
