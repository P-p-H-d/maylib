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

/* Internal structure used to define a pair coefficient / integer exponent */
struct may_coeff_deg_s {
  may_t a;
  mpz_srcptr z;
};

/* Compare to may_coeff_deg_s pairs using their exponent (Used a callback for qsort
   in may_extract_all_coeff_deg) */
static int
cmp (const void *a, const void *b) {
  const struct may_coeff_deg_s *a2 = a;
  const struct may_coeff_deg_s *b2 = b;
  return mpz_cmp (a2->z, b2->z);
}

/* Extract all the coefficients from 'a' view as a polynomial in 'x'
   and store them in 'tab[na]'
   Return the number of filled item or 0 if there as an error while extracting them.
   Such an error is typically that 'a' can't be seen as a polynomial of the variable 'x'
   If the table is not big enough to get all the terms (Expand 'a' and alloc a table of nops(a)+1 terms)
   an assertion will fail */
static unsigned long
may_extract_all_coeff_deg (unsigned long na,
                           struct may_coeff_deg_s temp[na],
                           may_t a, may_t x)
{
  unsigned long i, n;
  may_t tab[na];

  MAY_ASSERT (na >= 1);
  MAY_ASSERT ((MAY_FLAGS (a) & MAY_EXPAND_F) == MAY_EXPAND_F);

  /* Special case if it isn't a sum: there is only one term */
  if (MAY_TYPE (a) != MAY_SUM_T) {
    if (MAY_UNLIKELY (may_extract_coeff_deg (&temp[0].a, &temp[0].z, a, x) == 0))
      return 0;
    MAY_ASSERT (may_identical (temp[0].a, may_expand (temp[0].a)) == 0);
    MAY_SET_FLAG (temp[0].a,MAY_EXPAND_F);
    return 1;
  }

  /* Fill the table */
  n = MAY_NODE_SIZE(a);
  MAY_ASSERT (n >= 2 && n <= na);
  /* TO PARALELIZE */
  for (i = 0; i < n; i++)
    if (MAY_UNLIKELY (may_extract_coeff_deg (&temp[i].a, &temp[i].z, MAY_AT (a, i), x) == 0))
      return 0;

  /* Sort table according to the exponent */
  qsort (temp, n, sizeof temp[0], &cmp );

  /* Merge table to regroup each term of same exponent */
  struct may_coeff_deg_s *dest = &temp[0], *ref = &temp[0];
  unsigned long nn = 0, np;
  for (i = 1; i < n; i++) {
    if (mpz_cmp (temp[i].z, ref->z) != 0) {
      dest->z = ref->z;
      np = 0;
      do {
        tab[np++] = ref++->a;
        MAY_ASSERT (np <= na);
      } while (ref != &temp[i]);
      dest->a = may_eval (may_add_vc (np, tab));
      MAY_ASSERT (may_identical (dest->a, may_expand (dest->a)) == 0);
      MAY_SET_FLAG (dest->a,MAY_EXPAND_F);
      dest++;
      nn++;
      ref = &temp[i];
    }
  }
  dest->z = ref->z;
  np = 0;
  do {
    tab[np++] = ref++->a;
    MAY_ASSERT (np <= na);
  } while (ref != &temp[n]);
  dest->a = may_eval (may_add_vc (np, tab));
  MAY_ASSERT (may_identical (dest->a, may_expand (dest->a)) == 0);
  MAY_SET_FLAG (dest->a,MAY_EXPAND_F);
  nn++;
  MAY_ASSERT (nn <= na);
  /* Return the number of element used in the table */
  return nn;
}

/* Extract 'a' view as a polynomial of 'x' and fills tab
   with all coefficients of 'a' regrouped by their degrees.
   It doesn't try to keep the exponent of the original polynom.
   If the table is not big enough to get all the terms (Expand 'a' and alloc a table of nops(a)+1 terms)
   an assertion will fail */
unsigned long
may_extract_coeff (unsigned long na, may_t tab[na],
                   may_t a, may_t x)
{
  unsigned long i;
  struct may_coeff_deg_s temp[na];

  MAY_LOG_FUNC (("na=%lu tab[0]='%Y' tab[1]=='%Y', a='%Y', x='%Y'",na,tab[0],tab[1], a, x));

  /* Expand the expression (if needed) and extract all coefficients with their exponents */
  a = may_expand (a);
  na = may_extract_all_coeff_deg (na, temp, a, x);
  /* Then recopy the coefficients of the table to tab */
  for (i = 0; i < na; i++)
    tab[i] = temp[i].a;
  return na;
}

