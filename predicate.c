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

/* Many predicates have only basic detection.
   More elaborated detection will come later (sum or product or power) */

int
may_zero_p (may_t x)
{
  return (*may_g.frame.zero_cb) (x);
}

int
may_zero_fastp (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  return ((MAY_TYPE (x) == MAY_INT_T && mpz_sgn (MAY_INT (x)) == 0)
	  || (MAY_UNLIKELY (MAY_TYPE (x) == MAY_FLOAT_T) && mpfr_zero_p (MAY_FLOAT (x)))
	  || (MAY_UNLIKELY (MAY_EXT_P (x)) && (MAY_EXT_GETX(x)->zero_p != 0) && (*(MAY_EXT_GETX (x)->zero_p))(x)));
}

int
may_nonzero_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
    return mpz_sgn(MAY_INT (x)) != 0;
  case MAY_FLOAT_T:
    return !mpfr_nan_p (MAY_FLOAT (x)) && !mpfr_zero_p (MAY_FLOAT (x));
  case MAY_RAT_T:
  case MAY_COMPLEX_T:
    /* Rational and complex are simplified to INT if they are equal to 0. */
    return 1;
  case MAY_STRING_T:
    return MAY_CHECK_DOMAIN_P (x, MAY_NONZERO_D);
  case MAY_FUNC_T:
    return MAY_CHECK_DOMAIN_P (MAY_AT (x, 0), MAY_NONZERO_D);
  default:
    if (MAY_UNLIKELY (MAY_NUM_P (x)))
      return !(may_compute_sign (x) & 1);
    if (MAY_UNLIKELY (MAY_EXT_P (x))) {
      int (*p) (may_t);
      p = MAY_EXT_GETX (x)->nonzero_p;
      if (p != 0)
	return (*p) (x);
    }
  }
  return 0;
}

int
may_one_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  if (MAY_TYPE(x) == MAY_INT_T)
    return mpz_cmp_ui(MAY_INT(x), 1)==0;
  else if (MAY_TYPE(x) == MAY_FLOAT_T)
    return !mpfr_nan_p(MAY_FLOAT(x)) && mpfr_cmp_ui(MAY_FLOAT(x), 1)==0;
  else if (MAY_UNLIKELY (MAY_EXT_P (x))) {
      int (*p) (may_t);
      p = MAY_EXT_GETX (x)->one_p;
      if (p != 0)
	return (*p) (x);
  }
  return 0;
}

int
may_pos_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
    return mpz_sgn (MAY_INT (x)) > 0;
  case MAY_RAT_T:
    return mpq_sgn (MAY_RAT (x)) > 0;
  case MAY_FLOAT_T:
    return !mpfr_nan_p (MAY_FLOAT (x)) && mpfr_sgn (MAY_FLOAT (x)) > 0;
  case MAY_COMPLEX_T:
    return 0;
  case MAY_STRING_T:
    return MAY_CHECK_DOMAIN_P (x, MAY_REAL_POS_D);
  case MAY_FUNC_T:
     return MAY_CHECK_DOMAIN_P (MAY_AT (x, 0), MAY_REAL_POS_D);
  case MAY_ABS_T:
    return may_nonzero_p (MAY_AT (x, 0));
  default:
    if (MAY_UNLIKELY (MAY_NUM_P (x)))
      return (may_compute_sign (x) & 2);
  }
  return 0;
}

int
may_nonneg_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
    return mpz_sgn (MAY_INT (x)) >= 0;
  case MAY_RAT_T:
    return mpq_sgn (MAY_RAT (x)) >= 0;
  case MAY_FLOAT_T:
    return !mpfr_nan_p (MAY_FLOAT (x)) && mpfr_sgn (MAY_FLOAT (x)) >= 0;
  case MAY_COMPLEX_T:
    return 0;
  case MAY_STRING_T:
    return MAY_CHECK_DOMAIN_P (x, MAY_REAL_NONNEG_D);
  case MAY_FUNC_T:
    return MAY_CHECK_DOMAIN_P (MAY_AT (x, 0), MAY_REAL_NONNEG_D);
  case MAY_ABS_T:
    return 1;
  default:
    if (MAY_UNLIKELY (MAY_NUM_P (x)))
      return (may_compute_sign (x) & (2+1));
  }
  return 0;
}

