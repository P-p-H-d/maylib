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

#ifndef __MAY_H__
#define __MAY_H__

#include <stddef.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <gmp.h>
#include <mpfr.h>

#define MAY_MAJOR_VERSION 0
#define MAY_MINOR_VERSION 7
#define MAY_PATCHLEVEL_VERSION 6

#if defined (__cplusplus)
extern "C" {
#endif

  typedef struct may_s *may_t;
  typedef struct {may_t first, second; } may_pair_t;

  typedef enum {
    MAY_COMPLEX_D=0,
    MAY_NONZERO_D=1, MAY_NONREPOS_D=2, MAY_NONRENEG_D=4,
    MAY_NONIMPOS_D=8, MAY_NONIMNEG_D=16, MAY_CINTEGER_D=32,
    MAY_CRATIONAL_D=64, MAY_EVEN_D=128, MAY_ODD_D=256, MAY_PRIME_D=512,
    MAY_OUT_UNIT_CIRCLE_D=1024, MAY_IN_UNIT_CIRCLE_D=2048,
    MAY_REAL_D        = MAY_NONIMPOS_D|MAY_NONIMNEG_D,
    MAY_REAL_NONPOS_D = MAY_NONREPOS_D|MAY_REAL_D,
    MAY_REAL_NONNEG_D = MAY_NONRENEG_D|MAY_REAL_D,
    MAY_REAL_POS_D    = MAY_REAL_NONNEG_D|MAY_NONZERO_D,
    MAY_REAL_NEG_D    = MAY_REAL_NONPOS_D|MAY_NONZERO_D,
    MAY_INTEGER_D     = MAY_CINTEGER_D|MAY_REAL_D,
    MAY_INT_NONPOS_D  = MAY_NONREPOS_D|MAY_INTEGER_D,
    MAY_INT_NONNEG_D  = MAY_NONRENEG_D|MAY_INTEGER_D,
    MAY_INT_POS_D     = MAY_INT_NONNEG_D|MAY_NONZERO_D,
    MAY_INT_NEG_D     = MAY_INT_NONPOS_D|MAY_NONZERO_D,
    MAY_INT_EVEN_D    = MAY_EVEN_D|MAY_INTEGER_D,
    MAY_INT_ODD_D     = MAY_ODD_D|MAY_INTEGER_D,
    MAY_INT_PRIME_D   = MAY_PRIME_D|MAY_INTEGER_D,
    MAY_RATIONAL_D    = MAY_CRATIONAL_D|MAY_REAL_D
  } may_domain_e;

  typedef enum {
    MAY_EXP_EXP_P=1, MAY_LOG_EXP_P=2,
    MAY_SIN_EXP_P=4, MAY_COS_EXP_P=8, MAY_TAN_EXP_P=16,
    MAY_ASIN_EXP_P=32, MAY_ACOS_EXP_P=64, MAY_ATAN_EXP_P=128,
    MAY_SINH_EXP_P=256, MAY_COSH_EXP_P=512, MAY_TANH_EXP_P=1024,
    MAY_ASINH_EXP_P=2048, MAY_ACOSH_EXP_P=4096, MAY_ATANH_EXP_P=8192
  } may_exp_p_flags_e;

  typedef struct {
    const char  *name;
    unsigned int priority;
    unsigned int flags;
    int (*zero_p) (may_t);
    int (*nonzero_p) (may_t);
    int (*one_p) (may_t);
    may_t          (*eval) (may_t);
    unsigned long  (*add)  (unsigned long, unsigned long, may_pair_t *);
    unsigned long  (*mul)  (unsigned long, unsigned long, may_pair_t *);
    may_t          (*pow)  (may_t, may_t);
    void  (*stringify) (may_t,int,void (*)(may_t,int),void (*)(const char*));
    may_t (*constructor) (may_t);
    may_t (*exp) (may_t);
    may_t (*log) (may_t);
    may_t (*sin) (may_t);
    may_t (*cos) (may_t);
    may_t (*tan) (may_t);
    may_t (*asin) (may_t);
    may_t (*acos) (may_t);
    may_t (*atan) (may_t);
    may_t (*exph) (may_t);
    may_t (*sinh) (may_t);
    may_t (*cosh) (may_t);
    may_t (*tanh) (may_t);
    may_t (*asinh) (may_t);
    may_t (*acosh) (may_t);
    may_t (*atanh) (may_t);
    may_t (*floor) (may_t);
    may_t (*sign) (may_t);
    may_t (*gamma) (may_t);
    may_t (*conj) (may_t);
    may_t (*real) (may_t);
    may_t (*imag) (may_t);
    may_t (*argument) (may_t);
    may_t (*abs) (may_t);
    may_t (*diff) (may_t, may_t);
  } may_extdef_t;

  typedef unsigned int may_ext_t;

  typedef enum {MAY_EXT_INSTALL, MAY_EXT_UPDATE} may_extreg_e;

  typedef enum {MAY_INDETS_NONE=0, MAY_INDETS_NUM=1, MAY_INDETS_RECUR=2
  } may_indets_e;

  typedef enum {
    MAY_NO_ERR=0,
    MAY_INVALID_TOKEN_ERR,
    MAY_MEMORY_ERR,
    MAY_CANT_BE_CONVERTED_ERR,
    MAY_DIMENSION_ERR,
    MAY_SINGULAR_MATRIX_ERR,
    MAY_INVALID_MAT_SIZE_ERR,
    MAY_VALUATION_NOT_POS_ERR
  } may_error_e;

  typedef union {long l; unsigned long ul; size_t s; may_t m; may_t *pm; char *c; void *v; int b;} may_iterator_t[4];
  typedef union may_mark_s {long l; unsigned long ul; size_t s; may_t m; may_t *pm; char *c; void *v; int b;} may_mark_t[3];

  typedef enum {MAY_COMBINE_NORMAL=0, MAY_COMBINE_FORCE=1} may_combine_flags_e;

  /* Define Kernel Functions */
  const char*may_get_version (void);

  void      may_kernel_start (size_t,int);
  void      may_kernel_end   (void);
  void      may_kernel_stop  (void);
  void      may_kernel_restart  (void);

  mp_rnd_t  may_kernel_rnd   (mp_rnd_t);
  int       may_kernel_base  (int);
  mp_prec_t may_kernel_prec  (mp_prec_t);
  may_domain_e may_kernel_domain   (may_domain_e );
  may_t     may_kernel_intmod (may_t);
  unsigned long may_kernel_intmaxsize (unsigned long);
  int     (*may_kernel_sort_cb (int (*n)(may_t, may_t)))(may_t, may_t);
  int     (*may_kernel_zero_cb (int (*n)(may_t)))(may_t);
  int       may_kernel_num_presimplify (int);
  int       may_kernel_worker(int,size_t);

  void      may_kernel_info  (FILE *, const char []);

  void      may_error_catch   (void(*)(may_error_e,const char[],const void*),
                               const void*);
  void      may_error_uncatch (void);
  void      may_error_get     (may_error_e*, const char **);
  void      may_error_throw   (may_error_e, const char []);
  const char *may_error_what  (may_error_e);

  void     *may_alloc        (size_t);
  void     *may_realloc      (void *, size_t, size_t);
  void      may_free         (void *, size_t);


  /* Define MARK functions */
  void      may_mark         (may_mark_t);
  may_t     may_compact      (may_mark_t, may_t);
  may_t*    may_compact_v    (may_mark_t, size_t, may_t *);
  void      may_compact_va   (may_mark_t, may_t *, ...);
  void      may_chained_compact1 (void);
  may_t     may_chained_compact2 (may_mark_t, may_t);

  /* Define constructors */
  may_t     may_set_si       (long);
  may_t     may_set_ui       (unsigned long);
  may_t     may_set_si_ui    (long, unsigned long);
  may_t     may_set_d        (double);
  may_t     may_set_ld       (long double);
  may_t     may_set_str      (const char []);
  may_t     may_set_str_domain (const char [], may_domain_e);
  may_t     may_set_str_local (may_domain_e);
  may_t     may_set_z        (mpz_srcptr);
  may_t     may_set_zz       (mpz_srcptr);
  may_t     may_set_q        (mpq_srcptr);
  may_t     may_set_fr       (mpfr_srcptr);
  may_t     may_set_cx       (may_t, may_t);
  may_t     may_set_si_c     (long);
  may_t     may_set_ui_c     (unsigned long);
  may_t     may_set_z_c      (mpz_srcptr);

  char     *may_parse_c      (may_t *, const char []);
  may_t     may_parse_str    (const char []);

  /* Define function constructors (Return value not evaluated)  */
  may_t     may_add_c        (may_t, may_t);
  may_t     may_addinc_c     (may_t, may_t);
  may_t     may_sub_c        (may_t, may_t);
  may_t     may_mul_c        (may_t, may_t);
  may_t     may_mulinc_c     (may_t, may_t);
  may_t     may_div_c        (may_t, may_t);
  may_t     may_pow_c        (may_t, may_t);
  may_t     may_pow_si_c     (may_t, long);
  may_t     may_neg_c        (may_t);
  may_t     may_sqr_c        (may_t);
  may_t     may_sqrt_c       (may_t);
  may_t     may_exp_c        (may_t);
  may_t     may_log_c        (may_t);
  may_t     may_abs_c        (may_t);
  may_t     may_sin_c        (may_t);
  may_t     may_cos_c        (may_t);
  may_t     may_tan_c        (may_t);
  may_t     may_asin_c       (may_t);
  may_t     may_acos_c       (may_t);
  may_t     may_atan_c       (may_t);
  may_t     may_sinh_c       (may_t);
  may_t     may_cosh_c       (may_t);
  may_t     may_tanh_c       (may_t);
  may_t     may_asinh_c      (may_t);
  may_t     may_acosh_c      (may_t);
  may_t     may_atanh_c      (may_t);
  may_t     may_func_c       (const char [], may_t);
  may_t     may_func_domain_c (const char [], may_t, may_domain_e);
  may_t     may_conj_c       (may_t);
  may_t     may_real_c       (may_t);
  may_t     may_imag_c       (may_t);
  may_t     may_argument_c   (may_t);
  may_t     may_sign_c       (may_t);
  may_t     may_floor_c      (may_t);
  may_t     may_ceil_c       (may_t);
  may_t     may_range_c      (may_t, may_t);
  may_t     may_mod_c        (may_t, may_t);
  may_t     may_gcd_c        (may_t, may_t);
  may_t     may_fact_c       (may_t);
  may_t     may_gamma_c      (may_t);

  may_t     may_list_vc      (size_t, const may_t *);
  may_t     may_add_vc       (size_t, const may_t *);
  may_t     may_mul_vc       (size_t, const may_t *);

  may_t     may_list_vac     (may_t, ...);
  may_t     may_add_vac      (may_t, ...);
  may_t     may_mul_vac      (may_t, ...);

  may_t     may_copy_c       (may_t, int);

  /* Function (Return value evaluated) */
  may_t     may_add          (may_t, may_t);
  may_t     may_sub          (may_t, may_t);
  may_t     may_mul          (may_t, may_t);
  may_t     may_div          (may_t, may_t);
  may_t     may_pow          (may_t, may_t);
  may_t     may_neg          (may_t);
  may_t     may_sqr          (may_t);
  may_t     may_sqrt         (may_t);
  may_t     may_exp          (may_t);
  may_t     may_log          (may_t);
  may_t     may_abs          (may_t);
  may_t     may_sin          (may_t);
  may_t     may_cos          (may_t);
  may_t     may_tan          (may_t);
  may_t     may_asin         (may_t);
  may_t     may_acos         (may_t);
  may_t     may_atan         (may_t);
  may_t     may_sinh         (may_t);
  may_t     may_cosh         (may_t);
  may_t     may_tanh         (may_t);
  may_t     may_asinh        (may_t);
  may_t     may_acosh        (may_t);
  may_t     may_atanh        (may_t);
  may_t     may_conj         (may_t);
  may_t     may_real         (may_t);
  may_t     may_imag         (may_t);
  may_t     may_argument     (may_t);
  may_t     may_sign         (may_t);
  may_t     may_floor        (may_t);
  may_t     may_ceil         (may_t);
  may_t     may_gcd          (unsigned long, const may_t []);
  may_t     may_lcm          (unsigned long, const may_t []);
  int       may_gcdex        (may_t *, may_t *, may_t *,
			      may_t, may_t, may_t, may_t);
  void      may_content      (may_t *, may_t *, may_t, may_t);
  may_t     may_sqrfree      (may_t, may_t);
  may_t     may_ratfactor    (may_t, may_t);

  /* Define get functions */
  int       may_get_ui        (unsigned long *, may_t);
  int       may_get_si        (long *, may_t);
  int       may_get_str       (const char **, may_t);
  int       may_get_d         (double *, may_t);
  int       may_get_ld        (long double *, may_t);
  int       may_get_z         (mpz_t, may_t);
  int       may_get_q         (mpq_t, may_t);
  int       may_get_fr        (mpfr_t, may_t);
  int       may_get_cx        (may_t *, may_t *, may_t);

  /* Define Replace Functions */
  may_t     may_subs_c        (may_t, unsigned long, size_t,
                               const char *const*, const void *const*);
  may_t     may_replace       (may_t, may_t, may_t);
  int       may_match_p       (may_t *, may_t, may_t, int, int (**)(may_t));
  may_t     may_rewrite       (may_t, may_t, may_t);
  may_t     may_rewrite2      (may_t, may_t, may_t, int, int (**)(may_t));

  /* Define traversal functions */
  const char *may_get_name    (may_t);
  size_t    may_nops          (may_t);
  may_t     may_op            (may_t, size_t);
  may_t     may_map_c         (may_t, may_t (*)(may_t));
  may_t     may_map           (may_t, may_t (*)(may_t));
  may_t     may_map2_c        (may_t, may_t (*)(may_t,void*),void*);
  may_t     may_map2          (may_t, may_t (*)(may_t,void*),void*);

  /* Define evaluators */
  may_t     may_eval          (may_t);
  may_t     may_reeval        (may_t);
  may_t     may_evalf         (may_t);
  may_t     may_evalr         (may_t);
  may_t     may_hold          (may_t);

  may_t     may_approx        (may_t, unsigned int, unsigned long, mp_rnd_t);

  /* Define Predicates */
  int       may_num_p         (may_t);
  int       may_zero_p        (may_t);
  int       may_nonzero_p     (may_t);
  int       may_zero_fastp    (may_t);
  int       may_one_p         (may_t);
  int       may_pos_p         (may_t);
  int       may_neg_p         (may_t);
  int       may_nonpos_p      (may_t);
  int       may_nonneg_p      (may_t);
  int       may_real_p        (may_t);
  int       may_integer_p     (may_t);
  int       may_imminteger_p  (may_t);
  int       may_cinteger_p    (may_t);
  int       may_rational_p    (may_t);
  int       may_crational_p   (may_t);
  int       may_posint_p      (may_t);
  int       may_negint_p      (may_t);
  int       may_nonposint_p   (may_t);
  int       may_nonnegint_p   (may_t);
  int       may_even_p        (may_t);
  int       may_odd_p         (may_t);
  int       may_prime_p       (may_t);
  int       may_nan_p         (may_t);
  int       may_inf_p         (may_t);
  int       may_undef_p       (may_t);
  int       may_purenum_p     (may_t);
  int       may_purereal_p    (may_t);
  int       may_eval_p         (may_t);

  /* Iterator functions */
  int       may_sum_p (may_t);
  may_t     may_sum_iterator_init (may_iterator_t, may_t);
  void      may_sum_iterator_next (may_iterator_t);
  int       may_sum_iterator_end  (may_t *, may_t *, may_iterator_t);
  may_t     may_sum_iterator_ref  (may_iterator_t);
  may_t     may_sum_iterator_tail  (may_iterator_t);
  bool      may_sum_extract (may_t *, may_t *, may_t);
  int       may_product_p (may_t);
  may_t     may_product_iterator_init (may_iterator_t, may_t);
  void      may_product_iterator_next (may_iterator_t);
  int       may_product_iterator_end  (may_t *, may_t *, may_iterator_t);
  int       may_product_iterator_end2  (may_t *, may_t *, may_iterator_t);
  may_t     may_product_iterator_ref  (may_iterator_t);
  may_t     may_product_iterator_tail  (may_iterator_t);
  bool      may_product_extract (may_t *, may_t *, may_t);

  /* More advanced predicates functions */
  int       may_compute_sign  (may_t); /* 0 unkwown, 1 = 0, 2 >0, 4 <0 */
  int       may_independent_p (may_t, may_t);
  int       may_independent_vp (may_t, may_t);
  int       may_func_p        (may_t, const char *);
  int       may_exp_p         (may_t, may_exp_p_flags_e);

  /* Define comparison functions */
  int       may_identical     (may_t, may_t); /* FAST */
  int       may_cmp           (may_t, may_t); /* LEXICOGRAPHIC ORDER */

  /* Define basic operators */
  int       may_degree        (may_t *, mpz_srcptr [], may_t *,
                               may_t, size_t, const may_t[]);
  long      may_degree_si     (may_t, may_t);
  int       may_ldegree       (may_t *, mpz_srcptr [], may_t *,
                               may_t, size_t, const may_t[]);
  long      may_ldegree_si    (may_t, may_t);

  int       may_div_qr        (may_t *, may_t *, may_t, may_t, may_t);
  void      may_div_qr_xexp   (may_t *, may_t *, may_t, may_t, may_t);

  may_t     may_indets        (may_t, may_indets_e);

  may_t     may_diff_c        (may_t, may_t, may_t, may_t);

  may_t     may_trig2exp      (may_t);
  may_t     may_trig2tan2     (may_t);
  may_t     may_exp2trig      (may_t);
  may_t     may_tan2sincos    (may_t);
  may_t     may_sin2tancos    (may_t);
  may_t     may_exp2pow       (may_t);
  may_t     may_pow2exp       (may_t);
  may_t     may_abs2sign      (may_t);
  may_t     may_sign2abs      (may_t);

  may_t     may_combine       (may_t, may_combine_flags_e);
  may_t     may_normalsign    (may_t);
  may_t     may_tcollect      (may_t);

  may_t     may_expand        (may_t);
  may_t     may_collect       (may_t, may_t);
  may_t     may_diff          (may_t, may_t);
  may_t     may_antidiff      (may_t, may_t);
  may_t     may_sqrtsimp      (may_t);
  void      may_rectform      (may_t *, may_t *, may_t);
  void      may_comdenom      (may_t *, may_t *, may_t);
  may_t     may_partfrac      (may_t, may_t, may_t (*) (may_t, may_t));
  may_t     may_taylor        (may_t, may_t, may_t, unsigned long);
  may_t     may_series        (may_t, may_t, unsigned long);
  may_t     may_texpand       (may_t);
  may_t     may_rationalize   (may_t);
  may_t     may_eexpand       (may_t);
  may_t     may_recursive     (may_t, may_t (*) (may_t), unsigned long);
  may_t     may_smod          (may_t, may_t);

  bool      may_upol2array    (unsigned long *, may_t **,
                               may_t , may_t, bool);
  may_t     may_array2upol    (unsigned long, may_t *, may_t);

  /* I/O functions */
  void      may_dump          (may_t);
  size_t    may_in_string     (may_t *, FILE *);
  size_t    may_out_string    (FILE *, may_t);
  char     *may_get_string    (char [], size_t, may_t);

  /* User DATA functions */
  may_t     may_data_c        (size_t);
  size_t    may_data_size     (may_t);
  void     *may_data_ptr      (may_t);
  const void *may_data_srcptr (may_t);

  /* Type extension functions */
  may_ext_t may_ext_register  (const may_extdef_t *, may_extreg_e);
  int       may_ext_unregister (const char []);
  may_ext_t may_ext_find      (const char []);
  may_ext_t may_ext_p         (may_t);
  const may_extdef_t *may_ext_get (may_ext_t);
  may_t     may_ext_c         (may_ext_t, size_t);
  void      may_ext_set_c     (may_t, size_t, may_t);

  /* Some extensions */
  void      may_rootof_ext_init (void);

  /* Define name of internal functions and variables */
  extern const char may_sqrt_name[];
  extern const char may_exp_name[];
  extern const char may_log_name[];
  extern const char may_ln_name[];
  extern const char may_sin_name[];
  extern const char may_cos_name[];
  extern const char may_tan_name[];
  extern const char may_asin_name[];
  extern const char may_acos_name[];
  extern const char may_atan_name[];
  extern const char may_sinh_name[];
  extern const char may_cosh_name[];
  extern const char may_tanh_name[];
  extern const char may_asinh_name[];
  extern const char may_acosh_name[];
  extern const char may_atanh_name[];
  extern const char may_abs_name[];
  extern const char may_sign_name[];
  extern const char may_floor_name[];
  extern const char may_mod_name[];
  extern const char may_gcd_name[];
  extern const char may_real_name[];
  extern const char may_imag_name[];
  extern const char may_conj_name[];
  extern const char may_argument_name[];
  extern const char may_nan_name[];
  extern const char may_inf_name[];
  extern const char may_I_name[];
  extern const char may_pi_name[];
  extern const char may_integer_name[];
  extern const char may_rational_name[];
  extern const char may_float_name[];
  extern const char may_complex_name[];
  extern const char may_sum_name[];
  extern const char may_product_name[];
  extern const char may_product2_name[];
  extern const char may_pow_name[];
  extern const char may_string_name[];
  extern const char may_mat_name[];
  extern const char may_list_name[];
  extern const char may_upol_name[];
  extern const char may_range_name[];
  extern const char may_gamma_name[];
  extern const char may_diff_name[];
  extern const char may_data_name[];
  extern const char may_attribute_name[];

#define may_compact(...) MAY_2ARGS( __VA_ARGS__, may_compact(__VA_ARGS__), may_compact(__VA_ARGS__),may_compact(__VA_ARGS__),may_compact(may_my_mark,__VA_ARGS__),)
#define may_compact_v(...) MAY_3ARGS( __VA_ARGS__, may_compact_v(__VA_ARGS__), may_compact_v(__VA_ARGS__),may_compact_v(__VA_ARGS__),may_compact_v(may_my_mark,__VA_ARGS__),)
#define may_mark(...) MAY_1ARG(MAY_ ## __VA_ARGS__ ##_ONE_VAR, may_mark_t may_my_mark;may_mark(may_my_mark),may_mark(__VA_ARGS__),may_mark(__VA_ARGS__),may_mark(__VA_ARGS__), )
#define may_keep2(_m,_x) (may_chained_compact1(), may_chained_compact2(_m,_x))
#define may_keep(...) MAY_2ARGS( __VA_ARGS__, may_keep2(__VA_ARGS__), may_keep2(__VA_ARGS__),may_keep2(__VA_ARGS__),may_keep2(may_my_mark,__VA_ARGS__),)

#define MAY__ONE_VAR a,b,c,d
#define MAY_1ARG(...) MAY_2ARGS(__VA_ARGS__)
#define MAY_2ARGS(a,b,c,d,e,...) e
#define MAY_3ARGS(a,b,c,d,e,f,...) f

#if defined (__cplusplus)
}
#endif

#endif /* __MAY_H__   */
