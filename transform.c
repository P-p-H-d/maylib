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

/* FIXME: Recursif ? */

/**************************************************************************/

/* sin(x) = 2*t/(1+t^2) with t=tan(x/2) */
static may_t sin2tan2 (may_t x) {
  may_t t = may_tan_c (may_mul_c (MAY_HALF, x));
  may_t n = may_mul_c (MAY_TWO, t);
  may_t d = may_add_c (MAY_ONE, may_sqr_c (t));
  return may_div_c (n ,d);
}

/* cos(x) = (1-t^2)/(1+t^2) with t=tan(x/2) */
static may_t cos2tan2 (may_t x) {
  may_t t = may_tan_c (may_mul_c (MAY_HALF, x));
  may_t n = may_sub_c (MAY_ONE, may_sqr_c (t));
  may_t d = may_add_c (MAY_ONE, may_sqr_c (t));
  return may_div_c (n ,d);
}

/* tan(x) = 2*t/(1-t^2) with t=tan(x/2) */
static may_t tan2tan2 (may_t x) {
  may_t t = may_tan_c (may_mul_c (MAY_HALF, x));
  may_t d = may_sub_c (MAY_ONE, may_sqr_c (t));
  may_t n = may_mul_c (MAY_TWO, t);
  return may_div_c (n ,d);
}

static const char *const trig2tan2_name[] = {
  may_sin_name, may_cos_name, may_tan_name,
};
static const void *const trig2tan2_func[] = {
  sin2tan2, cos2tan2, tan2tan2
};

may_t
may_trig2tan2 (may_t x)
{
  may_t y;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (trig2tan2_name) == numberof (trig2tan2_func));

  MAY_LOG_FUNC (("%Y", x));

  MAY_RECORD ();
  y = may_subs_c (x, 1, numberof (trig2tan2_name),
                  trig2tan2_name, trig2tan2_func);
  MAY_RET_EVAL (y);
}

/**************************************************************************/

/* cos(x) = 1/2*[e^(I*x)+e^(-I*x)] */
static may_t cos_to_exp (may_t z) {
  may_t zi = may_mul_c (z, MAY_I);
  may_t y = may_exp_c (zi);
  y = may_add_c (y, may_div_c (MAY_ONE, y));
  y = may_mul_c (y, MAY_HALF);
  return y;
}

/* sin(x) = 1/2I*[e^(I*x)-e^(-I*x)] */
static may_t sin_to_exp (may_t z) {
  may_t zi = may_mul_c (z, MAY_I);
  may_t y = may_exp_c (zi);
  may_t f = may_mul_c (MAY_TWO, MAY_I);
  y = may_sub_c (y, may_div_c (MAY_ONE, y));
  y = may_div_c (y, f);
  return y;
}

/* tan(x) = (exp(2Ix)-1)/(I*(exp(2Ix)+1)) */
static may_t tan_to_exp (may_t z) {
  may_t f = may_mul_c (MAY_TWO, MAY_I);
  may_t y = may_exp_c (may_mul_c (f, z));
  may_t n = may_add_c (y, MAY_N_ONE);
  may_t d = may_mul_c (MAY_I, may_add_c (y, MAY_ONE));
  y = may_div_c (n, d);
  return y;
}

/* acos(x) = -I* log(x+(1-x^2)^(1/2)*I) */
static may_t acos_to_exp (may_t x) {
  may_t t;
  t = may_sqrt_c (may_sub_c (MAY_ONE, may_sqr_c (x)));  /* (1-x^2)^(1/2) */
  t = may_log_c (may_add_c (x, may_mul_c (t, MAY_I)));  /* log(x+t*I)    */
  t = may_neg_c (may_mul_c (t, MAY_I));                 /* -T*I          */
  return t;
}

/* asin(x) = -I* log(I*x+(1-x^2)^(1/2)) */
static may_t asin_to_exp (may_t x) {
  may_t t;
  t = may_sqrt_c ( may_sub_c (MAY_ONE, may_sqr_c (x)));  /* (1-x^2)^(1/2) */
  t = may_log_c (may_add_c (may_mul_c (x, MAY_I), t));   /* log(I*x+t)    */
  t = may_neg_c (may_mul_c (t, MAY_I));
  return t;
}

