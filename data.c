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

may_t
may_data_c (size_t s)
{
  may_t y = may_alloc (MAY_DATA_SIZE (s));
  MAY_OPEN_C (y, MAY_DATA_T);
  MAY_DATA (y).size = s;
#ifdef MAY_WANT_ASSERT
  memset (MAY_DATA (y).data, 0, s);
#endif
  return y;
}

size_t
may_data_size (may_t x)
{
  MAY_ASSERT (MAY_TYPE (x) == MAY_DATA_T);
  return MAY_DATA (x).size;
}

void *
may_data_ptr (may_t x)
{
  MAY_ASSERT (MAY_TYPE (x) == MAY_DATA_T);
  MAY_ASSERT (!MAY_EVAL_P (x));
  return MAY_DATA (x).data;
}

const void *
may_data_srcptr (may_t x)
{
  MAY_ASSERT (MAY_TYPE (x) == MAY_DATA_T);
  return MAY_DATA (x).data;
}
