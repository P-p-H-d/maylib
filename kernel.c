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

/* The global variables of may are ALL stored in one global variable.
   Faster on some archi (ARM), and cooler for THREAD */
MAY_THREAD_ATTR struct may_globals_s may_g;
struct may_common_s may_c;

/* Return the length in bytes of an expression.
   (Not really tested yet) */
MAY_REGPARM size_t
may_length (may_t x)
{
  size_t size;
  may_size_t i, n;

  switch (MAY_TYPE (x))
    {
    case MAY_INT_T:
      size = MAY_INT_SIZE
	+ mpz_size (MAY_INT (x)) * sizeof (mp_limb_t);
      break;
    case MAY_RAT_T:
      size = MAY_RAT_SIZE
	+ mpz_size (mpq_numref (MAY_RAT (x))) * sizeof (mp_limb_t)
	+ mpz_size (mpq_denref (MAY_RAT (x))) * sizeof (mp_limb_t);
      break;
    case MAY_FLOAT_T:
      size = MAY_FLOAT_SIZE
	+ (1+(mpfr_get_prec (MAY_FLOAT(x)) - 1) / (sizeof(mp_limb_t)*CHAR_BIT))
           * sizeof (mp_limb_t);
      break;
    case MAY_COMPLEX_T:
      size = MAY_NODE_ALLOC_SIZE (2)
        + may_length (MAY_RE(x)) + may_length (MAY_IM (x));
      break;
    case MAY_STRING_T:
      size = MAY_NAME_SIZE (MAY_SYMBOL_SIZE(x));
      break;
    case MAY_DATA_T:
      size = MAY_DATA_SIZE (MAY_DATA (x).size);
      break;
    default:
      n = MAY_NODE_SIZE(x);
      for (i = 0, size = MAY_NODE_ALLOC_SIZE (n) ; i < n ; i++)
	size += may_length (MAY_AT(x, i));
      break;
    }
  return size;
}

void MAY_NORETURN
may_assert_fail (const char filename[], int linenum,
                 const char expr[])
{
  fprintf (stderr, "[MAYLIB]: assertion failed in %s.%d: %s\n",
	   filename, linenum, expr);
#ifdef __CYGWIN__
  exit (-1); /* Can't use abort: too slow! */
#else
  abort ();
#endif
}

/* Init two integers for kernel booting (positive & negative) */
static void
init_num (long n, may_t *out_pos, may_t *out_neg)
{
  MAY_RECORD ();
  may_t y = MAY_SLONG_C (n);
  MAY_CLOSE_C (y, MAY_NUM_F|MAY_EVAL_F, may_mpz_hash (MAY_INT (y)));
  MAY_SET_FLAG (y, MAY_EXPAND_F);
  *out_pos = y;
  /* Reuse mantissa between positive and negative value */
  y = MAY_MPZ_NOCOPY_C(MAY_INT(y));
  mpz_neg(MAY_INT(y), MAY_INT(y));
  MAY_CLOSE_C (y, MAY_NUM_F|MAY_EVAL_F, may_mpz_hash (MAY_INT (y)));
  MAY_SET_FLAG (y, MAY_EXPAND_F);
  *out_neg = y;
  MAY_COMPACT_2(*out_pos, *out_neg);
}

/* Init a real for kernel booting */
static may_t
init_d (int k)
{
  mpfr_t f;
  MAY_RECORD ();
  mpfr_init2 (f, MPFR_PREC_MIN);
  switch (k) {
  case 0: mpfr_set_nan (f); break;
  case 1: mpfr_set_inf (f, 1); break;
  case -1: mpfr_set_inf (f, -1); break;
  }
  may_t y = MAY_MPFR_NOCOPY_C (f);
  MAY_CLOSE_C (y, MAY_NUM_F|MAY_EVAL_F, may_mpfr_hash (MAY_FLOAT (y)));
  MAY_SET_FLAG (y, MAY_EXPAND_F);
  MAY_RET (y);
}

/* Init a rational for kernel booting */
static may_t
init_q (long n, long d)
{
  MAY_RECORD ();
  mpq_t q;
  mpq_init (q);
  mpq_set_si (q, n, d);
  may_t y = may_mpq_simplify (NULL, q);
  MAY_RET (y);
}

