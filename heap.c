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

/* Select the way of allocating the memory from the OS */
#if !defined(__CYGWIN__) && !defined(__PEDROM__BASE__) && !defined(WIN32)
# define WANT_MMAP
#endif

#ifdef WANT_SBRK
# include <unistd.h> /* For sbrk, since realloc can't do what we want */
#elif defined(WANT_MMAP)
# define _GNU_SOURCE
# include <unistd.h>
# include <errno.h>
# include <sys/mman.h>
#endif

#include "may-impl.h"

/* Store and update the max TOP reached */
MAY_INLINE void
update_max_top (void)
{
  if (may_g.Heap.top > may_g.Heap.max_top)
    may_g.Heap.max_top = may_g.Heap.top;
}

/* Finish the compact */
MAY_INLINE void
finish_compact (void *mark)
{
  /* If the integer modulo disapears, remove it */
  if (MAY_UNLIKELY (mark <= (void*)may_g.frame.intmod))
    may_g.frame.intmod = NULL;

  /* Free the set_str cache */
  may_g.frame.cache_set_str_i = may_g.frame.cache_set_str_n = 0;

  /* Free the MPFR cache */
  mpfr_free_cache ();

  /* Free the attached Heaps if MT mode after a compact */
  MAY_DEF_IF_THREAD (
    struct may_heap_s *next_heap = may_g.Heap.next_heap_to_free;
    while (MAY_UNLIKELY (next_heap != NULL)) {
      struct may_heap_s *next = next_heap->next_heap_to_free;
      may_heap_clear (next_heap);
      next_heap = next;
    }
    /* We no longer have any attached heaps. */
    may_g.Heap.next_heap_to_free = NULL;)
}

/* This MACRO returns TRUE if the may_t input variable has to be compacted */
/* If MT mode, TO_COMPACT is more complicated since there are multiple heaps */
static MAY_REGPARM may_t compact_recur1 (may_t);

#ifndef MAY_WANT_THREAD
MAY_INLINE may_t compact_if_needed(may_t x)
{
  return (may_g.Heap.comp_mark<=(void*)x) ? compact_recur1(x) : x;
}
#else
MAY_INLINE may_t compact_if_needed(may_t x)
{
  void *xv = x;
  /* Check if x is within the data to move (very likely) */
  if (MAY_LIKELY (may_g.Heap.comp_mark <= xv && xv < may_g.Heap.comp_limit))
    return compact_recur1(x);
  /* Check if x shall not be moved (likely) */
  if (MAY_LIKELY (may_g.Heap.comp_base <= xv && xv < may_g.Heap.comp_mark))
    return x;

  /* Check if x is within an heap which shall be freed after the compact
     in which case, we have to compact it now */
  /* We need to start from may_g.Heap since we may have scanned from
     may_g to another heap, then rego to the original one. Quite unlikely
     but possible. */
  struct may_heap_s *heap = &may_g.Heap;
  do {
    if (MAY_UNLIKELY (heap->base <= (char*)x && (char*)x < heap->limit)) {
      /* Future compact(s) are likely within this new heap.
         Temporary Change of the current heap for performance */
      void *comp_mark  = may_g.Heap.comp_mark;
      void *comp_base  = may_g.Heap.comp_base;
      void *comp_limit = may_g.Heap.comp_limit;
      may_g.Heap.comp_mark = heap->base;
      may_g.Heap.comp_base = heap->base;
      may_g.Heap.comp_limit = heap->limit;
      may_t z = compact_recur1 (x);
      may_g.Heap.comp_mark = comp_mark;
      may_g.Heap.comp_base = comp_base;
      may_g.Heap.comp_limit = comp_limit;
      return z;
    }
    heap = heap->next_heap_to_free;
  } while (MAY_UNLIKELY(heap != NULL));
  return x; /* Not found ==> No compact (likely in another heap) */
}
#endif

/* This functions hacks inside the "hidden" struct of GMP
   in order to move them (including their mantissa).
   It usually costs around 10% of a MAY program */
