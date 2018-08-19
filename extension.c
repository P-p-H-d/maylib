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

static const may_extdef_t empty_class = { .name = "" };

may_ext_t
may_ext_register (const may_extdef_t *extdef, may_extreg_e way)
{
  int i;
  int index;

  /* Refuse to install an extension which doesn't have a name */
  if (extdef->name == NULL)
    return 0;
  /* If the priority is invalid, refuse to register the extension */
  if (extdef->priority < 1 || extdef->priority > 1000)
    return 0;

  /* Check for update of extension */
  index = may_c.extension_size;
  for (i = 0; i<may_c.extension_size; i++) {
    if (strcmp (extdef->name, may_c.extension_tab[i]->name) == 0) {
      if (way == MAY_EXT_INSTALL)
	/* The extension has been already installed.
	   Refuse to install it again */
	return 0;
      may_c.extension_tab[i] = extdef;
      return MAY_INDEX2EXT (i);
    }
    /* If it is the empty class, reuse its index */
    if (may_c.extension_tab[i] == &empty_class)
      index = i;
  }

  /* Check if we can add one */
  if (index >= MAY_MAX_EXTENSION || way == MAY_EXT_UPDATE)
    return 0;

  /* Add new one */
  may_c.extension_tab[index] = extdef;
  /* Increment the array size if we have added it at the end */
  may_c.extension_size += (may_c.extension_size == index);
  return MAY_INDEX2EXT (index);
}

int
may_ext_unregister (const char name[])
{
  /* First extension is the LIST: It can't be unregistered */
  for (int i = 1; i < may_c.extension_size; i++)
    if (strcmp (may_c.extension_tab[i]->name, name) == 0) {
      /* Store an empty class instead */
      may_c.extension_tab[i] = &empty_class;
      /* Compact the table if possible */
      while (may_c.extension_size > 0
             && may_c.extension_tab[may_c.extension_size-1] == &empty_class)
        may_c.extension_size--;
      /* Success */
      return 1;
    }
  /* Failure */
  return 0;
}

may_ext_t
may_ext_find (const char name[])
{
  for (int i = 0; i < may_c.extension_size; i++)
    if (strcmp (may_c.extension_tab[i]->name, name) == 0)
      return MAY_INDEX2EXT(i);
  return 0;
}

may_ext_t
may_ext_p (may_t x)
{
  return MAY_EXT_P (x) ? MAY_TYPE (x) : 0;
}

const may_extdef_t *
may_ext_get (may_ext_t e)
{
  MAY_ASSERT (MAY_EXT_TYPE_P (e));
  return may_c.extension_tab[MAY_EXT2INDEX(e)];
}

may_t
may_ext_c (may_ext_t ext, size_t n)
{
  MAY_ASSERT (MAY_EXT_TYPE_P (ext));
  MAY_ASSERT (n > 0);
  return MAY_NODE_C (ext, n);
}

void
may_ext_set_c (may_t x, size_t i, may_t y)
{
  MAY_ASSERT (MAY_EXT_P (x));
  MAY_ASSERT (!MAY_EVAL_P (x));
  MAY_ASSERT (i < MAY_NODE_SIZE(x));
  MAY_SET_AT (x, i, y);
}


/* Non exported functions */
MAY_REGPARM may_t
may_eval_extension (may_t a)
{
  may_t (*eval) (may_t);
  MAY_ASSERT (MAY_EXT_P (a));
  eval = MAY_EXT_GETX (a)->eval;
  if (MAY_UNLIKELY (eval != 0))
    a = (*eval) (a);
  a = may_hold (a);
  return a;
}

/* Sort by priority then by extension number */
static int
extension_cmp2 (const void *a, const void *b)
{
  may_t ma, mb;
  int r;

  ma = (*(const may_pair_t*)a).second;
  mb = (*(const may_pair_t*)b).second;

  if (MAY_TYPE (ma) == MAY_TYPE (mb))
    return 0;

  if (!MAY_EXT_P (ma)) {
    if (!MAY_EXT_P (mb)) {
      return MAY_TYPE (ma) - MAY_TYPE (mb);
    } else
      return -1;
  } else if (!MAY_EXT_P (mb)) {
    return 1;
  }
  MAY_ASSERT (MAY_EXT_P (ma) && MAY_EXT_P (mb));

  r = MAY_EXT_GETX (ma)->priority
    - MAY_EXT_GETX (mb)->priority;
  if (MAY_UNLIKELY (r == 0))
    r = MAY_TYPE (ma) - MAY_TYPE (mb);
  return r;
}

