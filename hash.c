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

MAY_REGPARM may_hash_t
may_mpz_hash (mpz_t z)
{
  mp_size_t  n = z->_mp_size;
  mp_limb_t *p = z->_mp_d;
  MAY_HASH_DECL (hash);

  if (MAY_UNLIKELY (n <= 0)) {
    MAY_HASH_UP (hash, 0x7F5C1458);
    if (MAY_UNLIKELY (n == 0))
      return MAY_HASH_GET (hash);
    n = -n;
  } else
    MAY_HASH_UP (hash, 0x587F5C14);

  MAY_ASSERT (n >= 1);
  do {
    MAY_HASH_UP (hash, *p++);
  } while (--n > 0);

  return MAY_HASH_GET (hash);
}

MAY_REGPARM may_hash_t
may_mpq_hash (mpq_t q)
{
  return MAY_NEW_HASH (may_mpz_hash (mpq_numref(q)),
		       may_mpz_hash (mpq_denref(q)));
}

MAY_REGPARM may_hash_t
may_mpfr_hash (mpfr_t f)
{
  mp_limb_t *p = f[0]._mpfr_d;
  mp_size_t  n = (f[0]._mpfr_prec-1)/(sizeof(mp_limb_t)*CHAR_BIT)+1;
  MAY_HASH_DECL (h);

  MAY_HASH_UP (h, f->_mpfr_exp);

  if (MPFR_SIGN (f) < 0)
    MAY_HASH_UP (h, 1);

  /* Special value: NAN, INF and ZERO */
  if (MAY_UNLIKELY(mpfr_nan_p (f) || mpfr_inf_p (f) || mpfr_zero_p (f)))
    return MAY_HASH_GET (h);

  /* Mantissa */
  do {
    MAY_HASH_UP (h, *p++);
  } while (--n > 0);
  return MAY_HASH_GET (h);
}

MAY_REGPARM may_hash_t
may_string_hash (const char p[])
{
  MAY_HASH_DECL (hash);
  char c;

  do {
    c = *p++;
    MAY_HASH_UP (hash, (unsigned int) c);
  } while (c != 0);
  return MAY_HASH_GET (hash);
}

MAY_REGPARM may_hash_t
may_data_hash (const char p[], may_size_t size)
{
  MAY_HASH_DECL (hash);
  char c;

  while (size) {
    c = *p++;
    MAY_HASH_UP (hash, (unsigned int) c);
    size--;
  }
  return MAY_HASH_GET (hash);
}

MAY_REGPARM may_hash_t
may_node_hash (const may_t x[], may_size_t n)
{
  MAY_HASH_DECL (hash);

  MAY_ASSERT (n > 0);

  do {
      MAY_ASSERT (may_recompute_hash (*x) == MAY_HASH (*x));
      MAY_HASH_UP (hash, MAY_HASH (*x));
      x++;
  } while (-- n > 0);
  return MAY_HASH_GET (hash);
}

MAY_REGPARM may_hash_t
may_recompute_hash (may_t x)
{
  struct may_s ms;
  may_hash_t hash;

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
    hash = may_mpz_hash (MAY_INT (x));
    break;
  case MAY_RAT_T:
    hash = may_mpq_hash (MAY_RAT (x));
    break;
  case MAY_FLOAT_T:
    hash = may_mpfr_hash (MAY_FLOAT (x));
    break;
  case MAY_STRING_T:
    hash = may_string_hash (MAY_NAME (x));
    break;
  case MAY_DATA_T:
    hash = may_data_hash ((char*)MAY_DATA (x).data, MAY_DATA (x).size);
    break;
  case MAY_COMPLEX_T:
    hash = MAY_NEW_HASH (may_recompute_hash (MAY_RE (x)),
                         may_recompute_hash (MAY_IM (x)));
    break;
  case MAY_EXP_T ... MAY_UNARYFUNC_LIMIT:
    hash = may_recompute_hash (MAY_AT (x, 0));
    break;
  default:
    hash = may_node_hash (MAY_AT_PTR (x, 0), MAY_NODE_SIZE(x));
    break;
  }
  MAY_OPEN_C  (&ms, MAY_TYPE (x));
  MAY_CLOSE_C (&ms, MAY_FLAGS(x), hash);
#if 0
  if (1) {
    printf ("Previous=%u Recompute=%u for ",
            (unsigned int) MAY_HASH (x),
            (unsigned int) MAY_HASH (&ms));
    may_dump (x);
  }
#endif
  return MAY_HASH (&ms);
}

