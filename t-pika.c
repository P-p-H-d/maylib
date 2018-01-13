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

#include <stdio.h>
#include "may-impl.h"

#if !defined(__PEDROM__BASE__) && !defined(WIN32)
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/time.h>

int
cputime ()
{
#ifndef MAY_WANT_THREAD
  struct rusage rus;

  getrusage (0, &rus);
  return rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_usec / 1000;
#else
  struct timeval tv;
  gettimeofday (&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}
#else
int cputime () {return 0;}
#endif

void (*dump) (may_t) = may_dump;

static long
convert_long (const char *str, long def)
{
  char *end;
  long ret;
  ret = strtol (str, &end, 0);
  if (end == str)
    return def;
  else
    return ret;
}

int
main (int argc, char *argv[])
{
  int t1, t2, i, verbose;
  char *s;

  if (argc == 1) {
    printf ("Using %s\n", may_get_version ());
    printf("%s \"expression\" [-pPREC] [-mMODULO] [-verbose] [-recur] [-oOPERATION] [-o...]\n"
           "OPERATION may be:\n"
           "  -oexpand\n"
           "  -odiff var\n"
           "  -oantidiff var\n"
           "  -oevalf\n"
           "  -oevalr\n"
           "  -oifactor\n"
           "  -osubs var value\n"
           "  -otaylor var point order\n"
           "  -oseries var  order\n"
           "  -ocomdenom\n"
           "  -orectform\n"
           "  -osqrtsimp\n"
           "  -otan2sincos\n"
           "  -otrig2exp\n"
           "  -oexp2trig\n"
           "  -otrig2tan2\n"
           "  -opow2exp\n"
           "  -onormalsign\n"
           "  -ocombine\n"
           "  -ofactor\n"
           "  -ogcd exp2\n"
           "  -olcm exp2\n"
           "  -oapprox precision base rounding_mode\n"
           "  -orationalize\n"
           "  -otexpand\n"
           "  -oeexpand\n"
           "  -oindets\n"
           "  -odivqr divisor var\n"
           "  -osqrfree VAR\n",
           argv[0]);
    exit (0);
  }

  /* Maximum size of the stack. Don't extend it */
  may_kernel_start (0, 0);
  may_rootof_ext_init();

  // On parcourt les arguments pour rechercher les arguments pre operations
  verbose = 0;
  int prec_done = 0, intmod_done = 0;
  for (i = 2; i<argc;i++) {
    if (strcmp (argv[i], "-oapprox") == 0)
      may_kernel_num_presimplify (0);
    else if (strcmp (argv[i], "-verbose") == 0)
      verbose = 1;
    else if (prec_done == 0 && argv[i][0]=='-' && argv[i][1]=='p')
      may_kernel_prec (convert_long(&argv[i][2], 113)), prec_done = 1;
    else if (intmod_done == 0 && argv[i][0]=='-' && argv[i][1]=='m')
      may_kernel_intmod (may_set_ui (convert_long(&argv[i][2], 0))), intmod_done = 1;
  }

  if (verbose)
    may_kernel_info (stdout, "");

  t1 = cputime();

  MAY_TRY
    {
      may_t r;
      may_mark_t mark;

      may_mark (mark);
      char *end = may_parse_c (&r, argv[1]);
      int recur = 0;

      if (*end != 0) {
	printf ("Syntax Error in expression at '%s'.\n", end);
        exit (1);
      }
      r = may_eval (r);
      for (i = 2 ; i < argc ; i++)
	{
          if (strcmp(argv[i], "-recur") == 0)
            recur = 1-recur;
	  else if (strcmp(argv[i], "-oexpand") == 0)
	    r = may_expand (r);
	  else if (strcmp(argv[i], "-odiff") == 0)
            {
              may_t v = may_parse_str (argv[++i]);
              r = may_diff (r, v);
            }
	  else if (strcmp(argv[i], "-oantidiff") == 0)
            {
              may_t v = may_parse_str (argv[++i]);
              r = may_eval (may_diff_c (r, v, may_set_si (-1), v));
            }
	  else if (strcmp(argv[i], "-oevalf") == 0)
	    r = may_evalf (r);
	  else if (strcmp(argv[i], "-oevalr") == 0)
	    r = may_evalr (r);
	  else if (strcmp(argv[i], "-oifactor") == 0)
	    r = may_naive_ifactor (r);
          else if (strcmp (argv[i], "-otrig2exp") == 0)
            r = may_trig2exp (r);
          else if (strcmp (argv[i], "-oexp2trig") == 0)
            r = may_exp2trig (r);
          else if (strcmp (argv[i], "-osqrtsimp") == 0)
            r = may_sqrtsimp (r);
          else if (strcmp (argv[i], "-otrig2tan2") == 0)
            r = may_trig2tan2 (r);
          else if (strcmp (argv[i], "-otan2sincos") == 0)
            r = may_tan2sincos (r);
          else if (strcmp (argv[i], "-opow2exp") == 0)
            r = may_pow2exp (r);
          else if (strcmp (argv[i], "-oabs2sign") == 0)
            r = may_abs2sign (r);
          else if (strcmp (argv[i], "-osign2abs") == 0)
            r = may_sign2abs (r);
          else if (strcmp (argv[i], "-ofactor") == 0) {
            //may_t val = may_parse_str (argv[++i]);
            r = may_ratfactor (r, NULL);
          } else if (strcmp (argv[i], "-ogcd") == 0)
            {
	      may_t v = may_parse_str (argv[++i]);
              may_t tab[2] = {r, v};
              r = may_gcd (2, tab);
            }
	  else if (strcmp (argv[i], "-ogcdex") == 0)
            {
	      may_t v = may_parse_str (argv[++i]);
	      /* Argument must be the list {a,b,c} */
	      may_t s, t, g;
	      int errcode;
	      errcode = may_gcdex (&s, &t, &g,
				   may_op (r, 0), may_op (r, 1), may_op (r, 2), v);
	      if (errcode)
		r = may_list_vac (s, t, g, NULL);
	      else
		r = may_parse_str ("NAN");
	      r = may_eval (r);
            }
	  else if (strcmp (argv[i], "-olcm") == 0)
            {
	      may_t v = may_parse_str (argv[++i]);
              may_t tab[2] = {r, v};
              r = may_lcm (2, tab);
            }
	  else if (strcmp(argv[i], "-osubs") == 0)
	    {
	      may_t var = may_parse_str (argv[++i]);
	      may_t val = may_parse_str (argv[++i]);
	      r = may_replace (r, var, val);
	    }
	  else if (strcmp(argv[i], "-orewrite") == 0)
	    {
	      may_t pattern = may_parse_str (argv[++i]);
	      may_t value = may_parse_str (argv[++i]);
	      r = may_rewrite (r, pattern, value);
	    }
          else if (strcmp(argv[i], "-orectform") == 0)
            {
              may_t re, im;
              may_rectform (&re, &im, r);
              r = may_add_c (re, may_mul_c (may_set_cx (may_set_ui (0),
                                                        may_set_ui (1)), im));
              r = may_eval (r);
            }
	  else if (strcmp (argv[i], "-opartfrac") == 0)
            {
	      may_t v = may_parse_str (argv[++i]);
              r = may_partfrac (r, v, 0);
              if (r==0)
                r = may_set_d (0.0/0.0);
            }
          else if (strcmp (argv[i], "-ocomdenom") == 0)
            {
              may_t num, denom;
              may_comdenom (&num, &denom, r);
              r = may_div (may_expand (num), may_expand (denom));
            }
          else if (strcmp (argv[i], "-orationalize") == 0)
            {
              r = may_recursive (r, may_rationalize, -recur);
            }
          else if (strcmp (argv[i], "-otaylor") == 0)
            {
              may_t var = may_parse_str (argv[++i]);
              may_t pos = may_parse_str (argv[++i]);
              unsigned long n = atol (argv[++i]);
              r = may_taylor (r, var, pos, n);
            }
          else if (strcmp (argv[i], "-oseries") == 0)
            {
              may_t var = may_parse_str (argv[++i]);
              unsigned long n = atol (argv[++i]);
              r = may_series (r, var, n);
            }
          else if (strcmp (argv[i], "-oapprox") == 0)
            {
              long n = convert_long (argv[++i], 100);
              long base = convert_long (argv[++i], 10);
              long rnd = convert_long (argv[++i], 0);
             r = may_approx (r, base, n, rnd);
            }
          else if (strcmp (argv[i], "-otexpand") == 0)
            {
              r = may_texpand (r);
            }
          else if (strcmp (argv[i], "-oeexpand") == 0)
            {
              r = may_eexpand (r);
            }
          else if (strcmp (argv[i], "-ocombine") == 0)
            {
              r = may_combine (r, MAY_COMBINE_NORMAL);
            }
          else if (strcmp (argv[i], "-onormalsign") == 0)
            {
              r = may_normalsign (r);
            }
          else if (strcmp (argv[i], "-otcollect") == 0)
            {
              r = may_tcollect (r);
            }
          else if (strcmp (argv[i], "-osqrfree") == 0)
            {
	      may_t val = may_parse_str (argv[++i]);
              r = may_sqrfree (r, val);
            }
          else if (strcmp (argv[i], "-ocollect") == 0)
            {
	      may_t val = may_parse_str (argv[++i]);
              r = may_collect (r, val);
            }
          else if (strcmp (argv[i], "-oindets") == 0)
            {
              r = may_indets (r, MAY_INDETS_RECUR);
            }
          else if (strcmp (argv[i], "-odivqr") == 0)
            {
              may_t val = may_parse_str (argv[++i]);
              may_t var = may_parse_str (argv[++i]);
              may_t qq, rr;
              int i= may_div_qr (&qq, &rr, r, val, var);
              if (i == 0)
                r = may_set_d (0.0/0.0);
              else
                r = may_eval (may_list_vac (qq, rr, NULL));
            }
          else if (strcmp (argv[i], "-osmod") == 0)
            {
              may_t b = may_parse_str (argv[++i]);
              r = may_smod (r, b);
            }
          else if (argv[i][0]=='-' && argv[i][1]=='p')
            {
              may_kernel_prec (convert_long(&argv[i][2], 113));
              r = may_reeval (r);
            }
          else if (argv[i][0]=='-' && argv[i][1]=='m')
            {
              may_kernel_intmod (may_set_ui (convert_long(&argv[i][2], 0)));
              r = may_reeval (r);
            }
	  else if (strcmp (argv[i], "-verbose") == 0)
            {
              (void) r;
            }
          else
	    {
	      printf("Unkwon option: %s\n", argv[i]);
	      exit (1);
	    }
          r = may_compact (mark, r);
          t2 = cputime();
          if (t2 != 0 && t1!=t2) {
            fprintf(stderr, "CPU time=%dms  \n", t2-t1);
            t1 = cputime ();
          }
	}
      s = may_get_string (NULL, 0, r);
      printf("%s\n", s);
    }
  MAY_CATCH
    {
      may_kernel_info (stdout, "FATAL");
      printf("Exception %d caught: %s\n",
	     MAY_ERROR, may_error_what (MAY_ERROR));
    }
  MAY_ENDTRY;


  if (verbose)
    may_kernel_info (stdout, "");
  may_kernel_end ();
  return 0;
}

