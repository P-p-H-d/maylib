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

/* Return the euclidian division of a by var^n */
/* FIXME: On ne gère pas si exp(x) traine dans la boucle.......
   => Semantique différente ==> Fonction spéciale plutôt que spécialisation de div_qr */
void
may_div_qr_xexp (may_t *q, may_t *r, may_t a, may_t var, may_t n)
{
  MAY_ASSERT (MAY_TYPE (n) == MAY_INT_T);
  MAY_ASSERT (MAY_TYPE (var) == MAY_STRING_T);
  MAY_ASSERT (mpz_cmp_ui (MAY_INT (n), 1) >= 0);

  MAY_LOG_FUNC (("a='%Y' var='%Y' n='%Y'",a,var,n));

  /* Init input */
  if (r)
    *r = MAY_ZERO;
  if (q)
    *q = MAY_ZERO;

  MAY_RECORD ();
  // TODO: Shall use expand_var when it becomes efficient
  a = may_expand (a);
  may_iterator_t it;
  may_t c, b, sumnum, var_pow_n = may_pow (var, n);
  for (sumnum = may_sum_iterator_init (it, a) ;
       may_sum_iterator_end (&c, &b, it) ;
       may_sum_iterator_next (it) ) {
    may_iterator_t it2;
    may_t c2, b2;
    for (may_product_iterator_init (it2, b) ;
         may_product_iterator_end (&c2, &b2, it2);
         may_product_iterator_next (it2) ) {
      if (may_identical (var, b2) == 0) {
        if (may_num_cmp (c2, n) < 0)
          goto add_remainder;
        else
          goto add_quotient;
      }
    }
    /* Failed to found a degree of x ==> Add it to the remainder */
  add_remainder:
    if (r)
      *r = may_addinc_c (*r, may_sum_iterator_ref (it));
    continue;
  add_quotient:
    if (q)
      *q = may_addinc_c (*q, may_divexact (may_sum_iterator_ref (it), var_pow_n));
  }

  /* Finish separating quotient and reminder */
  if (r) {
    *r = may_addinc_c (*r, sumnum);
    *r = may_eval (*r);
    if (q) {
      *q = may_eval (*q);
      MAY_COMPACT_2 (*q, *r);
    } else
      MAY_COMPACT (*r);
  } else if (q) {
    *q = may_eval (*q);
    MAY_COMPACT (*q);
  }
}

/* Univariate division */
static int
may_div_qr_one (may_t *q, may_t *r, may_t a, may_t b, may_t var)
{
  MAY_ASSERT(MAY_TYPE(var) == MAY_STRING_T);
  may_mark_t mark;
  may_mark(mark);

  MAY_LOG_FUNC (("a='%Y' b='%Y var='%Y'", a, b, var));

  /* Transform the univariate polynomial into an array */
  may_t *a_tab, *b_tab;
  unsigned long na, nb;
  /* TO PARALELIZE */
  if (may_upol2array (&na, &a_tab, a, var) == 0)
    return may_compact (mark, NULL), 0;
  if (may_upol2array (&nb, &b_tab, b, var) == 0)
    return may_compact (mark, NULL), 0;
  long da = na-1;
  long db = nb-1;

  /* Trivial comparaison */
  if (MAY_UNLIKELY (da < db)) {
    if (q)
      *q = MAY_ZERO;
    if (r)
      *r = a;
    return may_compact (mark, NULL), 1;
  }

  /* Variables necessary for handling the gc */
  may_t cb = b_tab[db];
  long d = da, d_compact = d;
  unsigned int compact_counter = 0;
  may_mark_t mark_loop;
  may_mark(mark_loop);

  /* Main loop */
  while (d >= db) {
    if (!MAY_ZERO_P(a_tab[d])) {
      may_t tmp = may_divexact (a_tab[d], cb);
      if (MAY_UNLIKELY (tmp == NULL))
        tmp = may_div (a_tab[d], cb);
      a_tab[d] = tmp;
      tmp = may_neg(tmp);
      for (long i = 1; i <= db; i++) {
        a_tab[d-i] = may_addinc_c (a_tab[d-i], may_mul_c (tmp, b_tab[db-i]));
        /* FIXME: Really needed ? Good idea ?
           Why not performing it only when divising by the element ?*/
        a_tab[d-i] = may_expand (may_eval (a_tab[d-i]));
      }
    }
    d = d - 1;
    if (++compact_counter >= 10) {
      /* Optimized to compact only what has changed:
         from d_compact to d-db */
      MAY_ASSERT (d_compact > d);
      may_compact_v (mark_loop, d_compact-d+db+1, &a_tab[d-db]);
      may_mark (mark_loop);
      d_compact = d;
      compact_counter = 0;
    }
  }

  /* Transform back q and R */
  /* TO PARALELIZE */
   if (q != NULL)
    *q = may_array2upol (da-db+1, &a_tab[db], var);
  if (r != NULL)
    *r = may_array2upol (db, &a_tab[0], var);

  /* Final compact */
  may_t temp[2];
  temp[0] = q ? *q : MAY_ZERO;
  temp[1] = r ? *r : MAY_ZERO;
  may_compact_v (mark, 2, temp);
  if (q) *q = temp[0];
  if (r) *r = temp[1];
  return 1;
}

