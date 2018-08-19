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

struct data_s {
  may_t (*func) (may_t);
  unsigned long level;
};

static may_t localfunc (may_t x, void *data)
{
  struct data_s *mydata = (struct data_s *) data;
  return may_recursive (x, mydata->func, mydata->level-1);
}

/* FIXME: Extension? List? Matrix? Handle them in a special way? */
may_t
may_recursive (may_t expr, may_t (*func) (may_t), unsigned long level)
{
  MAY_ASSERT (func != 0);

  MAY_RECORD ();
  /* Recursively apply the function */
  if (level != 0 && !MAY_ATOMIC_P (expr)) {
    may_t listvar = may_indets (expr, MAY_INDETS_NUM);
    may_size_t i, n = may_nops (listvar);
    for (i = 0; i < n; i++) {
      may_t var = may_op (listvar, i);
      struct data_s data = { .func = func, .level = level };
      may_t newvar = may_map2 (var, localfunc, &data);
      expr = may_replace (expr, var, newvar);
    }
  }
  /* Apply the function to the core */
  expr = (*func) (expr);
  MAY_ASSERT (expr != NULL);
  MAY_RET (expr);
}