int
may_upol2array(unsigned long *n, may_t **tab,
               may_t upol, may_t var)
{
  MAY_ASSERT (MAY_TYPE(var) == MAY_STRING_T);

  MAY_LOG_FUNC (("upol='%Y' var='%Y'", upol, var));

  /* 1. Expand */
  upol = may_expand (upol);

  /* 2. Extract the couple (val, power) */
  unsigned long na = may_nops(upol)+1;
  struct may_coeff_deg_s temp[na]; /* TODO: Avoid stack allocation if too big */
  na = may_extract_all_coeff_deg (na, temp, upol, var);
  if (MAY_UNLIKELY (na == 0))
    return 0;

  /* 3. Extract the biggest power */
  mpz_srcptr max = temp[0].z;
  for (unsigned long i = 1; i < na; i++) {
    if (mpz_cmp (max, temp[i].z) < 0)
      max = temp[i].z;
    if (MAY_UNLIKELY (mpz_sgn(temp[i].z) < 0))
      return 0;
  }
  if (MAY_UNLIKELY (!mpz_fits_ulong_p (max)))
    return 0;
  unsigned long deg = mpz_get_ui(max);

  /* 4. Create the array */
  may_t *table = may_alloc ((deg+1)*sizeof(may_t));
  unsigned long j = 0;
  for (unsigned long i = 0; i <= deg; i++) {
    if (mpz_cmp_ui (temp[j].z, i) == 0) {
      table[i] = temp[j].a;
      j++;
    } else
      table[i] = MAY_ZERO;
  }

  /* Output */
  *n = deg+1;
  *tab = table;
  return 1;
}

may_t
may_array2upol (unsigned long n, may_t *table, may_t var)
{
  may_mark ();
  may_t s = may_set_ui (0);
  may_t var2pow = may_set_ui(1);
  for (unsigned long i = 0; i < n ; i++) {
    s = may_addinc_c (s, may_mul_c (table[i], var2pow));
    var2pow = may_eval (may_mul_c (var2pow, var));
  }
  return may_keep (may_eval (s));
}

/* Compare two arrays of degrees using lexicographic order. */
int
may_cmp_multidegree (may_size_t n, mpz_srcptr b[n], mpz_srcptr a[n])
{
  MAY_ASSERT (n > 0);

  for (may_size_t i = 0; i<n; i++) {
    int cmp = mpz_cmp (b[i], a[i]);
    if (MAY_LIKELY (cmp))
      return cmp;
  }
  return 0;
}

/* Extract the degree 'd' and the coefficient 'c' from 'x',
   view as of a monomial in 'v' (Unidegree)
   Return 0 in case of failure (can't see 'x' as a polynomial of the variable 'v'), 1 otherwise */
