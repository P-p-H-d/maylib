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

/* FIXME: It seems to be VERY buggy. */

may_domain_e
may_get_domain (may_t x)
{
  may_domain_e d, e;
  may_size_t i, n;

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
    if (mpz_sgn (MAY_INT (x)) == 0)
      d = MAY_INT_EVEN_D | MAY_INT_NONPOS_D | MAY_INT_NONNEG_D;
    else {
      d  = mpz_sgn (MAY_INT (x)) > 0 ? MAY_INT_POS_D : MAY_INT_NEG_D;
      d |= mpz_even_p (MAY_INT (x)) ? MAY_INT_EVEN_D : MAY_INT_ODD_D;
    }
    break;
  case MAY_RAT_T:
    d  = MAY_RATIONAL_D;
    d |= mpq_sgn (MAY_RAT (x)) > 0 ? MAY_REAL_POS_D : MAY_REAL_NEG_D;
    break;
  case MAY_FLOAT_T:
    if (mpfr_zero_p (MAY_FLOAT (x)))
      d = MAY_REAL_D;
    else
      d = mpfr_sgn (MAY_FLOAT (x)) > 0 ? MAY_REAL_POS_D : MAY_REAL_NEG_D;
    break;
  case MAY_COMPLEX_T:
    d = may_get_domain (MAY_RE (x));
    e = may_get_domain (MAY_IM (x));
    MAY_ASSERT (MAY_CHECK_DOMAIN(d, MAY_REAL_D));
    MAY_ASSERT (MAY_CHECK_DOMAIN(e, MAY_REAL_D|MAY_NONZERO_D));
    /* There is no way to get the domain execpt setting all the
       major domains by hand. MAY_NONREPOS_D & MAY_NONRENEG_D
       are already set by getting the real domain */
    if (MAY_CHECK_DOMAIN (e, MAY_REAL_POS_D))
      d &= ~MAY_NONIMNEG_D;
    else
      d &= ~MAY_NONIMPOS_D;
    if (MAY_CHECK_DOMAIN (d, MAY_INTEGER_D) && !MAY_CHECK_DOMAIN (e, MAY_INTEGER_D))
      d &= ~MAY_CINTEGER_D;
    if (MAY_CHECK_DOMAIN (d, MAY_RATIONAL_D) && !MAY_CHECK_DOMAIN (e, MAY_RATIONAL_D))
      d &= ~MAY_CRATIONAL_D;
    d &= ~(MAY_EVEN_D|MAY_ODD_D|MAY_PRIME_D);
    d |= MAY_NONZERO_D|MAY_COMPLEX_D;
    break;
  case MAY_STRING_T:
    d = MAY_SYMBOL (x).domain;
    break;
  case MAY_FUNC_T:
    d = MAY_SYMBOL (MAY_AT (x, 0)).domain;
    break;
  case MAY_SUM_T:
    n = MAY_NODE_SIZE(x);
    d = may_get_domain (MAY_AT (x, 0));
    for (i = 1; i < n; i++)
      d &= may_get_domain (MAY_AT (x, i));
    /* Disable odd, even and prime */
    d &= ~(MAY_EVEN_D|MAY_ODD_D|MAY_PRIME_D);
    break;
  case MAY_PRODUCT_T:
  case MAY_FACTOR_T:
    n = MAY_NODE_SIZE(x);
    d = may_get_domain (MAY_AT (x, 0));
    for (i = 1; i < n; i++)
      d &= may_get_domain (MAY_AT (x, i));
    /* Disable odd, even and prime */
    d &= ~(MAY_EVEN_D|MAY_ODD_D|MAY_PRIME_D);
    break;
  case MAY_POW_T:
    /* Easy case:
       domain(base^INT) = domain (base) if INT >= 0
       domain(%) = domain(base) excluding complex integer if INTEGER
       domain(%) = domain(base) = domain (base) if REAL and BASE>=0*/
    d = may_get_domain (MAY_AT (x, 0));
    e = may_get_domain (MAY_AT (x, 1));
    if (MAY_CHECK_DOMAIN (e, MAY_INT_NONNEG_D))
      d &= ~(MAY_EVEN_D|MAY_ODD_D|MAY_PRIME_D);
    else if (MAY_CHECK_DOMAIN (e, MAY_INTEGER_D))
      d &= ~(MAY_CINTEGER_D|MAY_EVEN_D|MAY_ODD_D|MAY_PRIME_D);
    else if (MAY_CHECK_DOMAIN (e, MAY_REAL_D) && MAY_CHECK_DOMAIN (d, MAY_REAL_NONNEG_D))
      d &= ~(MAY_EVEN_D|MAY_ODD_D|MAY_PRIME_D);
    else
      d = MAY_COMPLEX_D;
    break;
  case MAY_EXP_T:
  case MAY_SIN_T:
  case MAY_COS_T:
  case MAY_TAN_T:
    d = may_get_domain (MAY_AT (x, 0));
    d = MAY_CHECK_DOMAIN (d, MAY_REAL_D) ? MAY_REAL_D : MAY_COMPLEX_D;
    break;
  case MAY_REAL_T:
  case MAY_IMAG_T:
    d = may_get_domain (MAY_AT (x, 0)) | MAY_REAL_D;
    break;
  case MAY_ARGUMENT_T:
    /*  Fixme: maybe improve */
    d = MAY_REAL_D;
    break;
  case MAY_ABS_T:
    /* Fixme: Int & rat, but non cint and crat ? */
    d = MAY_REAL_NONNEG_D;
    break;
  default:
    d = MAY_COMPLEX_D;
    break;
  }
  return d;
}
