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

/* PRSGCD: Subresultant GCD of 'a' and 'b' using the main variable 'x' */
may_t
may_sr_gcd (may_t a, may_t b, may_t x)
{
  MAY_ASSERT (MAY_TYPE (x) == MAY_STRING_T);
  MAY_ASSERT ((MAY_FLAGS (a) & MAY_EXPAND_F) == MAY_EXPAND_F);
  MAY_ASSERT ((MAY_FLAGS (b) & MAY_EXPAND_F) == MAY_EXPAND_F);

  MAY_LOG_FUNC (("a='%Y' b='%Y' x='%Y'",a,b,x));
  MAY_ASSERT (a != 0 && b != 0 && x != 0);

  /* Extract the coefficients and compute their gcd.
     a and b may not be a node */
  unsigned long na = may_nops (a) + 1;
  unsigned long nb = may_nops (b) + 1;
  may_t gcd;
  {
    may_t tab[na+nb];
    may_mark ();
    na = may_extract_coeff (na, tab, a, x);
    nb = may_extract_coeff (nb, tab+na, b, x);
    gcd = may_keep (may_gcd (na+nb, tab));
  }

  /* Inverse a and b if deg(a) < deg(b) */
  mpz_srcptr db, da;
  int retvalue;
  UNUSED (retvalue);
  /* The given var 'x' ensures that 'may_degree' succeded */
  retvalue = may_degree (NULL, &db, NULL, b, 1, &x);
  MAY_ASSERT (retvalue != 0);
  retvalue = may_degree (NULL, &da, NULL, a, 1, &x);
  MAY_ASSERT (retvalue != 0);

  if (mpz_cmp (da, db) < 0) {
    swap (a, b);
    swap (da, db);
  }

  /* Compute the sub-resultant gcd */
  may_t g, h, cb, r, d;
  mpz_t dz;
  g = h = MAY_ONE;
  may_mark ();
  for (;;) {
    /* Extract the leader coefficient and the degree */
    retvalue = may_degree (&cb, &db, NULL, b, 1, &x);
    MAY_ASSERT (retvalue != 0);
    MAY_ASSERT (mpz_cmp (da, db) >= 0);

    /* Compute the difference between the degrees */
    mpz_init (dz);
    mpz_sub (dz, da, db);
    d = may_set_zz (dz);

    /* Compute rem (leader(b)^(degree(a)-degree(b)+1)*a,b,x) */
    if (!MAY_ONE_P (cb))
      a = may_eval (may_mul_c (may_pow_c (cb, may_add_c (d, MAY_ONE)), a));
    retvalue = may_div_qr (NULL, &r, a, b, x);
    MAY_ASSERT (retvalue != 0);

    /* If the remainder is 0, we have found the GCD */
    if (may_zero_fastp (r)) {
      /* Get the primpart of the GCD ('b') */
      may_content (NULL, &b, b, x);
      /* Return the content previously computed of the gcd times the gcd */
      gcd = may_mul (gcd, b);
      return may_keep (may_expand (gcd));
    }

    /* Update the vars for the next step */
    a  = b;
    da = db;
    b  = may_eval (may_mul_c (g, may_pow_c (h, d)));
    b  = may_divexact (r, b);
    MAY_ASSERT (b != NULL);
    g  = cb;
#if 1
    /* Usually d is 1 */
    h  = may_divexact (may_eval (may_pow_c (cb, d)),
                       may_eval (may_pow_c (h, may_sub_c (d, MAY_ONE))));
    MAY_ASSERT (h != NULL);
#else
    h = may_divexact (cb, h);
    MAY_ASSERT (h != NULL);
    h = may_expand (may_eval (may_mul_c (cb, may_pow_c (h, may_sub_c (d, MAY_ONE)))));
#endif
    /* Keep a, da, b, g, h for the next step */
    {
      may_t ktab[5] = {a, MAY_MPZ_NOCOPY_C (da), b, g, h};
      may_compact_v (5, ktab);
      a  = ktab[0];
      da = MAY_INT (ktab[1]);
      b  = ktab[2];
      g  = ktab[3];
      h  = ktab[4];
    }
  }
}

