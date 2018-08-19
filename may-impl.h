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

#ifndef __MAY_IMPL_H__
#define __MAY_IMPL_H__

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include <ctype.h>

#include "may.h"
#include "may-macros.h"

#ifndef MAY_MAX_EXTENSION
# define MAY_MAX_EXTENSION 20
#endif

#ifndef MAY_MAX_CACHE_SET_STR
# define MAY_MAX_CACHE_SET_STR 8
#endif

#ifndef MAY_MAX_TRY_HEUGCD
# define MAY_MAX_TRY_HEUGCD 4
#endif

#ifndef MAY_MAX_MPZ_CONSTANT
# define MAY_MAX_MPZ_CONSTANT 512
#endif

#ifndef MAY_SORT_THRESHOLD1
# define MAY_SORT_THRESHOLD1 7
#endif

#ifndef MAY_SORT_THRESHOLD2
# define MAY_SORT_THRESHOLD2 27837
#endif

#ifndef MAY_SORT_THRESHOLD3
# define MAY_SORT_THRESHOLD3 79803
#endif

#ifndef MPFR_VERSION
# error "MPFR v2.1.0 or above required"
#endif

#if !defined (__GNUC__) || (__GNUC__ < 3) || defined (__cplusplus)
# error "GCC 3.x or above needed!"
#endif


/************* Constants for supporting different compiler *****************/
#ifdef __GNUC__

#define MAY_LIKELY(x)    (__builtin_expect(!!(x),1))
#define MAY_UNLIKELY(x)  (__builtin_expect((x),0))
#define MAY_INLINE       static __inline__ __attribute__((always_inline, unused))
#define MAY_NORETURN     __attribute__((__noreturn__))
#define MAY_PUREFUNC     __attribute__((__const__))

#if defined (__i386__) || defined (__m68k__)
# define MAY_REGPARM      __attribute__((__regparm__(3)))
#endif

#if __GNUC__ >= 4
# define MAY_HAVE_BUILTIN_CLZ
#endif

#else

#define MAY_LIKELY(x)    (x)
#define MAY_UNLIKELY(x)  (x)
#define MAY_INLINE       static inline
#define MAY_NORETURN
#define MAY_PUREFUNC

/* Not perfect, but it should work (with warnings) */
#define typeof(x)        intptr_t

#endif /* __GNUC__ */

#ifndef MAY_REGPARM
# define MAY_REGPARM
#else
# define MAY_REGPARM_DEFINED
#endif

/*********************************************************************/
/* GCC 3.4.4 has some problems implemeting the inlining:
 * "sorry, unimplemented: inlining failed in call to '....' */
#if __GNUC__ == 3 && __GNUC_MINOR__ == 4
#undef MAY_INLINE
#define MAY_INLINE static __attribute__((unused))
#endif

/*********************************************************************/
#if defined(MAY_WANT_THREAD)
# include "may-thread.h"
#else
# define MAY_MAX_THREAD         1
# define MAY_THREAD_ATTR        /* empty */
# define MAY_SPAWN_BLOCK(block,mark) /* empty */
# define MAY_SPAWN_SYNC(block)  /* empty */
# define MAY_SPAWN(block, input, core, output) core
# define MAY_ATOMIC_ATTR        /* empty */
# define MAY_DEF_IF_THREAD(...)   /* x */
# define MAY_ATOMIC_ADD(var, val) ( (var) += (val), (var) - (val) )

/* Define index of an array for threaded for */
typedef long may_int_t;

# define MAY_SPAWN_FOR(_mark, _var, _begin, _end, _in, _core)            \
  for(may_int_t (_var) = (_begin); (_var) < (_end); (_var)++) {         \
    _core ;                                                             \
  }
# define MAY_SPAWN_FOR_REDUCE(_mark, _var, _begin, _end, _in,           \
                              _reduce_macro, _global_reduced_var, _local_out, \
                              _core)                                    \
  for(may_int_t (_var) = (_begin); (_var) < (_end); (_var)++) {         \
    _core ;                                                             \
    if ((_var) == (_begin)) _global_reduced_var = _local_out;           \
    else _reduce_macro(_global_reduced_var, _local_out);                \
  }
#endif


/**************** Define ASSERT system *******************************/

/* ASSUME is like assert(), but it is a hint to a compiler about a
   statement of fact in a function call free expression, which allows
   the compiler to generate better machine code.
   __builtin_unreachable has been introduced in GCC 4.5 but it works
   fine since 4.8 only (before it generated illegal code).
   Use of __builtin_constant_p is to avoid calling function in assertions:
   it detects if the expression can be reduced to a constant.
*/
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 8 || __GNUC__ > 5) /* 4.8 -> */
# define MAY_ASSUME(x)                                                  \
  (__builtin_constant_p( !!(x) || !(x)) == 0 || (x) ? (void)0 : __builtin_unreachable())
#elif defined(_MSC_VER)
# define MAY_ASSUME(x) __assume(x)
#else
# define MAY_ASSUME(x) ((void) 0)
#endif

