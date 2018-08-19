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

/* For better understanding of the prototypes of the static functions */
typedef may_t may_series_t;

/* SERIES(Regular Part in X of at max N, X, N=ORDER)
   Example:
   sin(x) = SERIES(x-1/6*x^3,x,5) + O(x^5) */
#define SERIES_DL(s) (MAY_ASSERT (may_ext_p (s) == may_c.series_ext), may_op(s,0))
#define SERIES_X(s)  (MAY_ASSERT (may_ext_p (s) == may_c.series_ext), may_op(s,1))
#define SERIES_N(s)  (MAY_ASSERT (may_ext_p (s) == may_c.series_ext), may_op(s,2))

/* Return the valuation of s */
static mpz_srcptr
series_val (may_series_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  mpz_srcptr ldeg;
  may_t x = SERIES_X (s);

  /* If the series is zero, return the order of the list as the valuation */
  if (MAY_ZERO_P (SERIES_DL(s)))
    return MAY_INT (SERIES_N(s));
  /* Otherwise get the lowest degree, and return it */
  else if (may_ldegree (NULL, &ldeg, NULL, SERIES_DL (s), 1, &x) == 0)
    may_error_throw (MAY_DIMENSION_ERR, __func__);
  return ldeg;
}

/* Return the quotient order / valuation
   to compute the needed order for a composition of series.
   The valuation is assumed to be > 0 otherwise it throws an error.
   It assumes that the result is representable in an unsigned long  */
static unsigned long
series_composition_order (may_series_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  /* Compute the needed order = floor ((order-1)/ldegree)+1 */
  mpz_srcptr order = MAY_INT (SERIES_N (s));
  mpz_srcptr val   = series_val (s);
  mpz_t z;
  if (mpz_cmp_ui (val, 0) <= 0)
    may_error_throw (MAY_VALUATION_NOT_POS_ERR, __func__);
  mpz_init_set (z, order);
  mpz_sub_ui (z, z, 1);
  mpz_fdiv_q (z, z, val);
  mpz_add_ui (z, z, 1);
  if (!mpz_fits_ulong_p (z))
    may_error_throw (MAY_DIMENSION_ERR, __func__);
  unsigned long n = mpz_get_ui (z);
  mpz_clear (z);
  MAY_ASSERT (n >= 1);
  return n;
}

/* Return the minimum of a and b in dest */
static void
my_mpz_min (mpz_ptr dest, mpz_srcptr a, mpz_srcptr b)
{
  if (mpz_cmp (a, b) < 0)
    mpz_set (dest, a);
  else
    mpz_set (dest, b);
}

/* Return a new evaluated series.
   TODO: Evaluate the needs for expanding expr. It may hurts the performance for no gain.
 */
static may_t
series_c (may_t expr, may_t x, may_t n)
{
  MAY_ASSERT (MAY_EVAL_P (expr) && MAY_EVAL_P (x) && MAY_EVAL_P (n));
  MAY_ASSERT ((MAY_FLAGS (expr) & MAY_EXPAND_F) == MAY_EXPAND_F);
  MAY_ASSERT (MAY_TYPE (x) == MAY_STRING_T);
  MAY_ASSERT (MAY_TYPE (n) == MAY_INT_T);

  may_t s = may_ext_c (may_c.series_ext, 3);
  may_ext_set_c (s, 0, expr);
  may_ext_set_c (s, 1, x);
  may_ext_set_c (s, 2, n);
  return may_hold (s);
}

/* Evaluate a series */
static may_t
series_eval (may_series_t l)
{
  /* Check the arguments */
  if (may_nops (l) != 3)
    may_error_throw (MAY_DIMENSION_ERR, __func__);
  may_t x  = may_eval (SERIES_X (l));
  may_t n  = may_eval (SERIES_N (l));

  /* If x is numerical, returns the evaluated series without the order */
  if (may_num_p (x))
    return may_eval (SERIES_DL (l));
  else if (may_ext_p (x) == may_c.series_ext)
    /* TODO: Implement composition of series */
    may_error_throw (MAY_DIMENSION_ERR, __func__);
  else if (may_get_name (x) != may_string_name || may_get_name (n) != may_integer_name)
    may_error_throw (MAY_DIMENSION_ERR, __func__);

  /* Eval Regular part */
  may_t rp = may_expand (may_eval (SERIES_DL (l)));
  /* FIXME: What to do if rp is a series? */
  return series_c (rp, x, n);
}

/* For a sum of series, just sum the regular part.
   The final order is the minimum of all the orders */
