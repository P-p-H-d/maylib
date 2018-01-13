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

/* Either it is the first inclusion of this file, and we have to define the functions.
   Or it the second (or third) inclusion, and we have to define
   the template functions (may_degree / may_ldegree) based on:
      MAY_FUNCTION_NAME
      MAY_OPERATOR
*/

#ifndef MAY_FUNCTION_NAME

/* Reinclude this file by defining the may_degree function */
#define MAY_FUNCTION_NAME degree
#define MAY_OPERATOR >
#include "degree.c"

/* Reinclude this file by defining the may_ldegree function */
#define MAY_FUNCTION_NAME ldegree
#define MAY_OPERATOR <
#include "degree.c"

#else

/* Template functions for may_degree and may_ldegree:
   MAY_FUNCTION_NAME is either degree or ldegree
   MAY_OPERATOR is either > or <  */
static int
MAY_CONCACT(may_uni, MAY_FUNCTION_NAME) (may_t *coeff, mpz_srcptr *deg, may_t *leader,
                                         may_t x, may_t v)
{
  may_size_t i, n;
  may_t c, c1, *c1_ptr;
  mpz_srcptr d;

  MAY_ASSERT (MAY_TYPE (v) == MAY_STRING_T);
  MAY_ASSERT (deg != NULL);

  /* If it is not a sum, it is a monomial. */
  if (MAY_UNLIKELY (MAY_TYPE (x) != MAY_SUM_T)) {
    if (leader)
      *leader = x;
    return may_extract_coeff_deg (coeff, deg, x, v);
  }

  /* Let's the fun begin! */
  c1_ptr = coeff != NULL ? &c : NULL;
  if (MAY_UNLIKELY (!may_extract_coeff_deg (c1_ptr, &d, MAY_AT (x, 0), v)))
    return 0;

  n = MAY_NODE_SIZE(x);
  MAY_ASSERT (n >= 2);
  MAY_LOG_MSG(("n=%lu\n", (unsigned long) n));

  may_t sum_coeff[n], sum_leader[n];
  may_size_t nsum = 1;
  sum_coeff[0] = c;
  sum_leader[0] = MAY_AT (x, 0);

  c1 = NULL;
  c1_ptr = coeff != NULL ? &c1 : NULL;

  for (i = 1; i < n; i++) {
    /* Extract the current monomial */
    mpz_srcptr d1;
    int cmp;
    if (MAY_UNLIKELY (!may_extract_coeff_deg (c1_ptr, &d1, MAY_AT (x, i), v)))
      return 0;
    /* Compare degrees. */
    cmp = mpz_cmp (d1, d);
    if (cmp MAY_OPERATOR 0) {
      d = d1;
      sum_coeff[0]  = c1;
      sum_leader[0] = MAY_AT (x, i);
      nsum = 1;
    } else if (MAY_UNLIKELY (cmp == 0)) {
      MAY_ASSERT (nsum < n);
      sum_coeff[nsum] = c1;
      sum_leader[nsum ++] = MAY_AT (x, i);
    }
  }

  /* Eval the returned values */
  if (MAY_LIKELY (coeff != NULL)) {
    may_t tmp = MAY_LIKELY (nsum == 1) ? sum_coeff[0]
      : may_eval (may_add_vc (nsum, sum_coeff));
    MAY_ASSERT (MAY_EVAL_P (tmp));
    *coeff = tmp;
  }
  if (MAY_LIKELY (leader != NULL)) {
    may_t tmp = MAY_LIKELY (nsum == 1) ? sum_leader[0]
      : may_eval (may_add_vc (nsum, sum_leader));
    MAY_ASSERT (MAY_EVAL_P (tmp));
    *leader = tmp;
  }
  *deg   = d;

  return 1;
}

