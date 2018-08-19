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

/* Level: 0 -> +
   Level: 1 -> * .
   Level: 2 -> ^
   Level: 3 -> Func
   Level: 4 -> Int
*/
/* This function should not provide any assertion failure for invalid input */
MAY_REGPARM size_t
may_dump_rec (FILE *f, const may_t x, int p)
{
  size_t l = 0;

#ifndef MAY_WANT_THREAD
  if ((char*)x < may_g.Heap.base || (char*) x >= may_g.Heap.top) {
    return fprintf (f, "@@INVALID ADDRESS@@(%p)", x);
  }
#endif
  if ((((unsigned long)x) % sizeof(long)) != 0) {
    return fprintf (f, "@@UNALIGNED ADDRESS@@(%p)", x);
  }

  switch (MAY_TYPE(x))
    {
    case MAY_INT_T:
      l += mpz_out_str ( f, may_g.frame.base, MAY_INT(x));
      break;
    case MAY_RAT_T:
      l += mpq_out_str ( f, may_g.frame.base, MAY_RAT(x));
      break;
    case MAY_FLOAT_T:
      l += mpfr_out_str (f, may_g.frame.base, 0, MAY_FLOAT(x), MAY_RND);
      break;
    case MAY_COMPLEX_T:
      if (p!=0)
	l += fprintf(f,"(");
      if (!MAY_PURENUM_P (MAY_RE(x)) || !may_num_zero_p (MAY_RE (x)))
	{
	  l+=may_dump_rec (f, MAY_RE (x), 1);
	  if (!MAY_PURENUM_P (MAY_IM(x)) || !may_num_neg_p (MAY_IM(x)))
	    l += fprintf(f,"+");
	}
      if (!MAY_PURENUM_P (MAY_IM(x)) || !may_num_zero_p (MAY_IM (x))) {
        l+=may_dump_rec (f, MAY_IM(x), 1);
        l += fprintf(f,".I");
      }
      if (p!=0)
	l += fprintf(f,")");
      break;
    case MAY_STRING_T:
      l += fprintf(f,"%s", MAY_NAME(x));
      break;

    case MAY_EXP_T:
    case MAY_LOG_T:
    case MAY_SIN_T:
    case MAY_COS_T:
    case MAY_TAN_T:
    case MAY_ASIN_T:
    case MAY_ACOS_T:
    case MAY_ATAN_T:
    case MAY_SINH_T:
    case MAY_COSH_T:
    case MAY_TANH_T:
    case MAY_ASINH_T:
    case MAY_ACOSH_T:
    case MAY_ATANH_T:
    case MAY_ABS_T:
    case MAY_SIGN_T:
    case MAY_FLOOR_T:
    case MAY_CONJ_T:
    case MAY_REAL_T:
    case MAY_IMAG_T:
    case MAY_ARGUMENT_T:
    case MAY_GAMMA_T:
      l += fprintf(f,"%s(", may_get_name (x) );
      l += may_dump_rec(f, MAY_AT(x,0),0);
      l += fprintf(f,")");
      break;
    case MAY_MOD_T:
    case MAY_GCD_T:
    case MAY_RANGE_T:
      l += fprintf(f,"%s(", may_get_name (x) );
      l+=may_dump_rec (f, MAY_AT (x, 0), 0);
      l+=fprintf (f, ",");
      l+=may_dump_rec (f, MAY_AT (x, 1), 0);
      l+=fprintf (f, ")");
      break;
    case MAY_DIFF_T:
      {
        may_size_t i, n = MAY_NODE_SIZE(x);
        l += fprintf(f,"diff(");
        for (i = 0; i < n; i++) {
          l+=may_dump_rec (f, MAY_AT (x, i), 0);
          if (i != (n-1))
            l+=fprintf (f, ",");
        }
        l+=fprintf (f, ")");
        break;
      }

    case MAY_SUM_T:
    case MAY_SUM_RESERVE_T:
      {
        may_size_t i, n = MAY_NODE_SIZE(x);
        if (n != 0) {
          if (p!=0)
            l += fprintf(f,"(");
          for (i = 0; i < n ; i++) {
            l+=may_dump_rec (f, MAY_AT(x, i), 1);
            if (i!=n-1)
              l += fprintf(f," + ");
          }
          if (p!=0)
            l += fprintf(f,")");
        }else{
          l+=fprintf(f,"@@EMPTY SUM@@");
        }
      }
      break;
    case MAY_PRODUCT_T:
    case MAY_PRODUCT_RESERVE_T:
      {
        may_size_t i, n = MAY_NODE_SIZE(x);
        if (n!=0) {
          if (p>1) l += fprintf(f,"(");
          for (i = 0; i < n ; i++) {
            l+=may_dump_rec (f, MAY_AT(x, i), 2);
            if (i!=n-1)
              l += fprintf(f," * ");
          }
          if (p>1) l += fprintf(f,")");
        }else{
          l+=fprintf(f,"@@EMPTY PRODUCT@@");
        }
      }
      break;
    case MAY_POW_T:
      if (MAY_NODE_SIZE(x) != 2) {
        l+=fprintf(f,"@@INVALID POWER[%u]@@", MAY_NODE_SIZE(x));
      } else {
        if (p>2) l += fprintf(f,"(");
        l += may_dump_rec (f, MAY_AT(x, 0), 3);
        l += fprintf(f," ^ ");
        l += may_dump_rec (f, MAY_AT(x, 1), 3);
        if (p>2) l += fprintf(f,")");
      }
      break;
    case MAY_FACTOR_T:
      if (MAY_NODE_SIZE(x) != 2) {
        l+=fprintf(f,"@@INVALID FACTOR[%u]@@", MAY_NODE_SIZE(x));
      } else {
        if (p>1) l += fprintf(f,"(");
        l += may_dump_rec (f, MAY_AT(x, 0), 2);
        l += fprintf(f," . ");
        l += may_dump_rec (f, MAY_AT(x, 1), 2);
        if (p>1) l += fprintf(f,")");
      }
      break;
    case MAY_FUNC_T:
      l += fprintf(f,"%s(", MAY_NAME(MAY_AT(x,0)));
      l += may_dump_rec(f, MAY_AT(x, 1), 0);
      l += fprintf(f,")");
      break;

    case MAY_INDIRECT_T:
      l += fprintf (f, "@INDIRECT@[");
      l += may_dump_rec (f, MAY_INDIRECT (x), 0);
      l += fprintf (f, "]");
      break;

    default:
      /* Can't use MAY_EXT_P otherwise we can get some assertion failure */
      if (MAY_TYPE (x) <= MAY_END_LIMIT+may_c.extension_size
          && MAY_TYPE (x) > MAY_END_LIMIT) {
        l += fprintf (f, "%s[", MAY_EXT_GETX (x)->name);
        may_size_t i, n = MAY_NODE_SIZE(x);
	for ( i = 0 ; i < n ; i++) {
          l+=may_dump_rec(f, MAY_AT(x, i), 0);
          if (i != (n-1))
            l += fprintf (f, ",");
        }
        l += fprintf (f, "]");
      } else
        l += fprintf (f, "@@INVALID TOKEN %02X@@", (unsigned int) MAY_TYPE(x));
      break;

    }
  return l;
}

void
may_dump (may_t x)
{
  may_dump_rec (stdout, x, 0);
  putchar ('\n');
}