static unsigned long
series_add (unsigned long start, unsigned long end, may_pair_t *tab)
{
  may_pair_t temp[end];
  may_t x, n;

  /* First pass: Get the minimum m, fill temp with Regular Part
     and check if it is the same var */
  memcpy (&temp[0], &tab[0], start*sizeof (may_pair_t));
  x = SERIES_X(tab[start].second);
  n = SERIES_N(tab[start].second);
  for (unsigned long i = start; i < end; i++) {
    temp[i].first  = tab[i].first;
    temp[i].second = SERIES_DL(tab[i].second);
    n = may_num_min (n, SERIES_N(tab[i].second));
    if (may_identical (x, SERIES_X(tab[i].second)) != 0)
      may_error_throw (MAY_DIMENSION_ERR, __func__);
  }

  /* Second pass: compute it */
  may_t expr = may_eval (may_addmul_vc (end, temp));
  may_div_qr_xexp (NULL, &expr, expr, x, n);
  tab[0].first  = MAY_ONE;
  tab[0].second = series_c (may_expand (expr), x, n);
  return 1;
}

/* For a product of series, just product the regular part.
   The final order is the sum of all lowest degree + the minimum of all the sum (order - lowest degree) */
static unsigned long
series_mul (unsigned long start, unsigned long end, may_pair_t *tab)
{
  may_pair_t temp[end];
  may_t x;
  mpz_srcptr ldeg;
  mpz_t n1, n2, n3;

  mpz_init (n1);
  mpz_init (n2);
  mpz_init (n3);

  /* First pass: Get the new order, fill temp with Regular Part
     and check if it is the same var */
  memcpy (&temp[0], &tab[0], start*sizeof (may_pair_t));
  temp[start].first  = tab[start].first;
  temp[start].second = SERIES_DL(tab[start].second);
  x = SERIES_X(tab[start].second);
  ldeg = series_val (tab[start].second);
  mpz_set (n3, ldeg);
  mpz_sub (n1, MAY_INT (SERIES_N(tab[start].second)), ldeg);

  for (unsigned long i = start+1; i < end; i++) {
    temp[i].first  = tab[i].first;
    temp[i].second = SERIES_DL(tab[i].second);
    ldeg = series_val (tab[i].second);
    mpz_add (n3, n3, ldeg);
    mpz_sub (n2, MAY_INT (SERIES_N(tab[i].second)), ldeg);
    my_mpz_min (n1, n1, n2);
    if (may_identical (x, SERIES_X(tab[i].second)) != 0)
      may_error_throw (MAY_DIMENSION_ERR, __func__);
  }
  mpz_add (n1, n3, n1);
  may_t n = may_set_zz (n1);

  /* Second pass: compute it */
  may_t expr = may_eval (may_mulpow_vc (end, temp));
  may_div_qr_xexp (NULL, &expr, expr, x, n);
  tab[0].first  = MAY_ONE;
  tab[0].second = series_c (may_expand (expr), x, n);
  return 1;
}

/* Compute the composition of serie1 o serie2 with serie1 defined
   as the n first term of its DL.
   n = Order of the serie1  (= O(x^n))
   ldeg1 = Valuation of serie1 without taking into account the constant term >=1 (usually 1 or 2)
   serie1 = an array of n elements defining the terms of the serie
   serie2 = the serie
   WARNING: serie1 storage is reuse and is destroyed.
   TODO: Add a flag for optimizing the case where only one per two element of serie1
   it not null (<= Use a precomputed serie2*serie2)
 */
static may_t
series_compo (may_size_t n, may_size_t ldeg1, may_t serie1[n], may_series_t serie2)
{
  MAY_ASSERT (may_ext_p (serie2) == may_c.series_ext);
  MAY_ASSERT (ldeg1 >= 1);

  MAY_RECORD ();

  may_t x          = SERIES_X (serie2);
  mpz_srcptr ldeg2 = series_val (serie2);
  if (mpz_cmp_ui (ldeg2, 1) < 0)
    may_error_throw (MAY_VALUATION_NOT_POS_ERR, __func__);

  /* Compute the resulting order:
     MIN ( (ldeg1-1)*ldeg2+order2, ldeg2*(order1)+1)*/
  mpz_t n1, n2;
  mpz_init_set_si (n1, (long)ldeg1-1);
  mpz_mul (n1, n1, ldeg2);
  mpz_add (n1, n1, MAY_INT (SERIES_N(serie2)));
  mpz_init_set_ui (n2, n);
  mpz_mul (n2,n2,ldeg2);
  mpz_add_ui (n2,n2,1);
  my_mpz_min (n1, n1, n2);
  may_t order = may_set_zz (n1);

  /* Compute the composition while keeping 'order' terms maximum */
  may_t rp = may_set_ui (1);
  for (may_size_t i = 1; i < n ; i++) {
    rp = may_mul (rp, SERIES_DL (serie2));
    may_div_qr_xexp (NULL, &rp, rp, x, order);
    if (may_zero_p (rp)) {
      n = i;
      break;
    }
    serie1[i] = may_mulinc_c (serie1[i], rp);
  }
  rp = may_eval (may_add_vc (n, serie1));
  MAY_RET (series_c (may_expand (rp), x, order));
}

