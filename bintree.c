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

/* The color is stored as the least signigicant bit of num.
   Normaly, since 'num' is a pointer to a struct may_s, on most
   architectures, it should be aligned, and though be '0'.
   It isn't 100% portable, but works fine. */
#define GET_NUM(x)   ((may_t) ((uintptr_t)((x)->num) & ~(uintptr_t)1))
#define SET_RED(x)   (((x)->num) = (may_t) ((uintptr_t)((x)->num) | 1))
#define SET_BLACK(x) (((x)->num) = (may_t) ((uintptr_t)((x)->num) & ~(uintptr_t)1))
#define IS_RED(x)    (((uintptr_t)((x)->num) & 1) != 0)
#define IS_BLACK(x)  (((uintptr_t)((x)->num) & 1) == 0)

unsigned long
may_bintree_size (may_bintree_t tree)
{
  if (tree == NULL)
    return 0;
  return 1 + may_bintree_size (tree->child[0])
    + may_bintree_size (tree->child[1]);
}

static void
fill_in (may_t **a, may_bintree_t t)
{
  if (t == NULL) return;
  fill_in (a, t->child[0]);
  may_t num = GET_NUM(t);
  if (may_num_one_p (num)) {
    *((*a)++) = t->key;
  } else if (may_num_zero_p (num)) {
    /* Nothing to do */
  } else {
    /* TODO: Don't use this low level construction.
       Use may_mul: optimize may_mul construction? */
    may_t z = MAY_NODE_C (MAY_FACTOR_T, 2);
    MAY_SET_AT (z, 0, may_num_simplify(num));
    MAY_ASSERT(MAY_PURENUM_P(MAY_AT(z,0)));
    MAY_SET_AT (z, 1, t->key);
    MAY_CLOSE_C (z, MAY_FLAGS (t->key),
                 MAY_NEW_HASH2 (MAY_AT (z, 0), MAY_AT (z, 1)));
    *((*a)++) = z;
  }
  fill_in (a, t->child[1]);
}

may_t
may_bintree_get_sum (may_t num, may_bintree_t tree)
{
  MAY_ASSERT (MAY_PURENUM_P (num) && MAY_EVAL_P (num));

  if (MAY_UNLIKELY (tree == NULL))
    return num;

  int not_zero_p = !MAY_ZERO_P (num);
  unsigned long n = may_bintree_size (tree) + not_zero_p;
  may_t z = MAY_NODE_C (MAY_SUM_T, n);
  may_t *a = MAY_AT_PTR (z, 0);

  if (not_zero_p)
    *a++ = num;
  fill_in (&a, tree);

  /* Due to the potential '0' in the num, fix 'n' */
  n = a - MAY_AT_PTR (z, 0);

  /* TODO: Too low level construction.
     Use may_addinc_c instead ? */
  /* FIXME: hash computation is wrong, no? */
  MAY_NODE_SIZE(z) = n;
  MAY_CLOSE_C (z, MAY_EVAL_F|MAY_EXPAND_F,
               may_node_hash (MAY_AT_PTR (z, 0), n));

  return z;
}

may_bintree_t
may_bintree_insert (may_bintree_t tree, may_t num, may_t key)
{
   /* 64 is enought for 32 bits systems: ln(n+1)-1 <= h <= 2ln(n+1) . */
  may_bintree_t tab[2*CHAR_BIT*sizeof (void*)];
  char which[2*CHAR_BIT*sizeof (void*)];
  int cpt = 0;

  MAY_ASSERT (MAY_PURENUM_P (num));
  MAY_ASSERT (MAY_TYPE (key) != MAY_SUM_T && MAY_TYPE (key) != MAY_FACTOR_T);

  if (MAY_UNLIKELY (tree == NULL)) {
    /* If there was no tree, return a new constructed tree */
    may_bintree_t n = may_alloc (sizeof *n);
    /* num shall be rewritable: create a new one */
    n->num   = may_num_set (MAY_DUMMY, num);
    n->key   = key;
    n->child[0] = n->child[1] = NULL;
    SET_BLACK (n);
    return n;
  }

  /* Search for key in the tree and fill in the traverse table */
  may_bintree_t n = tree;
  int i;
  tab[cpt] = n;

  while (n != NULL && (i = may_identical (n->key, key)) != 0)
    {
      if (i > 0) {
        which[cpt++] = 0;
        n = n->child[0];
      } else {
        which[cpt++] = 1;
        n = n->child[1];
      }
      tab[cpt] = n;
    }

  /* If key was found, add num */
  if (MAY_LIKELY (n != NULL)) {
    may_t nnum = GET_NUM(n);
    n->num = may_num_add (nnum, nnum, num);
    return tree;
  }

  /* New key: add a new node in the bin tree */
  /* Add a new node */
  n = may_alloc (sizeof *n);
  /* Create a copy of the numerical */
  n->num   = may_num_set (MAY_DUMMY, num);
  n->key   = key;
  n->child[0] = n->child[1] = NULL;
  SET_RED (n);
  tab[cpt] = n;

  /* Add it in the tree */
  // assert cpt>1 && fix[cpt-1] == 0 or 1
  tab[cpt-1]->child[(int) which[cpt-1]] = n;

  /* Fix the tree */
  while (cpt>=2
         && IS_RED(tab[cpt-1])
         && tab[cpt-2]->child[1-which[cpt-2]] != NULL
         && IS_RED(tab[cpt-2]->child[1-which[cpt-2]])) {
    SET_BLACK(tab[cpt-1]);
    SET_BLACK(tab[cpt-2]->child[1-which[cpt-2]]);
    SET_RED(tab[cpt-2]);
    cpt-=2;
  }
  /* root is always black */
  SET_BLACK(tab[0]);
  if (cpt <= 1 || IS_BLACK(tab[cpt-1]))
    return tree;

  /* Read the grand-father, the father and the element */
  may_bintree_t pp = tab[cpt-2];
  may_bintree_t p  = tab[cpt-1];
  may_bintree_t x  = tab[cpt];

  /* We need to do some rotations */
  if (which[cpt-2] == 0) {
    /* The father is the left child of the grand-father */
    if (which[cpt-1] == 0) {
      /* The child is the left child of its father */
      /* Right rotation: cpt is the new grand-father.
         x is its left child, the grand-father is the right one */
      pp->child[0] = p->child[1];
      p->child[0] = x;
      p->child[1] = pp;
      SET_BLACK(p);
      SET_RED(pp);
    } else {
      /* The child is the right child of its father */
      /* Left rotation */
      pp->child[0] = x->child[1];
      p->child[1]  = x->child[0];
      x->child[0]  = p;
      x->child[1]  = pp;
      SET_BLACK(x);
      SET_RED(p);
      SET_RED(pp);
      p = x;
    }
  } else {
    /* The father is the right child of the grand-father */
    if (which[cpt-1] == 0) {
      /* The child is the left child of its father */
      pp->child[1] = x->child[0];
      p->child[0]  = x->child[1];
      x->child[1]  = p;
      x->child[0]  = pp;
      SET_BLACK(x);
      SET_RED(p);
      SET_RED(pp);
      p = x;
    } else {
      /* The child is the right child of its father */
      pp->child[1] = p->child[0];
      p->child[1] = x;
      p->child[0] = pp;
      SET_BLACK(p);
      SET_RED(pp);
    }
  }

  /* Insert the new grand father */
  if (MAY_UNLIKELY (cpt == 2))
    tree = p;
  else
    tab[cpt-3]->child[(int) which[cpt-3]] = p;

  return tree;
}