/* Return the max coefficient of 'a' (positive or negative)
   or NULL, if 'a' contains non-integer part (Used by Heuristic GCD) */
mpz_srcptr
may_max_coefficient (may_t a)
{
  if (MAY_LIKELY (MAY_TYPE (a) == MAY_FACTOR_T))
    a = MAY_AT (a, 0);
  if (MAY_LIKELY (MAY_TYPE (a) == MAY_INT_T))
    return MAY_INT (a);
  if (MAY_UNLIKELY (MAY_PURENUM_P (a)))
    return NULL;
  if (MAY_UNLIKELY (MAY_TYPE (a) == MAY_SUM_T)) {
    may_size_t i, n = MAY_NODE_SIZE(a);
    mpz_srcptr z = may_max_coefficient (MAY_AT (a, 0));
    if (z == NULL)
      return NULL;
    for (i = 1; i < n; i++) {
      mpz_srcptr c = may_max_coefficient (MAY_AT (a, i));
      if (c == NULL)
        return NULL;
      if (mpz_cmpabs (z, c) < 0)
        z = c;
    }
    return z;
  }
  /* string, power, ... */
  return MAY_INT (MAY_ONE);
}

/* Rebuild g from g_xi assuming  x interpolated at xi(Used by Heuristic GCD) */
may_t
may_rebuild_gcd (may_t g_xi, may_t xi, may_t x)
{
  may_t e;
  may_t g = MAY_ZERO;
  long i = 0;
  /* If the computed GCD is one, it remains one */
  if (MAY_ONE_P (g_xi))
    return MAY_ONE;
  MAY_RECORD ();
  e = may_expand (g_xi);
  while (!MAY_ZERO_P (e)) {
    may_t gi = may_smod (e, xi);
    g = may_addinc_c (g, may_mul_c (gi, may_pow_si_c (x, i)));
    /* If e was expanded, e remains expanded */
    /* e may be computed by may_smod */
    e = may_eval (may_div_c (may_sub_c (e, gi), xi));
    /* Next coefficient */
    i++;
  }
  MAY_RET_EVAL (g);
}

/* Compute the exponentiation tower of val up to dega */
static may_t *
compute_exp_tower(long dega, mpz_srcptr val)
{
  MAY_ASSERT (dega >= 0);
  if (MAY_UNLIKELY (dega >= INT_MAX))
    return NULL;

  /* Precompute all x^i for i=0..dega */
  may_t *deg_table = may_alloc ((dega+1)*sizeof (may_t));
  mpz_t z;
  mpz_init_set_ui (z, 1);
  for (long tmp = 0; tmp < dega; tmp++) {
    deg_table[tmp] = may_set_z (z);
    mpz_mul (z, z, val);
  }
  deg_table[dega] = may_set_z (z);

  MAY_LOG_MSG(("Size of evaluation point in exp tower: %lu\n", (unsigned long)mpz_sizeinbase(z, 2)));

  return deg_table;
}


/* Designed to work for univariate polynomial */
static may_t
replace_upol (may_t a, may_t x, long dega, may_t deg_table[dega+1])
{
  may_t s;
  may_iterator_t it1, it2;
  may_t b, c, p, t;

  MAY_ASSERT (MAY_TYPE (x) == MAY_STRING_T);
  MAY_ASSERT (may_identical (a, may_expand (a)) == 0);

  may_mark ();

  /* Replace x by val in the univariate polynomial */
  for (s = may_sum_iterator_init (it1, a) ;
       may_sum_iterator_end (&c, &t, it1) ;
       may_sum_iterator_next (it1)) {
    for (may_product_iterator_init (it2, t) ;
         may_product_iterator_end (&p, &b, it2) ;
         may_product_iterator_next (it2)) {
      if (MAY_LIKELY (may_identical (b, x) == 0)) {
        long tmp;
        if (MAY_UNLIKELY (may_get_si (&tmp, p)))
          return NULL;
        MAY_ASSERT (tmp <= dega);
        c = may_mulinc_c (c, deg_table[tmp]);
      } else
        c = may_mulinc_c (c, may_product_iterator_ref (it2));
    }
    c = may_eval (c);
    s = may_addinc_c (s, c);
  }
  return may_keep (may_eval (s));
}