/* Compute exp(series) */
static may_t
series_exp (may_series_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Extract the constant term:
     exp(a+x+...) = exp(a)*exp(x+...) */
  may_t rp = SERIES_DL (s);
  may_t x  = SERIES_X  (s);
  may_t a  = may_replace (rp, x, may_set_ui (0));
  rp = may_expand (may_sub (rp, a));
  
  /* Compute the needed order = floor ((order-1)/ldegree) */
  s = series_c (rp, x, SERIES_N (s));
  unsigned long n = series_composition_order (s);

  /* Compute the list { 1/k! } */
  may_t *list = may_alloc (n * sizeof (may_t));
  mpq_t q;
  mpq_init (q);
  mpq_set_ui (q, 1, 1);
  for (unsigned long i = 0; i < n ; i++) {
    list[i] = may_set_q (q);
    mpz_mul_ui (mpq_denref(q), mpq_denref(q), (i+1));
  }

  /* Composition of both series */
  s = series_compo (n, 1, list, s);

  /* Take into account the constant factor */
  rp = may_expand (may_eval (may_mul_c (may_exp_c (a), SERIES_DL (s))));
  return may_keep (series_c (rp, x, SERIES_N (s)));
}

/* Compute log(series) */
static may_t
series_log (may_series_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Extract the constant term:
     log(a+x+...) = log(a*(1+x/a...)) = log(a)+log(1+x/a...) */
  may_t rp = SERIES_DL (s);
  may_t x  = SERIES_X  (s);
  may_t a  = may_replace (rp, x, may_set_ui (0));
  rp = may_expand (may_div (may_sub (rp, a), a));
  if (may_zero_p (a))
    may_error_throw (MAY_VALUATION_NOT_POS_ERR, __func__);

  /* Compute the needed order = floor ((order-1)/ldegree) */
  s = series_c (rp, x, SERIES_N (s));
  unsigned long n = series_composition_order (s);

  /* Compute the list { (-1)^(k+1)/k } */
  may_t *list = may_alloc (n * sizeof (may_t));
  mpq_t q;
  mpq_init (q);
  mpq_set_ui (q, 1, 1);
  list[0] = may_set_ui (0);
  for (unsigned long i = 1; i < n ; i++) {
    list[i] = may_set_q (q);
    mpz_neg (mpq_numref(q), mpq_numref(q));
    mpz_set_ui (mpq_denref(q), i+1);
  }

  /* Composition of both series */
  s = series_compo (n, 1, list, s);

  /* Take into account the constant factor */
  rp = may_expand (may_eval (may_add_c (may_log_c (a), SERIES_DL (s))));
  return may_keep (series_c (rp, x, SERIES_N (s)));
}

/* compute series^m where m is not an integer */
static may_t
series_pow_dl (may_series_t s, may_t m)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);
  MAY_ASSERT (may_ext_p (m) != may_c.series_ext);

  /* (a0+a1*x+...)^m = a0^m*(1+a1/a0*x+...)^m */
  may_t rp = SERIES_DL (s);
  may_t x  = SERIES_X  (s);
  may_t a  = may_replace (rp, x, MAY_ZERO);
  rp = may_expand (may_div (may_sub (rp, a), a));
  if (may_zero_p (a))
    /* As an extention, we may factor out the lowest term
       and multiply by (x^lowest)^m, but it isn't a laurent series anymore */
    may_error_throw (MAY_VALUATION_NOT_POS_ERR, __func__);

  /* Compute the needed order = floor ((order-1)/ldegree) */
  s = series_c (rp, x, SERIES_N (s));
  unsigned long n = series_composition_order (s);

  /* Compute the list { m*(m-1)*...*(m-k+1)/k! } */
  may_t *list = may_alloc (n * sizeof (may_t));
  list[0] = may_set_ui (1);
  for (unsigned long i = 1; i < n ; i++) {
    /* Next = (m-i+1)*previous/i */
    list[i] = may_div_c (may_mul_c (may_sub_c (m, may_set_ui (i-1)), list[i-1]),
                         may_set_ui (i));
    list[i] = may_eval (list[i]);
  }
  /* Composition of both series */
  s = series_compo (n, 1, list, s);
  /* Take into account the constant factor */
  rp = may_expand (may_eval (may_mul_c (may_pow_c (a, m), SERIES_DL (s))));
  return series_c (rp, x, SERIES_N (s));
}

