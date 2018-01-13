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

/* Replace in 'x' all expressions 'old' by 'new' */
may_t
may_replace_c (may_t x, may_t old, may_t new)
{
  /* First check if x should be replaced */
  if (MAY_UNLIKELY (may_identical (x, old) == 0))
    return new;
  /* Else check if atomic */
  else if (MAY_ATOMIC_P (x))
    return x;
  /* Else go down the tree */
  else {
    MAY_RECORD ();
    may_size_t i, n = MAY_NODE_SIZE(x);
    may_t y = MAY_NODE_C (MAY_TYPE(x), n);
    int isnew = 0;
    for (i = 0; i < n; i++) {
      may_t zo = MAY_AT (x, i);
      may_t z = may_replace_c (zo, old, new);
      isnew |= (z != zo);
      MAY_SET_AT (y, i, z);
    }
    if (isnew)
      return y;
    /* Nothing has changed. Return the original 'x' */
    MAY_CLEANUP ();
    return x;
 }
}

may_t
may_replace (may_t x, may_t old, may_t new)
{
  MAY_ASSERT (MAY_EVAL_P (x) && MAY_EVAL_P (old) && MAY_EVAL_P (new));
  MAY_LOG_FUNC (("x='%Y' old='%Y' new='%Y'", x, old, new));
  /* Check case where there is no change */
  if (MAY_UNLIKELY (may_identical (old, new) == 0))
    return x;
  /* No need to GC: may_replace_c is already optimum */
  return may_eval (may_replace_c (x, old, new));
}

/* Replace old by new in x view as polynomial of old
   (Doesn't replace inside indents of x which may depend on old */
may_t
may_replace_upol_c (may_t x, may_t old, may_t new)
{
  /* First check if x should be replaced */
  if (MAY_UNLIKELY (may_identical (x, old) == 0))
    return new;
  /* Else check if atomic */
  else if (MAY_NODE_P (x)) {
    /* Else go down the tree */
    if (MAY_TYPE (x) == MAY_POW_T) {
      may_t op0 = MAY_AT (x, 0);
      may_t base = may_replace_upol_c (op0, old, new);
      if (base != op0) {
        may_t y = MAY_NODE_C (MAY_POW_T, 2);
        MAY_SET_AT (y, 0, base);
        MAY_SET_AT (y, 1, MAY_AT (x, 1));
        return y;
      }
    } else if (MAY_TYPE (x) == MAY_SUM_T
               || MAY_TYPE (x) == MAY_FACTOR_T
               || MAY_TYPE (x) == MAY_PRODUCT_T) {
      MAY_RECORD ();
      may_size_t i, n = MAY_NODE_SIZE(x);
      may_t y = MAY_NODE_C (MAY_TYPE(x), n);
      int isnew = 0;
      for (i = 0; i < n; i++) {
        may_t zo = MAY_AT (x, i);
        may_t z = may_replace_upol_c (zo, old, new);
        isnew |= (z != zo);
        MAY_SET_AT (y, i, z);
      }
      if (isnew)
        return y;
      /* Nothing has changed. Return the original 'x' */
      MAY_CLEANUP ();
    }
  }
  return x;
}

may_t
may_replace_upol (may_t x, may_t old, may_t new)
{
  MAY_ASSERT (MAY_EVAL_P (x) && MAY_EVAL_P (old) && MAY_EVAL_P (new));
  MAY_LOG_FUNC (("x='%Y' old='%Y' new='%Y'", x, old, new));
  /* Check case where there is no change */
  if (MAY_UNLIKELY (may_identical (old, new) == 0))
    return x;
  /* No need to GC: may_replace_upol_c is already optimum */
  return may_eval (may_replace_upol_c (x, old, new));
}


/* Return an index in the hash table from the given hash code */
MAY_INLINE size_t
gindex (may_hash_t hx, struct may_subs_s *context)
{
  return (size_t) ((double) hx * context->hash);
}

