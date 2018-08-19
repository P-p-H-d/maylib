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

static may_t even_function (may_t x, may_t (*constructor) (may_t))
{
  if (!may_sum_p(x))
    return NULL;
  may_iterator_t it;
  may_t ter, num = may_sum_iterator_init (it, x);
  if (may_zero_fastp (num))
    may_sum_iterator_end (&num, &ter, it);
  if (may_get_name (num) == may_complex_name ?
      (!may_num_negzero_p (MAY_RE (num)) || may_num_pos_p (MAY_IM (num)))
      : may_num_pos_p (num))
    return NULL;
  return (*constructor) (may_neg_c (x));
}

static may_t odd_function (may_t x, may_t (*constructor) (may_t))
{
  if (!may_sum_p(x))
    return NULL;
  may_iterator_t it;
  may_t ter, num = may_sum_iterator_init (it, x);
  if (may_zero_fastp (num))
    may_sum_iterator_end (&num, &ter, it);
  if (may_get_name (num) == may_complex_name ?
      (!may_num_negzero_p (MAY_RE (num)) || may_num_pos_p (MAY_IM (num)))
      : may_num_pos_p (num))
    return NULL;
  return may_neg_c ((*constructor) (may_neg_c (x)));
}

static may_t ns_sin  (may_t x) { return odd_function (x, may_sin_c); }
static may_t ns_cos  (may_t x) { return even_function (x, may_cos_c); }
static may_t ns_tan  (may_t x) { return odd_function (x, may_tan_c); }
static may_t ns_sinh (may_t x) { return odd_function (x, may_sinh_c); }
static may_t ns_cosh (may_t x) { return even_function (x, may_cosh_c); }
static may_t ns_tanh (may_t x) { return odd_function (x, may_tanh_c); }
static may_t ns_abs  (may_t x) { return even_function (x, may_abs_c); }
static may_t ns_sign (may_t x) { return odd_function (x, may_sign_c); }
static may_t ns_conj (may_t x) { return odd_function (x, may_conj_c); }
static may_t ns_real (may_t x) { return odd_function (x, may_real_c); }
static may_t ns_imag (may_t x) { return odd_function (x, may_imag_c); }

static const char *const name[] = {
  may_sin_name, may_cos_name, may_tan_name,
  may_sinh_name, may_cosh_name, may_tanh_name,
  may_abs_name, may_sign_name, may_conj_name,
  may_real_name, may_imag_name
};
static const void *const func[] = {
  ns_sin, ns_cos, ns_tan,
  ns_sinh, ns_cosh, ns_tanh,
  ns_abs, ns_sign, ns_conj,
  ns_real, ns_imag
};

may_t
may_normalsign (may_t x)
{
  MAY_LOG_FUNC (("%Y", x));
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (name) == numberof (func));

  may_mark();
  may_t y = may_subs_c (x, 1, numberof (name), name, func);
  return may_keep (may_eval (y));
}
