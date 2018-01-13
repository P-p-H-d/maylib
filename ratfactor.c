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

/* For details of the used algorithm, see :
   http://www-fourier.ujf-grenoble.fr/~parisse/giac/doc/fr/algo.html#htoc11
   or http://www-fourier.ujf-grenoble.fr/~parisse/cas/rmc1.ps */

/* Compute the GCD of a and b modulo m */
MAY_INLINE may_t gcd (may_t a, may_t b, may_t m) {
  may_kernel_intmod (m);
  may_t temp[2] = {may_reeval (a), may_reeval (b)};
  may_t g = may_gcd (2, temp);
  may_kernel_intmod (NULL);
  return g;
}

/* From P an univariate polynomial over the integer of the variable x
   Returns all the unitary terms of the factorisation of P */
static may_t
ratfactor (may_t p, may_t x)
{
  /* P is sqrfree, ie PGCD (P, P') = 1 */
  MAY_ASSERT (MAY_TYPE (p) == MAY_SUM_T);
  MAY_ASSERT (may_g.frame.intmod == NULL);
  MAY_ASSERT (MAY_TYPE (x) == MAY_STRING_T);
  MAY_RECORD ();

  /* 1. Get the dominant coefficient an, and its constant coefficient a0 */
  may_t an, a0;
  int i = may_degree (&an, NULL, NULL, p, 1, &x);
  if (i == 0)
    MAY_RET (p); /* Fail to get the degree. Can't factorise */
  MAY_ASSERT (MAY_PURENUM_P (MAY_AT (p, 0)));
  a0 = MAY_AT (p, 0);

  /* 1'. Get the diff and its dominant coeffient */
  may_t diff_p = may_diff (p, x), diff_an;
  i = may_degree (&diff_an, NULL, NULL, diff_p, 1, &x);
  MAY_ASSERT (i != 0);

  /*  2. Find m prime integer such as:
       - an  % m != 0
       - an' % m != 0
       - GCD (P, P') % m = 1  (ie. P remains square-free modulo m) */
  mpz_t zm;
  mpz_init_set_ui (zm, 2);
  may_t m = may_eval (MAY_MPZ_NOCOPY_C (zm));
  while (MAY_ZERO_P (may_smod (an, m))
         || MAY_ZERO_P (may_smod (diff_an, m))
         || !MAY_ONE_P (gcd (p, diff_p, m))) {
    mpz_nextprime (zm, zm);
    m = may_eval (MAY_MPZ_NOCOPY_C (zm));
  }

  /* If the found modulo doesn't fit in an unsigned long, we can't seach for
     all the potential candidates, so abort */
  unsigned long im;
  if (may_get_ui (&im, m) != 0)
    MAY_RET (p);

  /* 3. For i from 0 to m-1 do
         If  P(i) % m = 0
           xi = i
           Find k such as m^k > 2*abs(an)*(a0), ie k = ceil(log(2*abs(an*a0)+1)/log(m))
           For j from 1 to k do
             assert (P'(xi) % m != 0)
             xi = xi - P(xi) / (smod (P'(xi), m^j))
           EndFor
           f = PrimPart (smod (an*X - an*xi, m^k))
           If f divides P,
              Add f to the list of factors of P
           EndIf
         Endif
       Endfor
   */
  may_t result = MAY_ONE;
  /* 3. For each potential candidate */
  for (unsigned long i = 0; i < im; i++)
    /* if this candidate is 0 in Z/mZ */
    if (MAY_ZERO_P (may_smod (may_replace (p, x, may_set_ui (i)), m))) {
      /* Rebuild it in Z */
      MAY_RECORD ();
      may_t xi = may_set_ui (i);
      /* Find k such as m^k > 2*abs(an)*(a0) */
      may_t k = may_evalf (may_div_c (may_log_c (may_add_c (may_abs_c (may_mul_vac (MAY_TWO, an, a0, NULL)), MAY_ONE)),
                                      may_log_c (m)));
      k = may_ceil (k);
      unsigned long ik;
      if (may_get_ui (&ik, k) != 0)
        continue;
      may_t m_pow_j = m;
      for (unsigned long j = 1; j <= ik; j++) {
        /* Compute the inverse modulo m^j */
        may_kernel_intmod (m_pow_j);
        may_t inv = may_div (MAY_ONE, may_replace (diff_p, x, xi));
        /* Compute new evaluation point */
        xi = may_sub (xi, may_mul (may_replace (p, x, xi), inv));
        may_kernel_intmod (NULL);
        if (j != ik)
          m_pow_j = may_mul (m_pow_j, m);
      }
      may_t f = may_eval (may_sub_c (may_mul_c (an, x), may_mul_c (an, xi)));
      may_content (NULL, &f, may_smod (f, m_pow_j), NULL);
      if (may_divexact (p, f) != NULL)
        result = may_mul (result, f);
      MAY_COMPACT (result);
    }

  /* 4. Add the remaining terms */
  p = may_divexact (p, result);
  MAY_ASSERT (p != NULL);
  result = may_mul (result, p);
  MAY_RET (result);
}