/* Compute GCD of multivariate polynomials using the heuristic GCD algorithm
   of 'a' and 'b' using the main variable 'x' */
may_t
may_heur_gcd (may_t a, may_t b, may_t x)
{
  MAY_ASSERT (MAY_TYPE (x) == MAY_STRING_T);
  MAY_ASSERT ((MAY_FLAGS (a) & MAY_EXPAND_F) == MAY_EXPAND_F);
  MAY_ASSERT ((MAY_FLAGS (b) & MAY_EXPAND_F) == MAY_EXPAND_F);

  MAY_LOG_FUNC (("a='%Y' b='%Y' x='%Y'",a,b,x));

  MAY_RECORD ();

  /* If there is an integer modulo, abort.
     The Heuristic Gcd doesn't work */
  if (MAY_UNLIKELY (may_g.frame.intmod != NULL))
    MAY_RET (NULL);

  /* Remove the integer content from a and b */
  may_t int_a, int_b;
  /* TO PARALELIZE */
  may_content (&int_a, NULL, a, NULL);
  may_content (&int_b, NULL, b, NULL);
  may_t int_gcd = may_num_simplify (may_num_gcd (int_a, int_b));
  if (MAY_UNLIKELY (int_gcd != MAY_ONE)) {
    /* TO PARALELIZE */
    a = may_divexact (a, int_gcd);
    b = may_divexact (b, int_gcd);
  }

  /* Scan 'a' and get the max coefficient.
     If 'a' is not a sum over the integer, returns NULL */
  /* TO PARALELIZE */
  mpz_srcptr max_a = may_max_coefficient (a);
  if (MAY_UNLIKELY (max_a == NULL))
    MAY_RET (NULL);

  /* Same for b */
  mpz_srcptr max_b = may_max_coefficient (b);
  if (MAY_UNLIKELY (max_b == NULL))
    MAY_RET (NULL);

  /* Compute evaluation point zi = 2*max(max_a, max_b)+2 */
  mpz_t zi;
  mpz_init_set (zi, max_a);
  if (mpz_cmpabs (max_a, max_b) < 0)
    mpz_set (zi, max_b);
  mpz_abs (zi, zi);
  mpz_mul_2exp (zi, zi, 1);
  mpz_add_ui (zi, zi, 2);
  may_t xi = may_mpz_simplify (MAY_MPZ_NOCOPY_C (zi));

  /* For each try ;
      + Eval a_xi='a' at 'xi' and b_xi='b' at 'xi'
      + Compute g_xi = gcd(a_xi, b_xi)
      + Rebuild g using g_xi
      + Remove its integer content (Get the prim part)
      + Check if g divide a and b ==> return g
      + Next evaluation point
  */
  for (int try = 0; try < MAY_MAX_TRY_HEUGCD; try++) {
    may_t temp[2];
    /* Heuristic GCD needs to compute with very HUGE integer. Remove any limits */
    unsigned long p  = may_kernel_intmaxsize (-1UL);
    /* Compute the exponential tower of the evaluation point */
    long dega        = may_degree_si (a, x);
    long degb        = may_degree_si (b, x);
    may_t *deg_table = compute_exp_tower (MAX(dega, degb), MAY_INT (xi));
    if (MAY_UNLIKELY (deg_table == NULL))
      MAY_RET (NULL);
    /* Replace x by xi in a and b using build exponential tower */
    /* TO PARALELIZE */
    temp[0] = replace_upol (a, x, dega, deg_table);
    temp[1] = replace_upol (b, x, degb, deg_table);
    if (MAY_UNLIKELY (temp[0] == NULL || temp[1] == NULL))
      MAY_RET (NULL);
    may_kernel_intmaxsize (p);
    /* Compute (integer) gcd */
    may_t g_xi = may_gcd (2, temp);
    may_t g = may_rebuild_gcd (g_xi, xi, x);
    may_content (NULL, &g, g, NULL);
    /* TO PARALELIZE */
    if (MAY_LIKELY (may_divexact (a, g) != NULL
                    && may_divexact (b, g) != NULL)) {
      /* Restore the removed integer part */
      g = may_mul (int_gcd, g);
      /* Set the GCD as expanded */
      MAY_ASSERT (may_identical (g, may_expand (g)) == 0);
      MAY_SET_FLAG (g, MAY_EXPAND_F);
      return g;
    }
    MAY_LOG_MSG (("Heuristic GCD failed (%d/%d)\n", try, MAY_MAX_TRY_HEUGCD));
    /* Fail. Compute next evaluation point */
    xi = may_ceil_c (may_div_c (may_mul_vac (xi,
                                             may_sqrt_c (may_sqrt_c (xi)),
                                             may_set_ui (73794), NULL),
                                may_set_ui (27011)));
    xi = may_eval (xi);
    MAY_COMPACT (xi);
  }

  /* Fail */
  MAY_CLEANUP ();
  return NULL;
}

