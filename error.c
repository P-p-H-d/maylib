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

void
may_error_catch (void (*handler)(may_error_e,const char [],const void*),
                 const void *data)
{
  struct may_error_frame_s *ef;

  MAY_LOG_MSG (("Trap function for errors\n"));

  /* Save the frame */
  ef  = may_alloc (sizeof *ef);
  *ef = may_g.frame;
  /* Register the handler */
  may_g.frame.next = ef;
  may_g.frame.error_handler = handler;
  may_g.frame.error_handler_data = data;
}

void
may_error_uncatch (void)
{
  MAY_LOG_MSG (("Remove Trap function for errors\n"));

  /* Everything is OK. Keep the globals. Remove the handler */
  may_g.frame.error_handler = may_g.frame.next->error_handler;
  may_g.frame.error_handler_data = may_g.frame.next->error_handler_data;
  may_g.frame.next = may_g.frame.next->next;
}

void
may_error_get (may_error_e *e, const char **str)
{
  if (e)   *e = may_g.last_error;
  if (str) *str = may_g.last_error_str;
}

void
may_error_throw   (may_error_e error, const char description[])
{
  void (*handler) (may_error_e, const char [], const void *);

  MAY_LOG_MSG(("Throw error %d (%s)\n", error, description));

  /* Check if there is an error handler */
  handler = may_g.frame.error_handler;
  if (handler) {
    const void *data = may_g.frame.error_handler_data;
    /* Restore globals and remove handler */
    may_g.frame = *(may_g.frame.next);
    /* Save new errors */
    may_g.last_error = error;
    may_g.last_error_str = description;
    /* Call error handler */
    (*handler) (error, description, data);
  }
  /* No Error handler or error handler returned: abort program */
  fprintf (stderr, "[MAYLIB]: Uncatched exception %d [%s]\n",
           error, may_error_what(error));
  abort ();
}

void
may_error_setjmp_handler (may_error_e e, const char desc[], const void *data)
{
  UNUSED (desc);
  longjmp ( *(jmp_buf*)data, e);
}

const char *
may_error_what (may_error_e error)
{
  switch (error)
    {
    case MAY_NO_ERR:
      return "No error";
    case MAY_INVALID_TOKEN_ERR:
      return "Invalid Token";
    case MAY_MEMORY_ERR:
      return "Memory";
    case MAY_CANT_BE_CONVERTED_ERR:
      return "Object can't be converted";
    case MAY_DIMENSION_ERR:
      return "Dimension";
    case MAY_SINGULAR_MATRIX_ERR:
      return "Singular Matrix";
    case MAY_VALUATION_NOT_POS_ERR:
      return "Valuation is not strictly positive.";
    default:
      return "Unkwnow";
    }
}