void MAY_NORETURN may_assert_fail (const char *, int, const char *);
#ifdef MAY_WANT_ASSERT
# define MAY_ASSERT(expr) \
  ((void) ((expr) || (may_assert_fail (__FILE__, __LINE__, #expr), 0)))
#else
# define MAY_ASSERT(expr) MAY_ASSUME(expr)
#endif




/******************* Define enum types *********************************/
enum may_enum_t {
  MAY_INT_T, MAY_RAT_T, MAY_FLOAT_T, MAY_COMPLEX_T, MAY_NUM_LIMIT,
  MAY_STRING_T, MAY_DATA_T, MAY_INDIRECT_T, MAY_ATOMIC_LIMIT,
  MAY_EXP_T, MAY_LOG_T, MAY_ABS_T, MAY_SIGN_T, MAY_FLOOR_T,
  MAY_SIN_T, MAY_COS_T, MAY_TAN_T, MAY_ASIN_T, MAY_ACOS_T, MAY_ATAN_T,
  MAY_SINH_T, MAY_COSH_T, MAY_TANH_T, MAY_ASINH_T, MAY_ACOSH_T, MAY_ATANH_T,
  MAY_CONJ_T, MAY_REAL_T, MAY_IMAG_T, MAY_ARGUMENT_T,
  MAY_GAMMA_T, MAY_UNARYFUNC_LIMIT,
  MAY_FACTOR_T, MAY_POW_T, MAY_BINARYTREE_T,
  MAY_DIFF_T, MAY_MOD_T, MAY_GCD_T, MAY_RANGE_T, MAY_BINARYFUNC_LIMIT,
  MAY_FUNC_T,
  MAY_SUM_T, MAY_PRODUCT_T,
  MAY_SUM_RESERVE_T, MAY_PRODUCT_RESERVE_T,
  MAY_END_LIMIT, MAY_LIST_T, MAY_MAT_T
};


/************************* Define Model ******************************/
#if !defined(MAY_WANT_SMALL_MODEL) && !defined(MAY_WANT_MICRO_MODEL)    \
  && !defined(MAY_WANT_HUGE_MODEL)  && !defined(MAY_WANT_NORMAL_MODEL)  \
  && !defined(MAY_WANT_BIG_MODEL)
# if (INT_MAX == 32767)
#  define MAY_WANT_SMALL_MODEL    /* 16 Bits */
# elif (LONG_MAX == 2147483647)
#  define MAY_WANT_NORMAL_MODEL   /* 32 Bits */
# else
#  define MAY_WANT_BIG_MODEL      /* 64 Bits */
# endif
#endif

/* Huge or normal model */
#if defined(MAY_WANT_HUGE_MODEL) || defined(MAY_WANT_NORMAL_MODEL)      \
  || defined(MAY_WANT_BIG_MODEL)

#if defined(MAY_WANT_HUGE_MODEL)     /* 64 Bits Header */
typedef uint16_t may_type_t;
typedef uint16_t may_flags_t;
typedef uint32_t may_hash_t;
typedef unsigned long may_size_t;
#define MAY_HASH_MAX (1UL<<20)
#define MAY_SMALL_ALLOC_SIZE 1000
#elif defined(MAY_WANT_NORMAL_MODEL) || defined(MAY_WANT_BIG_MODEL)
 /* 32 Bits Header */
typedef uint8_t  may_type_t;
typedef uint8_t  may_flags_t;
typedef uint16_t may_hash_t;
typedef unsigned int may_size_t;
#define MAY_HASH_MAX (1UL<<16)
#ifdef MAY_WANT_NORMAL_MODEL
# define MAY_SMALL_ALLOC_SIZE 100
#else
# define MAY_SMALL_ALLOC_SIZE 200
#endif
#endif

enum may_flags_e { MAY_EVAL_F = 1, MAY_NUM_F = 2, MAY_ALL_F = 3,
                   MAY_EXPAND_F = 4};
typedef struct {
  may_type_t  type;
  may_flags_t flags;
  may_hash_t  hash;
} may_header_t;

#define MAY_TYPE(x)           ((x)->header.type+0)
#define MAY_FLAGS(x)          ((x)->header.flags+0)
#define MAY_HASH(x)           ((x)->header.hash+0)
#define MAY_OPEN_C(x, t)      ((x)->header.type=(t), (x)->header.flags = 0)
#define MAY_CLOSE_C(x, f, h)  ((x)->header.flags=(f)&MAY_ALL_F, (x)->header.hash = ((h)^(x)->header.type)&(MAY_HASH_MAX-1))
#define MAY_SET_FLAG(x,f)     ((x)->header.flags |=  (f))
#define MAY_CLEAR_FLAG(x,f)   ((x)->header.flags &= ~(f))

#ifdef MAY_WANT_BIG_MODEL
# define MAY_DEFINE_IF_PADDING(x) x
# define MAY_DEFINE_IF_NO_PADDING(x)
#endif

/* Small or Micro model */
#elif defined(MAY_WANT_SMALL_MODEL) || defined(MAY_WANT_MICRO_MODEL)

typedef uint16_t may_header_t;
typedef uint16_t may_flags_t;
typedef uint16_t may_hash_t;
typedef uint16_t may_type_t;
typedef unsigned int may_size_t;
enum may_flags_e { MAY_EVAL_F = 1*64, MAY_NUM_F = 2*64, MAY_ALL_F = 3*64 };
#define MAY_EXPAND_F 0

#define MAY_TYPE(x)           ((int)((x)->header & 63))
#define MAY_FLAGS(x)          ((int)((x)->header & (3*64)))
#define MAY_HASH(x)           ((x)->header >> 8)
#define MAY_OPEN_C(x, t)      ((x)->header = (t))
#define MAY_CLOSE_C(x, f, h)  ((x)->header|= ((f)&MAY_ALL_F)|(((h)^((x)->header & 63))<<8))
#define MAY_SET_FLAG(x,f)     ((x)->header|=  (f))
#define MAY_CLEAR_FLAG(x,f)   ((x)->header&= ~(f))

#define MAY_HASH_MAX 256
#define MAY_USE_DJB_HASH
#define MAY_SMALL_ALLOC_SIZE 5
#endif

/* If the model has not overloaded the padding macros, define them in neutral way. */
#ifndef MAY_DEFINE_IF_PADDING
# define MAY_DEFINE_IF_PADDING(x)
# define MAY_DEFINE_IF_NO_PADDING(x) x
#endif


/************************* Define Header type ***************************/
#define MAY_NUM_P(x)          ((MAY_FLAGS(x)&MAY_NUM_F) != 0)
#define MAY_PURENUM_P(x)      (MAY_TYPE(x) < MAY_NUM_LIMIT)
#define MAY_PUREREAL_P(x)     (MAY_TYPE (x) <= MAY_FLOAT_T)
#define MAY_EVAL_P(x)         ((MAY_FLAGS(x)&MAY_EVAL_F) != 0)
#define MAY_ATOMIC_P(x)       (MAY_TYPE(x) < MAY_ATOMIC_LIMIT)
#define MAY_NODE_P(x)         (MAY_TYPE(x) > MAY_ATOMIC_LIMIT)


/************************* Define HASH system ***************************/
#if   defined(MAY_USE_DJB_HASH)
#define MAY_HASH_INIT 5381UL
#define MAY_HASH_CALC(h1,h2)  (((h1) * (uint32_t) 33) + (h2))
#elif defined(MAY_USE_JSHASH)
#define MAY_HASH_INIT 1315423911UL
#define MAY_HASH_CALC(h1,h2)  ((h1) ^ (((h1) << 5) + (h2) + (h1) >> 2))
#elif defined(MAY_USE_BKDRHASH)
#define MAY_HASH_INIT 0UL
#define MAY_HASH_CALC(h1,h2)  (((h1) * 131) + (h2))
#elif defined(MAY_USE_SDBMHASH)
#define MAY_HASH_INIT 0UL
#define MAY_HASH_CALC(h1,h2)  ((h2) + (h1) << 6 + ((h1) << 16) - (h1))
#elif defined(MAY_USE_DEKHASH)
#define MAY_HASH_INIT 0UL /* should be len */
#define MAY_HASH_CALC(h1,h2)  (((h1)<<5)  ^ (hash >> 27) ^(h2))
#elif defined(MAY_USE_BPHASH)
#define MAY_HASH_INIT 0UL
#define MAY_HASH_CALC(h1,h2)  ((h1) << 7 ^ (h2))
#elif defined(MAY_USE_FNVHASH)
#define MAY_HASH_INIT 0UL
#define MAY_HASH_CALC(h1,h2)  (((h1) * (uint32_t) 0x811C9DC5) ^ (h2))
#else
#define MAY_HASH_INIT 0UL
#define MAY_HASH_CALC(h1,h2)  (((h1) * 31421) + (h2) + 6927)
#endif
#define MAY_NEW_HASH(h1,h2)   MAY_HASH_CALC(MAY_HASH_CALC(MAY_HASH_INIT,(h1)),(h2))
#define MAY_NEW_HASH2(x,y)    MAY_NEW_HASH(MAY_HASH(x), MAY_HASH(y))

#define MAY_HASH_DECL(hash)   uint32_t hash = MAY_HASH_INIT
#define MAY_HASH_UP(hash,h)   hash = MAY_HASH_CALC(hash, (h))
#define MAY_HASH_GET(hash)    ((may_hash_t) (hash))


/* Define FLAGS system */
#define MAY_FLAG_DECL(flag)   unsigned int flag = MAY_ALL_F
#define MAY_FLAG_UP(flag,f)   (flag &= (unsigned int) (f))
#define MAY_FLAG_GET(flag)    ((may_flags_t) flag)


/******************** Define main structures ********************************/
/* In 64 bits, we get typically some paddings and lose some space...
   Try to reuse the padding for something else */
struct may_node_s {
  MAY_DEFINE_IF_NO_PADDING(may_size_t    size;)
  struct may_s *tab[1];
};
struct may_symbol_s {
  MAY_DEFINE_IF_NO_PADDING(may_size_t    size;)
  may_domain_e domain;
  char name[];
};
struct may_data_s {
  size_t size;
  void  *data[]; /* We use void* instead of char for alignement reasons */
};


/* Define generic type: HEADER + VALUE */
struct may_s {
  may_header_t header;
  MAY_DEFINE_IF_PADDING(may_size_t    size;)
  union {
    mpz_t  z;
    mpq_t  q;
    mpfr_t f;
    struct may_symbol_s s;
    struct may_data_s data;
    struct may_s *ptr;
    struct may_node_s n;
  } val[];
};

/* Define access to generic type with assertion control */
#define MAY_DEREF(_x,_t,_op) ((MAY_ASSERT(MAY_TYPE(_x)==_t), (_x))->val[0]._op)
#define MAY_INT(x)      MAY_DEREF(x,MAY_INT_T,z)
#define MAY_RAT(x)      MAY_DEREF(x,MAY_RAT_T,q)
#define MAY_FLOAT(x)    MAY_DEREF(x,MAY_FLOAT_T,f)
#define MAY_SYMBOL(x)   MAY_DEREF(x,MAY_STRING_T,s)
#define MAY_NAME(x)     MAY_DEREF(x,MAY_STRING_T,s.name)
#define MAY_DATA(x)     MAY_DEREF(x,MAY_DATA_T,data)
#define MAY_NODE(x)     ((x)->val[0].n)
#define MAY_RE(x)       MAY_DEREF(x,MAY_COMPLEX_T,n.tab[0])
#define MAY_IM(x)       MAY_DEREF(x,MAY_COMPLEX_T,n.tab[1])

#define MAY_NODE_SIZE(x)                        \
  MAY_DEFINE_IF_PADDING( (x)->size )            \
  MAY_DEFINE_IF_NO_PADDING(MAY_NODE(x).size)

#define MAY_SYMBOL_SIZE(x)                      \
  MAY_DEFINE_IF_PADDING( (x)->size )            \
  MAY_DEFINE_IF_NO_PADDING(MAY_SYMBOL_SIZE(x))

/* INDIRECT is only used in non evaluated expressions */
#define MAY_INDIRECT(x) MAY_DEREF(x,MAY_INDIRECT_T,ptr)

/* Insecure DEREF for soon to be initialized vars. DON'T USE THEM! */
#define MAY_DEREF_2(_x,_t,_op) (((_x))->val[0]._op)
#define MAY_SYMBOL_2(x)   MAY_DEREF_2(x,MAY_STRING_T,s)
#define MAY_NAME_2(x)     MAY_DEREF_2(x,MAY_STRING_T,s.name)

/* Define size to alloc in function of type. Not very clean */
#define MAY_INT_SIZE     (sizeof (struct may_s) + sizeof (mpz_t))
#define MAY_RAT_SIZE     (sizeof (struct may_s) + sizeof (mpq_t))
#define MAY_FLOAT_SIZE   (sizeof (struct may_s) + sizeof (mpfr_t))
#define MAY_NODE_ALLOC_SIZE(n) (sizeof (struct may_s) + sizeof (struct may_node_s) + ((n)-1)*sizeof (may_t))
#define MAY_NAME_SIZE(_s) ({unsigned long _n = sizeof (struct may_s) + sizeof (struct may_symbol_s) + MAY_ALIGNED_SIZE(_s); _n; })
#define MAY_COMPLEX_SIZE MAY_NODE_ALLOC_SIZE(2)
#define MAY_DATA_SIZE(_s) (sizeof (struct may_s) + sizeof (struct may_data_s) + (_s)*sizeof (unsigned char))

/***** Define global structures *****/

/*Define Heap Globals. The heap is a stack which grows.
  + base is the start of the stack (the pointer allocated by the system)
  + top is the current pointer to the free area of the stack.
  + limit is the end of the stack
    We have the constraint: base <= top <= limit
  + max_top is the maximal value reached by top (statistic)
  + num_resize is the number of times the stack has been resized (statistic)
  + allow_extend is a boolean indicating if the stack is allowed to be extended.
  + compact_func_disable is a boolean indicating if the next mark shall not perform its compacting of the memory (through may_keep)
 */
struct may_heap_s {
  char *top, *limit;
  char *base;
  void *comp_mark;
  MAY_DEF_IF_THREAD (void *comp_base, *comp_limit;)
  unsigned long compdiff;
  char compact_func_disable;
  char allow_extend;
  unsigned int num_resize;
  char *max_top, *current_mark;
  MAY_DEF_IF_THREAD (struct may_heap_s *next_heap_to_free;)
};

/* Define Subs globals used by may_subs to avoid pushing too many parameters:
   See may_subs_c for details
 */
struct may_subs_s {
  size_t *gtab;
  size_t gsize;
  const char **gvars;
  void **gvalue;
  unsigned long long mask;
  double hash;
};

/* Define rewrite globals used by may_rewrite to avoid pushing too many parameters
   See may_rewrite for details */
struct may_rewrite_s {
  int size;
  may_t *value;
  may_t p1, p2;
  int (**funcp) (may_t);
  char **vars;
};

/* Types used by may_karatsuba:
   + may_karatsuba_unsigned_long: defines the used size for the exponent
   + may_linked_term_t defines a list of multivariate monomials */
typedef unsigned long may_karatsuba_unsigned_long;
typedef struct may_linked_term_s {
  struct may_linked_term_s *next;
  may_t coeff;
  may_karatsuba_unsigned_long expo[];
} *may_linked_term_t;

/* Define karatsuba globals used by may_karatsuba.
   See may_karatsuba for details and use.
   There is no need to define it inside the error frame since no user code
   can be called while in the function (so the function is not reentrant) */
struct may_karatsuba_s {
  may_linked_term_t freeterm;
  unsigned long threshold;
  may_t tmpnum;
};

/* Types used by may_antidiff */
/* Define the different kind of conditions for a parameter
   in a formula. We have 3 parameters A, B & C & D */
typedef enum {
  NOTHING, A_POSITIF, B_POSITIF, B_NEGATIF, ApB_POSITIF, ApB_NEGATIF, B2_m_4AC_POSITIF, B2_m_4AC_NEGATIF, B_NEQ_D, B_LIKELY_NEGATIF
 } may_antidiff_condition_e;
/* Define some global variables used everywhere to avoid passing them
   to all functions */
struct may_antidiff_s {
 may_t X, A, B, C, D, P;
 may_antidiff_condition_e condition;
};

/* Define globals which must be saved in the error frame:
   + intmod: All integer operations are performed modulo this number (!=0) or not
   + intmaxsize: Maximal estimated size of an integer
   + sort_cb: a callback to sort the elements (not used)
   + zero_cb: a callback used to determine if an element is zero
   + next: the next error_frame in the linked list of error frame
   + error_handler: the handler to call in case of a throw
   + error_handler_data: data used by the error handler to parametrize its behaviour
   + prec: the precision used for the new floating point
   + rnd_mode: the rounding mode used for the floating point operation
   + base: the base used to convert integer/float for the I/O operations
   + num_presimplify: presimplify the float at parsing times (1) or wait until we know which prec we needs (0)
   + domain: domain of all new variables
   + cache_set_str_i / cache_set_str_n / cache_set_str : cache used by may_set_str to return a previously created number instead of a new one.
 */
struct may_error_frame_s {
  may_t intmod;
  unsigned long intmaxsize;
  int (*sort_cb)(may_t, may_t);
  int (*zero_cb)(may_t);
  struct may_error_frame_s *next;
  void (*error_handler)(may_error_e,const char*,const void*);
  const void *error_handler_data;
  mp_prec_t prec;
  mp_rnd_t rnd_mode;
  int base;
  int num_presimplify;
  may_domain_e domain;
  unsigned int cache_set_str_i, cache_set_str_n;
  may_t cache_set_str[MAY_MAX_CACHE_SET_STR];
};

/* Define globals.
   + Heap: the heap
   + frame: the error frame
   + complimit / compdiff : used by may_compact
   + local_counter: used for creating a new temporary variable
   + last_error_str / last_error : the last error code and string
   + org_gmp_alloc / org_hgmp_realloc / org_gmp_free: the GMP functions to allocate / reallocate / free the memory before MAY overwrites them
   + mpz_constant: 0, 1, 2, 3, 0, -1, -2, -3
   + half, n_half, p_inf, n_inf, nan, I, Pi: 1/2, -1/2, +INF,-INF, NAN, I, PI
   + dummy: invalid variable used to determine that the num isn't allocated
   + extension_size: size of the extension_tab table
   + series_ext: the ext number of the series extension
   + extension_tab: the extension table
 */
struct may_globals_s{
  struct may_heap_s Heap;
  struct may_error_frame_s frame;
  struct may_karatsuba_s kara;
  struct may_antidiff_s  antidiff;
  const char *last_error_str;
  may_error_e last_error;
};
struct may_common_s {
  /* Shared by all threads */
  MAY_ATOMIC_ATTR unsigned int local_counter;
  /* After initialization, all theses variables should be constant */
  void *(*org_gmp_alloc) (size_t);
  void *(*org_gmp_realloc) (void *, size_t, size_t);
  void (*org_gmp_free) (void *, size_t);
  may_t mpz_constant[MAY_MAX_MPZ_CONSTANT*2];
  may_t half, n_half, p_inf, n_inf, nan, dummy, I, Pi;
  int extension_size;
  may_ext_t series_ext;
  const may_extdef_t *extension_tab[MAY_MAX_EXTENSION];
};
extern MAY_THREAD_ATTR struct may_globals_s may_g;
extern                 struct may_common_s  may_c;


/******* Define Heap Functions ********/
void               may_throw_memory (void) MAY_NORETURN;
MAY_REGPARM void  *may_heap_extend (unsigned long);
MAY_REGPARM may_t  may_compact_internal(may_t x, void *up);
may_t*               may_compact_vector (may_t *x, may_size_t num, void *mark);

void               may_heap_init  (struct may_heap_s *heap, unsigned long size,
                                   unsigned long low, int allow_extend);
void               may_heap_clear (struct may_heap_s *heap);

#define MAY_ALLOC_FAILED(_m) may_heap_extend (_m)
#define MAY_ALIGNED_SIZE(_n) ((size_t) ((_n)+sizeof(long)-1)& ~(size_t)(sizeof(long)-1))
#define MAY_ALLOC(_m) ({unsigned long _n = MAY_ALIGNED_SIZE(_m); (MAY_UNLIKELY (may_g.Heap.top + (_n) >= may_g.Heap.limit) ? MAY_ALLOC_FAILED(_n) : (may_g.Heap.top += (_n), may_g.Heap.top - (_n))); })
#define MAY_REALLOC(_x, _o1, _n1) ({unsigned long _m = MAY_ALIGNED_SIZE(_n1), _o = MAY_ALIGNED_SIZE(_o1); ( _o>=_m ? (_x) : may_g.Heap.current_mark <= (char*) (_x) && may_g.Heap.top == (char*)(_x)+_o ? (MAY_ALLOC(_m-_o), _x) : memcpy (MAY_ALLOC(_m), _x, _o));})


/******* Overload MARK functions **********/
/* Define the fiels of the may_mark structure */
#define MAY_MARK_CURRENT_MARK(_m)         ((_m)[0].c)
#define MAY_MARK_COMPACT_FUNC_DISABLE(_m) ((_m)[1].b)
#define MAY_MARK_HEAP_TO_FREE(_m)         ((_m)[2].v)

#undef may_mark
#undef may_compact
#undef may_compact_v
#define MAY_OVERLOADED_MARK(_m)                                         \
  ((_m)[0].c = may_g.Heap.current_mark = may_g.Heap.top,                \
   (_m)[1].b = may_g.Heap.compact_func_disable,                         \
   MAY_DEF_IF_THREAD ( (_m)[2].v = NULL, )                              \
   may_g.Heap.compact_func_disable = 0)
#define MAY_OVERLOADED_COMPACT(_m, _x)                                  \
  (MAY_DEF_IF_THREAD (may_g.Heap.next_heap_to_free = (_m)[2].v ,)       \
   MAY_DEF_IF_THREAD ((_m)[2].v = NULL ,)                               \
   may_compact_internal ((_x), (_m)[0].c) )
#define MAY_OVERLOADED_COMPACT_V(_m, _s, _t)                            \
  (MAY_DEF_IF_THREAD (may_g.Heap.next_heap_to_free = (_m)[2].v ,)       \
   MAY_DEF_IF_THREAD ((_m)[2].v = NULL ,)                               \
   may_compact_vector ((_t), (_s), (_m)[0].c) )
#define MAY_OVERLOADED_CHAINED_COMPACT1()       \
  (may_g.Heap.compact_func_disable = 1)
#define MAY_OVERLOADED_CHAINED_COMPACT2(_m, _x)                         \
  ({may_t _z = (_x);                                                    \
    may_g.Heap.compact_func_disable = (_m)[1].b;                        \
    MAY_DEF_IF_THREAD (may_g.Heap.next_heap_to_free = (_m)[2].v);       \
    MAY_DEF_IF_THREAD ((_m)[2].v = NULL);                               \
    MAY_LIKELY((_m)[1].b == 0) ?                                        \
      may_compact_internal (_z, (_m)[0].c) : _z;})

#define may_compact(...) MAY_2ARGS( __VA_ARGS__, MAY_OVERLOADED_COMPACT(__VA_ARGS__),MAY_OVERLOADED_COMPACT(__VA_ARGS__),MAY_OVERLOADED_COMPACT(__VA_ARGS__),MAY_OVERLOADED_COMPACT(may_my_mark, __VA_ARGS__),)
#define may_compact_v(...) MAY_3ARGS( __VA_ARGS__, MAY_OVERLOADED_COMPACT_V(__VA_ARGS__), MAY_OVERLOADED_COMPACT_V(__VA_ARGS__),MAY_OVERLOADED_COMPACT_V(__VA_ARGS__),MAY_OVERLOADED_COMPACT_V(may_my_mark,__VA_ARGS__),)
#define may_mark(...) MAY_1ARG(MAY_ ## __VA_ARGS__ ##_ONE_VAR, may_mark_t may_my_mark; MAY_OVERLOADED_MARK(may_my_mark), MAY_OVERLOADED_MARK(__VA_ARGS__),MAY_OVERLOADED_MARK(__VA_ARGS__),MAY_OVERLOADED_MARK(__VA_ARGS__), )
#define may_chained_compact1() MAY_OVERLOADED_CHAINED_COMPACT1()
#define may_chained_compact2(_m,_x) MAY_OVERLOADED_CHAINED_COMPACT2(_m,_x)

/******* Define garbage compact system *********/
#define MAY_RECORD()         may_mark_t may_record; may_mark(may_record)
#define MAY_COMPACT(_x)      (_x = may_compact (may_record, _x))
#define MAY_COMPACT_VOID()   (may_compact (may_record,NULL))
#define MAY_CLEANUP()        (may_compact (may_record,NULL))
#define MAY_RET(_x)          do {return may_keep (may_record, _x); } while (0)
#define MAY_RET_EVAL(_x)     do {may_t _y = may_eval (_x); return may_chained_compact2 (may_record, _y); } while (0)
#define MAY_RET_CTYPE(c)     do {may_compact (may_record, NULL); return c; } while (0)
#define MAY_COMPACT_2(_x,_y) do {may_t _tab[2] = {(_x), (_y)}; may_compact_v (may_record, 2, _tab); (_x) = _tab[0]; (_y) = _tab[1]; } while (0)


/******** Define DUMP functions *********/
#define MAY_DUMP(_x) do {printf(__FILE__":%d "#_x"[%p]= ", __LINE__, _x); may_dump(_x);} while (0)


/******** Define constants *************/
#define MAY_DUMMY  (may_c.dummy)
#define MAY_ZERO   (may_c.mpz_constant[0])
#define MAY_ONE    (may_c.mpz_constant[1])
#define MAY_TWO    (may_c.mpz_constant[2])
#define MAY_THREE  (may_c.mpz_constant[3])
#define MAY_N_ONE  (may_c.mpz_constant[MAY_MAX_MPZ_CONSTANT+1])
#define MAY_N_TWO  (may_c.mpz_constant[MAY_MAX_MPZ_CONSTANT+2])
#define MAY_N_THREE (may_c.mpz_constant[MAY_MAX_MPZ_CONSTANT+3])
#define MAY_P_INF  (may_c.p_inf)
#define MAY_N_INF  (may_c.n_inf)
#define MAY_NAN    (may_c.nan)
#define MAY_HALF   (may_c.half)
#define MAY_N_HALF (may_c.n_half)
#define MAY_I      (may_c.I)
#define MAY_PI     (may_c.Pi)

#define MAY_RND    (may_g.frame.rnd_mode)


/********* Define Node Constructors (TODO: Support Micro Model) ****/
#define MAY_NODE_C(_t, _s) ({may_t _x = MAY_ALLOC(MAY_NODE_ALLOC_SIZE(_s)); MAY_OPEN_C (_x, (_t)); MAY_NODE_SIZE(_x)=(_s); _x; })
#define MAY_NODE_RESIZE_C(_z, _t, _n)                             \
 (MAY_TYPE (_z) <= MAY_ATOMIC_LIMIT || MAY_NODE_SIZE(z) < (_n)   \
 ? MAY_NODE_C ((_t), (_n))                                        \
 : (MAY_OPEN_C ((_z), (_t)), MAY_NODE_SIZE(_z) = (_n), (_z)))
#define MAY_AT(_x, _n) (MAY_ASSERT(MAY_TYPE(_x)>MAY_ATOMIC_LIMIT && MAY_TYPE(_x)<=MAY_END_LIMIT+may_c.extension_size), (_x)->val[0].n.tab[(_n)])
#define MAY_SET_AT(_x,_n,_y) (MAY_ASSERT(MAY_TYPE(_x)>MAY_ATOMIC_LIMIT && MAY_TYPE(_x)<=MAY_END_LIMIT+may_c.extension_size), (_x)->val[0].n.tab[(_n)] = (_y) )
#define MAY_AT_PTR(_x, _n) (MAY_ASSERT (MAY_TYPE(_x)>MAY_ATOMIC_LIMIT && MAY_TYPE (_x)<=MAY_END_LIMIT+may_c.extension_size), &(_x)->val[0].n.tab[_n])

#define MAY_SET_RE(z,r)      (MAY_RE(z) = (r))
#define MAY_SET_IM(z,i)      (MAY_IM(z) = (i))


/********* Define indirect hack ******/
#define MAY_SET_INDIRECT(x, y)                                         \
 do {MAY_OPEN_C (x, MAY_INDIRECT_T); MAY_INDIRECT (x) = y;} while (0)

/********* Define Float Constructors *******/
#define MAY_LONG_DOUBLE_C(_x) ({may_t _y = MAY_ALLOC(MAY_FLOAT_SIZE); MAY_OPEN_C (_y, MAY_FLOAT_T); mpfr_init (MAY_FLOAT(_y)); mpfr_set_ld (MAY_FLOAT (_y), _x, MAY_RND); _y; })
#define MAY_DOUBLE_C(_x) ({may_t _y = MAY_ALLOC(MAY_FLOAT_SIZE); MAY_OPEN_C (_y, MAY_FLOAT_T); mpfr_init_set_d (MAY_FLOAT (_y), _x, MAY_RND); _y; })
#define MAY_MPFR_C(_x) ({may_t _y = MAY_ALLOC(MAY_FLOAT_SIZE); MAY_OPEN_C (_y, MAY_FLOAT_T); mpfr_init_set (MAY_FLOAT(_y), _x, MAY_RND); _y; })
#define MAY_MPFR_NOCOPY_C(_x) ({may_t _y = MAY_ALLOC(MAY_FLOAT_SIZE);  MAY_OPEN_C(_y, MAY_FLOAT_T); MAY_FLOAT(_y)[0] = (_x)[0]; _y; })


/********* Define Integer Constructors ******/
#define MAY_ULONG_C(_x) ({may_t _y = MAY_ALLOC(MAY_INT_SIZE); MAY_OPEN_C (_y, MAY_INT_T); mpz_init_set_ui (MAY_INT(_y), _x); _y; })
#define MAY_SLONG_C(_x) ({may_t _y = MAY_ALLOC(MAY_INT_SIZE); MAY_OPEN_C (_y, MAY_INT_T); mpz_init_set_si (MAY_INT(_y), _x); _y; })
#define MAY_MPZ_C(_x) ({may_t _y = MAY_ALLOC(MAY_INT_SIZE); MAY_OPEN_C (_y, MAY_INT_T); mpz_init_set (MAY_INT (_y), _x); _y; })
#define MAY_MPZ_NOCOPY_C(_x) ({may_t _y = MAY_ALLOC(MAY_INT_SIZE); MAY_OPEN_C (_y, MAY_INT_T); MAY_INT(_y)[0] = (_x)[0]; _y;})


/********* Define Rationnal Constructors *******/
#define MAY_MPQ_C(_x) ({may_t _y = MAY_ALLOC(MAY_RAT_SIZE); MAY_OPEN_C(_y, MAY_RAT_T); mpq_init (MAY_RAT(_y)); mpq_set(MAY_RAT(_y), _x);_y; })
#define MAY_MPQ_NOCOPY_C(_x) ({may_t _y = MAY_ALLOC(MAY_RAT_SIZE); MAY_OPEN_C (_y, MAY_RAT_T); MAY_RAT(_y)[0] = (_x)[0]; _y;})


/********* Define string constructors ******/
#define MAY_STRING_C(_x,_d) ({may_size_t _s = strlen(_x)+1; may_t _y = MAY_ALLOC(MAY_NAME_SIZE(_s)); MAY_SYMBOL_2 (_y).domain = (_d); memcpy (MAY_NAME_2(_y), _x, _s); memset (MAY_NAME_2(_y)+_s, 0, (sizeof (long) - _s % sizeof (long)) ); MAY_OPEN_C (_y, MAY_STRING_T); MAY_SYMBOL_SIZE(_y) = MAY_ALIGNED_SIZE(_s);  MAY_CLOSE_C (_y, MAY_EVAL_F, may_string_hash(MAY_NAME_2(_y))); MAY_SET_FLAG (_y, MAY_EXPAND_F); _y; })


/********* Define complex constructors ******/
#define MAY_COMPLEX_C(r, i)  ({may_t _z = MAY_ALLOC (MAY_COMPLEX_SIZE), _r = (r), _i = (i); MAY_ASSERT (MAY_PURENUM_P (_r) && MAY_PURENUM_P (_i)); MAY_OPEN_C (_z, MAY_COMPLEX_T); MAY_SET_RE (_z, _r); MAY_SET_IM (_z, _i); _z; })


/********* Define empty constructors *********/
#define MAY_INT_INIT_C() ({may_t _y = MAY_ALLOC(MAY_INT_SIZE); MAY_OPEN_C(_y, MAY_INT_T); mpz_init(MAY_INT(_y)); _y; })
#define MAY_RAT_INIT_C() ({may_t _y = MAY_ALLOC(MAY_RAT_SIZE); MAY_OPEN_C(_y, MAY_RAT_T); mpq_init(MAY_RAT(_y)); _y; })
#define MAY_FLOAT_INIT_C() ({may_t _y = MAY_ALLOC(MAY_FLOAT_SIZE); MAY_OPEN_C(_y, MAY_FLOAT_T); mpfr_init (MAY_FLOAT(_y)); _y; })
#define MAY_COMPLEX_INIT_C() ({may_t _y = MAY_ALLOC(MAY_COMPLEX_SIZE); MAY_OPEN_C (_y, MAY_COMPLEX_T); MAY_SET_RE (_y, MAY_DUMMY); MAY_SET_IM (_y, MAY_DUMMY); _y; })

/********* Define Domain ********/
#define MAY_CHECK_DOMAIN_P(x,d) ((MAY_SYMBOL (x).domain & (d)) == (d))
#define MAY_CHECK_DOMAIN(e,d) (((e) & (d)) == (d))

/********* Define Extension ******/
#define MAY_EXT_TYPE_P(x) (MAY_ASSERT (((int) (x)) <= MAY_END_LIMIT+may_c.extension_size), ((int)(x)) > MAY_END_LIMIT)
#define MAY_EXT_P(x) MAY_EXT_TYPE_P(MAY_TYPE(x))
#define MAY_EXT2INDEX(t) ((t)-MAY_END_LIMIT-1)
#define MAY_INDEX2EXT(i) ((i)+MAY_END_LIMIT+1)
#define MAY_EXT_GETX(x) (may_c.extension_tab[MAY_EXT2INDEX(MAY_TYPE(x))])


/********* Define ERROR Handling *********/
void may_error_setjmp_handler (may_error_e, const char *, const void *);

#define MAY_THROW(_error) (may_error_throw (_error, NULL), abort ())
#define MAY_TRY {jmp_buf _buffer; int _error_code; _error_code = setjmp(_buffer); if (_error_code == 0) { may_error_catch (may_error_setjmp_handler, &_buffer);
#define MAY_CATCH may_error_uncatch (); } else
#define MAY_ERROR (_error_code+0)
#define MAY_ENDTRY }


/********* Define fast predicate macros ********/
#define MAY_ZERO_P(_x) ((_x) == MAY_ZERO || (MAY_TYPE(_x)==MAY_FLOAT_T && mpfr_zero_p(MAY_FLOAT(_x))))
#define MAY_FASTZERO_P(_x) ((_x) == MAY_ZERO)
#define MAY_ONE_P(_x) ((_x) == MAY_ONE || (MAY_TYPE(_x)==MAY_FLOAT_T && !mpfr_nan_p(MAY_FLOAT(_x)) && mpfr_cmp_ui(MAY_FLOAT(_x),1) == 0))
#define MAY_FASTONE_P(_x) ((_x) == MAY_ONE)
#define MAY_POS_P(_x)  may_pos_p(_x)
#define MAY_NEG_P(_x)  may_neg_p(_x)
#define MAY_INT_P(_x)  (MAY_TYPE(_x) == MAY_INT_T)
#define MAY_REAL_P(_x) may_real_p(_x)
#define MAY_PI_P(_x)   ((_x) == may_c.Pi)
#define MAY_EVEN_P(_x) may_even_p(_x)
#define MAY_ODD_P(_x)  may_odd_p(_x)
#define MAY_NAN_P(_x)  (MAY_TYPE(_x)==MAY_FLOAT_T&&mpfr_nan_p (MAY_FLOAT(_x)))


/********* Name which are not exported *********/
extern const char may_ceil_name[];

/******* Define internal functions *********/
MAY_REGPARM size_t   may_length (may_t);
MAY_REGPARM int      may_size_in_bits(unsigned long);
int                  may_get_cpu_count(void);

/* Define Hash functions */
MAY_REGPARM may_hash_t may_mpz_hash (mpz_t);
MAY_REGPARM may_hash_t may_mpq_hash (mpq_t);
MAY_REGPARM may_hash_t may_mpfr_hash (mpfr_t);
MAY_REGPARM may_hash_t may_string_hash (const char *);
MAY_REGPARM may_hash_t may_data_hash (const char *, may_size_t);
MAY_REGPARM may_hash_t may_node_hash (const may_t *, may_size_t);
MAY_REGPARM may_hash_t may_recompute_hash (may_t);

/* Define extended numerical functions */
MAY_REGPARM may_t may_mpz_simplify (may_t x);
MAY_REGPARM may_t may_mpq_simplify (may_t org, mpq_t q);
MAY_REGPARM may_t may_mpfr_simplify (may_t x);
MAY_REGPARM may_t may_cx_simplify  (may_t x);
MAY_REGPARM may_t may_num_simplify (may_t x);

/* Define Complex Float Functions */
may_t may_cxfr_exp (may_t x);
may_t may_cxfr_cos (may_t x);
may_t may_cxfr_sin (may_t x);
may_t may_cxfr_tan (may_t x);
may_t may_cxfr_cosh (may_t x);
may_t may_cxfr_sinh (may_t x);
may_t may_cxfr_tanh (may_t x);
may_t may_cxfr_angle (may_t x);
may_t may_cxfr_log (may_t x);
may_t may_cxfr_asin (may_t x);
may_t may_cxfr_acos (may_t x);
may_t may_cxfr_atan (may_t x);
may_t may_cxfr_asinh (may_t x);
may_t may_cxfr_acosh (may_t x);
may_t may_cxfr_atanh (may_t x);

/* Define Generalized Numerical Functions */
MAY_REGPARM int may_num_zero_p (may_t);
MAY_REGPARM int may_num_pos_p (may_t);
MAY_REGPARM int may_num_poszero_p (may_t);
MAY_REGPARM int may_num_neg_p (may_t);
MAY_REGPARM int may_num_negzero_p (may_t);
MAY_REGPARM int may_num_one_p (may_t);
MAY_REGPARM may_t may_num_set (may_t, may_t);
MAY_REGPARM may_t may_num_add (may_t, may_t, may_t);
MAY_REGPARM may_t may_num_neg (may_t, may_t);
MAY_REGPARM may_t may_num_sub (may_t, may_t, may_t);
MAY_REGPARM may_t may_num_mul (may_t, may_t, may_t);
MAY_REGPARM may_t may_num_inv (may_t, may_t);
MAY_REGPARM may_t may_num_div (may_t, may_t, may_t);
MAY_REGPARM may_t may_num_abs (may_t, may_t);
MAY_REGPARM may_t may_num_conj (may_t, may_t);
MAY_REGPARM may_t may_num_pow (may_t, may_t);
MAY_REGPARM may_t may_num_gcd (may_t, may_t);
MAY_REGPARM int   may_num_cmp (may_t, may_t);
MAY_REGPARM may_t may_num_lcm (may_t, may_t);
MAY_REGPARM may_t may_num_smod (may_t, may_t);
MAY_INLINE may_t  may_num_min (may_t a, may_t b) { if (may_num_cmp (a, b) < 0) return a; else return b; }
MAY_INLINE may_t  may_num_max (may_t a, may_t b) { if (may_num_cmp (a, b) > 0) return a; else return b; }

may_t may_num_mpfr_cx_eval (may_t z,
			    int (*fcmpfr)(mpfr_ptr, mpfr_srcptr, mp_rnd_t),
			    may_t (*fccx)(may_t));

may_t     may_replace_c       (may_t, may_t, may_t);
may_t     may_replace_upol_c  (may_t, may_t, may_t);
may_t     may_replace_upol    (may_t, may_t, may_t);

/* Define Predicated Functions */
int   may_str_local_p (may_t);

may_t may_compute_num_floor (may_t) ;

/* Define Naive Ifactor */
may_t     may_naive_factor  (may_t);
may_t may_naive_ifactor (may_t);
may_t may_naive_gcd     (may_size_t, const may_t *);
may_t may_sr_gcd        (may_t, may_t, may_t);
may_t may_heur_gcd      (may_t, may_t, may_t);
mpz_srcptr may_max_coefficient (may_t);
may_t may_rebuild_gcd (may_t, may_t, may_t);
MAY_INLINE may_t may_gcd2 (may_t a, may_t b) { may_t temp[2] = {a,b}; return may_gcd(2,temp);}
may_t may_divexact      (may_t, may_t);
MAY_REGPARM may_t may_naive_gce     (may_t, may_t);
may_t may_naive_lcm     (may_size_t, const may_t *);
int   may_cmp_multidegree (may_size_t, mpz_srcptr [], mpz_srcptr []);
int   may_extract_coeff_deg (may_t *, mpz_srcptr *, may_t, may_t);
int   may_extract_coeff_multideg (may_t *c, mpz_srcptr deg[], may_t x, may_size_t n,
                                  const may_t var[n], const may_size_t index_var[n],
                                  mpz_srcptr tempdeg[n], may_t tempsum[n]);
unsigned long may_extract_coeff (unsigned long n, may_t [n], may_t, may_t);
may_t may_find_one_polvar (unsigned long , const may_t []);
may_t may_find_unused_polvar (unsigned long, const may_t []);
may_t may_trig2exp2 (may_t);
may_t may_karatsuba (may_t, may_t, may_t);

/* Define extended Eval Functions */
MAY_REGPARM may_t may_eval_sin (may_t z, may_t x);
MAY_REGPARM may_t may_eval_cos (may_t z, may_t x);
MAY_REGPARM may_t may_eval_tan (may_t z, may_t x);
MAY_REGPARM may_t may_eval_asin (may_t z, may_t x);
MAY_REGPARM may_t may_eval_acos (may_t z, may_t x);
MAY_REGPARM may_t may_eval_atan (may_t z, may_t x);
MAY_REGPARM may_t may_eval_sinh (may_t z, may_t x);
MAY_REGPARM may_t may_eval_cosh (may_t z, may_t x);
MAY_REGPARM may_t may_eval_tanh (may_t z, may_t x);
MAY_REGPARM may_t may_eval_asinh (may_t z, may_t x);
MAY_REGPARM may_t may_eval_acosh (may_t z, may_t x);
MAY_REGPARM may_t may_eval_atanh (may_t z, may_t x);
MAY_REGPARM may_t may_eval_diff (may_t);
MAY_REGPARM may_t may_eval_range (may_t);
MAY_REGPARM may_t may_eval_exp (may_t z, may_t x);
MAY_REGPARM may_t may_eval_log (may_t z, may_t x);
MAY_REGPARM may_t may_eval_floor (may_t z, may_t x);
MAY_REGPARM may_t may_eval_sign (may_t z, may_t x);
MAY_REGPARM may_t may_eval_gamma (may_t z, may_t x);
MAY_REGPARM may_t may_eval_conj (may_t z, may_t x);
MAY_REGPARM may_t may_eval_real (may_t z, may_t x);
MAY_REGPARM may_t may_eval_imag (may_t z, may_t x);
MAY_REGPARM may_t may_eval_argument (may_t z, may_t x);

/* Define extension related functions */
MAY_REGPARM may_t may_eval_extension (may_t);
void may_eval_extension_sum (may_t num, may_size_t *nsymb, may_pair_t symb[]);
void may_eval_extension_product (may_t, may_size_t *, may_pair_t tab[],
                                 may_size_t , may_pair_t org_tab[]);

may_t may_eval_extension_pow (may_t base, may_t power);
may_t may_parse_extension (const char *name, may_t list);
may_t may_addmul_vc (size_t size, const may_pair_t *tab);
may_t may_mulpow_vc (size_t size, const may_pair_t *tab);

void may_elist_init (void);

MAY_REGPARM unsigned long may_test_overflow_pow_ui (mpz_srcptr a, mpz_srcptr b);

MAY_REGPARM size_t may_dump_rec (FILE *f, const may_t x, int p);

/* Define internal identical cmp */
MAY_REGPARM int may_identical_internal (may_t, may_t);
MAY_REGPARM may_t may_eval_internal (may_t);

#ifdef MAY_REGPARM_DEFINED
#define may_eval(x) may_eval_internal (x)
#define may_identical(x,y) may_identical_internal(x,y)
#endif

/* If var is not a string, replace var byt a temporary string into val
   before evaluating expr
   Then revert back the substitution */
#define MAY_COMPUTE_EXPR_WITH_VAR_AS_A_STRING(expr, val, var)          \
do { may_t _local = 0;                                                 \
     if (MAY_TYPE (var) != MAY_STRING_T) {                             \
        _local = may_set_str_local(MAY_COMPLEX_D);                     \
        val = may_replace (val, var, _local);                          \
        swap(_local, var);                                             \
     }                                                                 \
     val = (expr);                                                     \
     if (_local != 0)                                                  \
       val = may_replace (val, var, _local);                           \
   } while (0)


/*********************** Dynamic 'List' functions ****************************/
/* Implemented as vector */
typedef struct {
  may_size_t size;
  may_size_t alloc;
  may_t list;
  may_t *base;
} may_list_t[1];

void       may_list_init (may_list_t, may_size_t);
may_t      may_list_quit (may_list_t);
void       may_list_resize (may_list_t, may_size_t);
void       may_list_push_back (may_list_t, may_t);
void       may_list_push_back_single (may_list_t, may_t);
void       may_list_set_at (may_list_t, may_size_t, may_t);
may_t      may_list_at (may_list_t, may_size_t);
may_size_t may_list_get_size (may_list_t);
void       may_list_compact (may_mark_t, may_list_t);
void       may_list_set (may_list_t, may_list_t);
void       may_list_sort (may_list_t, int (*) (const may_t *, const may_t *));
void       may_list_swap (may_list_t, may_list_t);

#ifndef MAY_WANT_ASSERT
# define may_list_set_at(l,i,x) ((l)->base[(i)] = (x))
# define may_list_at(l,i) ((l)->base[(i)])
# define may_list_get_size(l) ((l)->size)
#endif




/*************************** BINTREE Functions *******************************/
typedef struct node_s {
  may_t num;
  may_t key;
  struct node_s *child[2];
} *may_bintree_t;
may_bintree_t may_bintree_insert (may_bintree_t tree, may_t num, may_t key);
may_t  may_bintree_get_sum (may_t, may_bintree_t tree);
unsigned long may_bintree_size (may_bintree_t tree);

may_bintree_t may_genbintree_insert (may_bintree_t tree, may_t num, may_t key);
may_t  may_genbintree_get_sum (may_t, may_bintree_t tree);
unsigned long may_genbintree_size (may_bintree_t tree);




/************************* Iterator functions ********************************/
#ifndef MAY_WANT_ASSERT

#define MAY_INLINE_ITERATOR
#include "iterator.c"

/*
  Define MACROS version for some functions if some arguments are constants
  Theses macros won't work if some integer modulo are set.
*/
#define may_set_si(_s)                                                  \
  (__builtin_constant_p (_s)                                            \
   && -MAY_MAX_MPZ_CONSTANT < (_s) && (_s) < MAY_MAX_MPZ_CONSTANT ?     \
   ((_s) < 0 ? may_c.mpz_constant[MAY_MAX_MPZ_CONSTANT-(_s)]            \
    : may_c.mpz_constant[(_s)]):                                        \
   may_set_si (_s))
#define may_set_ui(_s)                                                  \
  (__builtin_constant_p (_s) && ((unsigned long) (_s)) < MAY_MAX_MPZ_CONSTANT ? \
   may_c.mpz_constant[(_s)]: may_set_ui (_s))

#endif




/****************************** Logging Macros *******************************/

#ifdef MAY_WANT_LOGGING

/* The different kind of LOG */
#define MAY_LOG_INPUT_F    1
#define MAY_LOG_TIME_F     2
#define MAY_LOG_MSG_F      4
#define MAY_LOG_STAT_F     8

extern FILE *may_log_file;
extern int   may_log_type;
extern int   may_log_level;
extern int   may_log_current;
int        may_get_cputime (void);

#define MAY_LOG_VAR(x) MAY_LOG_MSG (("%s=%Y\n",#x, x))

#define MAY_LOG_MSG2(format, ...)                                          \
 if ((MAY_LOG_MSG_F&may_log_type)&&(may_log_current<=may_log_level))       \
   fprintf (may_log_file, "%20.20s*%d:\t"format, __func__,              \
            may_log_current, ##__VA_ARGS__);
#define MAY_LOG_MSG(x) MAY_LOG_MSG2 x

#define MAY_LOG_BEGIN2(format, ...)                                        \
  may_log_current ++;                                                      \
  if ((MAY_LOG_INPUT_F&may_log_type)&&(may_log_current<=may_log_level))    \
    fprintf (may_log_file, "%20.20s*%d:\tIN  "format"\n",__func__,                \
             may_log_current, ##__VA_ARGS__);                              \
  if ((MAY_LOG_TIME_F&may_log_type)&&(may_log_current<=may_log_level))     \
    __gmay_log_time = may_get_cputime ();
#define MAY_LOG_FUNC(begin)                                                \
  static const char *__may_log_fname = __func__;                           \
  static unsigned long __may_call_func = 0;                                \
  __may_call_func ++;                                                      \
  auto void __attribute__((destructor)) __may_log_destruct (void);         \
  auto void __attribute__((destructor)) __may_log_destruct (void) {        \
   if (__may_call_func != 0 && MAY_LOG_STAT_F&may_log_type)                \
      fprintf (may_log_file, "%20.20s:\tSTAT: %lu call(s)\n", __may_log_fname,  \
               __may_call_func);                                           \
  }                                                                        \
  auto void __may_log_cleanup (int *time);                                 \
  void __may_log_cleanup (int *time) {                                     \
    int __gmay_log_time = *time;                                           \
    if ((MAY_LOG_TIME_F&may_log_type)&&(may_log_current<=may_log_level))   \
      fprintf (may_log_file, "%20.20s*%d:\tTIM %dms\n", __may_log_fname,        \
               may_log_current, may_get_cputime () - __gmay_log_time);     \
    may_log_current --; }                                                  \
  int __gmay_log_time __attribute__ ((cleanup (__may_log_cleanup)));       \
  __gmay_log_time = 0;                                                     \
  MAY_LOG_BEGIN2 begin

#else /* MAY_WANT_LOGGING */

/* Define void macro for logging */
#define MAY_LOG_VAR(x)
#define MAY_LOG_MSG(x)
#define MAY_LOG_FUNC(x)

#endif /* MAY_WANT_LOGGING */



/************************** Define Common macro ******************************/

/* Number of elements of an array */
#undef numberof
#define numberof(x)  (sizeof (x) / sizeof ((x)[0]))

/* MAX of 2 elements */
#undef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))

/* MIN of 2 elements */
#undef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))

/* swap the 2 elements */
#undef swap
#define swap(_x,_y) ({typeof(_x) _tmp = (_y); (_y) = (_x); (_x) = _tmp; })

/* Mark the parameter as unused */
#undef UNUSED
#define UNUSED(x) ((void) (x))

/* Return ceil(log2(n)) */
#define MAY_INT_CEIL_LOG2(n) MAY_SIZE_IN_BITS ((n)-1)

/* Return the size in bits of the input */
#ifdef MAY_HAVE_BUILTIN_CLZ
# define MAY_SIZE_IN_BITS(n)                                            \
  (MAY_ASSERT (n > 0),                                                  \
   sizeof (unsigned long)*CHAR_BIT - __builtin_clzl ((unsigned long)(n)))
#else
# define MAY_SIZE_IN_BITS(n) may_size_in_bits(n)
#endif


/* Concat 2 elements for macro, performing an expansion of the macros before */
#define MAY_CONCACT3(a,b) a ## b
#define MAY_CONCACT2(a,b) MAY_CONCACT3(a,b)
#define MAY_CONCACT(a,b)  MAY_CONCACT2(a,b)

#endif
