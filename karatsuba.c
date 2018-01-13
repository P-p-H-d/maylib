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

/* Karatsuba multiplication for multivariate and univariate polynomials */

/* dump */
#if 0
static void
dump (may_linked_term_t a, int n)
{
  for ( ;a!= NULL; a = a->next) {
    printf ("[");
    for (int i = 0; i < n ;i ++)
      printf ("%lu%c", (unsigned long) a->expo[i], (i==(n-1)) ? ']' : ',');
    may_dump (a->coeff);
  }
}
#endif

/* Allocate a new uninitialized linked term */
MAY_INLINE may_linked_term_t
new_term (int n)
{
  may_linked_term_t t;
  if (may_g.kara.freeterm == NULL) {
    t = MAY_ALLOC(sizeof (struct may_linked_term_s) + n*sizeof (may_karatsuba_unsigned_long));
    t->coeff = MAY_DUMMY;
  } else {
    t = may_g.kara.freeterm;
    may_g.kara.freeterm = may_g.kara.freeterm->next;
  }
  MAY_ASSERT ((t->coeff == MAY_DUMMY) || !MAY_EVAL_P (t->coeff));
  return t;
}

/* Free and next */
MAY_INLINE may_linked_term_t
free_and_next (may_linked_term_t a)
{
  may_linked_term_t b = a->next;
  a->next = may_g.kara.freeterm;
  may_g.kara.freeterm = a;
  return b;
}

/* Compare two linked_term_t */
MAY_INLINE int
compare (may_linked_term_t a, may_linked_term_t b, int n)
{
  MAY_ASSERT (n > 0);
  for (int i = 0; i < n; i ++)
    if (a->expo[i] != b->expo[i])
      return a->expo[i] < b->expo[i] ? -1 : 1;
  return 0;
}

/* Sort the linked list.
   Use a merge sort */
static may_linked_term_t
sort (may_linked_term_t a, int n)
{
  may_linked_term_t b, c, d;

  /* If there is no element, one element */
  if (a == NULL || a->next == NULL)
    return a;
  /* If there is 2 elements */
  if (a->next->next == NULL) {
    if (compare (a, a->next, n) >= 0)
      return a;
    b = a->next;
    b->next = a;
    a->next = NULL;
    return b;
  }
  /* Split */
  b = c = NULL;
  while (a != NULL) {
    d = a->next;
    a->next = b;
    b = a;
    if (d == NULL)
      break;
    a = d->next;
    d->next = c;
    c = d;
  }
  /* Sort recursive */
  b = sort (b, n);
  c = sort (c, n);
  /* Merge */
  may_linked_term_t *ap = &a;
  while (b != NULL && c != NULL) {
    if (compare (b, c, n) >= 0) {
      *ap = b;
      ap  = &(b->next);
      b = b->next;
    } else {
      *ap = c;
      ap  = &(c->next);
      c = c->next;
    }
  }
  *ap = (b != NULL) ? b : c;
  return a;
}

/* Convertion of an expression composed of n variables varlist[n]
   to a linked list of term.
   Return NULL if the convertion failed */
static may_linked_term_t
convert_to_linked_term (may_t x, int n, may_t varlist[n])
{
  MAY_ASSERT ((MAY_FLAGS(x) & MAY_EXPAND_F) == MAY_EXPAND_F);
  may_linked_term_t list = NULL;

  may_iterator_t it;
  may_t num, cx, bx;
  for (num = may_sum_iterator_init (it, x) ;
       may_sum_iterator_end (&cx, &bx, it) ;
       may_sum_iterator_next (it)) {
    may_iterator_t it2;
    may_t bx2, pw2;
    may_linked_term_t tmp = new_term (n);
    tmp->coeff = may_num_set (MAY_DUMMY, cx);
    for (int i = 0; i < n; i++)
      tmp->expo[i] = 0;
    for (may_product_iterator_init (it2, bx) ;
         may_product_iterator_end (&pw2, &bx2, it2) ;
         may_product_iterator_next (it2)) {
      unsigned long p;
      /* If we can't convert the exponent into an integer
         between 0 and ULONG_MAX/2, abort karatsuba */
      if (may_get_ui (&p, pw2) && p < (((may_karatsuba_unsigned_long)-1)/2) )
        return NULL;
      for (int i = 0; i < n; i++) {
        if (may_identical (varlist[i], bx2) == 0) {
          tmp->expo[i] = p;
          goto convertion_done;
        }
      } /* for i */
      /* Fail to find a correspondance between the factor and the list
         Can not convert */
      return NULL;
    convertion_done:
      (void) 0;
    } /* for product iterator */
    /* Add the new term into the list */
    tmp->next = list;
    list = tmp;
  } /* for sum iterator */
  /* The numerical constant term */
  if (!MAY_ZERO_P (num)) {
    may_linked_term_t tmp = new_term (n);
    tmp->coeff = may_num_set (MAY_DUMMY, num);
    for (int i = 0; i < n; i++)
      tmp->expo[i] = 0;
    tmp->next = list;
    list = tmp;
  }

  /* Sort list */
  list = sort (list, n);

  return list;
}

