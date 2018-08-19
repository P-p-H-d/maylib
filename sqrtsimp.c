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

/* BUG: -14^(1/2)*15^(1/2)+6^(1/2)*35^(1/2) is not recognized as a 0
   Solution: Add to the base for the pass 2, the GCDs of the pairs
   of the current base (Removing GCD=1 and the redundancy).
 */

/* Return X, Y, Z such that (sqrt(X)+sqrt(Y))^2/Z = A+sqrt(B)
   Return 1 if succeed,
   Return 0 if fail.
   sqrt(B) must be algrebic */
static int
solve_sqrt_of_sum_of_sqrt (mpz_ptr x, mpz_ptr y, mpz_ptr z,
                           mpz_srcptr a, mpz_srcptr b)
{
  /* (sqrt(X)+sqrt(Y))^2/Z = (X+Y+sqrt(4*X*Y))/Z = A+sqrt(B)
     and X > Y
     ie  { X+Y = A*Z and X*Y = B*Z^2/4
     For z = 1 to 2 do
        if (B*z^2 % 4 != 0)
           continue;
        Solve X^2-S*X+P with S=X+Y=A*Z and P=X*Y=B*Z^2/4
          delta = isqrt(S^2-4*P)
          if delta non integer || delta < 0 || S+delta non even
             continue
          X = (S+delta)/2
          Y = (S-delta)/2
          return 1
     return 0
  */
  unsigned int zi;

  /* We only check for z=1 or 2. In practice, it is sufficient */
  for (zi = 1; zi <= 2 ;zi++) {
    mpz_mul_ui (z, b, zi*zi);
    if (mpz_fdiv_r_ui (x, z, 4) != 0)
      continue;
    mpz_mul_ui (x, a, zi);
    mpz_mul (y, x, x);
    mpz_sub (y, y, z);
    if (mpz_sgn (y) < 0 || !mpz_perfect_square_p (y))
      continue;
    mpz_sqrt (y, y);
    mpz_add (z, x, y);
    if (!mpz_even_p (z))
      continue;
    mpz_fdiv_q_2exp (z, z, 1);
    mpz_sub (x, x, y);
    if (mpz_sgn (x) < 0)
      continue;
    mpz_fdiv_q_2exp (x, x, 1);
    mpz_set (y, z);
    mpz_swap (x, y); /* X > Y */
    mpz_set_ui (z, zi);
    MAY_ASSERT (mpz_cmp (x, y) >= 0);
    return 1;
  }
  return 0;
}

static int
isqrt_p (may_t x)
{
  int ret = MAY_TYPE (x) == MAY_POW_T
    && MAY_TYPE (MAY_AT (x, 0)) == MAY_INT_T
    && may_identical (MAY_AT (x, 1), MAY_HALF) == 0;
  return ret;
}