/* Compute 1/series.
   NOTE: The way it does the inversion is very naive, but is not so unefficient.
   It is quite difficult currently to detect a good and unique way of
   doing the inversion. Each way as its drawback. A good candidate should be
   the Newton way if and only if expand can be optimised to the max...
   There is also the Leonhard Euler used by Ginac */
static may_t
series_inv (may_series_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  /* Extract the lowest degree:
     1/(a0+a1*x+...) = 1/(1+a1*x/a0+...)*1/a0 */
  may_t rp = SERIES_DL(s), x  = SERIES_X (s), a;
  if (may_ldegree (NULL, NULL, &a, rp, 1, &x) == 0)
    may_error_throw (MAY_DIMENSION_ERR, __func__);

#if 1
  /* It may be a good choice if it is multivariate or small
     or if it is quite sparse (1/(1+tan(x)^5)) */
  rp = may_expand (may_div (may_sub (rp, a), a));
  /* Compute the needed order = floor ((order-1)/ldegree) */
  s = series_c (rp, x, SERIES_N (s));
  unsigned long n = series_composition_order (s);
  /* Compute the list { (-1)^k } */
  may_t *list = may_alloc ((n+2) * sizeof (may_t));
  list[0] = MAY_ONE;
  list[1] = MAY_N_ONE;
  for (unsigned long i = 2; i < n ; i++)
    list[i] = list[i-2];
  /* Composition of both series */
  s = series_compo (n, 1, list, s);
#else
  if (!may_one_p (a))
    rp = may_expand (may_add (MAY_ONE, may_div (may_sub (rp, a), a)));
  /* Use Newton iteration to compute the inverse */
  /* Slower due to slow expand for dense polynomial over non integer.
     For example, tan(2+x). */
  /* Faster if rp was an univariate integer polynomial. */
  unsigned long order, order_final;
  if (may_get_ui (&order_final, SERIES_N (s)))
    may_error_throw (MAY_DIMENSION_ERR, __func__);
  order = 2;
  may_t n = may_sub (MAY_ONE, rp);
  do {
    order = MIN (order*2, order_final);
    n = may_sub_c (may_mul_c (MAY_TWO, n), may_mul_c (rp, may_pow_c (n, MAY_TWO)));
    n = may_eval (n);
    may_div_qr_xexp (NULL, &n, n, x, may_set_ui (order));
  } while (order < order_final);
  s = series_c (may_expand (n), x, may_set_ui (order_final));
#endif
  /* Take into account the constant factor */
  if (!may_one_p (a)) {
    rp = may_expand (may_eval (may_div_c (SERIES_DL (s), a)));
    return series_c (rp, x, SERIES_N (s));
  } else
    return s;
}

/* Compute series^z where z is an integer */
static may_t
series_pow_z (may_series_t s, mpz_srcptr z)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  /* Get the inverse */
  if (mpz_cmp_ui (z, 0) < 0)
    s = series_inv (s);
  /* Check if we requested only degree 1 */
  if (mpz_cmpabs_ui (z, 1) <= 0)
    return s;

  /* If regular part = the variable, we can compute it quickly */
  if (may_identical (SERIES_DL(s), SERIES_X(s)) == 0) {
    may_t tmp;
    if (mpz_cmp(z,MAY_INT(SERIES_N(s))) >= 0)
      tmp = MAY_ZERO;
    else
      tmp = may_expand (may_pow (SERIES_X(s), may_set_zz (z)));
    return series_c (tmp, SERIES_X(s),SERIES_N(s));
  }
  /* TODO: If rp(s) = x(s)^something ==> Faster */

  /* Get the absolute value of z */
  mpz_t absz;
  mpz_init_set (absz, z);
  mpz_abs (absz, absz);

  /* Perform binary exponeniation */
  long i = mpz_sizeinbase (absz, 2);
  MAY_ASSERT (i >= 2);
  may_pair_t temp[3];
  temp[0].first = temp[1].first = temp[2].first = may_set_ui (1);
  temp[0].second = temp[1].second = temp[2].second = s;
  series_mul (0, mpz_tstbit (absz, i-2) ? 3 : 2, temp);

  for (i-=3; i>=0; i--) {
    temp[1].second = temp[0].second;
    temp[2].second = s;
    series_mul (0, mpz_tstbit (absz, i) ? 3 : 2, temp);
  }
  return temp[0].second;
}

