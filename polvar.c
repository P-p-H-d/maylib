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

/* Add the identifiers found in x view as a polynomial to list
   with their maximum estimated exponent */
static void
add_polvar (may_list_t list, may_t x, may_t expo)
{
  unsigned long i, n;

  MAY_ASSERT (MAY_TYPE (expo) == MAY_INT_T);

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
  case MAY_RAT_T:
  case MAY_REAL_T:
  case MAY_COMPLEX_T:
    break;
  case MAY_FACTOR_T:
    add_polvar (list, MAY_AT (x, 1), expo);
    break;
  case MAY_SUM_T:
  case MAY_PRODUCT_T:
    n = MAY_NODE_SIZE(x);
    MAY_ASSERT (n >= 2);
    for (i = 0; i < n; i++)
      add_polvar (list, MAY_AT (x, i), expo);
    break;
  case MAY_POW_T:
    if (MAY_LIKELY (MAY_TYPE (MAY_AT (x, 1)) == MAY_INT_T
                    && mpz_sgn (MAY_INT (MAY_AT (x, 1))) > 0)) {
      expo = may_mul (expo, MAY_AT (x, 1));
      add_polvar (list, MAY_AT (x, 0), expo);
      break;
    } /* else fall down to default */
    /* Falls through. */
  default:
    /* Check if x is already in list */
    n = may_list_get_size (list);
    MAY_ASSERT ((n%2) == 0);
    for (i = 0; i < n; i+=2)
      if (may_identical (x, may_list_at (list, i)) == 0) {
        if (may_num_cmp (may_list_at (list, i+1), expo) < 0)
          may_list_set_at (list, i+1, expo);
        return ;
      }
    /* Add x in the list */
    may_list_push_back (list, x);
    may_list_push_back (list, expo);
    break;
  }
}


/* Add in 'list' the identifiers found in x view as a polynomial
   iff there are found in 'org' too (with their maximal extimated exponent) */
static void
remove_polvar (may_list_t list, may_list_t org, may_t x, may_t expo)
{
  unsigned long i, n;

  MAY_ASSERT (MAY_TYPE (expo) == MAY_INT_T);

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
  case MAY_RAT_T:
  case MAY_REAL_T:
  case MAY_COMPLEX_T:
    break;
  case MAY_FACTOR_T:
    remove_polvar (list, org, MAY_AT (x, 1), expo);
    break;
  case MAY_SUM_T:
  case MAY_PRODUCT_T:
    n = MAY_NODE_SIZE(x);
    MAY_ASSERT (n >= 2);
    for (i = 0; i < n; i++)
      remove_polvar (list, org, MAY_AT (x, i), expo);
    break;
  case MAY_POW_T:
    if (MAY_LIKELY (MAY_TYPE (MAY_AT (x, 1)) == MAY_INT_T
                    && mpz_sgn (MAY_INT (MAY_AT (x, 1))) > 0)) {
      expo = may_mul (expo, MAY_AT (x, 1));
      remove_polvar (list, org, MAY_AT (x, 0), expo);
      break;
    } /* else fall down to default */
    /* Falls through. */
  default:
    /* Add x in list if it is found in org */
    n = may_list_get_size (org);
    MAY_ASSERT ((n%2) == 0);
    for (i = 0; i < n; i+=2)
      if (may_identical (x, may_list_at (org, i)) == 0) {
        /* Keep the max between expo and previous one */
        if (may_num_cmp (may_list_at (org,i+1), expo) > 0)
          expo = may_list_at (org,i+1);
        /* Check if x is already in list */
        n = may_list_get_size (list);
        MAY_ASSERT ((n%2) == 0);
        for (i = 0; i < n; i+=2)
          if (may_identical (x, may_list_at (list, i)) == 0) {
            if (may_num_cmp (may_list_at (list, i+1), expo) < 0)
              may_list_set_at (list, i+1, expo);
            return ;
          }
        /* Add x in list */
        may_list_push_back (list, x);
        may_list_push_back (list, expo);
        break;
      }
    break;
  }
}

/* Find the commun var with the minimal estimated exponent
   from all items of tab */
