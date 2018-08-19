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


const char *
may_get_name (may_t x)
{
  switch (MAY_TYPE (x))
    {
    case MAY_INT_T:
      return may_integer_name;
    case MAY_RAT_T:
      return may_rational_name;
    case MAY_FLOAT_T:
      return may_float_name;
    case MAY_COMPLEX_T:
      return may_complex_name;
    case MAY_STRING_T:
      return may_string_name;
    case MAY_DATA_T:
      return may_data_name;
    case MAY_SUM_T:
      return may_sum_name;
    case MAY_FACTOR_T:
    case MAY_PRODUCT_T:
      return may_product_name;
    case MAY_POW_T:
      return may_pow_name;
    case MAY_EXP_T:
      return may_exp_name;
    case MAY_LOG_T:
      return may_log_name;
    case MAY_SIN_T:
      return may_sin_name;
    case MAY_COS_T:
      return may_cos_name;
    case MAY_TAN_T:
      return may_tan_name;
    case MAY_ASIN_T:
      return may_asin_name;
    case MAY_ACOS_T:
      return may_acos_name;
    case MAY_ATAN_T:
      return may_atan_name;
    case MAY_SINH_T:
      return may_sinh_name;
    case MAY_COSH_T:
      return may_cosh_name;
    case MAY_TANH_T:
      return may_tanh_name;
    case MAY_ASINH_T:
      return may_asinh_name;
    case MAY_ACOSH_T:
      return may_acosh_name;
    case MAY_ATANH_T:
      return may_atanh_name;
    case MAY_ABS_T:
      return may_abs_name;
    case MAY_SIGN_T:
      return may_sign_name;
    case MAY_FLOOR_T:
      return may_floor_name;
    case MAY_MOD_T:
      return may_mod_name;
    case MAY_GCD_T:
      return may_gcd_name;
    case MAY_CONJ_T:
      return may_conj_name;
    case MAY_REAL_T:
      return may_real_name;
    case MAY_IMAG_T:
      return may_imag_name;
    case MAY_ARGUMENT_T:
      return may_argument_name;
    case MAY_GAMMA_T:
      return may_gamma_name;
    case MAY_DIFF_T:
      return may_diff_name;
    case MAY_FUNC_T:
      return MAY_NAME(MAY_AT(x,0));
    case MAY_LIST_T:
      return may_list_name;
    case MAY_MAT_T:
      return may_mat_name;
    case MAY_RANGE_T:
      return may_range_name;
    default:
      if (MAY_UNLIKELY (MAY_EXT_P (x)))
        return may_c.extension_tab[MAY_EXT2INDEX (MAY_TYPE (x))]->name;
      return NULL;
    }
}