/* Compute sqrt(A+sqrt(B)) */
static may_t
compute_sqrt_of_sum_of_sqrt (may_t sum)
{
  mpz_t x, y, z, a, b;
  may_t ret;
  int neg = 0;

  MAY_ASSERT (MAY_TYPE (sum) == MAY_SUM_T);
  MAY_ASSERT (MAY_NODE_SIZE(sum) == 2);
  MAY_ASSERT (MAY_NUM_P (sum));
  MAY_ASSERT (MAY_EVAL_P (sum));

  MAY_RECORD ();
  /* INT+sqrt(INT), INT+INT*sqrt(INT), sqrt(INT)+INT or INT*sqrt(INT)+INT */
  /* Extract A & B */
  if (MAY_TYPE (MAY_AT (sum, 0)) == MAY_INT_T) {
    mpz_init_set (a, MAY_INT (MAY_AT (sum, 0)));
    if (isqrt_p (MAY_AT (sum, 1))) {
      mpz_init_set (b, MAY_INT (MAY_AT (MAY_AT (sum, 1), 0)));
    } else if (MAY_TYPE (MAY_AT (sum, 1)) == MAY_FACTOR_T
               && MAY_TYPE (MAY_AT (MAY_AT (sum, 1), 0)) == MAY_INT_T
               && isqrt_p (MAY_AT (MAY_AT (sum, 1), 1))) {
      mpz_init_set (b, MAY_INT (MAY_AT (MAY_AT (MAY_AT (sum, 1), 1),0)));
      mpz_mul (b, b, MAY_INT (MAY_AT (MAY_AT (sum, 1), 0)));
      mpz_mul (b, b, MAY_INT (MAY_AT (MAY_AT (sum, 1), 0)));
      neg = mpz_sgn (MAY_INT (MAY_AT (MAY_AT (sum, 1), 0))) < 0;
    } else {
      MAY_CLEANUP ();
      return NULL;
    }
  }
  MAY_ASSERT (!MAY_PURENUM_P (MAY_AT (sum, 1)));
  /*  else if (MAY_TYPE (MAY_AT (sum, 1)) == MAY_INT_T) {
    mpz_init_set (a, MAY_INT (MAY_AT (sum, 1)));
    if (isqrt_p (MAY_AT (sum, 0))) {
      mpz_init_set (b, MAY_INT (MAY_AT (MAY_AT (sum, 0), 0)));
    } else if (MAY_TYPE (MAY_AT (sum, 1)) == MAY_FACTOR_T
               && MAY_TYPE (MAY_AT (MAY_AT (sum, 0), 0)) == MAY_INT_T
               && isqrt_p (MAY_AT (MAY_AT (sum, 0), 1))) {
      mpz_init_set (b, MAY_INT (MAY_AT (MAY_AT (MAY_AT (sum, 0), 1), 0)));
      mpz_mul (b, b, MAY_INT (MAY_AT (MAY_AT (sum, 0), 0)));
      mpz_mul (b, b, MAY_INT (MAY_AT (MAY_AT (sum, 0), 0)));
      neg = mpz_sgn (MAY_INT (MAY_AT (MAY_AT (sum, 0), 0))) < 0;
    } else {
      MAY_CLEANUP ();
      return NULL;
    }
    }*/
  /* Solve the problem */
  mpz_init (x);
  mpz_init (y);
  mpz_init (z);
  if (!solve_sqrt_of_sum_of_sqrt (x, y, z, a, b)) {
    MAY_CLEANUP ();
    return NULL;
  }
  /* Construct the solution */
  /* Ret = (sqrt(x)+sqrt(y))/sqrt(z)
         = sqrt(x)*sqrt(z)/z+sqrt(y)*sqrt(z)/z  if neg = 0
         = sqrt(x)*sqrt(z)/z-sqrt(y)*sqrt(z)/z  if neg = 0
  */
  ret = MAY_MPZ_NOCOPY_C (z);
  ret = may_div_c (may_sqrt_c (ret), ret);
  ret = (neg == 0 ? may_add_c : may_sub_c)
    (may_mul_c (may_sqrt_c (MAY_MPZ_NOCOPY_C (x)), ret),
     may_mul_c (may_sqrt_c (MAY_MPZ_NOCOPY_C (y)), ret));
  MAY_RET_EVAL (ret);
}

static may_t
is_sqrt_inside_sum (may_t sum)
{
  may_t found;
  may_size_t i, n, count;
  MAY_ASSERT (MAY_TYPE (sum) == MAY_SUM_T);
  n = MAY_NODE_SIZE(sum);
  found = NULL;
  for (i = count = 0; MAY_LIKELY (i < n); i++) {
    may_t z = MAY_AT (sum, i);
    MAY_ASSERT (MAY_EVAL_P (z));
    if ((MAY_TYPE (z) == MAY_POW_T
         && may_identical (MAY_AT (z, 1), MAY_HALF) == 0)
        || (MAY_TYPE (z) == MAY_FACTOR_T
            && MAY_TYPE (MAY_AT (z, 1)) == MAY_POW_T
            && may_identical (MAY_AT (MAY_AT (z, 1), 1), MAY_HALF) == 0))
      found = z, count++;
  }
  /* We can't simplify by conjugate value if there are more than 2 sqrt
     since we won't reduce the number of sqrt. There is still a way to
     remove the denominator from the sqrt, but the resulting expression is
     usually much more larger */
  return count > 2 ? NULL : found;
}