/* atan(x) = 1/2*I*ln( (1-x*I)/(1+x*I) ) */
static may_t atan_to_exp (may_t x) {
  may_t t;
  t = may_mul_c (x, MAY_I);
  t = may_div_c (may_sub_c (MAY_ONE, t), may_add_c (MAY_ONE, t));
  t = may_mul_c (may_mul_c (MAY_HALF, MAY_I), may_log_c (t) );
  return t;
}

/* cosh(x) = 1/2[e^(x)+e^(-x)] */
static may_t cosh_to_exp (may_t z) {
  may_t y = may_exp_c (z);
  y = may_add_c (y, may_div_c (MAY_ONE, y));
  y = may_mul_c (y, MAY_HALF);
  return  (y);
}

/* sinh(x) = 1/2 [e^(x)-e^(-x)] */
static may_t sinh_to_exp (may_t z) {
  may_t y = may_exp_c (z);
  y = may_sub_c (y, may_div_c (MAY_ONE, y));
  y = may_mul_c (y, MAY_HALF);
  return  (y);
}

/* tanh(x) = (exp(2x)-1)/(exp(2x)+1) */
static may_t tanh_to_exp (may_t z) {
  may_t y     = may_exp_c (may_mul_c (MAY_TWO, z));
  may_t num   = may_sub_c (y, MAY_ONE);
  may_t denom = may_add_c (y, MAY_ONE);
  y = may_div_c (num, denom);
  return  (y);
}

/* atanh(x) = log((1+x)/(1-x))/2 */
static may_t atanh_to_exp (may_t z) {
  may_t num  = may_add_c (MAY_ONE, z);
  may_t deno = may_sub_c (MAY_ONE, z);
  may_t frac = may_div_c (num, deno);
  may_t y    = may_mul_c (MAY_HALF, may_log_c (frac));
  return  (y);
}

/* acosh(x) = log(x+sqrt(x^2-1)) */
static may_t acosh_to_exp (may_t z) {
  may_t a    = may_sub_c (may_sqr_c (z), MAY_ONE);
  may_t s    = may_add_c (z, may_sqrt_c (a));
  may_t y    = may_log_c (s);
  return  (y);
}

/* asinh(x) = log(x+sqrt(x^2+1)) */
static may_t asinh_to_exp (may_t z) {
  may_t a    = may_add_c (may_sqr_c (z), MAY_ONE);
  may_t s    = may_add_c (z, may_sqrt_c (a));
  may_t y    = may_log_c (s);
  return  (y);
}

static const char *const name[] = {
  may_sin_name, may_cos_name, may_tan_name,
  may_asin_name, may_acos_name, may_atan_name,
  may_sinh_name, may_cosh_name, may_tanh_name,
  may_asinh_name, may_acosh_name, may_atanh_name
};
static const void *const func[] = {
  sin_to_exp, cos_to_exp, tan_to_exp,
  asin_to_exp, acos_to_exp, atan_to_exp,
  sinh_to_exp, cosh_to_exp, tanh_to_exp,
  asinh_to_exp, acosh_to_exp, atanh_to_exp
};

may_t
may_trig2exp (may_t x)
{
  may_t y;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (name) == numberof (func));
  MAY_RECORD ();
  y = may_subs_c (x, 1, numberof (name), name, func);
  MAY_RET_EVAL (y);
}

/**************************************************************************/

/* cos(x) = 1/2*[e^(I*x)+e^(-I*x)] */
static may_t cos_to_exp2 (may_t z) {
  may_t zi = may_mul_c (z, MAY_I);
  may_t y = may_exp_c (zi);
  y = may_add_c (y, may_exp_c (may_neg_c (zi)));
  y = may_mul_c (y, MAY_HALF);
  return y;
}