/* Compare function used for qsort in content_except_x */
static int
cmp (const void *a, const void *b) {
  return may_identical (((const may_pair_t *)a)->second,
                        ((const may_pair_t *)b)->second);
}
/* Content of an integer multivariate polynomial P over all variables except one X
   ie. the content is computed in Z[X] */
static may_t
content_except_x (may_t p, may_t x)
{
  MAY_RECORD ();

  /* First extract the multinomial in pair (coeff, base) with coeff in Z[X] */
  p = may_expand (p);
  may_size_t np = may_nops (p) + 2;
  may_pair_t temp[np];
  may_t tab[np];

  /* Fill the table with all the coefficient (coeff, base) */
  may_t *it;
  may_size_t n;
  if (MAY_TYPE (p) != MAY_SUM_T) {
    n = 1;
    it = &p;
  } else {
    n = MAY_NODE_SIZE(p);
    it = MAY_AT_PTR (p, 0);
  }
  MAY_ASSERT (n < np);
  for (may_size_t i = 0; i < n; i++, it++) {
    may_t a;
    mpz_srcptr z;
    may_extract_coeff_deg (&a, &z, *it, x);
    may_t num = MAY_ONE, base = MAY_ONE;
    if (MAY_TYPE (a) == MAY_FACTOR_T) {
      num = MAY_AT (a, 0);
      base = MAY_AT (a, 1);
    } else if (MAY_PURENUM_P (a)) {
      num = a;
    } else
      base = a;
    temp[i].first  = may_mul (num, may_pow (x, may_set_z (z)));
    temp[i].second = base;
  }

  /* Sort table according to their base */
  qsort (temp, n, sizeof temp[0], &cmp );
  /* To ensure that the final base will be stored in the next loop (base can't be 0) */
  temp[n].first = temp[n].second = MAY_ZERO;

  /* Merge table according to their base */
  may_pair_t *ref = &temp[0];
  may_size_t nn = 0;
  for (may_size_t i = 1; i <= n; i++) {
    if (may_identical (temp[i].second, ref->second) != 0) {
      temp[nn].second = ref->second;
      may_size_t nt = 0;
      while (ref != &temp[i]) {
        tab[nt++] = ref++->first;
        MAY_ASSERT (nt <= np);
      }
      temp[nn].first = may_eval (may_add_vc (nt, tab));
      MAY_ASSERT (may_identical (temp[nn].first, may_expand (temp[nn].first)) == 0);
      MAY_SET_FLAG (temp[nn].first, MAY_EXPAND_F);
      nn++;
      ref = &temp[i];
    }
  }
  MAY_ASSERT (nn <= np);

  /* Then compute the content */
  for (may_size_t i = 0; i < nn; i++)
    tab[i] = temp[i].first;
  may_t c = may_gcd (nn, tab);
  MAY_RET (c);
}