static may_t
may_sqrtsimp_recur_c (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  switch (MAY_TYPE (x)) {
  case MAY_INT_T ... MAY_ATOMIC_LIMIT:
    return x; /* Return the original value */
  case MAY_POW_T:
    /* Check if expo is a RATIONAL */
    if (MAY_TYPE (MAY_AT (x, 1)) == MAY_RAT_T) {
      mpq_ptr expo = MAY_RAT (MAY_AT (x, 1));
      may_t base = may_sqrtsimp_recur_c (MAY_AT (x, 0));
      if (mpz_cmpabs_ui (mpq_numref (expo), 1) != 0) {
        /* may_sqrtsimp (base^(sign/denom))^num */
        mpq_t q;
        may_t new, num;
        mpq_init (q);
        mpz_set_si (mpq_numref (q), mpz_sgn (mpq_numref (expo)));
        mpz_set (mpq_denref (q), mpq_denref (expo));
        new = MAY_MPQ_NOCOPY_C (q);
        new = may_pow_c (base, new);
        new = may_eval (new);
        num = MAY_MPZ_NOCOPY_C (mpq_numref (expo));
        mpz_abs (MAY_INT (num), MAY_INT (num));
        return may_pow_c (may_sqrtsimp_recur_c (new), num);
      } else if (mpq_sgn (expo) < 0 && MAY_TYPE (base) == MAY_INT_T) {
        /* INT^(-1/expo) -> (sqtrsimp(INT^(1/expo)))^(expo-1)/INT */
        mpq_t q;
        may_t new, num;
        mpq_init (q);
        mpz_set_ui (mpq_numref (q), 1);
        mpz_set (mpq_denref (q), mpq_denref (expo));
        new = MAY_MPQ_NOCOPY_C (q);
        new = may_eval (may_pow_c (base, new));
        new = may_sqrtsimp_recur_c (new);
        num = MAY_MPZ_NOCOPY_C (mpq_denref (expo));
        new = may_div_c (may_pow_c (new, may_sub_c (num, MAY_ONE)),
                         base);
        new = may_eval (new);
        return new;
      } else if (MAY_TYPE (base) == MAY_INT_T) {
        /* TODO: Type = rat ? */
        /* INT^(1/expo) --> Try to factorize INT */
        mpz_t q, r, gcd;
        may_t factors = may_naive_ifactor (base);
        mpz_srcptr denom = mpq_denref (expo);
        may_t in = MAY_ONE, out = MAY_ONE, gcdm;
        may_size_t i, n = MAY_NODE_SIZE(factors);
        mpz_init (q);
        mpz_init (r);
        /* Precompute the GCD of all the base of factors and the current
           exponent */
        mpz_init_set (gcd, denom);
        for (i = 0; MAY_LIKELY (i < n); i+=2)
          mpz_gcd (gcd, gcd, MAY_INT (MAY_AT (factors, i)));
        gcdm = MAY_MPZ_NOCOPY_C (gcd);
        /* Extract them */
        for (i = 0; MAY_LIKELY (i < n); i+=2) {
          MAY_ASSERT (MAY_TYPE (MAY_AT (factors, i)) == MAY_INT_T);
          MAY_ASSERT (MAY_TYPE (MAY_AT (factors, i+1)) == MAY_INT_T);
          if (mpz_cmp (MAY_INT (MAY_AT (factors, i)), denom) < 0) {
            in = may_mulinc_c (in, may_pow_c (MAY_AT (factors, i+1),
                                           may_div_c (MAY_AT (factors, i),
                                                      gcdm)));
          } else {
            /* (2^N)^(1/D) with N=qD+r = 2^q*((2^r)^(1/D)) */
            mpz_fdiv_qr (q, r, MAY_INT (MAY_AT (factors, i)), denom);
            out = may_mulinc_c (out, may_pow_c (MAY_AT (factors, i+1),
                                             may_set_z (q)));
            mpz_divexact (r, r, gcd);
            in = may_mulinc_c (in, may_pow_c (MAY_AT (factors, i+1),
                                           may_set_z (r)));
          }
        }
        return may_mul_c (out, may_pow_c (in, may_mul_c (MAY_AT (x, 1),gcdm)));
      } else if (MAY_TYPE (base) == MAY_SUM_T) {
        /* The above functions need an evaluated form */
        may_t z;
        base = may_eval (base);
        if (mpq_sgn (expo) < 0 && (z = is_sqrt_inside_sum (base)) != NULL) {
          /* Multiply by the conjugate value:
             (A+sqrt(B)^-expo =  (A-sqrt(B))^expo/(A^2-B)^expo */
          may_t new_base = may_sub_c (base, may_add_c (z, z));
          may_t new_denom, new;
          new_base = may_eval (new_base); /* A - sqrt (B) */
          new_denom = may_expand (may_mul (base, new_base));
          new = may_div_c (may_pow_c (new_denom, MAY_AT (x, 1)),
                           may_pow_c (new_base, MAY_AT (x, 1)));
          return may_sqrtsimp_recur_c (may_eval (new));
        } else if (MAY_NODE_SIZE(base) == 2 && MAY_NUM_P (base)
                   && ( z = compute_sqrt_of_sum_of_sqrt (base)) != NULL)
          return z;
        return may_pow_c (base, MAY_AT (x,1));
      }
      /* Check if expo is -1, in which case we may need to multiply by the
         conjugate value */
    } else if (may_identical (MAY_AT (x, 1), MAY_N_ONE) == 0) {
      /* Multiply by the conjugate value:
         (A+sqrt(B)^-1 =  (A-sqrt(B))/(A^2-B) */
      may_t z, base;
      base = may_eval (may_sqrtsimp_recur_c (MAY_AT (x, 0)));
      if (MAY_TYPE (base) == MAY_SUM_T) {
        z = is_sqrt_inside_sum (base);
        if (z != NULL) {
          /* Multiply by the conjugate value:
             (A+sqrt(B)^-expo =  (A-sqrt(B))^expo/(A^2-B)^expo */
          may_t new_base = may_sub_c (base, may_add_c (z, z));
          may_t new_denom, new;
          new_base = may_eval (new_base); /* A - sqrt (B) */
          new_denom = may_expand (may_mul (base, new_base));
          new = may_div_c (new_base, new_denom);
          return may_sqrtsimp_recur_c (may_eval (new));
        }
      } else
        return may_pow_c (base, MAY_N_ONE);
    }
    /* Go down */
    /* Falls through. */
  default:
    { /* Call the function recursively */
      may_size_t i, n = MAY_NODE_SIZE(x);
      may_t y;
      int isnew = 0;
      MAY_RECORD ();
      y = MAY_NODE_C (MAY_TYPE (x), n);
      for (i = 0; MAY_LIKELY (i < n); i++) {
        may_t z = may_sqrtsimp_recur_c (MAY_AT (x, i));
        isnew |= (z != MAY_AT(x, i));
        MAY_SET_AT (y, i, z);
      }
      if (isnew) /* If we have changed something, return the new item */
        return y;
      MAY_CLEANUP (); /* Clean used memory */
      return x; /* Return the original value */
    }
  }
}