static MAY_REGPARM may_t
compact_recur1 (may_t x)
{
  may_t y;
  may_type_t t = MAY_TYPE (x);
  size_t s;

  MAY_ASSERT (x != NULL);
  MAY_ASSERT (MAY_TYPE (x) < MAY_END_LIMIT || MAY_EXT_P (x));

  /* Hard code the copy of MPZ/MPQ/MPFR */
  if (MAY_UNLIKELY (t < MAY_ATOMIC_LIMIT)) {
    switch (t) {
    case MAY_INT_T:
      s = mpz_size (MAY_INT (x)) * sizeof (mp_limb_t);
      y = MAY_ALLOC (MAY_INT_SIZE + s);
      memcpy ((char*)y, x, MAY_INT_SIZE);
      memcpy ((char*)y+MAY_INT_SIZE, MAY_INT(x)->_mp_d, s);
      MAY_INT (y)->_mp_d = (void*) ((char*)y + MAY_INT_SIZE - may_g.Heap.compdiff);
      break;
    case MAY_RAT_T:
      {
	size_t s2;
	s2 = mpz_size (mpq_denref (MAY_RAT (x))) * sizeof (mp_limb_t);
	s  = mpz_size (mpq_numref (MAY_RAT (x))) * sizeof (mp_limb_t);
	y = MAY_ALLOC (MAY_RAT_SIZE + s + s2);
	memcpy ((char*)y, x, MAY_RAT_SIZE);
	memcpy ((char*)y+MAY_RAT_SIZE  , mpq_numref (MAY_RAT (x))->_mp_d,  s);
	memcpy ((char*)y+MAY_RAT_SIZE+s, mpq_denref (MAY_RAT (x))->_mp_d, s2);
	mpq_numref(MAY_RAT(y))->_mp_d = (void*)((char*)y + MAY_RAT_SIZE
                                                - may_g.Heap.compdiff);
	mpq_denref(MAY_RAT(y))->_mp_d = (void*)((char*)y + MAY_RAT_SIZE + s
                                                - may_g.Heap.compdiff);
      }
      break;
    case MAY_FLOAT_T:
      s = mpfr_custom_get_size(mpfr_get_prec(MAY_FLOAT(x)));
      y = MAY_ALLOC (MAY_FLOAT_SIZE + s);
      memcpy (y, x, MAY_FLOAT_SIZE);
      memcpy ((char*)y+MAY_FLOAT_SIZE,mpfr_custom_get_mantissa(MAY_FLOAT(x)),s);
      mpfr_custom_move(MAY_FLOAT (y), ((char*)y + MAY_FLOAT_SIZE - may_g.Heap.compdiff));
      break;
    case MAY_COMPLEX_T:
      {
	may_t r = MAY_RE (x), i = MAY_IM (x);
	r = compact_if_needed(r);
	i = compact_if_needed(i);
	y = MAY_ALLOC (MAY_COMPLEX_SIZE);
	memcpy (y, x, sizeof (struct may_s) + sizeof (struct may_node_s) );
	MAY_SET_RE (y, r);
	MAY_SET_IM (y, i);
      }
      break;
    case MAY_STRING_T:
      s = MAY_NAME_SIZE (MAY_SYMBOL_SIZE(x));
      y = MAY_ALLOC (s);
      memcpy (y, x, s);
      break;
    case MAY_DATA_T:
      s = MAY_DATA(x).size;
      y = MAY_ALLOC (MAY_DATA_SIZE (s));
      memcpy (y, x, s);
      break;
    default:
      MAY_ASSERT (MAY_TYPE (x) == MAY_INDIRECT_T);
      return MAY_INDIRECT (x);
    }
  }
  else /* Non atomic */
    {
      may_size_t n = MAY_NODE_SIZE(x);
      y = MAY_ALLOC (MAY_NODE_ALLOC_SIZE (n));
      memcpy ((void*)y, x, sizeof (may_header_t));
      MAY_NODE_SIZE(y) = n;
      MAY_ASSERT (n > 0);
      /* TODO: Analyse cache coherency for copying backward? */
      do {
	may_t z = MAY_AT(x, n-1);
	MAY_SET_AT (y, n-1, compact_if_needed(z) );
      } while (--n != 0);
    }
  y = (may_t) ((char*) y - may_g.Heap.compdiff);
  /* Even if x==y, this is still valid since y is not yet in the place of x */
  MAY_SET_INDIRECT (x, y);

  return y;
}