/* Return expand(a+s*f) assuming a and s are sum and f is monomial */
static may_t
mul_expand_c (may_t a, may_t s, may_t f)
{
  may_size_t i, n, na;
  may_t temp;

  if (MAY_TYPE (s) != MAY_SUM_T)
    return may_add_c (a, may_mul_c (s, f));

  na = MAY_TYPE (a) == MAY_SUM_T ? MAY_NODE_SIZE(a) : 1;
  n = MAY_NODE_SIZE(s);
  temp = MAY_NODE_C (MAY_SUM_T, n+na);
  /* TO PARALELIZE */
  for (i = 0; i < n; i++)
    MAY_SET_AT (temp, i, may_mul_c (MAY_AT (s,i), f));
  if (na == 1)
    MAY_SET_AT (temp, n, a);
  else
    memcpy (MAY_AT_PTR (temp, n), MAY_AT_PTR (a, 0), na*sizeof(may_t));

  return temp;
}

int
may_div_qr (may_t *q, may_t *r, may_t a, may_t b, may_t var)
{
  mpz_t dz;
  may_t la, cb, ca;
  may_list_t quo, rem;
  may_size_t n;
  may_size_t numvar;
  may_t *var_ptr;
  may_t qq, rr;

  MAY_LOG_FUNC (("a='%Y' b='%Y' var='%Y'",a,b,var));

  MAY_RECORD ();

  /* Expand b */
  b = may_expand (b);

  /* If b is a purenum, fast case */
  if (MAY_UNLIKELY (MAY_PURENUM_P (b))) {
    /* We fail to divide by 0*/
    if (MAY_ZERO_P (b))
      MAY_RET_CTYPE (0); /* FAIL */

    if (q != NULL)
      *q = may_div (a, b);
    if (r != NULL)
      *r = MAY_ZERO;
    return 1;
  }

  /* Expand a */
  a = may_expand (a);

  /* Create tables for storing the degrees */
  if (MAY_LIKELY (MAY_TYPE (var) == MAY_STRING_T)) {
    int retvalue = may_div_qr_one(q, r, a, b, var);
    /* FIXME: Sometimes when we divide a(var) by b(var) we may
       create some expressions which are rational expressions
       of other variables ==> q=b*q+r remains true but a comdenom
       is necessary (Recursive call...) ==> Assert disabled */
    /*MAY_ASSERT (retvalue == 0 || q == NULL || r == NULL
      || may_identical (a, may_expand (may_add(may_mul(*q, b), *r))) == 0); */
    return retvalue;
  }
  MAY_ASSERT (MAY_TYPE (var) == MAY_LIST_T);
  numvar  = MAY_NODE_SIZE(var);
  var_ptr = MAY_AT_PTR (var, 0);
  mpz_srcptr db[numvar], da[numvar];

  /* Get the degree of b*/
  if (MAY_UNLIKELY (!may_degree (&cb, db, NULL, b, numvar, var_ptr)))
    MAY_RET_CTYPE (0); /* Fail to divide */

  /* Reserve enought space for q and r */
  n = MAY_TYPE (a) == MAY_SUM_T ? MAY_NODE_SIZE(a) : 1;
  may_list_init (quo, n);
  may_list_resize (quo, 0);
  may_list_init (rem, n);
  may_list_resize (rem, 0);
  mpz_init (dz);

  /* Main loop: quadratic behavior */
  while (!may_zero_fastp (a)) {
    /* Get leader (A) and the degree of A */
    if (MAY_UNLIKELY (!may_degree (&ca, da, &la, a, numvar, var_ptr)))
      MAY_RET_CTYPE (0); /* Fail to divide */

    /* Check if leader (B) divide leader (A) */
    if (may_cmp_multidegree (numvar, da, db) >= 0) {
      /* Remove -leader(A)/leader(B)*B from A */
      may_t div, temp;
      int way;

      /* Divide ca by cb */
      div  = may_divexact (ca, cb);
      if (div == NULL)
        MAY_RET_CTYPE (0);
      way = MAY_PURENUM_P (div);
      {
        may_size_t i;
        may_t z = MAY_NODE_C (MAY_PRODUCT_T, numvar+1);
        for (i = 0; i < numvar; i++) {
          mpz_sub (dz, da[i], db[i]);
          MAY_SET_AT (z, i, may_pow_c (var_ptr[i], MAY_MPZ_C (dz)));
        }
        MAY_SET_AT (z, numvar, div);
        div = may_eval (z);
      }

      /* Compute A-(ca/cb)*var*(degA-degB)*b */
      MAY_ASSERT ((MAY_FLAGS (a) & MAY_EXPAND_F) == MAY_EXPAND_F);
      MAY_ASSERT ((MAY_FLAGS (b) & MAY_EXPAND_F) == MAY_EXPAND_F);
      if (way == 0) {
        temp = may_mul_c (may_neg_c (div), b);
        a    = may_add_c (a, temp);
        /* FIXME: Analyse why expand seems necessary */
        /* NOTE: Il semblerait que ca soit cet expand qui prenne 80% du temps CPU
           pour le dernier test de charge */
        a    = may_expand (may_eval (a));
      } else {
        /* HACK: Gain 25-60% sur univarie
           Si div est de la forme num*var^i, on peut aller plus vite
           en simulant l'expand sans problemes car a et b sont deja
           sous forme developpee. */
        a  = mul_expand_c (a, b, may_eval (may_neg_c (div)));
        a  = may_eval (a);
        MAY_ASSERT (may_identical (a, may_expand (a)) == 0);
        MAY_SET_FLAG (a, MAY_EXPAND_F);
      }

      /* TODO: For float, leader (A) may remain! */
      /* Add leader (A) / leader (B) to quotient */
      if (MAY_LIKELY (q != NULL))
        may_list_push_back (quo, div);

    } else {
      /* Leader(B) doesn't divide leader(A) */
      if (MAY_LIKELY (numvar == 1)) {
        /* If there is only one var, the division is finished.
           Hack to go faster */
        if (MAY_LIKELY (r != NULL))
          rr = a;
        else
          rr = NULL;
        goto quit_quotient;
      } else {
        /* Add leader (A) to remainder and continue. */
        if (MAY_LIKELY (r != NULL))
          may_list_push_back (rem, la);
        /* Remove leader (A) from A */
        a = may_sub (a, la);
      }
    }
  } /* End of while */

  /* Eval quo & rem if needed */
  if (MAY_LIKELY (r != NULL)) {
    if (MAY_UNLIKELY (may_list_get_size (rem) == 0))
      rr = MAY_ZERO;
    else {
      rr = may_list_quit (rem);
      MAY_OPEN_C(rr, MAY_SUM_T);
      rr = may_eval (rr);
    }
  } else
    rr = NULL;

 quit_quotient:
  if (MAY_LIKELY (q != NULL)) {
    if (MAY_UNLIKELY (may_list_get_size (quo) == 0))
        qq = MAY_ZERO;
    else {
      qq = may_list_quit (quo);
      MAY_OPEN_C(qq, MAY_SUM_T);
      qq = may_eval (qq);
    }
  } else
    qq = NULL;

  /* Compact and return */
  MAY_COMPACT_2 (qq ,rr);
  if (MAY_LIKELY (q != NULL))
    *q = qq;
  if (MAY_LIKELY (r != NULL))
    *r = rr;
  return 1; /* Success (Can't use MAY_RET_CTYPE!) */
}