int
may_extract_coeff_deg (may_t *c, mpz_srcptr *d,
                       may_t x, may_t v)
{
  may_t coeff, base;

  MAY_LOG_FUNC (("x='%Y', v='%Y'", x, v));

  /* c may be NULL */
  MAY_ASSERT (MAY_TYPE (v) == MAY_STRING_T);
  MAY_ASSERT (d != NULL);

  /* Extract the coefficient and the base */
  if (MAY_LIKELY (MAY_TYPE (x) == MAY_FACTOR_T)) {
    coeff = MAY_AT (x, 0);
    base  = MAY_AT (x, 1);
  /* Trivial numerical case */
  } else if (MAY_PURENUM_P (x)) {
    if (c != NULL)
      *c = x;
    *d = MAY_INT (MAY_ZERO);
    return 1;
  } else {
    coeff = MAY_ONE;
    base = x;
  }

  /* Check if x=v. Other pure identifiers are dealt inside the default case */
  if (MAY_UNLIKELY (may_identical (base, v) == 0)) {
    if (c != NULL)
      *c = coeff;
    *d = MAY_INT (MAY_ONE);
    return 1;
  }

  /* Int Power of v */
  if (MAY_LIKELY (MAY_TYPE (base) == MAY_POW_T
                  && may_identical (MAY_AT (base, 0), v) == 0)) {
    base = MAY_AT (base, 1);
    if (MAY_LIKELY (MAY_TYPE (base) == MAY_INT_T)) {
      if (c != NULL)
        *c = coeff;
      *d = MAY_INT (base);
      return 1;
    }
    /* Non integer power of 'v' found (x^(1/2) for example) */
    return 0;
  }

  /* Product of power ? */
  if (MAY_LIKELY (MAY_TYPE (base) == MAY_PRODUCT_T)) {
    may_size_t i, n;
    n = MAY_NODE_SIZE(base);
    MAY_ASSERT (n != 0);
    for (i = 0; i < n; i++) {
      may_t localbase = MAY_AT (base, i);
      /* Check for v or v^INTEGER */
      if (may_identical (localbase, v) == 0
          || (MAY_TYPE (localbase) == MAY_POW_T
              && MAY_TYPE (MAY_AT (localbase, 1)) == MAY_INT_T
              && may_identical (MAY_AT (localbase, 0), v) == 0)) {
        /* Found! */
        /* Check if what remains is independent... */
        for (may_size_t j = i+1; j<n; j++)
          if (MAY_UNLIKELY (!may_independent_p (MAY_AT (base, j), v)))
            return 0;
        /* Fill in output */
        if (c != NULL) {
          /* may_divexact is overkill (The function is more low level):
             Moreover the sort of the product can be kept,
             and the expand flag can be kept too.
             ==> We may short-circuit eval & expand.
             Moreover this function may be called by may_divexact through
             may_div_qr. */
          may_t cc;
          MAY_ASSERT (n >= 2);
          if (n == 2) {
            cc = MAY_AT (base, 1-i);
          } else {
            cc  = MAY_NODE_C(MAY_PRODUCT_T, n-1);
            memcpy (MAY_AT_PTR(cc, 0), MAY_AT_PTR(base, 0),
                    sizeof (may_t) * i);
            memcpy (MAY_AT_PTR(cc, i), MAY_AT_PTR(base, i+1),
                    sizeof (may_t) * (n-i-1));
            /* FIXME: Num flag may be wrong */
            MAY_CLOSE_C (cc, MAY_FLAGS (base),
                     may_node_hash (MAY_AT_PTR (cc, 0), MAY_NODE_SIZE(cc)));
            /* If expression was expanded, it remain expanded */
            MAY_SET_FLAG(cc, MAY_FLAGS (base));
          }
          if (coeff != MAY_ONE) {
            may_t cc2 = MAY_NODE_C (MAY_FACTOR_T, 2);
            MAY_SET_AT(cc2, 0, coeff);
            MAY_SET_AT(cc2, 1, cc);
            MAY_CLOSE_C (cc2, MAY_FLAGS (cc), MAY_NEW_HASH2 (coeff, cc));
            MAY_SET_FLAG(cc2, MAY_FLAGS (cc));
            cc = cc2;
          }
          *c = cc;
        }
        *d = MAY_TYPE (localbase) != MAY_POW_T
          ? MAY_INT (MAY_ONE) : MAY_INT (MAY_AT (localbase, 1));
        return 1;
      }
      /* Not 'v' or 'v^INT': Checks that it doesn't depend of 'v' */
      if (MAY_UNLIKELY (!may_independent_p (localbase, v)))
        return 0;
    }
    if (c != NULL)
      *c = x;
    *d = MAY_INT (MAY_ZERO);
    return 1; /* we known x is independent of v */
  }

  /* Default case. Degree 0 if independent of v. Impossible otherwise */
  if (c != NULL)
    *c = x;
  *d = MAY_INT (MAY_ZERO);
  return may_independent_p (base, v);
}

/* Personnal version of the independent function.
   With a list of variables to tests: x must be independent of all variables in 'list' */
static int
may_independent_vp2 (may_t x, may_size_t nl, const may_t list[])
{
  may_size_t i, n;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (nl >= 1);

  if (MAY_ATOMIC_P (x)) {
    if (MAY_TYPE (x) == MAY_STRING_T) {
      for (i = 0; i < nl; i++) {
        if (strcmp (MAY_NAME (x), MAY_NAME (list[i])) == 0)
          return 0;
      }
    }
    return 1;
  }
  n = MAY_NODE_SIZE(x);
  MAY_ASSERT (n >= 1);
  for (i = 0; i < n; i++) {
    if (!may_independent_vp2 (MAY_AT (x, i), nl, list))
      return 0;
  }
  return 1;
}

/* Extract the multi-degree and the coefficient of a monomial.
   d and v are arrays of size n. v must be sorted according to the
   current order in the potential product 'x' (ie. sorted against may_identical) */