/* Convertion from an expression composed of n variables varlist[n]
   to a linked list of term */
static may_t
convert_from_linked_term (may_linked_term_t term, int n, may_t varlist[n])
{
  may_t s = MAY_ZERO;
  for (; term != NULL; term = term->next) {
    if (may_num_zero_p (term->coeff))
      continue;
    may_mark ();
    may_t t = MAY_NODE_C (MAY_PRODUCT_T, n+1);
    for (int i = 0; i < n; i++)
      MAY_SET_AT (t, i, may_pow_c (varlist[i], may_set_ui (term->expo[i])));
    MAY_SET_AT (t, n, term->coeff);
    t = may_compact (may_eval (t));
    MAY_SET_FLAG (t, MAY_EXPAND_F);
    s = may_addinc_c (s, t);
  }
  s = may_eval (s);
  MAY_SET_FLAG (s, MAY_EXPAND_F);
  return s;
}

/* Split a linked list of term in two (reusing the memory).
   i is the index of the variable to split (in 0..n-1)
   It will separate according to the least significant bit of the exponent,
   so that A[X_i^2]*X_i+B(X_i^2) = org(X_i) */
static void
split (may_linked_term_t *a, may_linked_term_t *b,
              may_linked_term_t org, int i)
{
  may_linked_term_t *ap = a;
  may_linked_term_t *bp = b;
  unsigned int as = 0, bs = 0;

  MAY_ASSERT (org != NULL);

  while (org != NULL) {
    /* Check the least significant bit */
    if ((org->expo[i] & 1) == 0) {
      /* Add it in B keeping the order */
      *bp = org;
      bp = &(org->next);
      bs++;
    } else {
      /* Add it in A keeping the order */
      *ap = org;
      ap = &(org->next);
      as++;
    }
    /* Reduce the exponent */
    org->expo[i] = (org->expo[i] / 2);
    /* Next */
    org = org->next;
  }
  /* Terminate the lists */
  *ap = NULL;
  *bp = NULL;

  /* Let's see if the split was a success or not.
     If not, let's increase the threshold so that we will use the base case method
     for such sizes */
  if (!(as <= 2*bs && bs <= 2*as)) {
    if (!(as <= 3*bs && bs <= 3*as))
      may_g.kara.threshold = 3*(as+bs+1);
    else if (!(as <= 4*bs && bs <= 4*as))
      may_g.kara.threshold = 3*(as+bs+1);
    else
      may_g.kara.threshold = as+bs+1;
  }

}

/* Merge two list so that the returned multivariate poly is
   A[X_i^2]*X_i^2+B[X_i^2]*X_i+C(X_i^2) (Reusing memory)
   A & B may overlap. C doesn't overlap with B and C */
