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

may_t
may_hold (may_t x)
{
  MAY_LOG_FUNC (("%Y", x));

  if (MAY_EVAL_P (x))
    return x;
  else if (MAY_UNLIKELY (MAY_ATOMIC_P (x)))
    MAY_CLOSE_C (x, MAY_EVAL_F, may_recompute_hash (x));
  else {
    MAY_FLAG_DECL (flag);
    MAY_HASH_DECL (hash);

    if (MAY_UNLIKELY (MAY_TYPE (x) >= MAY_EXP_T
                      && MAY_TYPE (x) < MAY_UNARYFUNC_LIMIT)) {
      MAY_ASSERT (MAY_NODE_SIZE(x) == 1);
       may_t z = may_hold (MAY_AT (x, 0));
       MAY_SET_AT (x, 0, z);
       flag = MAY_FLAGS(z);
       hash = MAY_HASH (z);
    } else {
      may_size_t n = MAY_NODE_SIZE(x);
      MAY_ASSERT (n > 0);
      for (may_size_t i = 0; MAY_LIKELY (i < n); i++) {
        may_t z = may_hold (MAY_AT (x, i));
        MAY_SET_AT (x, i, z);
        MAY_FLAG_UP (flag, MAY_FLAGS (z));
        MAY_HASH_UP (hash, MAY_HASH (z));
      }
    }
    MAY_CLOSE_C (x, MAY_FLAG_GET (flag), MAY_HASH_GET (hash));
    MAY_ASSERT (may_recompute_hash (x) == MAY_HASH (x));
  }

  return x;
}