/* Return -1 if all variable listed in unused_var are not present in tabi
   Return the indice of the found variable otherwise */
MAY_INLINE may_size_t
my_independent_vp (may_t tabi, may_t unused_var)
{
  MAY_ASSERT (MAY_TYPE (unused_var) == MAY_LIST_T);
  may_size_t k, m = MAY_NODE_SIZE(unused_var);
  MAY_ASSERT (m >= 1);
  /* m is usually not too big */
  for (k  = 0; k < m; k++) {
    if (!may_independent_p (tabi, MAY_AT (unused_var, k)))
      return k;
  }
  return -1;
}

/* Calls the real GCD function (SR or Heur) */
static may_t
gcd_wrapper (may_t a, may_t b, may_t x)
{
  if (MAY_ZERO_P(a))
    return b;
  if (MAY_ZERO_P(b))
    return a;

  may_t g = may_heur_gcd (a, b, x);
  if (MAY_UNLIKELY (g == NULL))
    g = may_sr_gcd (a, b, x);
#if defined(MAY_WANT_ASSERT)
  /* Compare the different algorithms of GCD */
  may_t sr_gcd = may_sr_gcd (a, b, x);
  /* The returned GCD must be the same as the SR GCD (except the sign) */
  if (! (may_identical (g, sr_gcd) == 0
         || may_identical (g, may_neg(sr_gcd)) == 0)) {
    fprintf (stderr, "[MAYLIB]: assertion failed in %s.d: SR GCD & HEU GCD disagrees\n",
             __FILE__, __LINE__);
    fprintf (stderr, "a=");
    may_out_string (stderr, a);
    fprintf (stderr, "\nb=");
    may_out_string (stderr, b);
    fprintf (stderr, "\nsr_gcd=");
    may_out_string (stderr, sr_gcd);
    fprintf (stderr, "\nheu_gcd=");
    may_out_string (stderr, g);
    fprintf (stderr, "\n");
    if (may_divexact (a, g) == NULL)
      fprintf (stderr, "heu_gcd doesn't divide a\n");
    if (may_divexact (b, g) == NULL)
      fprintf (stderr, "heu_gcd doesn't divide b\n");
    if (may_divexact (a, sr_gcd) == NULL)
      fprintf (stderr, "sr_gcd doesn't divide a\n");
    if (may_divexact (b, sr_gcd) == NULL)
      fprintf (stderr, "sr_gcd doesn't divide b\n");
    MAY_ASSERT (0);
  }
#endif
  return g;
}

/* Return true if there at least one sum of product of sum in el
   The sum shall have more than 2 arguments (excluding the num) */
