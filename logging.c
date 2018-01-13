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

/* Logging MAY needs GCC >= 3.0 and GLIBC >= 2.0. */

#ifdef MAY_WANT_LOGGING

/* Can't include them before (in particular, printf.h) */
#include <printf.h>
#include <stdarg.h>
#include <time.h>

/* Define LOGGING variables */

FILE  *may_log_file;
int    may_log_type;
int    may_log_level;
int    may_log_current;
size_t may_log_size;

static int
may_vcb_printf (FILE *stream, const struct printf_info *info,
                const void * const *arg)
{
  size_t size;
  int length;
  int org_type_logging;
  may_t w;

  UNUSED (info);
  org_type_logging = may_log_type;
  may_log_type = 0; /* We disable the logging during this print! */

  w = *((may_t *) (arg[0]));
  /* Compute the size of w while being protective about invalid data */
  size = MAY_UNLIKELY ((char*)w < may_g.Heap.base
                       || (char*) w >= may_g.Heap.top
                       || (((unsigned long)w) % sizeof(long)) != 0) ? 0
    : may_length (w);
  if (size >= may_log_size) {
    const char * type = may_get_name (w);
    length = fprintf (stream, "@@EXPR[hash=%X,size=%lu,type=%s,length=%lu,var={",
                      (unsigned int) MAY_HASH (w), (unsigned long) size, type, may_nops (w));
    if (type == may_sum_name || type == may_product_name) {
      w = may_indets (w, MAY_INDETS_NUM);
      size_t n = may_nops (w);
      for (size_t i = 0; i < n; i++) {
        if (i != 0)
          length += fprintf (stream, ",");
        length += fprintf (stream, "%Y", may_op (w, i));
      }
    }
    length += fprintf (stream, "}]@@");
  } else
    length = may_dump_rec (stream, w, 0);

  may_log_type = org_type_logging;

  return length;
}

static int
may_vcb_arginfo (const struct printf_info *info, size_t n,
                 int *argtypes)
{
  UNUSED (info);
  if (n > 0)
    argtypes[0] = PA_POINTER;
  return 1;
}

static void may_log_begin (void) __attribute__((constructor));

/* We let the system close the LOG itself
   (Otherwise functions called by destructor can't use LOG File */
static void
may_log_begin (void)
{
  const char *var;
  time_t tt;

  /* Grab some information */
  var = getenv ("MAY_LOG_LEVEL");
  may_log_level = var == NULL || *var == 0 ? 4 : atoi (var);
  may_log_current = 0;

  var = getenv ("MAY_LOG_SIZE");
  may_log_size = var == NULL || *var == 0 ? 1000 : atol (var);

  /* Get what we need to log */
  may_log_type = 0;
  if (getenv ("MAY_LOG_INPUT") != NULL)
    may_log_type |= MAY_LOG_INPUT_F;
  if (getenv ("MAY_LOG_TIME") != NULL)
    may_log_type |= MAY_LOG_TIME_F;
  if (getenv ("MAY_LOG_MSG") != NULL)
    may_log_type |= MAY_LOG_MSG_F;
  if (getenv ("MAY_LOG_STAT") != NULL)
    may_log_type |= MAY_LOG_STAT_F;
  if (getenv ("MAY_LOG_ALL") != NULL)
    may_log_type = MAY_LOG_INPUT_F|MAY_LOG_TIME_F|MAY_LOG_MSG_F|MAY_LOG_STAT_F;

  /* Register printf functions */
  register_printf_function ('Y', may_vcb_printf, may_vcb_arginfo);

  /* Open filename if needed */
  var = getenv ("MAY_LOG_FILE");
  if (var == NULL || *var == 0)
    var = "may.log";
  if (may_log_type != 0) {
    may_log_file = fopen (var, "w");
    if (may_log_file == NULL) {
      fprintf (stderr, "[MAYLIB]: Can't open log file '%s' for writing.\n", var);
      abort ();
    } else {
      fprintf (stderr, "[MAYLIB]: Open log file '%s' for writing.\n", var);
    }
    time (&tt);
    fprintf (may_log_file, "MAY LOG FILE %s\n", ctime (&tt));
  }
}

/* Return user CPU time measured in milliseconds. */

#if defined (ANSIONLY) || defined (USG) || defined (__SVR4) \
 || defined (_UNICOS) || defined(__hpux)

int
may_get_cputime (void)
{
  return (int) ((unsigned long long) clock () * 1000 / CLOCKS_PER_SEC);
}

#else /* Use getrusage for cputime */

#include <sys/types.h>
#include <sys/resource.h>

int
may_get_cputime (void)
{
  struct rusage rus;
  getrusage (0, &rus);
  return rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_usec / 1000;
}

#endif /* cputime */

#endif /* MAY_WANT_LOGGING */
