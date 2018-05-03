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

static may_t
may_hold_diff (may_t f, may_t v, may_t o, may_t a)
{
  may_t local, z;

  MAY_ASSERT (MAY_EVAL_P (f) && MAY_EVAL_P (v)
              && MAY_EVAL_P (o) && MAY_EVAL_P (a));
  MAY_ASSERT (MAY_TYPE (v) == MAY_STRING_T);

  /* Check if order null ==> f(a) */
  if (may_zero_fastp (o))
    return may_replace (f, v, a);

  /* Replace v in f by a local variable if needed.
     It is a changement in a stub variable,
     ie we don't change the semantic of the expression.
     So we can use may_hold instead of may_eval.
     Moreover we should, otherwise we may get an infinite
     eval recursion due to evaluation of expression like
     "diff(diff(f(x,y),x),y)"                            */
  if (!may_str_local_p (v)) {
    may_c.local_counter = 0;
    do {
      local = may_set_str_local (may_get_domain (v));
    } while (!may_independent_p (f, local));
    f = may_hold (may_replace_c (f, v, local));
  } else
    local = v;
  /* Create a new node */
  z = may_diff_c (f, local, o, a);
  return may_hold (z);
}

static MAY_REGPARM may_t
may_diff_recur (may_t x, may_t vx)
{
  may_t y;
  may_size_t n, i, j;

  if (MAY_UNLIKELY (MAY_NUM_P (x)))
    return MAY_ZERO;

  switch (MAY_TYPE(x))
    {
    case MAY_STRING_T:
      y = may_identical (x, vx) == 0 ? MAY_ONE : MAY_ZERO;
      break;
    case MAY_FACTOR_T:
      y = may_mul_c (MAY_AT (x, 0), may_diff_recur (MAY_AT (x, 1), vx));
      break;
    case MAY_PRODUCT_T:
      /* u*v --> u'*v + u*v' */
      n = MAY_NODE_SIZE(x);
      y = MAY_NODE_C (MAY_SUM_T, n);
      /* TO PARALELIZE */
      for (i = 0; MAY_LIKELY (i < n); i++) {
        may_t z = MAY_NODE_C (MAY_PRODUCT_T, n);
        for (j = 0 ; MAY_LIKELY (j < n); j++)
          MAY_SET_AT (z, j, (i==j) ? may_diff_recur (MAY_AT (x, j), vx) :
                      MAY_AT(x, j));
        MAY_SET_AT (y, i, z);
      }
      break;
    case MAY_POW_T:
      {
	may_t base = MAY_AT (x, 0);
	may_t expo = MAY_AT (x, 1);
        if (may_independent_p (expo, vx)) {
          /* Faster formula: u^n --> n*u^(n-1)*u' */
          y = may_mul_vac (expo,
                           may_pow_c (base, may_sub_c (expo, MAY_ONE)),
                           may_diff_recur (base, vx),
                           NULL);
        } else {
          /* Generic formula : u^v  --> u^v * (v'*ln(u)+v*u'/u) */
          y = may_add_c (may_mul_c (may_diff_recur (expo, vx),
                                    may_log_c (base)),
                         may_mul_c (expo,
                                    may_div_c (may_diff_recur (base, vx),
                                               base)));
          y = may_mul_c (x, y);
        }
      }
      break;
    case MAY_SUM_T:
    case MAY_LIST_T:
      /* u + v --> u' + v' */
      n = MAY_NODE_SIZE(x);
      y = MAY_NODE_C (MAY_TYPE(x), n);
      /* TO PARALELIZE */
      for (i = 0 ; MAY_LIKELY (i < n); i++)
	MAY_SET_AT (y, i, may_diff_recur (MAY_AT(x, i), vx));
      break;
    case MAY_EXP_T:
      /* exp(u) --> u' * exp(u) */
      y = may_mul_c (may_diff_recur (MAY_AT(x,0), vx),
		     may_exp_c (MAY_AT(x,0)));
      break;
    case MAY_LOG_T:
      /* log(u) --> u' / u */
      y = may_div_c (may_diff_recur (MAY_AT(x,0), vx), MAY_AT(x,0));
      break;
    case MAY_COS_T:
      /* cos(u) --> -u'*sin(u) */
      y = may_mul_vac (MAY_N_ONE,
                       may_diff_recur (MAY_AT(x,0), vx),
                       may_sin_c (MAY_AT(x,0)), NULL);
      break;
    case MAY_SIN_T:
      /* sin(u) --> u' * cos(u) */
      y = may_mul_c (may_diff_recur (MAY_AT(x,0), vx),
		     may_cos_c (MAY_AT(x,0)));
      break;
    case MAY_TAN_T:
      /* tan(u) --> (1+tan(u)^2)*u' */
      y = may_mul_c (may_diff_recur (MAY_AT(x,0), vx),
		     may_add_c (MAY_ONE,
                                may_sqr_c (may_tan_c (MAY_AT (x,0)))));
      break;
    case MAY_ASIN_T:
      /* arcsin(u) --> u' / (1-u^2)^(1/2) */
      y = may_div_c (may_diff_recur (MAY_AT(x,0), vx),
		     may_sqrt_c (may_sub_c (MAY_ONE,
                                            may_sqr_c (MAY_AT (x,0)))));
      break;
    case MAY_ACOS_T:
      /* arccos(u) --> -DIFF(arcsin) = -u'/(1-u^2)^(1/2) */
      y = may_neg_c (may_div_c (may_diff_recur (MAY_AT(x,0), vx)
				, may_sqrt_c (may_sub_c (MAY_ONE
				       , may_sqr_c (MAY_AT(x,0))))));
      break;
    case MAY_ATAN_T:
      /* arctan(x) --> u' / (1+u^2) */
      y = may_div_c (may_diff_recur (MAY_AT(x,0), vx),
		     may_add_c (MAY_ONE, may_sqr_c (MAY_AT(x,0))));
      break;
    case MAY_COSH_T:
      /* cosh(u) --> u' * sinh(u) */
      y = may_mul_c (may_diff_recur (MAY_AT(x,0), vx)
		     , may_sinh_c (MAY_AT(x,0)) );
      break;
    case MAY_SINH_T:
      /* sinh(u) --> u' * cosh(u) */
      y = may_mul_c (may_diff_recur (MAY_AT(x,0), vx)
		     , may_cosh_c (MAY_AT(x,0)) );
      break;
    case MAY_TANH_T:
      /* tanh(u) --> (1-tanh(u)^2)*u' */
      y = may_mul_c (may_diff_recur (MAY_AT(x,0), vx),
		     may_sub_c (MAY_ONE, may_sqr_c (may_tanh_c(MAY_AT(x,0)))));
      break;
    case MAY_ASINH_T:
      /* arcsinh(u) --> u' / (1+u^2)^(1/2) */
      y = may_div_c (may_diff_recur (MAY_AT(x,0), vx),
		     may_sqrt_c (may_add_c (MAY_ONE, may_sqr_c(MAY_AT(x,0)))));
      break;
    case MAY_ACOSH_T:
      /* arccos(u) --> u'/[(u-1)^(1/2)*(u+1)^(1/2)] */
      y = may_div_c (may_diff_recur (MAY_AT(x,0), vx),
		     may_mul_c (may_sqrt_c (may_add_c(MAY_AT(x,0), MAY_N_ONE)),
				may_sqrt_c (may_add_c(MAY_AT(x,0), MAY_ONE))));
      break;
    case MAY_ATANH_T:
      /* arctanh(x) --> u' / (1-u^2) */
      y = may_div_c (may_diff_recur (MAY_AT(x,0), vx),
		     may_sub_c (MAY_ONE, may_sqr_c (MAY_AT(x,0))));
      break;
    case MAY_ABS_T:
      /* abs(u) --> u'*sign(u) */
      y = may_mul_c (may_diff_recur (MAY_AT (x, 0), vx),
                     may_sign_c (MAY_AT (x, 0)));
      break;
    case MAY_DIFF_T:
      /* diff(f,v,order,u) --> if f depends on vx, nothing,
                           otherwise u'*diff(f,v,order+1,u) */
      if (may_independent_p (MAY_AT (x, 0), vx)) {
        y = may_mul_c (may_diff_recur (MAY_AT (x, 3), vx),
                       may_hold_diff (MAY_AT (x, 0), MAY_AT (x, 1),
                                      may_add (MAY_AT (x, 2), MAY_ONE),
                                      MAY_AT (x, 3)));
        break;
      }
      /* Fall down to default code */
    /* Falls through. */
    default:
      if (MAY_EXT_P (x) && MAY_EXT_GETX (x)->diff)
        y = (*MAY_EXT_GETX (x)->diff) (x, vx);
      else if (may_independent_p (x, vx))
        y = MAY_ZERO;
      else
        y = may_hold_diff (x, vx, MAY_ONE, vx);
      break;
    }
  return y;
}