MAY_INLINE int
may_extract_coeff_multideg2 (may_t *c, mpz_srcptr d[],
                             may_t x, may_size_t n,
			     const may_t v[n],
                             may_t tempsum[n])
{
  may_t base;
  may_t *p;
  mpz_srcptr z;
  may_size_t i, j, m, ntemp;

  MAY_ASSERT (d != NULL && v != NULL && tempsum != NULL);
  MAY_ASSERT (!may_zero_fastp (x));
  MAY_ASSERT (n >= 2);

  /* Trivial numerical case */
  if (MAY_UNLIKELY (MAY_PURENUM_P (x))) {
    if (c != NULL)
      *c = x;
    z = MAY_INT (MAY_ZERO);
    for (i = 0; i < n; i++) {
      d[i] = z;
    }
    return 1;
  }

  /* Extract the coefficient and the base */
  base = MAY_LIKELY (MAY_TYPE (x) == MAY_FACTOR_T) ? MAY_AT (x, 1) : x;
  if (MAY_LIKELY (MAY_TYPE (base) == MAY_PRODUCT_T)) {
    p = MAY_AT_PTR (base, 0);
    m = MAY_NODE_SIZE(base);
  } else {
    p = &base;
    m = 1;
  }

  /* Main loop: Compare the sorted array 'product' to the sorted variable 'v' */
  MAY_ASSERT (m >= 1);
  for (i = j = ntemp = 0; i < m && j < n; /* void */) {
    may_t local = p[i];
    may_t deg  = MAY_ONE;
    int k;
    if (MAY_LIKELY (MAY_TYPE (local) == MAY_POW_T
                    && MAY_TYPE (MAY_AT (local, 1)) == MAY_INT_T)) {
      deg    = MAY_AT (local, 1);
      local  = MAY_AT (local, 0);
    }

    k = may_identical (local, v[j]);
    if (MAY_LIKELY (k == 0)) {
      d[j] = MAY_INT (deg);
      MAY_ASSERT (ntemp < n);
      tempsum[ntemp++] = p[i];
      j++;
      i++;
    } else if (k > 0) {
      d[j] = MAY_INT (MAY_ZERO);
      j++;
      /* We can have local which depends on the other variables
         But it can depends on them polynomially or not!
         Hoppefuly the polynomial case reduces to the variable case.
         That's why we have to treat only the case variable in a different way
      */
      if (MAY_UNLIKELY (!may_independent_vp2 (local,
                        MAY_TYPE (local) != MAY_STRING_T ? n : j, v)))
        return 0;
    } else {
      i++;
      /* We can't have the previous case */
      if (MAY_UNLIKELY (!may_independent_vp2 (local, n, v)))
        return 0;
    }
  }

  /* Finish it: fill in with zeros and check for independence of the remaining terms */
  z = MAY_INT (MAY_ZERO);
  for ( ; j < n; j++)
    d[j] = z;
  for ( ; i < m; i++)
    if (MAY_UNLIKELY (!may_independent_vp2 (p[i], n, v)))
      return 0;
  MAY_ASSERT (i == m && j == n && ntemp <= n);

  /* Compute coeff part if needed */
  if (MAY_LIKELY (c != NULL)) {
    MAY_RECORD ();
    base = may_eval (may_mul_vc (ntemp, tempsum));
    base = may_divexact (x, base);
    MAY_COMPACT (base);
    MAY_ASSERT (may_independent_vp2 (base, n, v));
    *c =  base;
  }
  return 1;
}

/* Extract the multi-degree and the coefficient of a monomial:
   + x is the monomial to decompose into c*PRODUCT(var[index_var[i]]^deg[i],i=0..(n-1))
   + n is the number of variables (>= 2).
   + var[] is the variable array containing the var to decompose sorted against may_identical (ie. for all i, may_identical (var[i], var[i+1]) < 0)
   + index_var[] is a permutation of [0...(n-1)] used to define the original sort of the variable (to store deg).
   + tempdeg is a buffer used internally (it is a parameter to avoid an allocation inside the function).
   + tempsum is a buffer used internally (it is a parameter to avoid an allocation inside the function). */
int
may_extract_coeff_multideg (may_t *c, mpz_srcptr deg[],
                            may_t x,
                            may_size_t n,
			    const may_t var[n], const may_size_t index_var[n],
                            mpz_srcptr tempdeg[n], may_t tempsum[n])
{
  MAY_ASSERT (n >= 2);

  /* index_var must be a parition of [0..(numvar-1)].
     var must be sorted according to may_identical. */
  if (MAY_UNLIKELY (may_extract_coeff_multideg2 (c, tempdeg, x, n, var, tempsum) == 0))
    return 0;

  may_size_t i;
  for(i = 0; i < n; i++) {
    MAY_ASSERT (index_var[i] < n);
    deg[index_var[i]] = tempdeg[i];
  }
  return 1;
}