static may_t
may_subs_recur2 (may_t x, unsigned long level, struct may_subs_s *context)
{
  may_t y, z;
  may_t (*cb) (may_t);
  size_t j;
  may_size_t i, n;
  int isnew;

  switch (MAY_TYPE (x))
    {
    case MAY_INT_T ...MAY_NUM_LIMIT:
    case MAY_DATA_T:
      return x;
    case MAY_STRING_T:
      /* Get the index of this symbol in the HASH table */
      j = gindex (MAY_HASH (x), context);
      while (context->gtab[j] != 0
             && strcmp (MAY_NAME (x),
                        context->gvars[context->gtab[j]-1]) != 0)
        if (++j >= context->gsize)
          j = 0;
      /* If not found, return the string itself */
      if (context->gtab[j] == 0)
        return x;
      /* Otherwise, read the replacement value */
      y = context->gvalue[context->gtab[j]-1];
      /* TODO: Doesn't work with thread */
      if (!((void*)y >= (void*)may_g.Heap.base && (void*)y < (void*)may_g.Heap.limit))
        return x; /* It is a function CB, not a symbol */
      /* Check if we have to replace the symbol once again */
      if (level > 1)
        y = may_subs_recur2 (y, level-1, context);
      return y;
    case MAY_FUNC_T:
      /* Get the index of the function name in the HASH table */
      z = MAY_AT (x, 0);
      j = gindex (MAY_HASH (z), context);
      while (context->gtab[j] != 0
             && strcmp (MAY_NAME (z),
                        context->gvars[context->gtab[j]-1]) != 0)
        if (++j >= context->gsize)
          j = 0;
      /* Substiture the arguments of the function */
      z = may_subs_recur2 (MAY_AT (x, 1), level, context);
      /* Check if we have found the symbol */
      if (context->gtab[j] == 0) {
      not_found_function:
        /* No, so check if we must rebuild the expression */
        if (z != MAY_AT (x, 1)) {
          y = MAY_NODE_C (MAY_FUNC_T, 2);
          MAY_SET_AT (y, 0, MAY_AT (x, 0));
          MAY_SET_AT (y, 1, z);
          x = y; /* Return the new expression */
        }
        return x;
      }
      /* Read the value */
      y = context->gvalue[context->gtab[j]-1];
      /* TODO: Doesn't work with thread */
      if (((void*)y >= (void*)may_g.Heap.base && (void*)y < (void*)may_g.Heap.limit)
          || y == 0)
        goto not_found_function; /* It is a symbol, not a CB */
      /* Call the registered function */
      cb = (may_t(*)(may_t)) ((void*) y);
      y = (*cb) (z);
      /* If the registered function returns NULL, return the original expression */
      if (y == 0)
        return x;
      /* Check if we have to replace the results once again */
      if (level > 1)
        y = may_subs_recur2 (y, level-1, context);
      return y;
    case MAY_EXP_T ... MAY_UNARYFUNC_LIMIT:
      /* Check if we have already searched for this function
         and fail to find it in the table */
      if ((context->mask & (1ULL << (MAY_TYPE (x) - MAY_EXP_T))) == 0) {
        const char *name = may_get_name (x);
        struct may_s ms;
        may_hash_t hash;
        /* Compute the HASH value associated to 'name' */
        /* FIXME: Precompute the HASH values? */
        hash = may_string_hash (name);
        MAY_OPEN_C  (&ms, MAY_STRING_T);
        MAY_CLOSE_C (&ms, MAY_EVAL_F, hash);
        /* Find the name in the table */
        j = gindex (MAY_HASH (&ms), context);
        while (context->gtab[j] != 0
               && strcmp (name, context->gvars[context->gtab[j]-1]) != 0)
          if (++j >= context->gsize)
            j = 0;
        /* Check if we have found the symbol */
        if (context->gtab[j] != 0) {
          /* Substiture the arguments of the function */
          z = may_subs_recur2 (MAY_AT (x, 0), level, context);
          /* Read the value */
          y = context->gvalue[context->gtab[j]-1];
          if ((void*)y >= (void*)may_g.Heap.base && (void*)y < (void*)may_g.Heap.limit)
            return x; /* It is a symbol, not a CB */
          /* Call the registered function */
          cb = (may_t(*)(may_t)) ((void*) y);
          y = (*cb) (z);
          /* If the registered function returns NULL, return the original expression */
          if (y == 0)
            return x;
          /* Check if we have to replace the results once again */
          if (level > 1)
            y = may_subs_recur2 (y, level-1, context);
          return y;
        } else
          /* Fail to find the function in the table.
             Memorize this result for future calls */
          context->mask |= 1ULL << (MAY_TYPE (x) - MAY_EXP_T);
      }
      /* Fall down to default code */
    default:
      n = MAY_NODE_SIZE(x);
      y = MAY_NODE_C (MAY_TYPE(x), n);
      isnew = 0;
      for (i = 0 ; i < n; i++) {
        may_t zo = MAY_AT (x, i);
        z = may_subs_recur2 (zo, level, context);
        isnew |= (z != zo);
        MAY_SET_AT (y, i, z);
      }
      return isnew ? y : x;
    }
}


may_t
may_subs_c (may_t x, unsigned long level,
            size_t sizevar, const char *const* vars, const void *const*value)
{
  size_t i, j;
  struct may_s ms;
  may_hash_t hash;
  struct may_subs_s context;

  MAY_LOG_FUNC (("x='%Y' level=%lu sizevar=%lu", x, level, (unsigned long) sizevar));

  if (MAY_UNLIKELY (sizevar == 0))
    return x;

  /* Generate a HASH table */
  context.gvars  = (const char **) vars;
  context.gvalue = (void**) value;
  context.gsize  = 3*sizevar;
  context.hash   = (double) context.gsize / (double) MAY_HASH_MAX;
  context.mask   = 0;
  context.gtab   = may_alloc (context.gsize * sizeof *context.gtab);
  memset (context.gtab, 0, context.gsize * sizeof *context.gtab);
  for (i = 0; MAY_LIKELY (i < sizevar); i++) {
    hash = may_string_hash (vars[i]);
    MAY_OPEN_C  (&ms, MAY_STRING_T);
    MAY_CLOSE_C (&ms, MAY_EVAL_F, hash);
    j = gindex (MAY_HASH (&ms), &context);
    while (context.gtab[j] != 0)
      if (++j >= context.gsize)
        j = 0;
    context.gtab[j] = i+1;
  }

  /* Replace the expression using the HASH table */
  x = may_subs_recur2 (x, level, &context);

  return x;
}
