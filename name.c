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

/* Name of Internal Functions */

const char may_sqrt_name[] = "sqrt";

const char may_exp_name[] = "exp";
const char may_log_name[] = "log";
const char may_ln_name[]  = "ln";

const char may_sin_name[] = "sin";
const char may_cos_name[] = "cos";
const char may_tan_name[] = "tan";
const char may_asin_name[] = "asin";
const char may_acos_name[] = "acos";
const char may_atan_name[] = "atan";
const char may_sinh_name[] = "sinh";
const char may_cosh_name[] = "cosh";
const char may_tanh_name[] = "tanh";
const char may_asinh_name[] = "asinh";
const char may_acosh_name[] = "acosh";
const char may_atanh_name[] = "atanh";

const char may_abs_name[] = "abs";
const char may_sign_name[] = "sign";
const char may_floor_name[] = "floor";
const char may_ceil_name[] = "ceil";

const char may_mod_name[] = "mod";
const char may_gcd_name[] = "gcd";

const char may_real_name[] = "real";
const char may_imag_name[] = "imag";
const char may_conj_name[] = "conj";
const char may_argument_name[] = "argument";
const char may_gamma_name[] = "gamma";

/* Name of Internal Variables */
const char may_nan_name[] = "NAN";
const char may_inf_name[] = "INFINITY";
const char may_I_name[] = "I";
const char may_pi_name[] = "PI";

/* Name of types */
const char may_integer_name[] = "INTEGER";
const char may_rational_name[] = "RATIONAL";
const char may_float_name[] = "FLOAT";
const char may_complex_name[] = "COMPLEX";
const char may_sum_name[] = "+";
const char may_product_name[] = "*";
const char may_pow_name[] = "^";
const char may_string_name[] = "IDENTIFIER";
const char may_mat_name[] = "[]";
const char may_list_name[] = "{}";
const char may_range_name[] = "RANGE";
const char may_diff_name[] = "diff";
const char may_data_name[] = "DATA";

static const char *get_name_tab[] =
  {
    [MAY_INT_T] = may_integer_name,
    [MAY_RAT_T] = may_rational_name,
    [MAY_FLOAT_T] = may_float_name,
    [MAY_COMPLEX_T] = may_complex_name,
    [MAY_STRING_T] = may_string_name,
    [MAY_DATA_T] = may_data_name,
    [MAY_SUM_T] = may_sum_name,
    [MAY_FACTOR_T] = may_product_name,
    [MAY_PRODUCT_T] = may_product_name,
    [MAY_POW_T] = may_pow_name,
    [MAY_EXP_T] = may_exp_name,
    [MAY_LOG_T] = may_log_name,
    [MAY_SIN_T] = may_sin_name,
    [MAY_COS_T] = may_cos_name,
    [MAY_TAN_T] = may_tan_name,
    [MAY_ASIN_T] = may_asin_name,
    [MAY_ACOS_T] = may_acos_name,
    [MAY_ATAN_T] = may_atan_name,
    [MAY_SINH_T] = may_sinh_name,
    [MAY_COSH_T] = may_cosh_name,
    [MAY_TANH_T] = may_tanh_name,
    [MAY_ASINH_T] = may_asinh_name,
    [MAY_ACOSH_T] = may_acosh_name,
    [MAY_ATANH_T] = may_atanh_name,
    [MAY_ABS_T]   = may_abs_name,
    [MAY_SIGN_T]  = may_sign_name,
    [MAY_FLOOR_T] = may_floor_name,
    [MAY_MOD_T]   = may_mod_name,
    [MAY_GCD_T]   = may_gcd_name,
    [MAY_CONJ_T]  = may_conj_name,
    [MAY_REAL_T]  = may_real_name,
    [MAY_IMAG_T]  = may_imag_name,
    [MAY_ARGUMENT_T] = may_argument_name,
    [MAY_GAMMA_T] = may_gamma_name,
    [MAY_DIFF_T] = may_diff_name,
    [MAY_LIST_T] = may_list_name,
    [MAY_MAT_T] = may_mat_name,
    [MAY_RANGE_T] = may_range_name
  };

const char *
may_get_name (may_t x)
{
  const char *name = NULL;
  unsigned u = MAY_TYPE(x);
  if (MAY_LIKELY ( u < numberof(get_name_tab))) {
    name = get_name_tab[u];
    if (MAY_UNLIKELY(name == NULL)) {
      if (u == MAY_FUNC_T)
        name = MAY_NAME(MAY_AT(x,0));
      else if (MAY_UNLIKELY (MAY_EXT_P (x)))
        name = may_c.extension_tab[MAY_EXT2INDEX (MAY_TYPE (x))]->name;
    }
  }
  return name;
}
