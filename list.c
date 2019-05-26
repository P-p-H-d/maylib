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

static MAY_REGPARM may_size_t
alloc_size (may_size_t size)
{
  return size + 1 + MAX (10, size>>3);
}

void
may_list_init (may_list_t list, may_size_t size)
{
  may_size_t i;
  MAY_LOG_FUNC(("size=%d", (int) size));

  list->size = size;
  list->alloc = alloc_size (size);
  list->list = MAY_NODE_C (MAY_LIST_T, list->alloc);
  list->base = MAY_AT_PTR (list->list, 0);
  for (i = 0; i < list->size; i++)
    list->base[i] = MAY_ZERO;
}

may_t
may_list_quit (may_list_t list)
{
  MAY_LOG_FUNC(("size=%d", (int) list->size));

 /* A node must always have children */
  if (list->size == 0)
    return MAY_NAN;
  MAY_NODE_SIZE(list->list) = list->size;
  return list->list;
}

void
may_list_resize (may_list_t list, may_size_t size)
{
  may_size_t i;

  MAY_LOG_FUNC(("size=%d newsize=%d", (int) list->size, (int) size));

  if (MAY_UNLIKELY (size <= list->size)) {
    list->size = size;
    return;
  } else if (MAY_UNLIKELY (size > list->alloc)) {
    may_size_t new_alloc = alloc_size (size);
    list->list = may_realloc (list->list,
                              MAY_NODE_ALLOC_SIZE (list->alloc),
                              MAY_NODE_ALLOC_SIZE (new_alloc));
    list->base = MAY_AT_PTR (list->list, 0);
    list->alloc = new_alloc;
  }
  for (i = list->size; i < size; i++)
    list->base[i] = MAY_ZERO;
  list->size = size;
}

void
may_list_push_back (may_list_t list, may_t x)
{
  may_size_t size = list->size;

  MAY_LOG_FUNC(("size=%d x=%Y", (int) list->size, x));

  if (MAY_UNLIKELY (size + 1> list->alloc)) {
    may_size_t new_alloc = alloc_size (size+1);
    list->list = may_realloc (list->list,
                              MAY_NODE_ALLOC_SIZE (list->alloc),
                              MAY_NODE_ALLOC_SIZE (new_alloc));
    list->base = MAY_AT_PTR (list->list, 0);
    list->alloc = new_alloc;
  }
  list->size = size + 1;
  list->base[size] = x;
}

/* Add x in 'list' iff it is not in 'list' */
void
may_list_push_back_single (may_list_t list, may_t x)
{
  unsigned long i, n;

  MAY_LOG_FUNC(("size=%d x=%Y", (int) list->size, x));

  n = may_list_get_size (list);
  for (i = 0; i < n; i++)
    if (may_identical (x, may_list_at (list, i)) == 0)
      return ;
  /* Add x in list */
  may_list_push_back (list, x);
}



void
(may_list_set_at) (may_list_t list, may_size_t i, may_t x)
{
  MAY_ASSERT (i < list->size);
  MAY_LOG_FUNC(("size=%d x=%Y", (int) list->size, x));
  list->base[i] = x;
}

may_t
(may_list_at) (may_list_t list, may_size_t i)
{
  MAY_ASSERT (i < list->size);
  MAY_LOG_FUNC(("size=%d", (int) list->size));
  return list->base[i];
}

may_size_t
(may_list_get_size) (may_list_t list)
{
  MAY_LOG_FUNC(("size=%d", (int) list->size));
  return list->size;
}

void
may_list_compact (may_mark_t mark, may_list_t list)
{
  MAY_NODE_SIZE(list->list) = list->size;
  MAY_LOG_FUNC(("size=%d", (int) list->size));
  list->list = may_compact (mark, list->list);
  list->base = MAY_AT_PTR (list->list, 0);
  list->alloc = list->size;
}

void
may_list_set (may_list_t dest, may_list_t src)
{
  may_size_t n;
  MAY_LOG_FUNC(("size=%d", (int) dest->size));
  n = may_list_get_size (src);
  may_list_resize (dest, n);
  memcpy (dest->base, src->base, n*sizeof (may_t));
}

void
may_list_swap (may_list_t list1, may_list_t list2)
{
  MAY_LOG_FUNC(("size1=%d size2=%d", (int) list1->size, (int)list2->size));

  may_list_t temp;
  memcpy(temp, list1, sizeof temp);
  memcpy(list1, list2, sizeof temp);
  memcpy(list2, temp, sizeof temp);
}

void
may_list_sort (may_list_t list, int (*cmp) (const may_t *, const may_t *))
{
  MAY_LOG_FUNC(("size=%d", (int) list->size));
  qsort (list->base, list->size, sizeof (may_t), (int (*)(const void*, const void*)) cmp);
}