/* Compute base^expo where either base or expo is a series */
static may_t
series_pow (may_t base, may_t expo)
{
  may_t result;

  may_mark ();

  if (may_ext_p (base) == may_c.series_ext && may_ext_p (expo) == may_c.series_ext) {
    if (may_identical (SERIES_X (base), SERIES_X (expo)) != 0)
      may_error_throw (MAY_DIMENSION_ERR, __func__);
    /* Compute exp(mul(expo,log(base))). */
    may_pair_t temp[2];
    temp[0].first  = temp[1].first = may_set_ui (1);
    temp[0].second = expo;
    temp[1].second = series_log (base);
    series_mul (0, 2, temp);
    result = series_exp (temp[0].second);
  } else if (may_ext_p (expo) == may_c.series_ext) {
    /* a^x=exp(x*ln(a)) */
    result = may_expand (may_mul (SERIES_DL (expo), may_log (base)));
    result = series_exp (series_c (result, SERIES_X (expo), SERIES_N (expo)));
  } else {
    MAY_ASSERT (may_ext_p (base) == may_c.series_ext);
    if (may_get_name (expo) == may_integer_name)
      result = series_pow_z (base, MAY_INT (expo));
    else
      result = series_pow_dl (base, expo);
  }
  result = may_keep (result);
  return result;
}

/* Compute tan(series) */
static may_t
series_tan (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Extract the constant term: */
  may_t rp = SERIES_DL (s);
  may_t x  = SERIES_X  (s);
  may_t a  = may_replace (rp, x, may_set_ui (0));
  rp = may_expand (may_sub (rp, a));

  /* Compute the needed order = floor ((order-1)/ldegree) */
  s = series_c (rp, x, SERIES_N (s));
  unsigned long n = series_composition_order (s);

  /* Compute the list  */
  may_t *list = may_alloc ((n+3) * sizeof (may_t));
  may_t num = MAY_DUMMY, tmp = MAY_DUMMY;
  list[0] = may_set_ui (0);
  list[1] = may_set_ui (1);
  list[2] = may_set_ui (0);

  for (unsigned long i = 3; i < n ; i+=2) {
    /* Compute reverse cross product */
    num = may_num_set (num, MAY_ZERO);
    for (unsigned long j = 1 ; j < (i-1); j+=2) {
      tmp = may_num_mul (tmp, list[j], list[i-1-j]);
      num = may_num_add (num, num, tmp);
    }
    list[i]   = may_num_div (MAY_DUMMY, num, may_set_ui (i));
    list[i+1] = MAY_ZERO;
  }

  /* Composition of both series */
  s = series_compo (n, 1, list, s);

  /* tan(a+x+...) = (tan(a)+tan(x+...))/(1-tan(a)*tan(x)) */
  if (!may_zero_p(a)) {
    may_pair_t temp[2];
    may_t tana = may_tan(a);
    temp[0].first = temp[0].second = MAY_ONE;
    temp[1].first = may_neg (tana);
    temp[1].second = s;
    series_add (1,2,temp);
    may_t invs = series_inv (temp[0].second);
    temp[0].first = temp[1].first = MAY_ONE;
    temp[0].second = tana;
    temp[1].second = s;
    series_add (1,2,temp);
    temp[1].second = invs;
    series_mul(0,2,temp);
    s = temp[0].second;
  }
  return may_keep (s);
}

