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

#include <stdlib.h>
#include "may-impl.h"

/* If p is an identifier, check if p is a wildcard, and return
   its index if it is */
MAY_INLINE int
get_index (may_t p)
{
  const char *name;
  char *end;
  int ret;

  name = MAY_NAME (p);
  if (*name++ != '$')
    return -1;
  ret = strtol (name, &end, 10);
  return end == name ? -1 : ret;
}

/* Return the max index of the wildcards which are used in 'x' */
static int
get_max_index (may_t x)
{
  if (MAY_ATOMIC_P (x))
    return MAY_TYPE(x) == MAY_STRING_T ? get_index (x) : -1;

  may_size_t i, n = MAY_NODE_SIZE(x);
  int max = -1, val;
  for (i = 0; MAY_LIKELY (i < n); i++)
    if ((val = get_max_index (MAY_AT (x, i))) > max)
      max = val;
  return max;
}

/* We must ensure that x is not reconstructed if not needed.
   Some code needs this feature to detect how much time
   may_rewrite may be used */
static may_t
rewrite (may_t x, struct may_rewrite_s *context)
{
  /* Check if match... */
  memset (context->value, 0,
          context->size * sizeof *context->value);
  if (may_match_p (context->value, x, context->p1,
                   context->size, context->funcp)) {
    x = may_subs_c (context->p2, 1, context->size,
                    (const char**) context->vars,
                    (void*) context->value);
    return may_eval (x);
  }

  /* Check if atomic */
  if (MAY_ATOMIC_P (x))
    return x;

  /* Otherwise go down */
  may_size_t i, n = MAY_NODE_SIZE(x);
  may_t nx = MAY_NODE_C (MAY_TYPE (x), n);
  int isnew = 0;
  for (i=0; i<n; i++) {
    may_t z = MAY_AT (x, i), y;
    y = rewrite (z, context);
    isnew |= (y != z);
    MAY_SET_AT (nx, i, y);
  }

  /* If nothing has been done, just return */
  /* Otherwise return the newly created number */
  return isnew ? may_eval (nx) : x;
}

may_t
may_rewrite (may_t x, may_t pattern1, may_t pattern2)
{
  struct may_rewrite_s context;

  MAY_LOG_FUNC (("x='%Y' pattern1='%Y' pattern2='%Y'", x, pattern1, pattern2));

  may_mark ();
  context.size = get_max_index (pattern1) + 1;
  MAY_ASSERT (context.size >= 0 && context.size < 100);

  context.value = may_alloc (context.size
                                   * sizeof *context.value);
  context.p1 = pattern1;
  context.p2 = pattern2;
  context.funcp = 0;
  context.vars = may_alloc (context.size
                                  * sizeof *context.vars);
  for (int i=0; i<context.size; i++) {
    context.vars[i] = may_alloc (4);
    sprintf (context.vars[i], "$%d", i);
  }
  /* Rewrite, compact and return */
  return may_keep (rewrite (x, &context));
}

may_t
may_rewrite2 (may_t x, may_t pattern1, may_t pattern2,
              int size, int (**funcp)(may_t))
{
  struct may_rewrite_s context;

  MAY_ASSERT (size >= 0 && size < 100);

  MAY_LOG_FUNC (("x='%Y' pattern1='%Y' pattern2='%Y'", x, pattern1, pattern2));

  may_mark ();
  context.size = get_max_index (pattern1) + 1;
  if (context.size > size)
    return x;
  context.size = size;
  context.value = may_alloc (context.size * sizeof *context.value);
  context.p1 = pattern1;
  context.p2 = pattern2;
  context.funcp = funcp;
  context.vars = may_alloc (context.size * sizeof *context.vars);
  for (int i=0; i<context.size; i++) {
    context.vars[i] = may_alloc (4);
    sprintf (context.vars[i], "$%d", i);
  }
  /* Rewrite, compact and return */
  return may_keep (rewrite (x, &context));
}