static may_linked_term_t
merge (may_linked_term_t a, may_linked_term_t b, may_linked_term_t c, int i, int n)
{
  may_linked_term_t *sp;

  /* Fix the degree of A for X_i */
  may_linked_term_t tmp = a;
  while (tmp != NULL) {
    tmp->expo[i] ++;
    tmp = tmp->next;
  }
  /* Fix the degree of B for X_i */
  tmp = b;
  while (tmp != NULL) {
    tmp->expo[i] = 2 * tmp->expo[i] + 1;
    tmp = tmp->next;
  }

  /* First sum A and C and fix the degree for X_i */
  sp = &tmp;
  while (a != NULL && c != NULL) {
    for (int j = 0; j < n ; j++) {
      if (a->expo[j] > c->expo[j]) {
        /* A is bigger than C */
        *sp = a;
        a->expo[i] = 2*a->expo[i];
        sp = &(a->next);
        a = a->next;
        goto next_term;
      }else if (a->expo[j] < c->expo[j]) {
        /* C is bigger than A */
        *sp = c;
        c->expo[i] = 2*c->expo[i];
        sp = &(c->next);
        c = c->next;
        goto next_term;
      }
    }
    /* Both exponents are equals: sum coeff */
    MAY_ASSERT (!MAY_EVAL_P (a->coeff) && !MAY_EVAL_P (c->coeff));
    *sp = a;
    a->expo[i] = 2*a->expo[i];
    sp = &(a->next);
    a->coeff = may_num_add (a->coeff, a->coeff, c->coeff);
    a = a->next;
    c = free_and_next (c);
  next_term:
    (void) 0;
  } /* while a and c != NULL */
  while (a != NULL) {
    *sp = a;
    a->expo[i] = 2*a->expo[i];
    sp = &(a->next);
    a = a->next;
  }
  while (c != NULL) {
    *sp = c;
    c->expo[i] = 2*c->expo[i];
    sp = &(c->next);
    c = c->next;
  }
  *sp = NULL;

  /* Merge tmp and b */
  a = tmp;
  sp = &tmp;
  while (a != NULL && b != NULL) {
    for (int j = 0; j < n ; j++) {
      if (a->expo[j] > b->expo[j]) {
        *sp = a;
        sp = &(a->next);
        a = a->next;
        break;
      }else if (a->expo[j] < b->expo[j]) {
        *sp = b;
        sp = &(b->next);
        b = b->next;
        break;
      }
    }
    /* Both exponents are equals. This case is not possible */
  } /* while (a!= NULL && b != NULL) */
  *sp = (a!=NULL) ? a : b;

  return tmp;
}

/* Sum two linked list of n variables
   Allocating a new area of memory
 */
static may_linked_term_t
add (may_linked_term_t a, may_linked_term_t b, int n)
{
  may_linked_term_t *sp;
  may_linked_term_t tmp, ret;
  int i;

  sp = &ret;
  while (a != NULL && b != NULL) {
    tmp = new_term (n);
    *sp = tmp;
    sp = &(tmp->next);
    MAY_ASSERT (n > 0);
    for (i = 0; i < n ; i++) {
      if (a->expo[i] > b->expo[i]) {
        tmp->coeff = may_num_set (tmp->coeff, a->coeff);
        memcpy (tmp->expo, a->expo, n*sizeof (may_karatsuba_unsigned_long));
        a = a->next;
        goto next_term;
      }else if (a->expo[i] < b->expo[i]) {
        tmp->coeff = may_num_set (tmp->coeff, b->coeff);
        memcpy (tmp->expo, b->expo, n*sizeof (may_karatsuba_unsigned_long));
        b = b->next;
        goto next_term;
      }
    }
    /* Both exponents are equals */
    memcpy (tmp->expo, a->expo, n*sizeof (may_karatsuba_unsigned_long));
    tmp->coeff = may_num_add (tmp->coeff, a->coeff, b->coeff);
    a = a->next;
    b = b->next;
  next_term:
    (void) 0;
  }
  /* Add the latest terms */
  for ( ; a!=NULL ; a = a->next) {
    tmp = new_term (n);
    tmp->coeff = may_num_set (tmp->coeff, a->coeff);
    memcpy (tmp->expo, a->expo, n*sizeof (may_karatsuba_unsigned_long));
    *sp = tmp;
    sp = &(tmp->next);
  }
  for ( ; b!=NULL ; b = b->next) {
    tmp = new_term (n);
    tmp->coeff = may_num_set (tmp->coeff, b->coeff);
    memcpy (tmp->expo, b->expo, n*sizeof (may_karatsuba_unsigned_long));
    *sp = tmp;
    sp = &(tmp->next);
  }
  /* Terminate the list */
  *sp = NULL;

  return ret;
}