void
may_eval_extension_sum (may_t num,
                        may_size_t *nsymb, may_pair_t tab[])
{
  may_size_t start, end, n;
  may_type_t e;

  n = *nsymb;

  /* Move the numerical term to the position n and let the sort move it again */
  if (num != MAY_ZERO) {
    /* tab is assumed to be at least of size (*nsymb+1) */
    tab[n].first  = MAY_ONE;
    tab[n].second = num;
    n++;
  }

  /* Sort by priority of the extension */
  qsort (&tab[0], n, sizeof *tab, extension_cmp2);

  /* Search for the first item which is an extension */
  start = 0;
  while (!MAY_EXT_P (tab[start].second)) {
    start++;
    MAY_ASSERT (start < n);
  }

  /* Compute the extensions */
  while (start < n) {
    /* Search for the greatest interval of the same extension. */
    e = MAY_TYPE (tab[start].second);
    MAY_ASSERT (MAY_EXT_P (tab[start].second));
    for (end = start+1; end < n && e == MAY_TYPE (tab[end].second); end++) ;
    /* Get the overloaded extension sum */
    unsigned long (*sum) (unsigned long, unsigned long,  may_pair_t*);
    sum = may_ext_get (e)->add;
    if (sum != 0) {
      /* The extension has overloaded the sum operator. Call it */
      may_size_t end2 = (*sum) (start, end, tab);
      MAY_ASSERT (end2 > 0);
      /* If there are less terms than before */
      if (end2 != end) {
	MAY_ASSERT (end2 < end);
	/* Compact the table from [end to n(excluded)] into end2. */
	memmove (&tab[end2], &tab[end], (n-end)*sizeof tab[0]);
        /* Reduce number of elements in the table */
	n -= (end-end2);
	end = end2;
      }
    }
    /* Next iteration */
    start = end;
    MAY_ASSERT (start <= n);
  }

  /* Return the new number of symbols in the table */
  *nsymb = n;
  return;
}

void
may_eval_extension_product (may_t num,
                            may_size_t *nsymb, may_pair_t tab[],
                            may_size_t org_nsymb, may_pair_t org_tab[])
{
  may_size_t start, end, n;
  may_type_t e;
  int is_non_commutative_ext = 0;

  n = *nsymb;
  MAY_ASSERT (n <= org_nsymb);

  /* Merge both tabs while keeping in tab, the extensions which are commutative,
     and in org_tab, thoses which aren't */
  for (start = end = 0; start < n ; start++) {
    if (!MAY_EXT_P (tab[start].second)
        || MAY_EXT_GETX (tab[start].second)->flags == 0) {
      tab[end++] = tab[start];
    } else {
      is_non_commutative_ext = 1;
    }
  }
  MAY_ASSERT (end <= n);
  if (is_non_commutative_ext) {
    MAY_ASSERT (end < n);
    for (start = 0; start < org_nsymb; start++) {
      if (MAY_EXT_P (org_tab[start].second)
          && MAY_EXT_GETX (org_tab[start].second)->flags != 0)
        tab[end++] = org_tab[start];
    }
  }
  n = end;
  MAY_ASSERT (n <= org_nsymb);

  /* Move the numerical term to the position n and let the sort move it again */
  if (num != MAY_ONE) {
    /* tab is assumed to be at least of size (*nsymb+1) */
    tab[n].first  = MAY_ONE;
    tab[n].second = num;
    n++;
  }

  /* Sort by priority of the extension */
  qsort (&tab[0], n, sizeof *tab, extension_cmp2);

  /* Search for the first item which is an extension */
  start = 0;
  while (!MAY_EXT_P (tab[start].second)) {
    start++;
    MAY_ASSERT (start < n);
  }

  /* Compute the extensions */
  while (start < n) {
    /* Search for the greatest interval of the same extension. */
    e = MAY_TYPE (tab[start].second);
    MAY_ASSERT (MAY_EXT_P (tab[start].second));
    for (end = start+1; end < n && e == MAY_TYPE (tab[end].second); end++) ;
    /* Get the overloaded extension sum */
    unsigned long (*prod) (unsigned long, unsigned long,  may_pair_t*);
    prod = may_ext_get (e)->mul;
    if (prod != 0) {
      /* The extension has overloaded the sum operator. Call it */
      may_size_t end2 = (*prod) (start, end, tab);
      MAY_ASSERT (end2 > 0);
      /* If there are less terms than before */
      if (end2 != end) {
	MAY_ASSERT (end2 < end);
	/* Compact the table from [end to n(excluded)] into end2. */
	memmove (&tab[end2], &tab[end], (n-end)*sizeof tab[0]);
        /* Reduce number of elements in the table */
	n -= (end-end2);
	end = end2;
      }
    }
    /* Next iteration */
    start = end;
    MAY_ASSERT (start <= n);
  }

  /* Return the new number of symbols in the table */
  *nsymb = n;
  return;
}

may_t
may_eval_extension_pow (may_t base, may_t power)
{
  int ext_base, ext_power;
  may_t (*pow) (may_t, may_t);
  may_t y;

  /* Check which callback to use */
  ext_base  = MAY_EXT_P (base);
  ext_power = MAY_EXT_P (power);
  if (ext_base && ext_power) {
    if (MAY_EXT_GETX (base)->priority >= MAY_EXT_GETX (power)->priority)
      pow = MAY_EXT_GETX (base)->pow;
    else
      pow = MAY_EXT_GETX (power)->pow;
  } else if (ext_base) {
    MAY_ASSERT (!ext_power);
    pow = MAY_EXT_GETX (base)->pow;
  } else {
    MAY_ASSERT (ext_power);
    pow = MAY_EXT_GETX (power)->pow;
  }

  /* Compute the return value */
  if (pow != 0)
    y = (*pow) (base, power);
  else
    y = may_hold (may_pow_c (base, power));

  MAY_ASSERT (MAY_EVAL_P (y));
  return y;
}

may_t
may_parse_extension (const char *name, may_t list)
{
  may_ext_t e;
  e = may_ext_find (name);
  if (MAY_LIKELY (e == 0))
    return NULL;
  may_t (*constructor) (may_t);
  constructor = may_ext_get (e)->constructor;
  if (constructor == NULL)
    return NULL;
  may_t y = (*constructor) (list);
  MAY_ASSERT (!MAY_EVAL_P (y));
  MAY_ASSERT (may_ext_p (y) == e);
  return y;
}