may_t
may_find_one_polvar (unsigned long n, const may_t tab[])
{
  may_list_t list, list2;
  may_t x, min;
  unsigned long i;

  MAY_ASSERT (n >= 1);

  MAY_LOG_FUNC (("n=%lu, tab[0]='%Y'", n, tab[0]));

  /* First pass, detect if there is a purenumerical.
     If so, we can't find a commun var. */
  for (i = 0; i < n; i ++)
    if (MAY_UNLIKELY (MAY_PURENUM_P (tab[i])))
      return NULL;

  MAY_RECORD ();
  may_list_init (list, 0);

  /* Second pass: search for all commun vars */
  add_polvar (list, tab[0], MAY_ONE);
  may_list_init (list2, may_list_get_size (list));
  for (i = 1; i < n && may_list_get_size (list) > 0; i++) {
    may_list_resize (list2, 0);
    remove_polvar (list2, list, tab[i], MAY_ONE);
    may_list_swap (list, list2);
  }

  /* If no comun var was found, return NULL */
  n = may_list_get_size (list);
  if (MAY_UNLIKELY (n == 0))
    MAY_RET (NULL);

  /* Find the common var with the minimal exponent */
  x   = may_list_at (list, 0);
  min = may_list_at (list, 1);
  for (i = 0; i < n; i+=2) {
    may_t y = may_list_at (list, i);
    if (MAY_UNLIKELY (MAY_TYPE (y) == MAY_STRING_T
                      && (MAY_TYPE (x) != MAY_STRING_T
                          || may_num_cmp (may_list_at (list, i+1), min) < 0))){
      x   = y;
      min = may_list_at (list, i+1);
    }
  }

  MAY_LOG_VAR(x);

  MAY_RET (x);
}

/***************************************************************************/

/* Return the list of the variables which are, at least, not used
   by one element of the tab. */
may_t
may_find_unused_polvar (unsigned long n, const may_t tab[])
{
  may_list_t used, common, unused;
  unsigned long i, j, nused, ncommon;

  MAY_ASSERT (n >= 1);

  MAY_LOG_FUNC (("n=%d", (int)n));

  MAY_RECORD ();
  may_list_init (used, 0);

  /* Search for all used and commun vars */
  add_polvar (used, tab[0], MAY_ONE);
  may_list_init (common, may_list_get_size (used));
  may_list_init (unused, may_list_get_size (used));
  may_list_set (common, used);

  for (i = 1; i < n; i++) {
    add_polvar (used, tab[i], MAY_ONE);
    if (may_list_get_size (common) > 0) {
      may_list_resize (unused, 0);
      remove_polvar (unused, common, tab[i], MAY_ONE);
      may_list_set (common, unused);
    }
  }

  /* Save in 'unused' all variables present in 'used' but not in 'commun' */
  nused   = may_list_get_size (used);
  ncommon = may_list_get_size (common);
  may_list_resize (unused, 0);
  for (i = 0; i < nused; i+=2) {
    may_t t = may_list_at (used, i);
    for (j = 0; j < ncommon; j+=2)
      if (may_identical (t, may_list_at (common, j)) == 0)
        break;
    if (j == ncommon)
      may_list_push_back_single (unused, t);
  }
  if (MAY_UNLIKELY (may_list_get_size (unused) == 0))
    MAY_RET (NULL);
  else
    MAY_RET_EVAL (may_list_quit (unused));
}

/***************************************************************************/

/* Add the identifiers found in x view as a rational function to list */
static void
add_ratvar (may_list_t list, may_t x, may_indets_e flags)
{
  unsigned long i, n;

  if ((flags & MAY_INDETS_NUM) == 0 && MAY_NUM_P (x))
    return;

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
  case MAY_RAT_T:
  case MAY_REAL_T:
  case MAY_COMPLEX_T:
    break;
  case MAY_FACTOR_T:
    add_ratvar (list, MAY_AT (x, 1), flags);
    break;
  case MAY_SUM_T:
  case MAY_PRODUCT_T:
    n = MAY_NODE_SIZE(x);
    MAY_ASSERT (n >= 2);
    for (i = 0; i < n; i++)
      add_ratvar (list, MAY_AT (x, i), flags);
    break;
  case MAY_FUNC_T:
    may_list_push_back_single (list, x);
    if ((flags & MAY_INDETS_RECUR) != 0)
      add_ratvar (list, MAY_AT (x, 1), flags);
    break;
  case MAY_POW_T:
    if (MAY_LIKELY (MAY_TYPE (MAY_AT (x, 1)) == MAY_INT_T)) {
      add_ratvar (list, MAY_AT (x, 0), flags);
      break;
    } /* else fall down to default */
    /* Falls through. */
  default:
    may_list_push_back_single (list, x);
    if (MAY_UNLIKELY ((flags & MAY_INDETS_RECUR) != 0 && MAY_NODE_P (x))) {
      n = MAY_NODE_SIZE(x);
      for (i = 0; i < n; i++)
        add_ratvar (list, MAY_AT (x, i), flags);
    }
    break;
  }
}

static int
cmp_ratvar (const may_t *a, const may_t *b) {
  return MAY_TYPE (*a) - MAY_TYPE (*b);
}

may_t
may_indets (may_t x, may_indets_e flags)
{
  may_list_t list;

  MAY_LOG_FUNC (("x=%Y flags=%d", x, (int)flags));

  may_mark();

  may_list_init (list, 0);
  add_ratvar (list, x, flags);
  may_list_sort (list, cmp_ratvar);
  may_t y = may_list_quit (list);

  return may_keep (may_eval (y));
}