/* Substract two linked list of n variables
   Reusing the memory if possible (even the coefficient) */
static may_linked_term_t
sub (may_linked_term_t a, may_linked_term_t b, int n)
{
  may_linked_term_t *sp;
  may_linked_term_t ret;
  int i;

  sp = &ret;
  while (a != NULL && b != NULL) {
    MAY_ASSERT (n > 0);
    for (i = 0; i < n ; i++) {
      if (a->expo[i] > b->expo[i]) {
        /* Reuse a */
        *sp = a;
        sp = &(a->next);
        a = a->next;
        goto next_term;
      }else if (a->expo[i] < b->expo[i]) {
        /* Reuse b */
        *sp = b;
        sp = &(b->next);
        b->coeff = may_num_neg (b->coeff, b->coeff);
        b = b->next;
        goto next_term;
      }
    }
    /* Both exponents are equals: reuse a or b and free the other */
    a->coeff = may_num_sub (a->coeff, a->coeff, b->coeff);
    if (may_num_zero_p (a->coeff)) {
      /* Result is zero. Don't add this term. Free A and B */
      a = free_and_next (a);
      b = free_and_next (b);
    } else {
      *sp = a;
      sp = &(a->next);
      b = free_and_next (b);
      a = a->next;
    }
  next_term:
    (void) 0;
  }

  /* Add the latest terms */
  if (a!=NULL) {
    *sp = a; /* Auto terminate the list */
  } else {
    for ( ; b!=NULL ; b = b->next) {
        *sp = b;
        sp = &(b->next);
        b->coeff = may_num_neg (b->coeff, b->coeff);
        b = b->next;
    }
    /* Terminate the list */
    *sp = NULL;
  }

  return ret;
}



/* Perform basecase multiplication */
static may_linked_term_t
basecase (may_linked_term_t a2, may_linked_term_t b2, int n)
{
  may_linked_term_t product = NULL;

  while (a2 != NULL) {
    /* Multiply A2[] by B2 and add it in product */
    /* Try to reuse memory if possible for a but not for b */
    may_linked_term_t *sp;
    may_linked_term_t a, b, tmp;
    int i;
    a  = product;
    b  = b2;
    sp = &product;
    while (a != NULL && b != NULL) {
      MAY_ASSERT (n > 0);
      for (i = 0; i < n ; i++) {
        if (a->expo[i] > b->expo[i]+a2->expo[i] ) {
          /* a exponent is bigger: reuse a */
          *sp = a;
          sp  = &(a->next);
          a   = a->next;
          goto next_term;
        }else if (a->expo[i] < b->expo[i]+a2->expo[i]) {
          /* b+a2 is bigger. Allocate a new term */
          tmp = new_term (n);
          tmp->coeff = may_num_mul (tmp->coeff,
                                    b->coeff, a2->coeff);
          *sp = tmp;
          sp  = &(tmp->next);
          for (i = 0; i < n; i++)
            tmp->expo[i] = b->expo[i] + a2->expo[i];
          b   = b->next;
          goto next_term;
        }
      }
      /* Both exponents are equals: reuse a */
      *sp = a;
      sp  = &(a->next);
      a->coeff = may_num_add (a->coeff,
                              a->coeff,
                              may_g.kara.tmpnum = may_num_mul (may_g.kara.tmpnum,
                                                               b->coeff, a2->coeff) );
      a = a->next;
      b = b->next;
    next_term:
      (void) 0;
    }
    /* Add the latest terms */
    if (a!=NULL) {
      *sp = a; /* Auto terminate the list */
    } else {
      for ( ; b!=NULL ; b = b->next) {
        tmp = new_term (n);
        tmp->coeff = may_num_mul (tmp->coeff, b->coeff, a2->coeff);
        *sp = tmp;
        sp  = &(tmp->next);
        for (i = 0; i < n; i++)
          tmp->expo[i] = b->expo[i] + a2->expo[i];
      }
      /* Terminate the list */
      *sp = NULL;
    }
    /* Add A2 in the free term list for later reallocating use */
    a2 = free_and_next (a2);
  } /* while a2 */

  /* B2 wasn't used to create A. Add them in the free term list */
  while (b2 != NULL)
    b2 = free_and_next (b2);

  return product;
}