/* Compact an expression from Heap.top to limit */
MAY_REGPARM may_t
may_compact_internal (may_t x, void *mark)
{
#ifdef MAY_WANT_ASSERT
  char *oldtop = may_g.Heap.top;
#endif
  if (MAY_LIKELY (mark <= (void*)x)) {
    unsigned long length;
    update_max_top ();
    /* Update heap variables for compact */
    may_g.Heap.comp_mark = mark;
    MAY_DEF_IF_THREAD (may_g.Heap.comp_base = may_g.Heap.base);
    MAY_DEF_IF_THREAD (may_g.Heap.comp_limit = may_g.Heap.limit);
    may_g.Heap.compdiff = ((char*) may_g.Heap.top) - (char*) mark;
    /* Compact */
    x = compact_recur1 (x);
    /* Compute the length of the expression */
    length = (char*) may_g.Heap.top - (char*)mark - may_g.Heap.compdiff;
    /* FIXME: memmove ! */
    memcpy (mark, (char*)mark + may_g.Heap.compdiff, length);
    may_g.Heap.top = (char*)mark + length;
  }
  else
    may_g.Heap.top = mark;
  finish_compact (mark);
#ifdef MAY_WANT_ASSERT
  /* Cleanup the recuperated memory */
  if (oldtop > may_g.Heap.top)
    memset (may_g.Heap.top, 0xA7, oldtop-may_g.Heap.top);
#endif
  return x;
}

/* Compact expressions from Heap.top to limit */
may_t *
may_compact_vector (may_t *x, may_size_t num, void *mark)
{
  size_t length;
  may_t *x_ret = x, *x_w = x;
  MAY_ASSERT (num >= 1);

  /* Update the maximum TOP reached */
  update_max_top ();
  /* Compute the SIZE to compact */
  may_g.Heap.comp_mark = mark;
  MAY_DEF_IF_THREAD (may_g.Heap.comp_base = may_g.Heap.base);
  MAY_DEF_IF_THREAD (may_g.Heap.comp_limit = may_g.Heap.limit);
  may_g.Heap.compdiff = (char*) may_g.Heap.top - (char*) mark;

  /* If the array has to be collected too (Warning it may be outside the HEAP too!) */
  if ((void*) x >= mark && (char*)x  < may_g.Heap.limit) {
    /* We have to GC the array 'x' too! */
    x_ret = mark;
    x_w = MAY_ALLOC (num*sizeof (may_t*));
  }

  /* Compact each element of x */
  for ( ; num != 0; num--, x++, x_w++) {
    if (MAY_LIKELY (*x != NULL))
      *x_w = compact_if_needed (*x);
    else
      *x_w = NULL;
  }
  length = (char*)may_g.Heap.top - (char*)mark - may_g.Heap.compdiff;

  /* Free memory */
  memmove (mark, (char*) mark + may_g.Heap.compdiff, length);
  may_g.Heap.top = (char*) mark + length;
  finish_compact (mark);

  /* Return new pointer to the array x if any */
  return x_ret;
}

MAY_NORETURN void
may_throw_memory (void)
{
  MAY_THROW (MAY_MEMORY_ERR);
}



/* Standard alloc functions inside the Heap */
void *
may_alloc (size_t s)
{
  return MAY_ALLOC (s);
}
void *
may_realloc (void *s, size_t old, size_t new)
{
  return MAY_REALLOC (s, old, new);
}
void
may_free (void *x, size_t s)
{
  /* Try to free memory if possible */
  if (MAY_UNLIKELY ((char*)x + MAY_ALIGNED_SIZE (s) == may_g.Heap.top))
    may_g.Heap.top = x;
}