int
may_neg_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
    return mpz_sgn (MAY_INT (x)) < 0;
  case MAY_RAT_T:
    return mpq_sgn (MAY_RAT (x)) < 0;
  case MAY_FLOAT_T:
    return !mpfr_nan_p (MAY_FLOAT (x)) && mpfr_sgn (MAY_FLOAT (x)) < 0;
  case MAY_COMPLEX_T:
    return 0;
  case MAY_STRING_T:
    return MAY_CHECK_DOMAIN_P (x, MAY_REAL_NEG_D);
  case MAY_FUNC_T:
    return MAY_CHECK_DOMAIN_P (MAY_AT (x, 0), MAY_REAL_NEG_D);
  default:
    if (MAY_UNLIKELY (MAY_NUM_P (x)))
      return (may_compute_sign (x) & 4);
  }
  return 0;
}

int
may_nonpos_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  switch (MAY_TYPE (x)) {
  case MAY_INT_T:
    return mpz_sgn (MAY_INT (x)) <= 0;
  case MAY_RAT_T:
    return mpq_sgn (MAY_RAT (x)) <= 0;
  case MAY_FLOAT_T:
    return !mpfr_nan_p (MAY_FLOAT (x)) && mpfr_sgn (MAY_FLOAT (x)) <= 0;
  case MAY_COMPLEX_T:
    return 0;
  case MAY_STRING_T:
    return MAY_CHECK_DOMAIN_P (x, MAY_REAL_NONPOS_D);
  case MAY_FUNC_T:
    return MAY_CHECK_DOMAIN_P (MAY_AT (x, 0), MAY_REAL_NONPOS_D);
  default:
    if (MAY_UNLIKELY (MAY_NUM_P (x)))
      return (may_compute_sign (x) & (4+1));
  }
  return 0;
}

int
may_real_p (may_t x)
{
  may_type_t t;

  MAY_ASSERT (MAY_EVAL_P (x));

  /* If real, or imag or abs or angle, then it is a real.
     We don't have to check the argument of the functions. */
  t = MAY_TYPE (x);
  return t == MAY_INT_T || t ==MAY_RAT_T || t == MAY_FLOAT_T
    || t == MAY_REAL_T || t == MAY_IMAG_T
    || t == MAY_ABS_T || t == MAY_ARGUMENT_T
    || (t == MAY_STRING_T && MAY_CHECK_DOMAIN_P (x, MAY_REAL_D))
    || (t == MAY_FUNC_T && MAY_CHECK_DOMAIN_P (MAY_AT (x, 0), MAY_REAL_D))
    ;
}

int
may_rational_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  return MAY_TYPE (x) == MAY_INT_T || MAY_TYPE (x) == MAY_RAT_T
    || (MAY_TYPE(x) == MAY_STRING_T && MAY_CHECK_DOMAIN_P (x, MAY_RATIONAL_D))
    || (MAY_TYPE(x) == MAY_FUNC_T && MAY_CHECK_DOMAIN_P (MAY_AT (x, 0),
                                                         MAY_RATIONAL_D));
}

int
may_crational_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  if (MAY_TYPE (x) == MAY_INT_T || MAY_TYPE (x) == MAY_RAT_T)
    return 1;
  if (MAY_TYPE (x) == MAY_COMPLEX_T)
    return may_rational_p (MAY_RE (x)) && may_rational_p (MAY_IM (x));
  if (MAY_TYPE (x) == MAY_STRING_T)
    return MAY_CHECK_DOMAIN_P (x, MAY_CRATIONAL_D);
  if (MAY_TYPE (x) == MAY_FUNC_T)
    return MAY_CHECK_DOMAIN_P (MAY_AT (x, 0), MAY_CRATIONAL_D);
  return 0;
}