/* Test if size is small than n */
static int
size_smaller_than_p (may_linked_term_t a, unsigned long n)
{
  unsigned long s = 0;
  for ( ;a != NULL; a = a->next)
    if (++s > n)
      return 0;
  return 1;
}

/* Perform Karatsuba multiplication */
static may_linked_term_t
karatsuba (may_linked_term_t a, may_linked_term_t b, int i, int n, may_mark_t mark)
{
  may_linked_term_t a1, a2, b1, b2;
  may_linked_term_t  a1pa2,b1pb2,a1pa2fb1pb2,a1fb1,a2fb2;

  /* If a or b is too small, use basecase */
  if (size_smaller_than_p (a, may_g.kara.threshold)
      || size_smaller_than_p (b, may_g.kara.threshold))
    return basecase (a, b, n);

  /* Select another variable to split: j = (i+1) % n */
  int j = (i+1) % n;

  MAY_SPAWN_BLOCK(block, mark);
  MAY_SPAWN(block, (a,i,n), {
      /* A(X_i) = A1(X_i^2)*X_i+A2(X_i^2) */
      split (&a1, &a2, a, i);
      /* (A1+A2)*(B1+B2) with j */
      a1pa2 = add (a1, a2, n);
    }, (a1, a2, a1pa2));
  /* B(X_i) = B1(X_i^2)*X_i+B2(X_i^2) */
  split (&b1, &b2, b, i);
  /* B(X_i) = B1(X_i^2)*X_i+B2(X_i^2) */
  b1pb2 = add (b1, b2, n);
  MAY_SPAWN_SYNC(block);

  MAY_SPAWN(block, (a1pa2, b1pb2, j, n), {
      a1pa2fb1pb2 = karatsuba(a1pa2, b1pb2, j, n, mark);
    }, (a1pa2fb1pb2));

  MAY_SPAWN(block, (a1, b1, j, n), {
      /* A1*B1 with j */
      a1fb1 = karatsuba (a1, b1, j, n, mark);
    }, (a1fb1));

  /* A2*B2 with j */
  a2fb2 = karatsuba (a2, b2, j, n, mark);
  MAY_SPAWN_SYNC(block);

  /* Result(X_i) = A1*B1(X_i^2)*X_i^2+(((A1+A2)*(B1+B2)-A1*B1-A2*B2)(X_i^2)*X_i+A2*B2(X_i^2) */
  a1pa2fb1pb2 = sub (a1pa2fb1pb2, add (a1fb1, a2fb2, n), n);
  return merge (a1fb1, a1pa2fb1pb2, a2fb2, i, n);
}

may_t
may_karatsuba (may_t a, may_t b, may_t varlist)
{
  MAY_LOG_FUNC (("a='%Y' b='%Y' varlist='%Y", a, b, varlist));
  may_mark_t mark;
  may_mark (mark);

  /* Define the threshold for which below we call the basecase.
     It is a dynamic threshold and may be increase if the function failed for some
     part of the input */
  may_g.kara.threshold = 10;
  /* Define the cache used by the base case */
  may_g.kara.tmpnum = MAY_DUMMY;
  /* Define the list of free terms */
  may_g.kara.freeterm = NULL;

  int n = may_nops (varlist);
  may_linked_term_t ac = convert_to_linked_term (a, n, MAY_AT_PTR (varlist, 0));
  if (ac == NULL)
    return NULL;
  may_linked_term_t bc = convert_to_linked_term (b, n, MAY_AT_PTR (varlist, 0));
  if (bc == NULL)
    return NULL;
  ac = karatsuba (ac, bc, 0, n, mark);

  MAY_LOG_MSG (("kara.threshold =%lu", may_g.kara.threshold));

  return may_keep (mark, convert_from_linked_term (ac, n, MAY_AT_PTR (varlist, 0)));
}