void
may_heap_init (struct may_heap_s *heap, unsigned long size, unsigned long low, int allow_extend)
{
  /* Align size to a multiple of 4096 */
  size = (size + 4095UL) & ~4096UL;

#if defined(WANT_SBRK)
  void *p;
  p = malloc (10); free (p);
  heap->base  = sbrk (size);
#elif defined(WANT_MMAP)
  void *p;
  p = malloc (10); free (p);
  errno = 0;
  heap->base = mmap (sbrk(0), size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
  if (heap->base == MAP_FAILED) {
    fprintf (stderr, "[MAYLIB]: mmap failed (ptr=%p size=%lu errno=%d)\n", heap->base, size, errno);
    exit (1);
  }
#else
  heap->base = malloc (size);
  if (heap->base == 0) {
    fprintf (stderr, "[MAYLIB]: malloc failed (%lu bytes)\n", size);
    exit (1);
  }
#endif
  MAY_ASSERT (size >= low);
  UNUSED (low);
  heap->top   = heap->base;
  heap->limit = heap->base + size;
  heap->max_top = heap->base;
  heap->current_mark = heap->base;
  heap->num_resize = 0;
  heap->allow_extend = allow_extend;
  heap->compact_func_disable = 0;
  MAY_DEF_IF_THREAD (heap->next_heap_to_free = NULL; )
}

void
may_heap_clear (struct may_heap_s *heap)
{
#ifdef WANT_MMAP
  munmap (heap->base, (char*)heap->limit-(char*)heap->base);
#elif !defined(WANT_SBRK)
  free (heap->base);
#endif
}

MAY_REGPARM void *
may_heap_extend (unsigned long n)
{
  MAY_LOG_MSG(("Request for an extension to alloc %ul bytes\n", n));
  may_g.Heap.num_resize ++;
  /* Test if we are allowed to extend the Heap */
  if (!may_g.Heap.allow_extend)
    MAY_THROW (MAY_MEMORY_ERR);

#if defined(WANT_SBRK)
  size_t heap_size;
  void *new_page;

  heap_size = ((void*) may_g.Heap.limit - (void*)may_g.Heap.base);
  new_page  = sbrk (heap_size);
  if (new_page != may_g.Heap.limit)
    MAY_THROW (MAY_MEMORY_ERR);
  may_g.Heap.limit = new_page + heap_size;
  fprintf (stderr, "[MAYLIB]: Heap extended to %lu\n",
	   (unsigned long) 2*heap_size);
  return may_alloc (n); /* We don't need the speed of the MACRO version */
#elif defined(WANT_MMAP)
  size_t heap_size;
  void *new_page;

  heap_size = ((char*) may_g.Heap.limit - (char*)may_g.Heap.base);
  if (2*heap_size < heap_size)
    MAY_THROW (MAY_MEMORY_ERR);
  new_page  = mremap (may_g.Heap.base, heap_size, 2*heap_size, 0);
  if (new_page == MAP_FAILED)
    MAY_THROW (MAY_MEMORY_ERR);
  may_g.Heap.limit = (char*) new_page + 2*heap_size;
  fprintf (stderr, "[MAYLIB]: Heap extended to %lu\n",
	   (unsigned long) 2*heap_size);
  return may_alloc (n); /* We don't need the speed of the MACRO version */
#else
  UNUSED (n);
  /* We can't extend the memory without moving it */
  MAY_THROW (MAY_MEMORY_ERR);
  return 0; /* Should never be reached */
#endif
}

/**** DEFINE NON MACROS versions for external use ******/

/* Push a Mark */
void (may_mark) (may_mark_t mark)
{
  /* MACRO version */
  MAY_OVERLOADED_MARK (mark);
}

/* Compact an expression */
may_t (may_compact) (may_mark_t mark, may_t x)
{
  /* MACRO version */
  return MAY_OVERLOADED_COMPACT (mark, x);
}

may_t* (may_compact_v) (may_mark_t mark, size_t size, may_t *tab)
{
  /* MACRO version */
  return MAY_OVERLOADED_COMPACT_V (mark, size, tab);
}

void
(may_chained_compact1) (void)
{
  /* MACRO version */
  MAY_OVERLOADED_CHAINED_COMPACT1 ();
}

may_t
(may_chained_compact2) (may_mark_t mark, may_t x)
{
  /* MACRO version */
  return MAY_OVERLOADED_CHAINED_COMPACT2 (mark, x);
}

void
may_compact_va (may_mark_t mark, may_t *x, ...)
{
  va_list arg;
  may_t temp[20];
  size_t n;
  may_t *p;

  /* First pass: Count # of args and copy (Max 20) */
  va_start (arg, x);
  temp[0] = *x;
  n = 1;
  for (;;) {
    p = va_arg (arg, may_t *);
    if (p == NULL)
      break;
    temp[n++] = *p;
    MAY_ASSERT (n <= numberof (temp));
  }
  va_end (arg);

  /* Second pass: Compact */
  may_compact_v (mark, n, temp);

  /* Third pass: Copy back */
  va_start (arg, x);
  *x = temp[0];
  n = 1;
  for (;;) {
    p = va_arg (arg, may_t *);
    if (p == NULL)
      break;
    *p = temp[n++];
    MAY_ASSERT (n <= numberof (temp));
  }
  va_end (arg);
}