/* Compute tanh(series) */
static may_t
series_tanh (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Extract the constant term: */
  may_t rp = SERIES_DL (s);
  may_t x  = SERIES_X  (s);
  may_t a  = may_replace (rp, x, may_set_ui (0));
  rp = may_expand (may_sub (rp, a));

  /* Compute the needed order = floor ((order-1)/ldegree) */
  s = series_c (rp, x, SERIES_N (s));
  unsigned long n = series_composition_order (s);

  /* Compute the list  */
  may_t *list = may_alloc ((n+3) * sizeof (may_t));
  may_t num = MAY_DUMMY, tmp = MAY_DUMMY;
  list[0] = may_set_ui (0);
  list[1] = may_set_ui (1);
  list[2] = may_set_ui (0);

  for (unsigned long i = 3; i < n ; i+=2) {
    /* Compute reverse cross product */
    num = may_num_set (num, MAY_ZERO);
    for (unsigned long j = 1 ; j < (i-1); j+=2) {
      tmp = may_num_mul (tmp, list[j], list[i-1-j]);
      num = may_num_add (num, num, tmp);
    }
    num  = may_num_div (num, num, may_set_ui (i));
    list[i]   = may_num_neg (MAY_DUMMY, num);
    list[i+1] = MAY_ZERO;
  }

  /* Composition of both series */
  s = series_compo (n, 1, list, s);

  /* tanh(a+x+...) = (tanh(a)+tanh(x+...))/(1+tanh(a)*tanh(x)) */
  if (!may_zero_p(a)) {
    may_pair_t temp[2];
    may_t tana = may_tanh(a);
    temp[0].first = temp[0].second = MAY_ONE;
    temp[1].first = tana;
    temp[1].second = s;
    series_add (1,2,temp);
    may_t invs = series_inv (temp[0].second);
    temp[0].first = temp[1].first = MAY_ONE;
    temp[0].second = tana;
    temp[1].second = s;
    series_add (1,2,temp);
    temp[1].second = invs;
    series_mul(0,2,temp);
    s = temp[0].second;
  }
  return may_keep (s);
}

static may_t series_cos (may_t);
static may_t
series_sin (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Extract the constant term:
     sin(a+x+...) = sin(a)*cos(x+...)+cos(a)*sin(x+..) */
  may_t rp = SERIES_DL (s);
  may_t x  = SERIES_X  (s);
  may_t a  = may_replace (rp, x, may_set_ui (0));
  rp = may_expand (may_sub (rp, a));
  s = series_c (rp, x, SERIES_N (s));
  if (!may_zero_p (a)) {
    may_pair_t temp[2];
    temp[0].first  = may_sin (a);
    temp[0].second = series_cos (s);
    temp[1].first  = may_cos (a);
    temp[1].second = series_sin (s);
    series_add (0,2,temp);
    return may_keep (temp[0].second);
  }

  /* Compute the needed order = floor ((order-1)/ldegree) */
  unsigned long n = series_composition_order (s);

  /* Compute the list */
  may_t *list = may_alloc ((n+2) * sizeof (may_t));
  mpq_t q;
  mpq_init (q);
  mpq_set_ui (q, 1, 1);
  list[0] = MAY_ZERO;
  for (unsigned long i = 1; i < n ; i+=2) {
    list[i]   = may_set_q (q);
    list[i+1] = MAY_ZERO;
    mpz_neg (mpq_numref(q), mpq_numref(q));
    mpz_mul_ui (mpq_denref(q), mpq_denref(q), (i+1)*(i+2));
  }

  /* Composition of both series */
  s = series_compo (n, 1, list, s);
  return may_keep (s);
}

static may_t
series_cos (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Extract the constant term:
     cos(a+x+...) = cos(a)*cos(x+...)-sin(a)*sin(x+..) */
  may_t rp = SERIES_DL (s);
  may_t x  = SERIES_X  (s);
  may_t a  = may_replace (rp, x, may_set_ui (0));
  rp = may_expand (may_sub (rp, a));
  s = series_c (rp, x, SERIES_N (s));
  if (!may_zero_p (a)) {
    may_pair_t temp[2];
    temp[0].first  = may_cos (a);
    temp[0].second = series_cos (s);
    temp[1].first  = may_neg(may_sin (a));
    temp[1].second = series_sin (s);
    series_add (0,2,temp);
    return may_keep (temp[0].second);
  }

  /* Compute the needed order = floor ((order-1)/ldegree) + 1 */
  unsigned long n = series_composition_order (s);

  /* Compute the list */
  may_t *list = may_alloc ((n+2) * sizeof (may_t));
  mpq_t q;
  mpq_init (q);
  mpq_set_ui (q, 1, 1);
  for (unsigned long i = 0; i < n ; i+=2) {
    list[i]   = may_set_q (q);
    list[i+1] = MAY_ZERO;
    mpz_neg (mpq_numref(q), mpq_numref(q));
    mpz_mul_ui (mpq_denref(q), mpq_denref(q), (i+1)*(i+2));
  }

  /* Composition of both series */
  s = series_compo (n, 2, list, s);
  return may_keep (s);
}

