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

#define MAY_MAX_INPUT_BUFFER_SIZE 1000

size_t
may_out_string (FILE *stream, may_t x)
{
  char *s;
  size_t length;

  may_mark();
  s = may_get_string (NULL, 0, x);
  length = fprintf (stream, "%s", s);
  may_keep(NULL);
  return length;
}

size_t
may_in_string (may_t *x, FILE *stream)
{
  char buffer[MAY_MAX_INPUT_BUFFER_SIZE];
  char *s;
  size_t length;

  s = fgets (buffer, MAY_MAX_INPUT_BUFFER_SIZE-1, stream);
  buffer[MAY_MAX_INPUT_BUFFER_SIZE-1] = 0;
  if (s == NULL)
    length = 0;
  else {
    length = strlen (buffer);
    if (buffer[length -1] == '\n')
      buffer[length -1] = 0;
    *x = may_parse_str (buffer);
  }
  return length;
}