void
may_kernel_start (size_t n, int allow_extend)
{
  /* Auto-enlarge if too small */
  if (n < 512) {
    if (n == 0) {
      int i, j;
      void *s = &i;
      /* Find upper limit for stack size */
      for (i = 10; i < (int) (CHAR_BIT*sizeof n) && s != NULL; i++) {
        n = 1UL << i;
        s =  malloc (n);
        if (s != NULL)
          free (s);
      }
      n = 1UL << (i-2);
      for (j = i-3; j > i-8; j--) {
        s = malloc (n + (1UL << j));
        if (s != NULL) {
          free (s);
	  n += 1UL <<j;
	}
      }
    } else
      n = 512;
  }

  /* Clean up globals */
  memset (&may_g, 0, sizeof may_g);

  /* Start Heap */
  may_heap_init (&may_g.Heap, n, n-n/4, allow_extend);
  /* Set GMP to the allocated heap */
  may_kernel_restart ();
  /* Set MPFR exponents to their maximum: no need to save them */
  mpfr_set_emin (mpfr_get_emin_min ());
  mpfr_set_emax (mpfr_get_emax_max ());

  /* Set default values */
  may_kernel_prec (113);
  may_kernel_base (10);
  may_kernel_rnd (GMP_RNDN);
  may_kernel_zero_cb (may_zero_fastp);
  may_kernel_sort_cb (may_identical);
  may_kernel_num_presimplify (1);
  may_kernel_intmod (NULL);
  may_kernel_intmaxsize (65536);
  may_kernel_domain (MAY_COMPLEX_D);

  /* Some default symbolic numbers (296 bytes used) */
  for(int i = 0; i < MAY_MAX_MPZ_CONSTANT; i++) {
    init_num(i,
             &may_c.mpz_constant[i],
             &may_c.mpz_constant[MAY_MAX_MPZ_CONSTANT+i]);
  }
  may_c.p_inf = init_d (1);
  may_c.n_inf = init_d (-1);
  may_c.nan   = init_d (0);
  may_c.dummy = MAY_ALLOC (sizeof (struct may_s));
  MAY_OPEN_C  (may_c.dummy, MAY_NUM_LIMIT);
  MAY_CLOSE_C (may_c.dummy, MAY_EVAL_F, (may_hash_t)0xDEADDEAD);
  may_c.half  = init_q (1, 2);
  may_c.n_half= init_q (-1, 2);
  may_c.I     = MAY_COMPLEX_C (MAY_ZERO, MAY_ONE);
  MAY_CLOSE_C(may_c.I, MAY_EVAL_F|MAY_NUM_F, MAY_NEW_HASH2(MAY_ZERO, MAY_ONE));
  may_c.Pi    = MAY_STRING_C ("PI", MAY_REAL_POS_D);
  MAY_SET_FLAG (may_c.Pi, MAY_NUM_F);
  MAY_ASSERT (MAY_HASH (may_c.Pi) == may_recompute_hash (may_c.Pi));

  /* Register ELIST extension (mandatory) */
  may_elist_init ();

  /* After initialisation of the variables of the kernel */
  may_kernel_worker(0, 0);

  MAY_LOG_MSG(("Starting MAYLIB with size=%ul and options=%d\n", (unsigned long) n, allow_extend));
}

void
may_kernel_end (void)
{
  MAY_DEF_IF_THREAD (may_thread_quit();)
  may_heap_clear (&may_g.Heap);
  MAY_LOG_MSG(("Ending MAYLIB (Used:%lu MaxUsed:%lu)\n", (unsigned long) (may_g.Heap.top-may_g.Heap.base), (unsigned long) (may_g.Heap.max_top-may_g.Heap.base)));
}

/* Set the current rounding mode */
mp_rnd_t
may_kernel_rnd (mp_rnd_t rnd)
{
  MAY_LOG_MSG (("New rounding mode: %d\n", rnd));
  mp_rnd_t old =  may_g.frame.rnd_mode;
  may_g.frame.rnd_mode = rnd;
  return old;
}

/* Set the current base for input / output conversion */
int
may_kernel_base (int base)
{
  MAY_LOG_MSG (("New base: %d\n", base));
  int old = may_g.frame.base;
  if (MAY_LIKELY (base != 0))
    may_g.frame.base = base;
  return old;
}

/* Set the precision of the mpfr_t */
mp_prec_t
may_kernel_prec (mp_prec_t p)
{
  MAY_LOG_MSG (("New base: %lu\n", (unsigned long) p));
  mp_prec_t old = may_g.frame.prec;
  if (MAY_LIKELY (p != 0)) {
    mpfr_set_default_prec (p);
    may_g.frame.prec = p;
  }
  return old;
}

/* Set the current Domain for variables */
may_domain_e
may_kernel_domain (may_domain_e domain)
{
  MAY_LOG_MSG (("New domain: %d\n", domain));
  may_domain_e old = may_g.frame.domain;
  may_g.frame.domain = domain;
  return old;
}