may_t
may_diff (may_t x, may_t vx)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (MAY_TYPE (vx) == MAY_STRING_T);

  MAY_LOG_FUNC (("x='%Y' vx='%Y'", x, vx));

  may_mark ();
  x = may_diff_recur (x, vx);
  return may_keep (may_eval (x));
}

may_t
may_diff_c (may_t x, may_t vx, may_t order, may_t a)
{
  may_t z;

  z = MAY_NODE_C (MAY_DIFF_T, 4);
  MAY_SET_AT (z, 0, x);
  MAY_SET_AT (z, 1, vx);
  MAY_SET_AT (z, 2, order);
  MAY_SET_AT (z, 3, a);
  return z;
}


/* diff (func, var, order, a)
   = df/dvar (order) (a) */
MAY_REGPARM may_t
may_eval_diff (may_t x)
{
  may_t f, v, o, a, y;
  long i, m;

  MAY_ASSERT (MAY_TYPE (x) == MAY_DIFF_T);
  MAY_ASSERT (MAY_NODE_SIZE(x) == 4);
  MAY_ASSERT (MAY_TYPE (MAY_AT (x, 1)) == MAY_STRING_T);

  /* eval sub arguments */
  f = may_eval (MAY_AT (x, 0));
  v = may_eval (MAY_AT (x, 1));
  o = may_eval (MAY_AT (x, 2));
  a = may_eval (MAY_AT (x, 3));

  /* Compute the diff if possible */
  if (may_get_si(&m, o) != 0 || may_str_local_p (v)) {
    y = may_hold_diff (f, v, o, a);
  } else if (m < 0) {
    /* Compute the antidiff */
    m = -m;
    for (y = f, i = 0; i < m && y != NULL; i++)
      y = may_antidiff (y, v);
    /* Check if the antidiff succeed */
    if (y == NULL)
      /* Not. Return unevaluated form */
      y = may_hold_diff (f, v, o, a);
    else
      /* Yes. Eval at point a */
      y = may_identical (v, a) == 0 ? y : may_replace (y, v, a);
  } else {
    /* Compute the Diff */
    for (i = 0; i < m; i++)
      f = may_diff (f, v);
    /* Eval at point a */
    y = may_identical (v, a) == 0 ? f : may_replace (f, v, a);
  }

  return y;
}