int
may_integer_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return MAY_TYPE (x) == MAY_INT_T
    || (MAY_TYPE (x) == MAY_STRING_T && MAY_CHECK_DOMAIN_P (x, MAY_INTEGER_D))
    || (MAY_TYPE (x) == MAY_FUNC_T && MAY_CHECK_DOMAIN_P (MAY_AT (x, 0),
                                                          MAY_INTEGER_D));
}

int
may_imminteger_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return MAY_TYPE (x) == MAY_INT_T;
}

int
may_cinteger_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return MAY_TYPE (x) == MAY_INT_T
    || (MAY_TYPE (x) == MAY_COMPLEX_T
        && MAY_TYPE (MAY_RE (x)) == MAY_INT_T
        && MAY_TYPE (MAY_IM (x)) == MAY_INT_T)
    || (MAY_TYPE(x) == MAY_STRING_T && MAY_CHECK_DOMAIN_P (x, MAY_CINTEGER_D))
    || (MAY_TYPE(x) == MAY_FUNC_T && MAY_CHECK_DOMAIN_P (MAY_AT (x, 0),
                                                         MAY_CINTEGER_D));
}

int
may_posint_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return (MAY_TYPE (x) == MAY_INT_T && mpz_sgn (MAY_INT (x)) > 0)
    || (MAY_TYPE(x) == MAY_STRING_T && MAY_CHECK_DOMAIN_P (x, MAY_INT_POS_D))
    || (MAY_TYPE(x) == MAY_FUNC_T && MAY_CHECK_DOMAIN_P (MAY_AT (x, 0),
                                                         MAY_INT_POS_D));
}

int
may_nonnegint_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return (MAY_TYPE (x) == MAY_INT_T && mpz_sgn (MAY_INT (x)) >= 0)
    || (MAY_TYPE(x)==MAY_STRING_T && MAY_CHECK_DOMAIN_P (x, MAY_INT_NONNEG_D))
    || (MAY_TYPE(x)==MAY_FUNC_T && MAY_CHECK_DOMAIN_P (MAY_AT (x, 0),
                                                       MAY_INT_NONNEG_D));
}

int
may_negint_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return (MAY_TYPE (x) == MAY_INT_T && mpz_sgn (MAY_INT (x)) < 0)
    || (MAY_TYPE(x) == MAY_STRING_T && MAY_CHECK_DOMAIN_P (x, MAY_INT_NEG_D))
    || (MAY_TYPE(x) == MAY_FUNC_T && MAY_CHECK_DOMAIN_P (MAY_AT (x, 0),
                                                         MAY_INT_NEG_D));
}

int
may_nonposint_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return (MAY_TYPE (x) == MAY_INT_T && mpz_sgn (MAY_INT (x)) <= 0)
    || (MAY_TYPE(x)==MAY_STRING_T && MAY_CHECK_DOMAIN_P (x, MAY_INT_NONPOS_D))
    || (MAY_TYPE(x)==MAY_FUNC_T && MAY_CHECK_DOMAIN_P (MAY_AT (x, 0),
                                                       MAY_INT_NONPOS_D));
}

int
may_even_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return (MAY_TYPE(x) == MAY_INT_T && mpz_even_p (MAY_INT(x)))
    || (MAY_TYPE(x)==MAY_STRING_T && MAY_CHECK_DOMAIN_P (x, MAY_INT_EVEN_D))
    || (MAY_TYPE(x)==MAY_FUNC_T && MAY_CHECK_DOMAIN_P (MAY_AT (x, 0),
                                                       MAY_INT_EVEN_D));
}

int
may_odd_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return (MAY_TYPE(x) == MAY_INT_T && mpz_odd_p (MAY_INT(x)))
    || (MAY_TYPE(x)==MAY_STRING_T && MAY_CHECK_DOMAIN_P (x, MAY_INT_EVEN_D))
    || (MAY_TYPE(x)==MAY_FUNC_T && MAY_CHECK_DOMAIN_P (MAY_AT (x, 0),
                                                       MAY_INT_EVEN_D));
}