/* Set the current comparaison function for sorting expressions */
int (*may_kernel_sort_cb (int (*new)(may_t, may_t)))(may_t, may_t)
{
  int (*old)(may_t, may_t) = may_g.frame.sort_cb;
  if (MAY_LIKELY(new != (int (*)(may_t, may_t))0))
    may_g.frame.sort_cb = new;
  return old;
}

/* Set the current zero_p function, which identify zero */
int (*may_kernel_zero_cb (int (*new)(may_t)))(may_t)
{
  int (*old)(may_t) = may_g.frame.zero_cb;
  if (MAY_LIKELY(new != (int (*)(may_t))0))
    may_g.frame.zero_cb = new;
  return old;
}

/* Set the current modulo for the Integers */
may_t
may_kernel_intmod (may_t new)
{
  MAY_LOG_MSG (("New IntMod: '%Y'\n", new));
  may_t old = may_g.frame.intmod;
  if (new != NULL) {
    MAY_ASSERT (MAY_TYPE (new) == MAY_INT_T);
    if (mpz_cmp_ui (MAY_INT (new), 0) == 0)
      new = NULL;
  }
  may_g.frame.intmod = new;
  return old;
}

 /* Set if MAY should evaluate the float to the current
    precision, or store them as strings, so that we let
    futher computing setting the precision without interference */
int
may_kernel_num_presimplify (int n)
{
  MAY_LOG_MSG (("New presimplify=%d\n", n));
  int old = may_g.frame.num_presimplify;
  may_g.frame.num_presimplify = n;
  return old;
}

/* Set the maximum for the computation of integers */
unsigned long
may_kernel_intmaxsize (unsigned long n)
{
  MAY_LOG_MSG (("New intmaxsize= %lu\n", (unsigned long) n));
  unsigned long old = may_g.frame.intmaxsize;
  may_g.frame.intmaxsize = n;
  return old;
}

/* Stop the MAY kernel */
void
may_kernel_stop (void)
{
  MAY_LOG_MSG (("Stop MAYLIB Kernel\n"));
  /* Free MPFR cache before changing the memory handler */
  mpfr_free_cache ();
  /* TODO: Stop all threads ! */
  /* Restore original memory handler */
  mp_set_memory_functions (may_c.org_gmp_alloc, may_c.org_gmp_realloc,
                           may_c.org_gmp_free );
}

/* Restart the MAY kernel */
void
may_kernel_restart (void)
{
  MAY_LOG_MSG (("Restart MAYLIB Kernel\n"));
  /* Free MPFR cache before changing the memory handler */
  mpfr_free_cache ();
  mp_get_memory_functions (&may_c.org_gmp_alloc,
                           &may_c.org_gmp_realloc, &may_c.org_gmp_free);
  /* Set the dedicated MAY functions for GMP */
  mp_set_memory_functions (may_alloc, may_realloc, may_free );
}

/* Display various infos */
void
may_kernel_info (FILE *stream, const char str[])
{
  char *max_top = MAX(may_g.Heap.top, may_g.Heap.max_top);
  fprintf(stream, "%s -- Base:%p Top:%p Used:%lu MaxUsed:%lu Max:%lu\n",
	  str, may_g.Heap.base, may_g.Heap.top,
          (unsigned long) (may_g.Heap.top-may_g.Heap.base),
          (unsigned long) (max_top-may_g.Heap.base),
          (unsigned long) (may_g.Heap.limit-may_g.Heap.base));
}

int
may_kernel_worker(int new_number, size_t size)
{
#ifdef MAY_WANT_THREAD
  /* If invalid new number, get the number of CPU of the system */
  if (new_number <= 0)
    new_number = may_get_cpu_count();
  MAY_ASSERT(new_number >= 1);
  new_number = MIN (new_number, MAY_MAX_THREAD);
  if (size == 0)
    size = (may_g.Heap.limit - may_g.Heap.base) / new_number;
  size = MAX (512, size);
  /* TODO: Allow while MT system is still working */
  int previous = may_thread_quit();
  may_thread_init(new_number, size);
  return previous;
#else
  UNUSED(new_number);
  UNUSED(size);
  return -1;
#endif
}


/* Define function to compute the size in bits if needed */
#ifndef MAY_HAVE_BUILTIN_CLZ
MAY_REGPARM int
may_size_in_bits(unsigned long a)
{
  if (MAY_UNLIKELY(a == 0))
    return 0;

  unsigned long p = 1;
  int c = 0;
  while (p <= a) {
    p = 2*p;
    c++;
  }
  return c;
}
#endif