static int
MAY_CONCACT(may_multi, MAY_FUNCTION_NAME) (may_t *coeff, mpz_srcptr deg[], may_t *leader,
                                           may_t x, may_size_t numvar,
                                           const may_t var[numvar])
{
  may_size_t i, n, nsum;
  may_t c;
  may_size_t index_var[numvar];
  may_t new_var[numvar];
  mpz_srcptr d1[numvar];
  mpz_srcptr tempdeg[numvar];
  may_t tempsum[numvar];

  MAY_ASSERT (deg != NULL);
  MAY_ASSERT (numvar >= 2);

  MAY_LOG_MSG(("numvar=%lu\n", numvar));

  /* Sort the vars following the current order.
     Since numvar is small, we can use a N ^2 algo */
  /* FIXME: BUGS for more than 2 vars ? */
  for (i = 0; i < numvar; i++)
    index_var[i] = i;
  for (i = 0; i < numvar; i++) {
    may_size_t j;
    may_size_t minimum = i;
    for (j = i+1; j < numvar; j++)
      if (may_identical (var[index_var[j]], var[index_var[minimum]]) < 0)
        minimum = j;
    swap (index_var[i], index_var[minimum]);
  }
  for (i = 0; i < numvar; i++)
    new_var[i] = var[index_var[i]];

  /* If it is not a sum, it is a monomial. */
  if (MAY_TYPE (x) != MAY_SUM_T) {
    if (leader)
      *leader = x;
    return may_extract_coeff_multideg (coeff, deg,
				       x, numvar, new_var, index_var,
                                       tempdeg, tempsum);
  }

  /* Let's the fun begin! */
  if (!may_extract_coeff_multideg (&c, deg,
				   MAY_AT (x, 0), numvar, new_var, index_var,
                                   tempdeg, tempsum))
    return 0;

  n = MAY_NODE_SIZE(x);
  MAY_ASSERT (n>=2);
  MAY_LOG_MSG(("n=%lu\n", (unsigned long) n));

  may_t sum_coeff[n], sum_leader[n];
  nsum = 1;
  sum_coeff[0] = c;
  sum_leader[0] = MAY_AT (x, 0);

  for (i = 1; i < n; i++) {
    may_t c1;
    int cmp;

    if (!may_extract_coeff_multideg (&c1, d1, MAY_AT (x, i), numvar, new_var,
                                     index_var, tempdeg, tempsum))
      return 0;
    cmp = may_cmp_multidegree (numvar, d1, deg);
    if (cmp MAY_OPERATOR 0) {
      memcpy (deg, d1, sizeof d1);
      sum_leader[0] = MAY_AT (x, i);
      sum_coeff[0] = c1;
      nsum = 1;
    } else if (cmp == 0) {
      MAY_ASSERT (nsum < n);
      sum_leader[nsum] = MAY_AT (x, i);
      sum_coeff[nsum ++] = c1;
    }
  }

  /* Eval the returned values */
  if (coeff != NULL) {
    may_t tmp = MAY_LIKELY (nsum == 1) ? sum_coeff[0]
      : may_eval (may_add_vc (nsum, sum_coeff));
    MAY_ASSERT (MAY_EVAL_P (tmp));
    *coeff = tmp;
  }
  if (leader != NULL) {
    may_t tmp = MAY_LIKELY (nsum == 1) ? sum_leader[0]
      : may_eval (may_add_vc (nsum, sum_leader));
    MAY_ASSERT (MAY_EVAL_P (tmp));
    *leader = tmp;
  }

  return 1;
}

int
MAY_CONCACT(may_, MAY_FUNCTION_NAME) (may_t *coeff, mpz_srcptr deg[], may_t *leader,
                                      may_t x, size_t numvar, const may_t var[])
{
  mpz_srcptr deg_temp[numvar];
  may_t      temp;
  int ret;

  MAY_ASSERT (var != NULL);
  MAY_ASSERT (numvar >= 1);

  MAY_LOG_FUNC (("x='%Y' numvar='%lu' var[0]='%Y'",x,numvar,var[0]));

  MAY_RECORD ();

  /* Do an expand */
  x = may_expand (x);

  /* Easy case (after potential expand !) */
  if (MAY_UNLIKELY (x == MAY_ZERO)) {
    MAY_CLEANUP ();
    if (coeff)
      *coeff = MAY_ZERO;
    if (leader)
      *leader = MAY_ZERO;
    if (deg) {
      may_size_t i;
      /* FIXME: We don't have deg(A*B)=deg(A)+deg(B)
         because we return -1 for 0. How to handle this? */
      for (i = 0; i< numvar; i++)
        deg[i] = MAY_INT (MAY_N_ONE);
    }
    return 1;
  }

  /* Computing deg is not heavy: always compute it */
  if (MAY_UNLIKELY (deg == NULL))
    deg = deg_temp;

  /* Call respective function */
  if (MAY_LIKELY (numvar == 1))
    ret = MAY_CONCACT(may_uni, MAY_FUNCTION_NAME) (coeff, &deg[0], leader, x, var[0]);
  else
    ret = MAY_CONCACT(may_multi,MAY_FUNCTION_NAME) (coeff, deg, leader, x, numvar, var);

  /* Compact and return */
  if (ret == 0) {
    MAY_CLEANUP ();
    return 0;
  }
  temp = NULL;
  if (coeff == NULL)
    coeff = &temp;
  if (leader == NULL)
    leader = &temp;
  MAY_COMPACT_2 (*coeff, *leader);
  return 1;
}

long
MAY_CONCACT(may_, MAY_CONCACT(MAY_FUNCTION_NAME,_si)) (may_t x, may_t v)
{
  mpz_srcptr z;
  int ret;

  ret = MAY_CONCACT(may_, MAY_FUNCTION_NAME) (NULL, &z, NULL, x, 1, &v);
  if (MAY_UNLIKELY (ret == 0) || !mpz_fits_slong_p (z))
    return LONG_MAX;
  else if (mpz_cmp_si (z, -1) == 0)
    return LONG_MIN; /* Return LONG_MIN for x==0 */
  else
    return mpz_get_si (z);
}


#undef MAY_FUNCTION_NAME
#undef MAY_OPERATOR

#endif