int
may_prime_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return (MAY_TYPE (x) == MAY_INT_T && mpz_probab_prime_p (MAY_INT (x), 5))
    || (MAY_TYPE(x)==MAY_STRING_T && MAY_CHECK_DOMAIN_P (x, MAY_INT_PRIME_D))
    || (MAY_TYPE(x)==MAY_FUNC_T && MAY_CHECK_DOMAIN_P (MAY_AT (x, 0),
                                                       MAY_INT_PRIME_D));
}

int
may_nan_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return MAY_NAN_P (x);
}

int
may_inf_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return (MAY_TYPE (x) == MAY_FLOAT_T && mpfr_inf_p (MAY_FLOAT (x)));
}

int
may_num_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return MAY_NUM_P (x);
}

int
may_purenum_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return MAY_PURENUM_P (x);
}

int
may_purereal_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  return MAY_TYPE (x) <= MAY_FLOAT_T;
}

/* Return TRUE is x contains NAN, i.e. x is UNDEF */
int
may_undef_p (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  if (MAY_UNLIKELY (MAY_ATOMIC_P (x)))
    return may_nan_p (x);

  may_size_t i, n = MAY_NODE_SIZE(x);
  for (i = 0 ; MAY_LIKELY (i < n); i++)
    if (may_undef_p (MAY_AT(x, i)))
      return 1;
  return 0;
}

static MAY_REGPARM int
may_independent_name_p (may_t x, const char var[])
{
  MAY_ASSERT (MAY_EVAL_P (x));

  if (MAY_ATOMIC_P (x)) {
    if (MAY_TYPE (x) == MAY_STRING_T)
      return strcmp (MAY_NAME (x), var);
    return 1;
  }
  may_size_t i, n = MAY_NODE_SIZE(x);
  MAY_ASSERT (n > 0);
  for (i = 0; i < n; i++) {
    if (MAY_UNLIKELY (!may_independent_name_p (MAY_AT (x, i), var)))
      return 0;
  }
  return 1;
}

static MAY_REGPARM int
may_independent_expr_p (may_t x, may_t var)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  if (may_identical (x, var) == 0)
    return 0;
  if (MAY_ATOMIC_P (x))
    return 1;
  may_size_t i, n = MAY_NODE_SIZE(x);
  MAY_ASSERT (n > 0);
  for (i = 0; i < n; i++) {
    if (MAY_UNLIKELY (!may_independent_expr_p (MAY_AT (x, i), var)))
      return 0;
  }
  return 1;
}

int
may_independent_p (may_t x, may_t var)
{
  if (MAY_LIKELY (MAY_TYPE (var) == MAY_STRING_T))
    return may_independent_name_p (x, MAY_NAME (var));
  else
    return may_independent_expr_p (x, var);
}

int
may_independent_vp (may_t x, may_t list)
{
  may_size_t i, n;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (MAY_TYPE (list) == MAY_LIST_T);
  MAY_ASSERT (MAY_NODE_SIZE(list) >= 1);

  if (MAY_ATOMIC_P (x)) {
    if (MAY_TYPE (x) == MAY_STRING_T) {
      n = MAY_NODE_SIZE(list);
      for (i = 0; i < n; i++) {
        if (strcmp (MAY_NAME (x), MAY_NAME (MAY_AT (list, i))) == 0)
          return 0;
      }
    }
    return 1;
  }
  n = MAY_NODE_SIZE(x);
  for (i = 0; i < n; i++) {
    if (!may_independent_vp (MAY_AT (x, i), list))
      return 0;
  }
  return 1;
}

/* Return TRUE if x contains type.
   If type==MAY_STRING_T, it returns true if x == string
   To check for a FUNC_T, check a STRING_T
   (since the first argument of a FUNC_T is a string) */
