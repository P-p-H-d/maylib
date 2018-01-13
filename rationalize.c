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
may_rationalize (may_t x)
{
  may_t temp[2];
  may_t y;

  MAY_LOG_FUNC (("%Y", x));

  may_mark ();
  may_comdenom (&(temp[0]), &(temp[1]), x);

  temp[0] = may_expand (temp[0]);
  temp[1] = may_expand (temp[1]);

  y = may_div_c (temp[0], temp[1]);
  return may_keep (may_eval (y));
}