/* Seconde passe qui fait:
    1. Recherche des irrationnels algébriques de l'expression.
       De la forme ENTIER^RATIONNEL
    2. Calcul du LCM des dénominateurs des exposants trouvés.
    3. Mettre les puissances trouvées au même dénominateur.
       Exemple: LCM=6, 2^(1/3) -> 4->(1/6)
    4. Classer les puissances dans l'ordre des entiers du plus petit au plus grand
    5. Recherche des décompositions des entiers de rang N dans la base [0..N-1
       Pour i de 2 a #ENTIER
        count[#entier] = 0
        Pour j de 1 a i FAIRE
         si tab[j] divide tab[i]
           count[#entier] ++
         sinon
           next j
        Construite l'expression remplacante: Product (entier^count)^(1/lcm)
    6. Si difference, remplacez dans l'expression initiale.
*/

/* Pass 2.1: Search for the irrationnal in the expression x
   and add them in the list. */
static void
sqrtsimp_search_irrational (may_list_t list, may_t x)
{
  if (MAY_ATOMIC_P (x))
    return;
  if (MAY_TYPE (x) == MAY_POW_T
      && MAY_TYPE (MAY_AT (x, 0)) == MAY_INT_T
      && MAY_TYPE (MAY_AT (x, 1)) == MAY_RAT_T) {
    may_list_push_back (list, x);
    return;
  }
  may_size_t i, n = MAY_NODE_SIZE(x);
  for (i = 0; i < n ; i++)
    sqrtsimp_search_irrational (list, MAY_AT (x, i));
  return;
}

/* Pass 2.2: Compute the LCM of the denominators of the exponent of the
   found irrationnals */
static may_t
sqrtsimp_compute_lcm (may_list_t list)
{
  may_size_t i, n = may_list_get_size (list);
  mpz_t lcm;

  MAY_RECORD ();
  mpz_init (lcm);
  mpz_set (lcm, mpq_denref (MAY_RAT (MAY_AT (may_list_at (list, 0), 1))));
  for (i = 1; i < n; i++) {
    mpz_srcptr it = mpq_denref (MAY_RAT (MAY_AT (may_list_at (list, i), 1)));
    mpz_lcm (lcm, lcm, it);
  }
  may_t result = MAY_MPZ_NOCOPY_C  (lcm);
  MAY_RET_EVAL (result);
}

/* Pass 2.3: Replace the base of the irrationnals by its equivalent
   using lcm as exponent */
static int
sqrtsimp_update_denom (may_list_t list, may_t lcm)
{
  may_size_t i, n = may_list_get_size (list);
  mpz_t num;
  mpz_init (num);
  for (i = 0; i < n; i++) {
    may_t it = may_list_at (list, i);
    mpz_srcptr base = MAY_INT (MAY_AT (it, 0));
    mpz_srcptr orga = mpq_numref (MAY_RAT (MAY_AT (it, 1)));
    mpz_srcptr orgb = mpq_denref (MAY_RAT (MAY_AT (it, 1)));
    /* Here we have base^(orga/orgb) to transform in
       ((base)^(orga*lcm/orgb))^(1/lcm) */
    mpz_mul (num, orga, MAY_INT (lcm));
    mpz_divexact (num, num, orgb);
    /* Check for overflow of powering */
    unsigned long e = may_test_overflow_pow_ui (base, num);
    if (e == 0)
      return 0; /* fail */
    mpz_pow_ui (num, base, e);
    may_list_set_at (list, i, may_eval (MAY_MPZ_C (num)));
  }
  return 1; /* success */
}