static may_t series_cosh (may_t);
static may_t
series_sinh (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Extract the constant term:
     sinh(a+x+...) = sinh(a)*cosh(x+...)+cosh(a)*sinh(x+..) */
  may_t rp = SERIES_DL (s);
  may_t x  = SERIES_X  (s);
  may_t a  = may_replace (rp, x, may_set_ui (0));
  rp = may_expand (may_sub (rp, a));
  s = series_c (rp, x, SERIES_N (s));
  if (!may_zero_p (a)) {
    may_pair_t temp[2];
    temp[0].first  = may_sinh (a);
    temp[0].second = series_cosh (s);
    temp[1].first  = may_cosh (a);
    temp[1].second = series_sinh (s);
    series_add (0,2,temp);
    return may_keep (temp[0].second);
  }

  /* Compute the needed order = floor ((order-1)/ldegree) */
  unsigned long n = series_composition_order (s);

  /* Compute the list  */
  may_t *list = may_alloc ((n+2) * sizeof (may_t));
  mpq_t q;
  mpq_init (q);
  mpq_set_ui (q, 1, 1);
  list[0] = MAY_ZERO;
  for (unsigned long i = 1; i < n ; i+=2) {
    list[i]   = may_set_q (q);
    list[i+1] = MAY_ZERO;
    mpz_mul_ui (mpq_denref(q), mpq_denref(q), (i+1)*(i+2));
  }

  /* Composition of both series */
  s = series_compo (n, 1, list, s);
  return may_keep (s);
}

static may_t
series_cosh (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Extract the constant term:
     cosh(a+x+...) = cosh(a)*cosh(x+...)+sinh(a)*sinh(x+..) */
  may_t rp = SERIES_DL (s);
  may_t x  = SERIES_X  (s);
  may_t a  = may_replace (rp, x, may_set_ui (0));
  rp = may_expand (may_sub (rp, a));
  s = series_c (rp, x, SERIES_N (s));
  if (!may_zero_p (a)) {
    may_pair_t temp[2];
    temp[0].first  = may_cosh (a);
    temp[0].second = series_cosh (s);
    temp[1].first  = may_sinh (a);
    temp[1].second = series_sinh (s);
    series_add (0,2,temp);
    return may_keep (temp[0].second);
  }

  /* Compute the needed order = floor ((order-1)/ldegree) + 1 */
  unsigned long n = series_composition_order (s);

  /* Compute the list */
  may_t *list = may_alloc ((n+2) * sizeof (may_t));
  mpq_t q;
  mpq_init (q);
  mpq_set_ui (q, 1, 1);
  for (unsigned long i = 0; i < n ; i+=2) {
    list[i]   = may_set_q (q);
    list[i+1] = MAY_ZERO;
    mpz_mul_ui (mpq_denref(q), mpq_denref(q), (i+1)*(i+2));
  }

  /* Composition of both series */
  s = series_compo (n, 2, list, s);
  return may_keep (s);
}

static may_t
series_asin (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Compute the needed order = floor ((order-1)/ldegree) + 1 */
  unsigned long n = series_composition_order (s);

  /* Compute the list product odd / product even */
  may_t *list = may_alloc ((n+2) * sizeof (may_t));
  mpq_t q1, q2;
  mpq_init (q1);
  mpq_init (q2);
  mpq_set_ui (q1, 1, 1);
  for (unsigned long i = 0; i < n ; i+=2) {
    list[i]   = MAY_ZERO;
    mpq_set (q2, q1);
    mpz_mul_ui (mpq_denref(q2), mpq_denref(q2), (i+1));
    list[i+1] = may_set_q (q2);
    mpz_mul_ui (mpq_numref(q1), mpq_numref(q1), (i+1));
    mpz_mul_ui (mpq_denref(q1), mpq_denref(q1), (i+2));
  }

  /* Composition of both series */
  s = series_compo (n, 1, list, s);
  return may_keep (s);
}

static may_t
series_acos (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* acos(x) = PI/2 - asin(x) */
  may_pair_t temp[2];
  temp[0].first  = MAY_HALF;
  temp[0].second = MAY_PI;
  temp[1].first  = MAY_N_ONE;
  temp[1].second = series_asin (s);
  series_add (1,2,temp);
  return may_keep (temp[0].second);
}

