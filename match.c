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

#include <stdlib.h>
#include "may-impl.h"

/* If p is an identifier, check if p is a wildcard, and return
   its index if it is */
MAY_INLINE int
get_index_of_wild (may_t p)
{
  const char *name;
  char *end;
  int ret;

  MAY_ASSERT (MAY_TYPE (p) == MAY_STRING_T);
  name = MAY_NAME (p);
  if (*name != '$')
    return -1;
  name ++;
  ret = strtol (name, &end, 10);
  return end == name ? -1 : ret;
}

int
may_match_p (may_t *value,
             may_t x, may_t pattern, int size, int (**funcp)(may_t))
{
  int idx;

  MAY_LOG_FUNC (("x='%Y' pattern='%Y'", x, pattern));

  if (!(MAY_TYPE (x) == MAY_TYPE (pattern) || MAY_TYPE (pattern) == MAY_STRING_T)){
    /* We fail except if x==factor and pattern=product where we have to be
       more carreful */
    if (MAY_TYPE (x) == MAY_FACTOR_T && MAY_TYPE (pattern) == MAY_PRODUCT_T) {
      /* The potential wildcard in the product may be the numerical part! */
      may_size_t i, m, n = MAY_NODE_SIZE(pattern);
      may_t newp, num = MAY_AT (x, 0), base = MAY_AT (x, 1);
      int ret, allready_wildcard;
      MAY_RECORD ();
      newp = MAY_NODE_C (MAY_PRODUCT_T, n-1);
      for (i = m = allready_wildcard = 0; MAY_LIKELY (i < n); i++) {
        may_t  z = MAY_AT (pattern, i);
        int ind;
        if (MAY_TYPE (z) == MAY_STRING_T && (ind = get_index_of_wild (z)) >= 0) {
          if (allready_wildcard
              || ind >= size
              || (value[ind] != NULL && may_identical (num, value[ind]) != 0)
              || (funcp != NULL  && !(funcp[ind])(num)))
            MAY_RET_CTYPE (0);
          value[ind] = num, allready_wildcard = 1;
        } else if (m == n-1)
          MAY_RET_CTYPE (0); /* No wildcard: exit */
        else
          MAY_SET_AT (newp, m++, z);
      }
      newp = may_eval (newp);
      ret =  may_match_p (value, base, newp, size, funcp);
      MAY_CLEANUP ();
      return ret;
    }
    return 0; /* Fail */
  } else if (MAY_TYPE (pattern) == MAY_STRING_T
             && (idx = get_index_of_wild (pattern)) >= 0) {
    if (idx >= size)
      return 0; /* Fail due to index too big */
    else if (value[idx] != NULL)
      return may_identical (x, value[idx]) == 0;
    else if (funcp == NULL || (funcp[idx])(x))
      return value[idx] = x, 1;
    else
      return 0;
  } else if (MAY_TYPE (x) != MAY_SUM_T && MAY_TYPE (x) != MAY_PRODUCT_T) {
    may_size_t i, n;
    if (MAY_ATOMIC_P (x))
      return may_identical (x, pattern) == 0;
    else if ((n = MAY_NODE_SIZE(x)) != MAY_NODE_SIZE (pattern))
      return 0;
    for (i = 0; MAY_LIKELY (i<n); i++)
      if (!may_match_p (value, MAY_AT (x, i), MAY_AT (pattern, i), size, funcp))
        return 0;
    return 1;
  } else {
    may_size_t n = MAY_NODE_SIZE(x);
    may_size_t m = MAY_NODE_SIZE(pattern);

    if (m > n+1)
      return 0; /* Pattern too big to be fit by x */
    char child_allocated[n];
    memset (child_allocated, 0, sizeof child_allocated);

    /* Pass 1: scan all except widlcards*/
    /* Complexity N^2 since we can't sort pattern in a useful way! */
    for (may_size_t j = 0; MAY_LIKELY (j < m); j++) {
      may_t sub_pattern = MAY_AT (pattern, j);
      /* If it is a wildcard, continue */
      if (MAY_TYPE (sub_pattern) == MAY_STRING_T
          && MAY_NAME (sub_pattern)[0] == '$')
        continue;
      int ok = 0;
      for (may_size_t i = 0; MAY_LIKELY (i < n); i++)
        if (!child_allocated[i]
            && may_match_p (value, MAY_AT (x, i), sub_pattern, size, funcp)) {
          child_allocated[i] = 1;
          ok = 1;
          break;
        }
      if (!ok)
        return 0; /* Pattern can't be matched: fail */
    }

    /* Pass 2: Check wildcards */
    /* Only one unaffected wildcard may remain */
    may_t wildcard = NULL;
    for (may_size_t j = 0; MAY_LIKELY (j < m); j++) {
      may_t sub_pattern = MAY_AT (pattern, j);
      /* If it is a wildcard */
      if (MAY_TYPE (sub_pattern) == MAY_STRING_T
          && MAY_NAME (sub_pattern)[0] == '$') {
        int idx = get_index_of_wild (sub_pattern);
        /* Check if its value has been choosen */
        if (value[idx] != NULL) {
          /* Compare with the current expression */
          if (MAY_TYPE (value[idx]) == MAY_TYPE (pattern)) {
            // TODO
            MAY_ASSERT(0);
          } else {
            int ok = 0;
            /* Look for value[idx] in a unaffected child of x */
            for(may_size_t i = 0; i < n; i++)
              if (!child_allocated[i]
                  && may_identical (MAY_AT (x, i), value[idx]) == 0) {
                ok = 1;
                child_allocated[i] = 1;
                break;
              }
            if (!ok)
              return 0;
          }
        } else if (wildcard == NULL) {
          wildcard = sub_pattern; /* Select it as unaffected */
        } else
          return 0; /* Error: multiple unaffected wildcards in expression */
      } /* else not a wildcard */
    } /* end for */

    /* Check if there remains something if there is no wildcard */
    if (wildcard == NULL) {
      for (may_size_t i = 0; MAY_LIKELY (i<n); i++)
        if (!child_allocated[i])
          return 0;
      return 1;
    }

    /* Assign to the unaffected wildcard what remains */
    may_t sumtab[n];
     m = 0;
    idx = get_index_of_wild (wildcard);
    if (idx < 0 || idx >= size)
      return 0;
    for (may_size_t i = 0; MAY_LIKELY (i<n); i++)
      if (!child_allocated[i])
        sumtab[m++] = MAY_AT (x, i);
    sumtab[0] = may_eval ((MAY_TYPE (pattern) == MAY_SUM_T ?
                           may_add_vc : may_mul_vc)(m, sumtab));

    /* Check value & return */
    if (value[idx] != NULL) {
      return may_identical (sumtab[0], value[idx]) == 0;
    } else if (funcp == NULL || (funcp[idx])(sumtab[0])) {
      value[idx] = sumtab[0];
      return 1;
    } else {
      return 0;
    }
  }
}