static int
check_for_sum_of_product_of_sum (may_t el)
{
  may_iterator_t it1, it2;
  may_t base, rbase;
  /* TODO: Should be checked recursively */
  if (MAY_NODE_P (el))
      //may_sum_p (el) && MAY_NODE_SIZE(el) >= 2)
    for(may_t numcoeff = may_sum_iterator_init (it1, el) ;
        may_sum_iterator_end (&numcoeff, &base, it1)     ;
        may_sum_iterator_next (it1) ) {
      for (may_t coeff = may_product_iterator_init (it2, base) ;
           may_product_iterator_end (&coeff, &rbase, it2) ;
           may_product_iterator_next (it2)) {
        if (MAY_UNLIKELY (MAY_TYPE (rbase) == MAY_SUM_T))
          return 1;
      }
    }
  return 0;
}

/* Return true if there at least one sum of product of sum in tab[] */
static int
check_for_sum_of_product_of_sum_in_tab (unsigned long n, const may_t tab[])
{
  MAY_ASSERT (n >= 2);

  for(unsigned long i = 0; i < n ;i ++) {
    if (MAY_UNLIKELY (check_for_sum_of_product_of_sum (tab[i])))
      return 1;
  }
  return 0;
}

may_t
may_gcd (unsigned long n, const may_t tab[])
{
  may_t x, gcd, naivegcd;
  may_t local_x;
  unsigned long i;

  if (MAY_UNLIKELY (n < 2)) {
    MAY_ASSERT (n == 1);
    return tab[0];
  }

  MAY_LOG_FUNC (("n=%lu tab[0]='%Y' tab[1]=='%Y'",n,tab[0],tab[1]));

  MAY_RECORD ();

  /* Compute the fast GCD (ie more or less the content with some
     obvious commun components) */
  /* TO PARALELIZE */
  naivegcd = may_naive_gcd (n, tab);

  /* Extract a commun var (before expand and divexact )*/
  x = may_find_one_polvar (n, tab);

  if (MAY_LIKELY (x == NULL)) {
    /* No commun variable found. Return the content */
    MAY_ASSERT (MAY_EVAL_P (naivegcd));
    /* Check for a degenerated case where the naive gcd is not the content */
    if (MAY_LIKELY (0 || !check_for_sum_of_product_of_sum_in_tab(n ,tab))) {
      /* No need to use MAY_RET since naivegcd is the only thing
         which has been constructed since the begining of the function. */
      return (naivegcd);
    }

    /* We have some sum of product of sum, which means that we can
       have cancellation in the higher term of an expression resulting
       in the deletion of one term. So the GCD may increase */
    /* Return the gcd of the contents (numerical) */
    gcd = NULL;
    /* TO PARALELIZE */
    for (unsigned long i = 0; i < n;i++) {
      may_t content;
      content = may_divexact (tab[i], naivegcd);
      MAY_ASSERT (content != NULL);
      may_content (&content, NULL, content, NULL);
      MAY_ASSERT (MAY_PURENUM_P (content));
      if (gcd == NULL)
        gcd = content;
      else
        gcd = may_num_gcd (gcd, content);
      if (may_num_one_p (gcd))
        MAY_RET (naivegcd);
    }
    MAY_ASSERT (gcd != NULL);
    MAY_RET_EVAL (may_mul_c (gcd, naivegcd));
  }

  /* Table for storing the expanding values of tab */
  may_t *expandtab = may_alloc (n*sizeof (may_t));
  unsigned long expandtab_size = n;

  /* Find the unused variables of the list */
  may_t unused_var = may_find_unused_polvar (n, tab);

  /* Remove the naive gcd from the elements and extract the unused variables */
  unsigned long nexpand = 0;
  /* TO PARALELIZE */
  for (i = 0; i<n; i++) {
    may_size_t k;
    expandtab[nexpand] = may_divexact (tab[i], naivegcd);
    /* If there is some variables which are not used by all the terms
       of the GCD, then separate each term by this variable and expand the GCD list. */
    if (MAY_UNLIKELY (unused_var != NULL)
        && (k = my_independent_vp (expandtab[nexpand], unused_var)) != (may_size_t) -1) {
      MAY_LOG_MSG(("Found one expression with some unused vars ==> expand it\n"));
      /* Enlarge expand_tab by may_nops (expand_tab[nexpand]) */
      may_t          tabi = may_expand (expandtab[nexpand]);
      unsigned long ntabi = may_nops (tabi);
      if (nexpand + (n-i) + ntabi > expandtab_size) {
        expandtab = may_realloc (expandtab, expandtab_size*sizeof (may_t),
                                 (nexpand+n+ntabi)*sizeof (may_t));
        expandtab_size = nexpand+n+ntabi;
      }
      /* Get the unused variable */
      may_t var = MAY_AT (unused_var, k);
      /* Check if the var is a string: if not, replace it by a local string
         since may_extract_coeff expects a string */
      if (MAY_TYPE (var) != MAY_STRING_T) {
        var  = may_set_str_local (MAY_COMPLEX_D);
        tabi = may_replace (tabi, MAY_AT (unused_var, k), var);
      }
      /* Extract the coeff of tabi in the enlarged table */
      may_size_t m = may_extract_coeff (ntabi+1, &expandtab[nexpand], tabi, var);
      if (MAY_UNLIKELY (m == 0)) {
        /* Failed to decompose it against this var: non polynomial dependance */
        MAY_RET_EVAL (naivegcd);
      }
      /* TODO: expandtab[...] may still have some unused variables ! */
      MAY_ASSERT (m <= (ntabi+1));
      nexpand += m;
      MAY_LOG_MSG(("Expansion done.\n"));
    } else
      nexpand++;
  }
  n = nexpand;
  MAY_LOG_MSG(("New value for n=%lu\n", n));

  /* TODO: Merge the identical terms in the list of the terms.
     They may appear due to the previous expansion due to unused variable.
     But they are usually handled quite efficiently by the sr_gcd, no? */

  /* Check if one term of the list is a product,
     in which case it is usually faster to return the product of the
     gcd which each terms (and avoids expanding it)
     Otherwise expands it */
  for (i = 0; i < n ; i++) {
    /* TODO: Support POW ? */
    if (MAY_TYPE (expandtab[i]) == MAY_FACTOR_T || MAY_TYPE (expandtab[i]) == MAY_PRODUCT_T) {
      may_size_t j, m;
      MAY_LOG_MSG(("Found product in expression: compute GCD of products\n"));
      /* Found it! Compute each partial gcd */
      may_t *temp = may_alloc (n * sizeof (may_t));
      memcpy (temp, expandtab, n*sizeof (may_t));
      m = may_nops (expandtab[i]);
      may_t *result = may_alloc ((m+1) * sizeof (may_t));
      for (j = 0; j < m ; j ++) {
        temp[i] = MAY_AT (expandtab[i], j);
        /* We have remove one product over a finite set of terms. It won't loop forever */
        result[j] = may_gcd (n, temp);
        /* If a partial GCD is found, remove it from every other term*/
        if (result[j] != MAY_ONE) {
          bool cont = true;
          /* Remove the partial GCD from every one */
          for (may_size_t k = 0; k < n; k++) {
            if (i == k)
              continue;
            temp[k] = may_divexact (temp[k], result[j]);
            MAY_ASSERT (temp[k] != NULL);
            /* If what remains is a pure numerical, we have reached the GCD */
            if (MAY_PURENUM_P (temp[k]))
              cont = false;
          }
          /* If some terms reduced to numerical, stop the computation */
          if (cont == false)
            for (++j ; j < m ; j++)
            result[j] = MAY_ONE;
        }
      }
      /* Don't forget the previously computed naivegcd */
      result[m] = naivegcd;
      gcd = may_mul_vc (m+1, result);
      MAY_RET_EVAL (gcd);
    }
    /* Not a product, so expands the expression */
    expandtab[i] = may_expand (expandtab[i]);
  }

  /* Extract a commun var (bis)
     Because removing the naive GCD may remove the previously computed common var,
     and because the expand may have remove it too! */
  x = may_find_one_polvar (n, expandtab);
  if (MAY_LIKELY (x == NULL)) {
    /* No commun variable found. Return the content */
    MAY_RET_EVAL (naivegcd);
  }
  MAY_LOG_VAR(x);

  /* x may be not a string. For example in "gcd(exp(x)-1,exp(x)^2-1)".
     Moreover, x may be a variable of another expression which have to be seen as a variable,
     for example (gcd (x*(x+abs(x)),x+abs(x))) ==> Replace is mandatory in all cases...
     since the underlying GCD functions don't handle this case
     FIXME: Found another way since it hurts the performance to always copy the input */
  /* TO PARALELIZE */
  local_x = may_set_str_local (MAY_COMPLEX_D);
  for (i = 0; i <n; i++) {
    MAY_ASSERT ((MAY_FLAGS (expandtab[i]) & MAY_EXPAND_F) != 0);
    expandtab[i] = may_replace_upol (expandtab[i], x, local_x);
    MAY_ASSERT (may_identical (expandtab[i], may_expand (expandtab[i])) == 0);
    MAY_SET_FLAG (expandtab[i], MAY_EXPAND_F);
  }
  swap (x, local_x);

  /* Compute the GCD */
  {
    /* New level of record used for the compactage of the gcd */
    MAY_RECORD ();
    gcd = gcd_wrapper (expandtab[0], expandtab[1], x);
    /* TO PARALELIZE */
    for (i = 2; i < n && !MAY_PURENUM_P (gcd); i++) {
      gcd = gcd_wrapper (gcd, expandtab[i], x);
      MAY_ASSERT (gcd != NULL);
      MAY_COMPACT (gcd);
    }
  }

  /* If it failed, return the naive gcd */
  if (MAY_PURENUM_P (gcd))
    MAY_RET_EVAL (naivegcd);

  /* We have done a replacement, reverse it */
  gcd = may_replace_upol  (gcd, x, local_x);

  MAY_RET_EVAL (may_mul_c (gcd, naivegcd));
}