/* sin(x) = 1/2I*[e^(I*x)-e^(-I*x)] = -I/2*[e^(I*x)-e^(-I*x)]*/
static may_t sin_to_exp2 (may_t z) {
  may_t zi = may_mul_c (z, MAY_I);
  may_t y = may_exp_c (zi);
  y = may_sub_c (y, may_exp_c (may_neg_c (zi)));
  y = may_mul_vac (y, MAY_N_HALF, MAY_I, NULL);
  return y;
}

/* tan(x) = [e^(I*x)-e^(-I*x)]/(I*[e^(I*x)+e^(-I*x)]) */
static may_t tan_to_exp2 (may_t z) {
  may_t y1 = may_exp_c (may_mul_c (MAY_I, z));
  may_t y2 = may_exp_c (may_mul_c (may_mul_c (MAY_N_ONE, MAY_I), z));
  may_t n = may_sub_c (y1, y2);
  may_t d = may_add_c (y1, y2);
  y1 = may_div_c (n, may_mul_c (MAY_I, d));
  return y1;
}

/* cosh(x) = 1/2[e^(x)+e^(-x)] */
static may_t cosh_to_exp2 (may_t z) {
  may_t y = may_exp_c (z);
  y = may_add_c (y, may_exp_c (may_neg_c (z)));
  y = may_mul_c (y, MAY_HALF);
  return  (y);
}

/* sinh(x) = 1/2 [e^(x)-e^(-x)] */
static may_t sinh_to_exp2 (may_t z) {
  may_t y = may_exp_c (z);
  y = may_sub_c (y, may_exp_c (may_neg_c (z)));
  y = may_mul_c (y, MAY_HALF);
  return  (y);
}

/* tanh(x) = (exp(x)-exp(-x))/(exp(x)+exp(-x)) */
static may_t tanh_to_exp2 (may_t z) {
  may_t y1    = may_exp_c (z);
  may_t y2    = may_exp_c (may_neg_c (z));
  may_t num   = may_sub_c (y1, y2);
  may_t denom = may_add_c (y1, y2);
  y1 = may_div_c (num, denom);
  return y1;
}

static const char *const trig2exp2_name[] = {
  may_sin_name, may_cos_name, may_tan_name,
  may_sinh_name, may_cosh_name, may_tanh_name,
};
static const void *const trig2exp2_func[] = {
  sin_to_exp2, cos_to_exp2, tan_to_exp2,
  sinh_to_exp2, cosh_to_exp2, tanh_to_exp2,
};

may_t
may_trig2exp2 (may_t x)
{
  may_t y;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (trig2exp2_name) == numberof (trig2exp2_func));

  MAY_LOG_FUNC (("%Y", x));

  MAY_RECORD ();
  y = may_subs_c (x, 1, numberof (trig2exp2_name),
                  trig2exp2_name, trig2exp2_func);
  MAY_RET_EVAL (y);
}

/**************************************************************************/

/* exp(x) = cosh(x) + sinh(x) */
static may_t exp_to_coshsinh (may_t z) {
  return may_add_c (may_cosh_c (z), may_sinh_c (z));
}

static const char *const exp2trig_name[] = {
  may_exp_name
};
static const void *const exp2trig_func[] = {
  exp_to_coshsinh
};

may_t
may_exp2trig (may_t x)
{
  may_t y;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (exp2trig_name) == numberof (exp2trig_func));

  MAY_LOG_FUNC (("%Y", x));

  MAY_RECORD ();
  y = may_subs_c (x, 1, numberof (exp2trig_name), exp2trig_name, exp2trig_func);
  MAY_RET_EVAL (y);
}

/**************************************************************************/

/* tan(x) = sin(x)/cos(x) */
static may_t tan_to_sincos (may_t z) {
  return may_div_c (may_sin_c (z), may_cos_c (z));
}

/* tanh(x) = sinh(x)/cosh(x) */
static may_t tanh_to_sincosh (may_t z) {
  return may_div_c (may_sinh_c (z), may_cosh_c (z));
}

static const char *const tan2sincos_name[] = {
  may_tan_name, may_tanh_name
};
static const void *const tan2sincos_func[] = {
  tan_to_sincos, tanh_to_sincosh
};