static may_t
series_atan (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Compute the needed order = floor ((order-1)/ldegree) + 1 */
  unsigned long n = series_composition_order (s);

  /* Compute the list 0 1 0 -1/3 0 1/5 ... */
  may_t *list = may_alloc ((n+2) * sizeof (may_t));
  mpq_t q;
  mpq_init (q);
  mpq_set_ui (q, 1, 1);
  for (unsigned long i = 0; i < n ; i+=2) {
    list[i]   = MAY_ZERO;
    list[i+1] = may_set_q (q);
    mpz_neg (mpq_numref (q), mpq_numref (q));
    mpz_set_ui (mpq_denref(q), i+3);
  }

  /* Composition of both series */
  s = series_compo (n, 1, list, s);
  return may_keep (s);
}

static may_t
series_asinh (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Compute the needed order = floor ((order-1)/ldegree) + 1 */
  unsigned long n = series_composition_order (s);

  /* Compute the list product odd / product even */
  may_t *list = may_alloc ((n+2) * sizeof (may_t));
  mpq_t q1, q2;
  mpq_init (q1);
  mpq_init (q2);
  mpq_set_ui (q1, 1, 1);
  for (unsigned long i = 0; i < n ; i+=2) {
    list[i]   = MAY_ZERO;
    mpq_set (q2, q1);
    mpz_mul_ui (mpq_denref(q2), mpq_denref(q2), (i+1));
    list[i+1] = may_set_q (q2);
    mpz_neg (mpq_numref(q1), mpq_numref(q1));
    mpz_mul_ui (mpq_numref(q1), mpq_numref(q1), (i+1));
    mpz_mul_ui (mpq_denref(q1), mpq_denref(q1), (i+2));
  }

  /* Composition of both series */
  s = series_compo (n, 1, list, s);
  return may_keep (s);
}

static may_t
series_acosh (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* acosh(x) = I*PI/2 - I*asin(x) */
  may_pair_t temp[2];
  temp[0].first  = may_mul (MAY_HALF, MAY_I);
  temp[0].second = MAY_PI;
  temp[1].first  = may_mul (MAY_N_ONE, MAY_I);
  temp[1].second = series_asin (s);
  series_add (1,2,temp);
  return may_keep (temp[0].second);
}

static may_t
series_atanh (may_t s)
{
  MAY_ASSERT (may_ext_p (s) == may_c.series_ext);

  may_mark ();

  /* Compute the needed order = floor ((order-1)/ldegree) + 1 */
  unsigned long n = series_composition_order (s);

  /* Compute the list 0 1 0 -1/3 0 1/5 ... */
  may_t *list = may_alloc ((n+2) * sizeof (may_t));
  mpq_t q;
  mpq_init (q);
  mpq_set_ui (q, 1, 1);
  for (unsigned long i = 0; i < n ; i+=2) {
    list[i]   = MAY_ZERO;
    list[i+1] = may_set_q (q);
    mpz_set_ui (mpq_denref(q), i+3);
  }

  /* Composition of both series */
  s = series_compo (n, 1, list, s);
  return may_keep (s);
}

static may_t
series_constructor (may_t list)
{
  /* FIXME: An extension must not use MAY_OPEN_C */
  MAY_OPEN_C (list, may_c.series_ext);
  return list;
}

static const may_extdef_t series_cb = {
  .name = "SERIES",
  .priority = 50,
  .eval = series_eval,
  .add = series_add,
  .mul = series_mul,
  .pow = series_pow,
  .exp = series_exp,
  .log = series_log,
  .sin = series_sin,
  .cos = series_cos,
  .tan = series_tan,
  .sinh = series_sinh,
  .cosh = series_cosh,
  .tanh = series_tanh,
  .asin = series_asin,
  .acos = series_acos,
  .atan = series_atan,
  .asinh = series_asinh,
  .acosh = series_acosh,
  .atanh = series_atanh,
  .constructor = series_constructor
};


/*****************************************************************/


/* This function works by replacing the variable 'x' in 'expr'
   by a series SERIES(x, x, m) and performing some computations
   over the series using the previously registered extension */
may_t
may_series (may_t expr, may_t x, unsigned long m)
{
  MAY_LOG_FUNC (("expr='%Y' x='%Y' m=%lu", expr, x, m));

  /* Register the eseries extension if not already done. */
  if (may_c.series_ext == 0) {
    may_c.series_ext = may_ext_register (&series_cb, MAY_EXT_INSTALL);
    if (may_c.series_ext == 0)
      may_error_throw (MAY_DIMENSION_ERR, __func__);
  }

  may_mark ();
  return may_keep (may_replace (expr, x, series_c (x, x, may_set_ui (m))));
}