/* Heuristic Factorisation: p
   If there is only one variable, calls the univariate algorithm
   x= Find the lowest degree variable
   Get the content of P for all other variables (ie. P is a polynomial which coeff are Z[x]), and P becomes the prim part
   L = Factorise this content recusively
   z = 2*abs(max(P))+2 (See gcd2.c)
   FOR ntry from 0 to 6 DO
       if P is a constant ==> Return {P,L}
       while GCD (P(z), diff(P(x),x)|z) is not one
         Inc z by one
       c_z = integer content of P(z)
       NewL = Factor P(z)/c_z recursively
       For each pj in NewL
           P_j = smod (c_z*pj, z)
           P_j = PrimPart (P_j)
           if P_j divise P
               Add P_j in L
               P = P /exact P_j
           EndIf
       EndFor
       z = ceil(z*sqrt(2))
   EndFor
   // Fail
   Return {P, L}
*/
static may_t
heur_unitfactor (may_t p)
{
  /* Get the list of indeterminate of x */
  may_t x = may_indets (p, MAY_INDETS_NONE);
  /* If there is less than 1 indeterminate, call the univariate function */
  if (may_nops (x) <= 1) {
    if (MAY_TYPE (p) != MAY_SUM_T)
      return p;
    x = may_op (x, 0);
    MAY_COMPUTE_EXPR_WITH_VAR_AS_A_STRING(ratfactor (p, x), p, x);
    return p;
  }
  /* Let's deal with more than 2 variables! */
  MAY_RECORD ();
  /* Find the main variable */
  x = may_find_one_polvar (1, &p);
  /* Extract the content and factorize it */
  may_t content = content_except_x (p, x);
  p = may_divexact (p, content);
  MAY_ASSERT (p != NULL);
  may_t l = heur_unitfactor (content);
  /* Compute the initial value of the evaluation point */
  mpz_srcptr max_p = may_max_coefficient (p);
  mpz_t zz;
  mpz_init_set (zz, max_p);
  mpz_abs (zz, zz);
  mpz_mul_2exp (zz, zz, 1);
  mpz_add_ui (zz, zz, 2);
  /* Let's give us 4 tries */
  for (int ntry = 0; ntry < 4; ntry++) {
    /* If P is constant, stops the loop */
    if (may_independent_p (p, x))
      break;
    /* Check that the evaluation point is good enought with the diff */
    may_t diff_p = may_diff (p, x);
    may_t z = may_eval (MAY_MPZ_NOCOPY_C (zz)), p_z;
    while (!may_one_p (may_gcd2 ((p_z = may_replace (p, x, z)),
                                 may_replace (diff_p, x, z)))) {
      mpz_add_ui (zz, zz, 1);
      z = may_eval (MAY_MPZ_NOCOPY_C (zz));
    }
    /* Extract the integer content */
    may_t intcontent;
    may_content (&intcontent, &p_z, p_z, NULL);
    /* Factorize the new p without x */
    may_t new_l = heur_unitfactor (p_z), p_j = NULL, *it;
    /* Iterate through the list of factors: */
    if (MAY_TYPE (new_l) == MAY_FACTOR_T) {
      p_j   = MAY_AT (new_l, 0);
      new_l = MAY_AT (new_l, 1);
    }
    long n;
    if (MAY_TYPE (new_l) == MAY_PRODUCT_T) {
      it = MAY_AT_PTR (new_l, 0);
      n = MAY_NODE_SIZE(new_l);
    } else {
      it = &new_l;
      n = 1;
    }
    if (p_j == NULL) {
      p_j = *it++;
      n--;
    }
    /* For each factor: */
    do {
      /* Reconstruct it from z to x */
      p_j = may_rebuild_gcd (may_mul (p_j, intcontent), z, x);
      may_content (NULL, &p_j, p_j, NULL);
      /* Check if this new factor divides p */
      may_t new_p = may_divexact (p, p_j);
      if (new_p != NULL) {
        /* Factor found! Add it in the list */
        l = may_mul (l, p_j);
        p = new_p;
      }
      /* Next factor to test */
      n--;
      if (n >= 0)
        p_j = *it++;
    } while (n >= 0);
    /* Next iteration for a new z */
    mpz_mul_ui (zz, zz, 73794);
    mpz_div_ui (zz, zz, 27011);
  }
  /* Add the remaining term into the list of factor */
  l = may_mul (p, l);
  MAY_RET (l);
}

/* Recursevely apply ratfactor */
static may_t
ratfactor_recur (may_t p)
{
  if (MAY_TYPE (p) == MAY_SUM_T)
    return heur_unitfactor (p);
  if (MAY_TYPE (p) == MAY_PRODUCT_T || MAY_TYPE (p) == MAY_FACTOR_T) {
    may_size_t i, n = MAY_NODE_SIZE(p);
    may_t y = MAY_NODE_C (MAY_TYPE (p), n);
    for (i = 0; i < n; i++)
      MAY_SET_AT (y, i, ratfactor_recur (MAY_AT (p, i)));
    return y;
  }
  if (MAY_TYPE (p) == MAY_POW_T)
    return may_pow_c (ratfactor_recur (MAY_AT (p, 0)), MAY_AT (p, 1));
  return p;
}

/* Rename into may_unitfactor (may_t p) ? */
may_t
may_ratfactor (may_t p, may_t v)
{
  MAY_ASSERT (MAY_EVAL_P (p));

  MAY_LOG_FUNC (("p=%Y var=%Y", p, v));
  may_mark ();

  if (v == NULL) {
    may_t varlist = may_indets (p, MAY_INDETS_NONE);
    may_size_t i, n = may_nops (varlist);
    for (i = 0; i < n ; i ++) {
      may_t v = may_op (varlist, i);
      MAY_COMPUTE_EXPR_WITH_VAR_AS_A_STRING(may_sqrfree(p,v),p,v);
    }
  } else
    MAY_COMPUTE_EXPR_WITH_VAR_AS_A_STRING(may_sqrfree(p,v),p,v);
  p = may_compact(p);

  /* TODO: Try all the possible values? */
  if (may_g.frame.intmod == NULL)
    p = ratfactor_recur (p);

  return may_keep (may_eval (p));
}
