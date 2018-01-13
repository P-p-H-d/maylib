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

#undef may_set_si
may_t
may_set_si (long d)
{
  return may_mpz_simplify (MAY_SLONG_C (d));
}

#undef may_set_ui
may_t
may_set_ui (unsigned long u)
{
  return may_mpz_simplify (MAY_ULONG_C (u));
}

may_t
may_set_z (mpz_srcptr z)
{
  return may_mpz_simplify (MAY_MPZ_C (z));
}

may_t
may_set_si_c (long d)
{
  return MAY_SLONG_C (d);
}

may_t
may_set_ui_c (unsigned long u)
{
  return MAY_ULONG_C (u);
}

may_t
may_set_z_c (mpz_srcptr z)
{
  return MAY_MPZ_C (z);
}

may_t
may_set_zz (mpz_srcptr z)
{
  may_t oldint = may_g.frame.intmod;
  may_g.frame.intmod = NULL;
  may_t zz = may_mpz_simplify (MAY_MPZ_C (z));
  may_g.frame.intmod = oldint;
  return zz;
}

may_t
may_set_d (double d)
{
  return may_mpfr_simplify (MAY_DOUBLE_C (d));
}

may_t
may_set_ld (long double d)
{
  return may_mpfr_simplify (MAY_LONG_DOUBLE_C (d));
}

/* SPECIAL INTERNAL STRINGS:
   + PI    : The PI number (parsable)
   + #2.34 : Floating Point in base 10 (Unparsable)
   + _L123 : Local variables (parsable)
   + $NUM  : Wildcard
   + NAN / INF / I must be translated into their own representations.
*/
may_t
may_set_str_domain (const char str[], may_domain_e domain)
{
  may_t y;

  /* NAN, INF, I and PI are in upper cases.
     Usually, users set their local variables in lower cases.
     Test if the first letter is in range. */
  if (MAY_UNLIKELY (str[0] >= 'I' && str[0] <= 'P')) {
    if (strcmp (str, may_nan_name) == 0)
      return MAY_NAN;
    else if (strcmp (str, may_inf_name) == 0)
      return MAY_P_INF;
    else if (strcmp (str, may_I_name) == 0)
      return MAY_I;
    else if (strcmp (str, may_pi_name) == 0)
      return MAY_PI;
  }
  /* Search for x in the small cache in order to reuse previously computed symbols
     It speeds up the handling of big parsing since it uses less memory and, moreover
     it speeds up its computation (may_identical is faster) */
  for (unsigned int i = 0; i < may_g.frame.cache_set_str_n; i++)
    if (str[0] ==  MAY_NAME (may_g.frame.cache_set_str[i])[0]
        && strcmp (str, MAY_NAME (may_g.frame.cache_set_str[i])) == 0) {
      MAY_ASSERT (domain == MAY_SYMBOL (may_g.frame.cache_set_str[i]).domain);
      return may_g.frame.cache_set_str[i];
    }
  /* Compute y */
  y = MAY_STRING_C (str, domain);

  /* Add y in cache */
  may_g.frame.cache_set_str[may_g.frame.cache_set_str_i] = y;
  may_g.frame.cache_set_str_i = (may_g.frame.cache_set_str_i + 1)%MAY_MAX_CACHE_SET_STR;
  if (may_g.frame.cache_set_str_n < MAY_MAX_CACHE_SET_STR)
    may_g.frame.cache_set_str_n ++;

  return y;
}

may_t
may_set_str (const char str[])
{
  return may_set_str_domain (str, may_g.frame.domain);
}

may_t
may_set_str_local (may_domain_e domain)
{
  /* Use a big enough buffer */
  char Buffer[(CHAR_BIT*sizeof may_c.local_counter)+1];
  unsigned int value = MAY_ATOMIC_ADD(may_c.local_counter, 1);
  sprintf (Buffer, "_L%u", value);
  return may_set_str_domain (Buffer, domain);
}

int
may_str_local_p (may_t x)
{
  char *p;
  return MAY_TYPE (x) == MAY_STRING_T
    && MAY_NAME (x)[0] == '_' && MAY_NAME (x)[1] == 'L'
    && (strtol (MAY_NAME (x)+2, &p, 10), p > MAY_NAME (x) + 2)
    && *p == 0;
}

may_t
may_set_q (mpq_srcptr q)
{
  may_t y = MAY_MPQ_C (q);
  return may_mpq_simplify (y, MAY_RAT (y));
}

may_t
may_set_fr (mpfr_srcptr f)
{
  return may_mpfr_simplify (MAY_MPFR_C (f));
}

may_t
may_set_si_ui (long num, unsigned long denom)
{
  mpq_t q;
  if (MAY_UNLIKELY (denom == 1))
    return may_set_si (num);
  else if (MAY_UNLIKELY (denom == 2)) {
    if (num == 1)
      return MAY_HALF;
    else if (num == -1)
      return MAY_N_HALF;
  } else if (MAY_UNLIKELY (num == 0 && denom == 0))
    return MAY_NAN;
  mpq_init (q);
  mpq_set_si (q, num, denom);
  return may_mpq_simplify (NULL, q);
}

may_t
may_set_cx (may_t r, may_t i)
{
  if (MAY_LIKELY (MAY_TYPE (r) < MAY_COMPLEX_T
                  && MAY_TYPE (i) < MAY_COMPLEX_T))
    return may_cx_simplify (MAY_COMPLEX_C (r, i));
  return may_eval (may_add_c (r, may_mul_c (i, MAY_I)));
}