may_t
may_tan2sincos (may_t x)
{
  may_t y;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (tan2sincos_name) == numberof (tan2sincos_func));

  MAY_LOG_FUNC (("%Y", x));

  MAY_RECORD ();
  y = may_subs_c (x, 1, numberof (tan2sincos_name),
                  tan2sincos_name, tan2sincos_func);
  MAY_RET_EVAL (y);
}

/**************************************************************************/

/* sin(x) = cos(x)*tan(x) */
static may_t sin_to_tancos (may_t z) {
  return may_mul_c (may_tan_c (z), may_cos_c (z));
}

/* sinh(x) = cosh(x)*tanh(x) */
static may_t sinh_to_tancosh (may_t z) {
  return may_mul_c (may_tanh_c (z), may_cosh_c (z));
}

static const char *const sin2tancos_name[] = {
  may_sin_name, may_sinh_name
};
static const void *const sin2tancos_func[] = {
  sin_to_tancos, sinh_to_tancosh
};

may_t
may_sin2tancos (may_t x)
{
  may_t y;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (sin2tancos_name) == numberof (sin2tancos_func));
  MAY_RECORD ();
  y = may_subs_c (x, 1, numberof (sin2tancos_name),
                  sin2tancos_name, sin2tancos_func);
  MAY_RET_EVAL (y);
}

/**************************************************************************/

/* exp(x) = exp(1)^x */
static may_t exp_to_pow (may_t z) {
  return may_pow_c (may_exp_c (MAY_ONE), z);
}

static const char *const exp2pow_name[] = {
  may_exp_name
};
static const void *const exp2pow_func[] = {
  exp_to_pow
};

may_t
may_exp2pow (may_t x)
{
  may_t y;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (exp2pow_name) == numberof (exp2pow_func));

  MAY_LOG_FUNC (("%Y", x));

  MAY_RECORD ();
  y = may_subs_c (x, 1, numberof (exp2pow_name),
                  exp2pow_name, exp2pow_func);
  MAY_RET_EVAL (y);
}

/**************************************************************************/

static may_t
pow2exp_c (may_t x)
{
  if (MAY_ATOMIC_P (x))
    return x;
  if (MAY_TYPE (x) == MAY_POW_T) {
    may_t a = pow2exp_c (MAY_AT (x, 0));
    may_t b = pow2exp_c (MAY_AT (x, 1));
    return may_exp_c (may_mul_c (b, may_log_c (a)));
  }
  return may_map_c (x, pow2exp_c);
}
may_t
may_pow2exp (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));

  MAY_LOG_FUNC (("%Y", x));

  MAY_RECORD ();
  x = pow2exp_c (x);
  MAY_RET_EVAL (x);
}

/**************************************************************************/

/* abs(x) = x/sign(x)=x*sign(x) */
static may_t abs_to_sign (may_t z) {
  return may_mul_c (z, may_sign_c (z));
}

static const char *const abs2sign_name[] = {
  may_abs_name
};
static const void *const abs2sign_func[] = {
  abs_to_sign
};

may_t
may_abs2sign (may_t x)
{
  may_t y;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (abs2sign_name) == numberof (abs2sign_func));

  MAY_LOG_FUNC (("%Y", x));

  MAY_RECORD ();
  y = may_subs_c (x, 1, numberof (abs2sign_name),
                  abs2sign_name, abs2sign_func);
  MAY_RET_EVAL (y);
}

/**************************************************************************/

/* sign(x) = x/abs(x)=abs(x)/x */
static may_t sign_to_abs (may_t z) {
  return may_div_c (z, may_abs_c (z));
}

static const char *const sign2abs_name[] = {
  may_sign_name
};
static const void *const sign2abs_func[] = {
  sign_to_abs
};

may_t
may_sign2abs (may_t x)
{
  may_t y;
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT (numberof (sign2abs_name) == numberof (sign2abs_func));

  MAY_LOG_FUNC (("%Y", x));

  MAY_RECORD ();
  y = may_subs_c (x, 1, numberof (sign2abs_name),
                  sign2abs_name, sign2abs_func);
  MAY_RET_EVAL (y);
}