/* Return the numerical associated to x */
MAY_INLINE may_t
get_num_coeff (may_t x)
{
  if (MAY_PURENUM_P (x))
    return x;
  else if (MAY_TYPE (x) == MAY_FACTOR_T)
    return MAY_AT (x, 0);
  else
    return MAY_ONE;
}

/* TODO: If b is a product of some terms, compute the content of
   each terms and return the product of each */
void
may_content (may_t *content, may_t *primpart, may_t b, may_t x)
{
  may_t c;

  MAY_LOG_FUNC (("b='%Y' x='%Y'", b, x));

  MAY_RECORD ();
  b = may_expand (b);

  /* If x is NULL, we have to return the numerical content: */
  if (MAY_LIKELY (x == NULL)) {
    may_t *it;
    may_size_t i, n;
    if (MAY_TYPE (b) == MAY_SUM_T)
      it = MAY_AT_PTR (b, 0), n  = MAY_NODE_SIZE(b);
    else
      it = &b, n  = 1;
    c = get_num_coeff (it[0]);
    MAY_ASSERT (n >= 1);
    MAY_LOG_MSG(("n=%lu\n", (unsigned long) n));
    /* TO PARALELIZE */
    for (i = 1; i<n && c != MAY_ONE; i++)
      c = may_num_gcd (c, get_num_coeff (it[i]));
    c = may_eval (c);
  } else {
    /* Compute the polynomial content */
    may_size_t n = may_nops (b) + 1;
    may_t tab[n];
    MAY_LOG_MSG(("n=%lu\n", (unsigned long) n));
    may_size_t nb = may_extract_coeff (n, tab, b, x);
    c = may_gcd (nb, tab);
  }

  /* Save the prim part if requested */
  if (primpart) {
    *primpart = may_divexact (b, c);
    MAY_ASSERT (*primpart != NULL);
    MAY_COMPACT_2 (c, (*primpart));
  } else
    MAY_COMPACT (c);

  /* Save it if requested */
  if (MAY_LIKELY (content))
    *content = c;
}