static int
may_contain_p (may_t x, int type, const char *string)
{
  may_type_t t = MAY_TYPE (x);

  MAY_ASSERT (MAY_EVAL_P (x));

  if (MAY_UNLIKELY ((int) t == type))
    return t == MAY_STRING_T ? !strcmp (MAY_NAME(x), string) : 1;
  if (t > MAY_ATOMIC_LIMIT) {
    may_size_t i, n = MAY_NODE_SIZE(x);
    MAY_ASSERT (n > 0);
    for (i = 0 ; i < n; i++)
      if (may_contain_p (MAY_AT(x, i), type, string))
	  return 1;
  }
  return 0;
}


static const struct FuncNameStruct_s {
  const char *name;
  may_type_t  type;
} FuncNameTab[]={
    {may_abs_name,   MAY_ABS_T},
    {may_acos_name,  MAY_ACOS_T},
    {may_acosh_name, MAY_ACOSH_T},
    {may_argument_name, MAY_ARGUMENT_T},
    {may_asin_name,  MAY_ASIN_T},
    {may_asinh_name, MAY_ASINH_T},
    {may_atan_name,  MAY_ATAN_T},
    {may_atanh_name, MAY_ATANH_T},
    {may_conj_name,  MAY_CONJ_T},
    {may_cos_name,   MAY_COS_T},
    {may_cosh_name,  MAY_COSH_T},
    {may_diff_name,  MAY_DIFF_T},
    {may_exp_name,   MAY_EXP_T},
    {may_floor_name, MAY_FLOOR_T},
    {may_gamma_name, MAY_GAMMA_T},
    {may_gcd_name,   MAY_GCD_T},
    {may_imag_name,  MAY_IMAG_T},
    {may_ln_name,    MAY_LOG_T},
    {may_log_name,   MAY_LOG_T},
    {may_real_name,  MAY_REAL_T},
    {may_sign_name,  MAY_SIGN_T},
    {may_sin_name,   MAY_SIN_T},
    {may_sinh_name,  MAY_SINH_T},
    {may_tan_name,   MAY_TAN_T},
    {may_tanh_name,  MAY_TANH_T} };

static int
FuncNameCmp (const void *a, const void *b)
{
  return strcmp ((const char *) a, ((const struct FuncNameStruct_s *)b)->name);
}

int
may_func_p (may_t x, const char *funcname)
{
  may_type_t type;
  const struct FuncNameStruct_s *r = (const struct FuncNameStruct_s *)
    bsearch (funcname, FuncNameTab,
	     numberof (FuncNameTab), sizeof (FuncNameTab[0]), FuncNameCmp);
  MAY_ASSERT (MAY_EVAL_P (x));
  type = r == NULL ? MAY_STRING_T : r->type;
  return may_contain_p (x, type, funcname);
}

/* Return != 0 iff the expression contains at least one of the specified
   functions
   TODO: May change the function so that it returns the found function too? */
int
may_exp_p (may_t x, may_exp_p_flags_e flags)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  switch (MAY_TYPE(x))
    {
    case MAY_INT_T ... MAY_ATOMIC_LIMIT:
      return 0;
    case MAY_EXP_T:
      return MAY_EXP_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_LOG_T:
      return MAY_LOG_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_SIN_T:
      return MAY_SIN_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_COS_T:
      return MAY_COS_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_TAN_T:
      return MAY_TAN_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_ASIN_T:
      return MAY_ASIN_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_ACOS_T:
      return MAY_ACOS_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_ATAN_T:
      return MAY_ATAN_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_SINH_T:
      return MAY_SINH_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_COSH_T:
      return MAY_COSH_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_TANH_T:
      return MAY_TANH_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_ASINH_T:
      return MAY_ASINH_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_ACOSH_T:
      return MAY_ACOSH_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);
    case MAY_ATANH_T:
      return MAY_ATANH_EXP_P & flags ? 1 : may_exp_p (MAY_AT (x, 0), flags);

    default:
      {
	may_size_t i, n = MAY_NODE_SIZE(x);
        MAY_ASSERT (n > 0);
	for (i = 0 ; MAY_LIKELY (i < n); i++)
	  if (may_exp_p (MAY_AT (x, i), flags))
	    return 1;
	return 0;
      }
    }

}