/* Pass 2.5: Compute the decomposition of the elements in list
   which lcm is lcm */
static void
sqrtsimp_compute_decomp (may_list_t list, may_t lcm)
{
  may_size_t i,j, n=may_list_get_size (list);
  unsigned int count[n];
  mpz_t temp;
  mpz_init (temp);

  for (i = 1; i < n ; i++) {
    mpz_srcptr a = MAY_INT (may_list_at (list, i));
    int decompose = 0;
    memset (count, 0, sizeof count);
    mpz_set (temp, a);
    for (j = 0; j < i; j++) {
      may_t it = may_list_at (list, j);
      /* If j was decomposed, skip it*/
      if (MAY_TYPE (it) != MAY_INT_T)
        continue;
      if (mpz_cmp (temp, MAY_INT (it)) < 0)
        break;
      /* Count the number of times [j] divide [i] */
      while (mpz_divisible_p (temp, MAY_INT (it))) {
        mpz_divexact (temp, temp, MAY_INT (it));
        count[j] ++;
        decompose = 1;
      }
    }
    /* Now we have decompose [i] on its previous base */
    if (decompose) {
      may_t r = MAY_NODE_C (MAY_PRODUCT_T, n+1);
      may_size_t k = 0;
      for (j = 0 ; j < n; j++)
        if (count[j] != 0)
          MAY_SET_AT (r, k++, may_pow_c (may_list_at (list,j),
                                         may_div_c (may_set_ui (count[j]), lcm)));
      MAY_ASSERT (k <= n);
      if (mpz_cmp_ui (temp, 1) != 0)
        MAY_SET_AT (r, k++, may_pow_c (may_set_z (temp),
                                       may_div_c (MAY_ONE, lcm)));
      MAY_NODE_SIZE(r) = k;
      r = may_eval (r);
      may_list_set_at (list, i, r);
    }
  } /* For i */
}

/* Pass 2.6: Replace the original irrationnals by their
   decomposition in the original expression */
static may_t
sqrtsimp_replace_in_org (may_list_t org, may_list_t newi, may_t x)
{
  may_size_t i, n = may_list_get_size (org);
  MAY_ASSERT (may_list_get_size (newi)== n);
  for (i = 0; i<n; i++) {
    may_t r = may_list_at (newi, i);
    if (MAY_TYPE (r) != MAY_INT_T) {
      x = may_replace (x, may_list_at (org, i), r);
    }
  }
  return x;
}

static may_t
sqrtsimp_pass2 (may_t x)
{
  may_list_t list;
  may_list_t orgi;
  may_t lcm;

  may_list_init (list, 0);
  sqrtsimp_search_irrational (list, x);
  if (may_list_get_size (list) <= 1)
    return x;

  /* Save in orgi the original values of the irrationnals */
  may_list_init (orgi, may_list_get_size (list));
  may_list_set (orgi, list);

  lcm = sqrtsimp_compute_lcm (list);
  if (!sqrtsimp_update_denom (list, lcm))
    return x;

  /* Pass 2.4: Sort the list */
  /* Not a fast sort, but who cares? n should be very small */
  {
    may_size_t i, j, k, n = may_list_get_size (list);
    for (i = 0; i < (n-1); i++) {
      k = i;
      for (j = i+1; j < n; j++)
        if (mpz_cmp (MAY_INT (may_list_at (list, k)),
                     MAY_INT (may_list_at (list, j))) > 0)
          k = j;
      /* Swap i and k */
      may_t temp = may_list_at (list, k);
      may_list_set_at (list, k, may_list_at (list, i));
      may_list_set_at (list, i, temp);
      temp = may_list_at (orgi, k);
      may_list_set_at (orgi, k, may_list_at (orgi, i));
      may_list_set_at (orgi, i, temp);
    }
  }
  sqrtsimp_compute_decomp (list, lcm);
  return sqrtsimp_replace_in_org (orgi, list, x);
}

may_t
may_sqrtsimp (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_LOG_FUNC (("%Y", x));

  may_mark();
  x = may_sqrtsimp_recur_c (x);
  x = may_compact (may_eval (x));
  x = sqrtsimp_pass2 (x);
  return may_keep (may_eval (x));
}
