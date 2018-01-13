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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "may.h"

static int verbose = 2;


static int
cputime (void)
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

int
test_construct (void)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("Construct (3*(a*x+b*y+c*z) with a=1/2, b=2/3 and c=4/5...");
    fflush (stdout);
  }

  int t0 = 0;
  unsigned long long m0, m;
  may_t a = may_set_si_ui (1,2);
  may_t b = may_set_si_ui (2,3);
  may_t c = may_set_si_ui (4,5);
  may_t x = may_set_str ("x");
  may_t y = may_set_str ("y");
  may_t z = may_set_str ("z");
  may_t three = may_set_ui (3);

  for (m0 = 300000; t0 < 50 ; m0*=2) {
    t0 = cputime ();
    may_mark ();
    for (m= 0; m < m0; m++) {
      may_t temp[3];
      temp[0] = may_mul_c (a, x);
      temp[1] = may_mul_c (b, y);
      temp[2] = may_mul_c (c, z);
      temp[0] = may_add_vc (3, temp);
      temp[0] = may_mul_c (three, temp[0]);
      may_eval (temp[0]);
    }
    may_keep (NULL);
    t0 = cputime () - t0;
  }

  if (verbose >= 2)
    printf ("%2.5fms [%Ld execs/sec]\n", ((double)t0)/(m0/2), 1000*(m0/2)/t0);

  may_keep (NULL);
  return t0/(m0/2);
}

int
test_eval (long n)
{
  may_t *base = malloc (n*sizeof (long));
  may_t x;
  long i;
  int  t0, t1, m0, m1, m;
  char buffer[100];
  may_mark_t mark;

  may_mark (mark);

  if (verbose >= 2) {
    printf ("eval (sum ai*ai*ai) - quite different - N=%ld...", n);
    fflush (stdout);
  }

  for (i = 0; i < n ; i++) {
    sprintf (buffer, "a%d*a%d*a%d", rand(), rand (), rand());
    base[i] = may_parse_str (buffer);
  }

  if (verbose >= 2) {
    printf ("...");
    fflush (stdout);
  }

  for (m0 = t0 = 1; m0 < 10000 && t0 < 10 ; m0++) {
    t0 = cputime ();
    for (m = 0; m < m0; m++) {
      x = may_add_vc (n, base);
      x = may_eval (x);
    }
    t0 = cputime () - t0;
  }

  if (verbose >= 2)
    printf ("%2.2fms\n", ((double) t0)/(m0-1));

  may_compact (mark, NULL);

  if (verbose >= 2) {
    printf ("eval (sum ai*ai*ai) -  quite similar  - N=%ld...", n);
    fflush (stdout);
  }

  for (i = 0; i < n ; i++) {
    sprintf (buffer, "a%d*a%d*a%d", rand() % 5, rand () % 5, rand()) % 5;
    base[i] = may_parse_str (buffer);
  }

  if (verbose >= 2) {
    printf ("...");
    fflush (stdout);
  }

  for (m1 = t1 = 1; m1 < 10000 && t1 < 10 ; m1++) {
    t1 = cputime ();
    for (m = 0; m < m1; m++) {
      x = may_add_vc (n, base);
      x = may_eval (x);
    }
    t1 = cputime () - t1;
  }

  if (verbose >= 2)
    printf ("%2.2fms\n", ((double)t1)/(m1-1));

  may_compact (mark, NULL);
  free (base);

  return t0/(m0-1)+t1/(m1-1);
}

/* Test add many numericals and a 'x' */
int
test_addnum (int n)
{
  int t0;
  may_t s;
  may_mark ();

  if (verbose >= 2) {
    printf ("eval(sum(i,i=0..%d)+x+sum(i,i=0..%d))...", n, n);
    fflush (stdout);
  }
  t0 = cputime ();

  s = may_set_ui (0);
  for (int i = 0; i < n; i++)
    s = may_addinc_c (s, may_set_ui (i));
  s = may_addinc_c (s, may_set_str ("x"));
  for (int i = 0; i < n; i++)
    s = may_addinc_c (s, may_set_ui (i));
  s = may_eval (s);

  t0 = cputime () - t0;
  if (verbose >= 2)
    printf ("%dms\n", t0);

  may_keep (NULL);
  return t0;
}

/* Test small expand with many variables, replace and evaluator */
int
test_expand (int n)
{
  may_t ai, a0, sum, s2;
  char tmp[10];
  int t0, i;

  may_mark ();
  if (verbose >= 2) {
    printf ("expand ((a0+...a%d)^2), replace a0, reeval...", n);
    fflush (stdout);
  }

  t0 = cputime ();

  sum = s2 = may_set_ui (0);
  for (i = 0; i < n; i++) {
    sprintf (tmp, "a%d", i);
    ai = may_set_str (tmp);
    sum = may_addinc_c (sum, ai);
    if (i >= 2)
      s2 = may_addinc_c (s2, ai);
    if (i == 0)
      a0 = ai;
  }
  sum = may_sqr_c (sum);
  sum = may_eval (sum);
  sum = may_expand (sum);

  if (verbose >= 3) {
    printf ("%dms...", cputime ()-t0);
    fflush (stdout);
  }

  s2  = may_eval (may_neg_c (s2));
  sum = may_replace (sum, a0, s2);

  if (verbose >= 3) {
    printf ("%dms...", cputime ()-t0);
    fflush (stdout);
  }

  sum = may_expand (sum);

  t0 = cputime () - t0;
  if (verbose >= 2)
    printf ("%dms\n", t0);

  may_keep (NULL);
  return t0;
}


/* Test multivariate polynomial */
int
test_expand2 (int n, int d)
{
  may_t x, sum;
  char buffer[10];
  int i, t0;
  may_mark_t mark;

  may_mark (mark);
  if (verbose >= 2) {
    printf ("expand ((x0+...x%d+1)^%d*(1+(x0+...x%d+1)^%d))...",
            n-1, d, n-1, d);
    fflush (stdout);
  }
  t0 = cputime ();

  /* DO IT */
  sum = may_set_ui (1);
  for (i =0; i < n; i++) {
    sprintf (buffer, "x%d", i);
    x = may_set_str (buffer);
    sum = may_addinc_c (sum, x);
  }
  sum = may_pow_c (sum, may_set_ui (d));
  sum = may_eval (sum);
  sum = may_expand (sum);
  x   = may_add (sum, may_set_ui (1));
  sum = may_mul (sum, x);
  sum = may_compact (mark, sum);

  if (verbose >= 3) {
    printf ("%dms...", cputime () - t0);
    fflush (stdout);
  }

  sum = may_expand (sum);
  t0 = cputime() - t0;

  /* End */
  if (verbose >= 2)
    printf ("%dms\n", t0);
  may_compact (mark, NULL);

  return t0;
}

int
test_expand3 (int d)
{
  may_t sum;
  char buffer[100];
  int t0;

  may_mark ();
  sprintf (buffer, "(x+y^400000000000+z)^%d*(1+x+y+z^-1)^%d", d, d);

  if (verbose >= 2) {
    printf ("expand (%s)...", buffer);
    fflush (stdout);
  }
  sum = may_parse_str (buffer);
  t0 = cputime ();
  sum = may_expand (sum);
  t0 = cputime() - t0;

  /* End */
  if (verbose >= 2)
    printf ("%dms\n", t0);
  may_keep (NULL);
  return t0;
}

int
test_expand4 (int d)
{
  char buffer1[100];
  char buffer2[100];
  int t0;

  may_mark ();
  sprintf (buffer1, "(1+x)^%d*(1+y)^%d", d, d);
  sprintf (buffer2, "(1-x)^%d*(2-y)^%d", d, d);

  if (verbose >= 2) {
    printf ("expand (expand(%s) * expand(%s))...", buffer1, buffer2);
    fflush (stdout);
  }

  may_t a = may_expand (may_parse_str (buffer1));
  may_t b = may_expand (may_parse_str (buffer2));

  t0 = cputime ();
  may_expand (may_mul (a, b));
  t0 = cputime() - t0;

  /* End */
  if (verbose >= 2)
    printf ("%dms\n", t0);
  may_keep (NULL);
  return t0;
}

int
test_expand5 (int d)
{
  char buffer[100];
  int t0;

  may_mark ();
  sprintf (buffer, "(a+b+c+d+e+f+g+h)^%d", d);

  if (verbose >= 2) {
    printf ("expand (expand(%s) * expand(%s))...", buffer, buffer);
    fflush (stdout);
  }

  may_t a = may_expand (may_parse_str (buffer));
  may_t b = may_expand (may_parse_str (buffer));

  t0 = cputime ();
  may_expand (may_mul (a, b));
  t0 = cputime() - t0;

  /* End */
  if (verbose >= 2)
    printf ("%dms\n", t0);
  may_keep (NULL);
  return t0;
}

int
test_expand6 (int n)
{
  int t0;

  may_mark ();

  if (verbose >= 2) {
    printf ("expand ((1+x+...+x^%d)*(1+2*x+x^2+...+x^%d))...", n-1, n-1);
    fflush (stdout);
  }

  t0 = cputime ();

  may_t s = may_set_ui (0);
  may_t x = may_set_str ("x");
  for (int i = 0; i < n; i++)
    s = may_addinc_c (s, may_pow_si_c (x, i));
  s = may_eval (s);
  may_t s2 = may_add (s, x);
  s = may_mul (s, s2);
  s = may_compact (s);
  s = may_expand (s);
  t0 = cputime() - t0;

  /* End */
  if (verbose >= 2)
    printf ("%dms\n", t0);
  may_keep (NULL);
  return t0;
}

/* Test univariate multiply */
int
test_univariate_multiply (int a, int b, int n)
{
  char Buffer[100];
  may_t x;
  int t0;

  /* Begin */
  may_mark ();
  sprintf (Buffer, "(%d+x)^%d*(%d+x)^%d", a, n, b, n);
  x = may_parse_str (Buffer);
  if (verbose >= 2) {
    printf ("expand ((%d+x)^%d*(%d+x)^%d)...", a, n, b, n);
    fflush (stdout);
  }

  /* DO IT */
  t0 = cputime ();
  x = may_expand (x);
  t0 = cputime() - t0;

  /* End */
  if (verbose >= 2)
    printf ("%dms\n", t0);
  may_keep (NULL);
  return t0;
}

/* Test expand ((1+sqrt(5)) ^N */
int
test_power_field (int n)
{
  char Buffer[100];
  may_t x;
  int t0;

  /* Begin */
  may_mark ();
  sprintf (Buffer, "(1+sqrt(5))^%d", n);
  x = may_parse_str (Buffer);
  if (verbose >= 2) {
    printf ("expand (%s)...", Buffer);
    fflush (stdout);
  }

  /* DO IT */
  t0 = cputime ();
  x = may_expand (x);
  t0 = cputime() - t0;

  /* End */
  if (verbose >= 2)
    printf ("%dms\n", t0);
  may_keep (NULL);
  return t0;
}

int
test_pow_multivariate (int n)
{
  char Buffer[100];
  may_t x;
  int t0;

  /* Begin */
  may_mark ();
  sprintf (Buffer, "(1+x+y)^%d", n);
  x = may_parse_str (Buffer);
  if (verbose >= 2) {
    printf ("expand (%s)...", Buffer);
    fflush (stdout);
  }

  /* DO IT */
  t0 = cputime ();
  x = may_expand (x);
  t0 = cputime() - t0;

  /* End */
  if (verbose >= 2)
    printf ("%dms\n", t0);
  may_keep (NULL);
  return t0;
}



/* Test evaluator and fast replace */
static may_t identity (may_t x) { return x; }
static may_t (*tfunc[]) (may_t) = {&identity};
static const char* tname[] = {"f"};

int
test_critical (int n)
{
  may_t x, s;
  int i, t0;

  may_mark ();
  if (verbose >= 2) {
    printf ("eval(x+f(x)+f(f(x))+...+f(%d)(x)), subs f to id...", n);
    fflush (stdout);
  }

  t0 = cputime ();
  x = may_set_str ("x");
  s = may_set_ui (0);

  for (i=0; i < n; i++) {
    s = may_addinc_c (s, x);
    x = may_func_c ("f", x);
  }
  s = may_eval (s);

  if (verbose >= 3) {
    printf ("%dms...", cputime () - t0);
    fflush (stdout);
  }

  s = may_subs_c (s, n+1,1, tname, (void*) tfunc);
  s = may_eval (s);

  t0 = cputime () - t0;
  if (verbose >= 2)
    printf ("%dms\n", t0);

  may_keep (NULL);
  return t0;
}

int
test_sum_x (int n)
{
  int t0;
  may_t s, x;
  may_mark ();

  if (verbose >= 2) {
    printf ("eval(sum(i*x^i, n=0..%d)...", n);
    fflush (stdout);
  }
  t0 = cputime ();
  s = may_set_ui (0);
  x = may_set_str ("x");
  for (int i = 0; i < n; i++)
    s = may_addinc_c (s, may_mul_c (may_set_ui (i), may_pow_c (x, may_set_ui (i))));
  s = may_eval (s);

  t0 = cputime () - t0;
  if (verbose >= 2)
    printf ("%dms\n", t0);

  may_keep (NULL);
  return t0;
}

int
test_sin_pi6 (int n)
{
  int t0;
  may_t s, pi6;
  may_mark ();

  if (verbose >= 2) {
    printf ("eval(sum(sin(n*PI/6), n=0..%d)...", n);
    fflush (stdout);
  }
  t0 = cputime ();
  s = may_set_ui (0);
  pi6 = may_div (may_set_str ("PI"), may_set_ui (6));
  for (int i = 0; i < n; i++)
    s = may_addinc_c (s, may_sin_c (may_mul_c (may_set_ui (i), pi6)));
  s = may_eval (s);

  t0 = cputime () - t0;
  if (verbose >= 2)
    printf ("%dms\n", t0);

  may_keep (NULL);
  return t0;
}

int
test_complex (int n)
{
  int t0;
  may_mark ();
  if (verbose >= 2) {
    printf ("eval((2+3*I/4)^%d)...", n);
    fflush (stdout);
  }
  t0 = cputime ();
  may_eval (may_pow_c (may_set_cx (may_set_ui (2), may_set_si_ui (3,4)), may_set_si (n)));
  t0 = cputime () - t0;
  if (verbose >= 2)
    printf ("%dms\n", t0);

  may_keep (NULL);
  return t0;
}

int
test_evalf (int p)
{
  int t0;
  may_mark ();
  if (verbose >= 2) {
    printf ("evalf(sin(1+PI)^2+3^sqrt(1+PI^2)) to %d bits...", p);
    fflush (stdout);
  }
  may_t e = may_parse_str ("sin(1+PI)^2+3^sqrt(1+PI^2)");
  may_kernel_prec (p);
  t0 = cputime ();
  may_evalf (e);
  t0 = cputime () - t0;
  may_kernel_prec (113);
  if (verbose >= 2)
    printf ("%dms\n", t0);

  may_keep (NULL);
  return t0;
}


/* DIVIDE */
int
test_divide (const char *buffer1, const char *buffer2, const char *var)
{
  int t0;
  may_t q, r, a, b, v;

  may_mark ();

  /* Input */
  a = may_expand (may_parse_str (buffer1));
  b = may_expand (may_parse_str (buffer2));
  v = may_parse_str (var);

  if (verbose >= 2) {
    printf ("divide ( %s , %s, %s)...", buffer1, buffer2, var);
    fflush (stdout);
  }
  t0 = cputime ();

  /* Do it */
  may_div_qr (&q, &r, a, b, v);

  /* End */
  t0 = cputime() - t0;
  if (verbose >= 2)
    printf ("%dms\n", t0);
  may_keep (NULL);
  return t0;
}

/* GCD (((i+1)*Sum(ai))^d,((-i*Sum(ai)+1)^d) */
int
test_gcd (const char *buffer1, const char *buffer2)
{
  may_t tab[2];
  int t0;
  char buf2print1[50];
  char buf2print2[50];
  int do_expand;

  /* BEGIN */
  may_mark ();
  tab[0] =  may_parse_str (buffer1);
  tab[1] =  may_parse_str (buffer2);
  if (may_nan_p (tab[0]) || may_nan_p (tab[1]))
    abort ();

  do_expand = may_get_name (tab[0]) != may_sum_name || may_get_name (tab[1]) != may_sum_name;

  strncpy (buf2print1, buffer1, sizeof buf2print1);
  buf2print1[sizeof buf2print1 - 3] = '.';
  buf2print1[sizeof buf2print1 - 2] = '.';
  buf2print1[sizeof buf2print1 - 1] = 0;
  strncpy (buf2print2, buffer2, sizeof buf2print2);
  buf2print2[sizeof buf2print2 - 3] = '.';
  buf2print2[sizeof buf2print2 - 2] = '.';
  buf2print2[sizeof buf2print2 - 1] = 0;

  if (verbose >= 2) {
    printf ("       gcd ( %s , %s )...", buf2print1, buf2print2);
    fflush (stdout);
  }

  /* DO IT */
  t0 = cputime ();
  may_gcd (2, tab);
  t0  = cputime () - t0;

  /* END */
  if (verbose >= 2)
    printf ("%dms\n", t0);
  may_compact (NULL);

  /* BEGIN */
  if (do_expand == 0)
    return t0;

  tab[0] = may_expand (may_parse_str (buffer1));
  tab[1] = may_expand (may_parse_str (buffer2));
  if (verbose >= 2) {
    printf ("expand+gcd ( %s , %s )...", buf2print1, buf2print2);
    fflush (stdout);
  }

  /* DO IT */
  t0 = cputime ();
  may_gcd (2, tab);
  t0  = cputime () - t0;

  /* END */
  if (verbose >= 2)
    printf ("%dms\n", t0);
  may_keep (NULL);
  return t0;
}

static may_t
sum_y_i (int start, int end, int v)
{
  may_t s;
  may_mark ();
  s = may_set_ui (0);
  for (int i = start ; i <= end; i++) {
    char buffer[100];
    sprintf (buffer, "y%d", i);
    may_t y = may_set_str (buffer);
    s = may_addinc_c (s, may_pow_si_c (y, v));
  }
  return may_keep (may_eval (s));
}

static may_t
mul_y_i (int start, int end, int v)
{
  may_t s;
  may_mark ();
  s = may_set_ui (1);
  may_t vy = may_set_si (v);
  for (int i = start ; i <= end; i++) {
    char buffer[100];
    sprintf (buffer, "y%d", i);
    may_t y = may_set_str (buffer);
    s = may_mul_c (s, may_add_c (y, vy));
  }
  return may_keep (may_eval (s));
}

/* From 'Evaluation of the Heuristic Polynomial GCD' by Fateman */
int
test_gcd2 (int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9, int n10)
{
  int t0 = 0, t;
  may_mark ();

  may_t d, f, g, x, y, z, y1, x_2, y1_2, tab[2];
  x    = may_set_str ("x");
  y    = may_set_str ("y");
  z    = may_set_str ("z");
  y1   = may_set_str ("y1");
  x_2  = may_mul (x, x);
  y1_2 = may_mul (y1, y1);

  /* F = (x   + sum(y[i],i=1..n) + 1) * (x + sum(y[i],i=1..n)+2)
     G = (x^2 + sum(y[i]^2,i=1..n) + 1) * (-3*y[1]*x^2+y[1]^2-1) */
  f = may_mul_c (may_add_vac (x, sum_y_i (1, n1, 1), may_set_ui (1), NULL),
                 may_add_vac (x, sum_y_i (1, n1, 1), may_set_ui (2), NULL));
  g = may_mul_c (may_add_vac (x_2, sum_y_i (1, n1, 2), may_set_ui (1), NULL),
                 may_add_vac (may_mul_vac (may_set_si(-3), y1, x_2, NULL), y1_2, may_set_si (-1), NULL));
  tab[0] = may_expand (may_eval (f));
  tab[1] = may_expand (may_eval (g));
  if (verbose >= 2) {
    printf ("GCDFT1: gcd = 1 (n=%d)...", n1);
    fflush (stdout);
  }
  t = cputime ();
  may_gcd (2, tab);
  t  = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  t0 += t;

  /* D = (x + sum(y[i],i=1,n)+1) ^2
     F = D* (x- sum(y[i],i=1,n)-2) ^2
     G = D* (x+ sum(y[i],i=1,n)+2) ^2 */
  d = may_pow_si_c (may_add_vac (x, sum_y_i (1, n2, 1), may_set_ui (1), NULL), 2);
  f = may_pow_si_c (may_sub_c (x, may_add_c (sum_y_i (1, n2, 1), may_set_ui (2))), 2);
  g = may_pow_si_c (may_add_vac (x, sum_y_i (1, n2, 1), may_set_ui (2), NULL), 2);
  tab[0] = may_expand (may_eval (may_mul_c (d, f)));
  tab[1] = may_expand (may_eval (may_mul_c (d, g)));
  if (verbose >= 2) {
    printf ("GCDFT2: gcd of linearly dense quartic inputs with quadratic GCDs (n=%d)...", n2);
    fflush (stdout);
  }
  t = cputime ();
  may_gcd (2, tab);
  t  = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  t0 += t;

  /* D = x^(n+1) + sum (y[i]^(n+1),i=1..n) + 1
     F = D* (x^(n+1) + sum (y[i]^(n+1),i=1..n) - 2)
     G = D* (x^(n+1) + sum (y[i]^(n+1),i=1..n) + 2) */
  d = may_add_vac (may_pow_si_c (x, n3+1), sum_y_i (1, n3, n3+1), may_set_ui (1), NULL);
  f = may_add_vac (may_pow_si_c (x, n3+1), sum_y_i (1, n3, n3+1), may_set_si (-2), NULL);
  g = may_add_vac (may_pow_si_c (x, n3+1), sum_y_i (1, n3, n3+1), may_set_si (+2), NULL);
  tab[0] = may_expand (may_eval (may_mul_c (d, f)));
  tab[1] = may_expand (may_eval (may_mul_c (d, g)));
  if (verbose >= 2) {
    printf ("GCDFT3: gcd of sparse inputs where degree // to #vars (n=%d)...", n3);
    fflush (stdout);
  }
  t = cputime ();
  may_gcd (2, tab);
  t  = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  t0 += t;

  /* D = x^(n+1) + sum (y[i]^n,i=1..n) + 1
     F = D* (x^(n+1) + sum (y[i]^(n+1),i=1..n) - 2)
     G = D* (x^(n+1) + sum (y[i]^(n+1),i=1..n) + 2) */
  d = may_add_vac (may_pow_si_c (x, n4+1), sum_y_i (1, n4, n4), may_set_ui (1), NULL);
  f = may_add_vac (may_pow_si_c (x, n4+1), sum_y_i (1, n4, n4+1), may_set_si (-2), NULL);
  g = may_add_vac (may_pow_si_c (x, n4+1), sum_y_i (1, n4, n4+1), may_set_si (+2), NULL);
  tab[0] = may_expand (may_eval (may_mul_c (d, f)));
  tab[1] = may_expand (may_eval (may_mul_c (d, g)));
  if (verbose >= 2) {
    printf ("GCDFT4: gcd of sparse inputs where degree // to #vars (second) (n=%d)...", n4);
    fflush (stdout);
  }
  t = cputime ();
  may_gcd (2, tab);
  t  = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  t0 += t;

  /* D = y1^2*x^2 + sum( y[i]^2,i=2..n) + 1
     F = D* (x^2-y1^2+sum(y[i]^2,i=2..n) -1)
     G = D* (y1*x+sum(y[i],i=2..n)+ 2)^2 */
  d = may_add_vac (may_mul_c (y1_2, x_2), sum_y_i (2,n5,2), may_set_ui (1), NULL);
  f = may_sub_c (may_add_c (x_2, sum_y_i (2,n5,2)), may_add_c (y1_2, may_set_ui (1)));
  g = may_add_vac (may_mul_c (y1, x), sum_y_i (2,n5,1),may_set_ui (2), NULL);
  g = may_pow_si_c (g, 2);
  tab[0] = may_expand (may_eval (may_mul_c (d, f)));
  tab[1] = may_expand (may_eval (may_mul_c (d, g)));
  if (verbose >= 2) {
    printf ("GCDFT5: gcd quadratic non-monic with other quadratic factors (n=%d)...", n5);
    fflush (stdout);
  }
  t = cputime ();
  may_gcd (2, tab);
  t  = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  t0 += t;

  /* D= (x+1) * product (y[i]+1, i=1..n)-3
     F=D* ((x-2)*product (y[i]-2, i=1..n)+3)
     G=D* ((x+2)*product (y[i]+2, i=1..n)-3) */
  d = may_add_c (may_mul_c (may_add_c (x, may_set_ui (1)), mul_y_i (1, n6, 1)), may_set_si (-3));
  f = may_add_c (may_mul_c (may_add_c (x, may_set_si (-2)), mul_y_i (1, n6, -2)), may_set_si (3));
  g = may_add_c (may_mul_c (may_add_c (x, may_set_si (2)), mul_y_i (1, n6, 2)), may_set_si (-3));
  tab[0] = may_expand (may_eval (may_mul_c (f, d)));
  tab[1] = may_expand (may_eval (may_mul_c (g, d)));
  if (verbose >= 2) {
    printf ("GCDFT7: gcd completly dense non-monic quadratic inputs (n=%d)...", n6);
    fflush (stdout);
  }
  t = cputime ();
  may_gcd (2, tab);
  t  = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  t0 += t;

  /* D= x * product (y[i], i=1..n)-1
     F=D* (x*product (y[i], i=1..n)+3)
     G=D* (x*product (y[i], i=1..n)-3) */
  d = may_add_c (may_mul_c (x, mul_y_i (1, n7, 0)), may_set_si (-1));
  f = may_add_c (may_mul_c (x, mul_y_i (1, n7, 0)), may_set_si (3));
  g = may_add_c (may_mul_c (x, mul_y_i (1, n7, 0)), may_set_si (-3));
  tab[0] = may_expand (may_eval (may_mul_c (f, d)));
  tab[1] = may_expand (may_eval (may_mul_c (g, d)));
  if (verbose >= 2) {
    printf ("GCDFT8: gcd sparse non-monic quadratic inputs with linear gcds (n=%d)...", n7);
    fflush (stdout);
  }
  t = cputime ();
  may_gcd (2, tab);
  t  = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  t0 += t;

  /* D = x^n*y*(z-1)
     F=D* (x^n+y^(n+1)*z^n+1)
     G=D* (x^(n+1)+y^n*z^(n+1)-7) */
  d = may_mul_vac (may_pow_si_c (x, n8), y, may_add_c (z, may_set_si (-1)), NULL);
  f = may_add_vac (may_pow_si_c (x, n8), may_mul_c (may_pow_si_c (y, n8+1), may_pow_si_c (z, n8)), may_set_si (1), NULL);
  g = may_add_vac (may_pow_si_c (x, n8+1), may_mul_c (may_pow_si_c (y, n8), may_pow_si_c (z, n8+1)), may_set_si (-7), NULL);
  tab[0] = may_expand (may_eval (may_mul_c (f, d)));
  tab[1] = may_expand (may_eval (may_mul_c (g, d)));
  if (verbose >= 2) {
    printf ("GCDFT9: trivariate inputs with increasing degrees (n=%d)...", n8);
    fflush (stdout);
  }
  t = cputime ();
  may_gcd (2, tab);
  t  = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  t0 += t;

  /* F = (x-y*z+1)^n9*(x-y+3*z)^n9
     G = (x-y*z+1)^n9*(x-y+3*z)^n10 */
  f = may_parse_str ("x-y*z+1");
  g = may_parse_str ("x-y+3*z");
  tab[0] = may_expand (may_eval (may_mul_c (may_pow_si_c (f, n9), may_pow_si_c (g, n9))));
  tab[1] = may_expand (may_eval (may_mul_c (may_pow_si_c (f, n9), may_pow_si_c (g, n10))));
  if (verbose >= 2) {
    printf ("GCDFT10: trivariate polynomials whose GCD has common factors with cofactors (j=%d,k=%d)...", n9, n10);
    fflush (stdout);
  }
  t = cputime ();
  may_gcd (2, tab);
  t  = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  t0 += t;

  may_keep (NULL);
  return t0;
}

int
test_diff (const char *buffer)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("diff ( %s , x)...", buffer);
    fflush (stdout);
  }
  int m0, t0;
  may_t x = may_set_str ("x");
  may_t e = may_parse_str (buffer);

  for (m0 = t0 = 1; t0 < 100 ; m0*=2) {
    may_mark ();
    t0 = cputime ();
    for (int m = 0; m < m0; m++)
      may_diff (e, x);
    t0 = cputime () - t0;
    may_keep (NULL);
  }

  if (verbose >= 2)
    printf ("%2.5fms\n", ((double)t0)/(2*m0));
  may_keep (NULL);
  return t0/(2*m0);
}


/* Function f(z)=sqrt(1/3)*z^2 + i/3 */
static may_t f_1_3, sqrt_1_3;
static may_t
f_c (may_t z)
{
  may_t y;
  y = may_mul_c (sqrt_1_3, may_sqr_c (z));
  y = may_add_c (y, f_1_3);
  return may_expand (may_eval (y));
}

int
test_R1 (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("R1: f(f(...) with f(z)=sqrt(1/3)*z^2 + i/3 and n=%d...", n);
    fflush (stdout);
  }
  int i, t = cputime();

  f_1_3    = may_set_si_ui(1, 3);
  sqrt_1_3 = may_sqrt (f_1_3);
  f_1_3    = may_set_cx (may_set_ui (0), f_1_3);

  may_t z = may_set_cx (may_set_ui(0), may_set_si_ui (1, 2));
  for (i = 0; i < n ;i++)
    z = may_func_c ("f", z);
  const char *vartab[] = {"f"};
  const void *valuetab[] = { (void*) &f_c};
  z = may_subs_c (z, 1, 1, vartab, valuetab);
  z = may_real (may_eval (z));

  t = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  may_keep (NULL);
  return t;
}

static may_t
hermite( int n, may_t y)
{
  if (n == 1) return may_mul (y, may_set_ui (2));
  if (n == 0) return may_set_ui (1);
  may_t z = may_sub_c (may_mul_c (may_mul_c (may_set_ui (2), y), hermite (n-1, y)),
                       may_mul_c (may_set_ui (2*(n-1)), hermite (n-2, y)));
  return may_expand (may_eval (z));
}

int
test_R2 (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("R2: hermite(%d,y)...", n);
    fflush (stdout);
  }
  int t = cputime();
  hermite(n, may_set_str ("y"));
  t = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  may_keep (NULL);
  return t;
}

int
test_R6 (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("R6: sum(((x+sin(i))/x+(x-sin(i))/x) for n=%d...", n);
    fflush (stdout);
  }
  int i, t = cputime();

  may_t x = may_set_str ("x"), s = may_set_ui (0);
  for (i = 0; i < n ; i++) {
    may_t sini = may_sin_c (may_set_ui (i));
    s = may_addinc_c (s, may_div_c (may_add_c (x, sini), x));
    s = may_addinc_c (s, may_div_c (may_sub_c (x, sini), x));
  }
  s = may_eval (s);
  s = may_rationalize (s);

  t = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  may_keep (NULL);
  return t;
}

int
test_R7 (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("R7: %d random float eval of x^24+34*x^12+45*x^3+9*x^18 +34*x^10+ 32*x^21...", n);
    fflush (stdout);
  }
  may_kernel_prec (53);
  int i, t = cputime();

  may_t w = may_parse_str ("x^24+34*x^12+45*x^3+9*x^18 +34*x^10+ 32*x^21");
  may_t x_str = may_set_str ("x"), x_val;
  for (i = 0; i < n; i++) {
    x_val = may_set_d (((double) rand()) / RAND_MAX);
    may_replace (w, x_str, x_val);
  }

  t = cputime () - t;
  may_kernel_prec (113);
  if (verbose >= 2)
    printf ("%dms\n", t);
  may_keep (NULL);
  return t;
}

static may_t
right (may_t (*f) (may_t), may_t a, may_t b, int n)
{
  may_t deltax = may_div (may_sub (b, a), may_set_ui (n));
  may_t c = a;
  may_t est = may_set_ui (0);
  int i;

  for (i = 0; i < n ; i++) {
    c = may_addinc_c (c, deltax);
    est = may_addinc_c (est, may_eval ((*f) (c)));
  }

  return may_eval (may_mul_c (est, deltax));
}


int
test_R8 (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("R8:right(x^2,0,5,%d) ...", n);
    fflush (stdout);
  }
  int t = cputime();
  may_t w = right (may_sqr_c, may_set_ui (0), may_set_ui (5), n);
  t = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  may_keep (NULL);
  return t;
}

int
test_S2 (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("S2:expand((x^sin(x) + y^cos(y) - z^(x+y))^%d) ...", n);
    fflush (stdout);
  }
  int t = cputime();

  may_t w = may_parse_str ("x^sin(x) + y^cos(y) - z^(x+y)");
  w = may_pow (w, may_set_ui (n));
  w = may_expand (w);

  t = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  may_keep (NULL);
  return t;
}

int
test_S3 (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("S3:diff(expand((x^y + y^z + z^x)^%d,x) ...", n);
    fflush (stdout);
  }
  int t = cputime();

  may_t w = may_parse_str ("x^y + y^z - z^x");
  w = may_pow (w, may_set_ui (n));
  w = may_expand (w);
  w = may_diff (w, may_set_str ("x"));

  t = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  may_keep (NULL);
  return t;
}

int
test_S4 (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("S4:series(sin(x)*cos(x),x=0,%d) ...", n);
    fflush (stdout);
  }
  int t = cputime();

  may_t w = may_parse_str ("sin(x)*cos(x)"), x = may_set_str ("x");
  w = may_series (w, x, n);

  t = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  may_keep (NULL);
  return t;
}

int
test_series_tan (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("series(tan(2+x),x=0,%d) ...", n);
    fflush (stdout);
  }
  int t = cputime();

  may_t w = may_parse_str ("tan(2+x)"), x = may_set_str ("x");
  w = may_series (w, x, n);

  t = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  may_keep (NULL);
  return t;
}

int
test_sum_rationalize (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("Rationalize sum((i*y*t^i)/(y+i*t)^i),i=1..%d ...", n);
    fflush (stdout);
  }
  int t1 = cputime();

  may_t s = may_set_ui(0);
  may_t y = may_set_str ("y");
  may_t t = may_set_str ("t");

  for (int i = 1; i <= n; i++) {
    may_t ii = may_set_ui (i);
    s = may_addinc_c (s, may_div_c (may_mul_vac (ii, y, may_pow_c (t, ii), NULL),
                                 may_pow_c  (may_add_c (y, may_mul_c (ii, t)), ii)));
  }
  s = may_eval (s);
  s = may_rationalize (s);

  t1 = cputime () - t1;
  if (verbose >= 2)
    printf ("%dms\n", t1);
  may_keep (NULL);
  return t1;
}

int
test_sum_rationalize2 (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("Rationalize sum((i*y*t^i)/(y+abs(5-i)*t)^i),i=1..%d ...", n);
    fflush (stdout);
  }
  int t1 = cputime();

  may_t s = may_set_ui(0);
  may_t y = may_set_str ("y");
  may_t t = may_set_str ("t");

  for (int i = 1; i <= n; i++) {
    may_t ii = may_set_ui (i);
    s = may_addinc_c (s, may_div_c (may_mul_vac (ii, y, may_pow_c (t, ii), NULL),
                                 may_pow_c  (may_add_c (y, may_mul_c (may_set_ui (abs(5-i)), t)), ii)));
  }
  s = may_eval (s);
  s = may_rationalize (s);

  t1 = cputime () - t1;
  if (verbose >= 2)
    printf ("%dms\n", t1);
  may_keep (NULL);
  return t1;
}

int
test_rationalize (int n)
{
  may_mark ();
  if (verbose >= 2) {
    printf ("Rationalize nested expression %d ...", n);
    fflush (stdout);
  }

  may_t w = may_parse_str ("1/(7+x)^2+1/(6+x)^2-((1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((7+x)*(6+x))+1/((7+x)*(8+x))-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))*(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((1+x)*(4+x))+1/((5+x)*(2+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((5+x)*(4+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))*(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((3+x)*(4+x))+1/((7+x)*(6+x))+1/((5+x)*(4+x)))*(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((6+x)*(4+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))*(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((1+x)*(4+x))+1/((5+x)*(2+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((7+x)*(5+x))+1/((8+x)*(6+x)))/(1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(2+x)^2+1/(4+x)^2))*(1/((5+x)*(3+x))+1/((9+x)*(7+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))*(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((3+x)*(4+x))+1/((7+x)*(6+x))+1/((5+x)*(4+x)))*(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((9+x)*(6+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))*(1/((5+x)*(1+x))+1/((7+x)*(3+x))+1/((6+x)*(2+x))+1/((8+x)*(4+x))+1/((9+x)*(5+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((5+x)*(2+x)))/(1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(2+x)^2+1/(4+x)^2)-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))*(1/((5+x)*(1+x))+1/((7+x)*(3+x))+1/((6+x)*(2+x))+1/((8+x)*(4+x))+1/((9+x)*(5+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((8+x)*(6+x)))/(1/(7+x)^2+1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))*(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((3+x)*(4+x))+1/((7+x)*(6+x))+1/((5+x)*(4+x)))^2/(1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(2+x)^2+1/(4+x)^2)-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(4+x)^2)+(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((1+x)*(4+x))+1/((5+x)*(2+x)))*(1/((5+x)*(1+x))+1/((7+x)*(3+x))+1/((6+x)*(2+x))+1/((8+x)*(4+x))+1/((9+x)*(5+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)-1/((5+x)*(6+x))+(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((9+x)*(6+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))*(1/((5+x)*(1+x))+1/((7+x)*(3+x))+1/((6+x)*(2+x))+1/((8+x)*(4+x))+1/((9+x)*(5+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((5+x)*(2+x)))*(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((6+x)*(4+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))*(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((1+x)*(4+x))+1/((5+x)*(2+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((7+x)*(5+x))+1/((8+x)*(6+x)))/(1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(2+x)^2+1/(4+x)^2)-1/((7+x)*(6+x))-1/((7+x)*(8+x))-1/((9+x)*(8+x))-1/((5+x)*(4+x)))^2/(1/(7+x)^2-(1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((7+x)*(6+x))+1/((7+x)*(8+x))-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))*(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((1+x)*(4+x))+1/((5+x)*(2+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((5+x)*(4+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))*(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((3+x)*(4+x))+1/((7+x)*(6+x))+1/((5+x)*(4+x)))*(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((6+x)*(4+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))*(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((1+x)*(4+x))+1/((5+x)*(2+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((7+x)*(5+x))+1/((8+x)*(6+x)))/(1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(2+x)^2+1/(4+x)^2))^2/(1/(7+x)^2+1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))*(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((3+x)*(4+x))+1/((7+x)*(6+x))+1/((5+x)*(4+x)))^2/(1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(2+x)^2+1/(4+x)^2)-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(4+x)^2)+1/(6+x)^2+1/(8+x)^2-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((6+x)*(4+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))*(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((1+x)*(4+x))+1/((5+x)*(2+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((7+x)*(5+x))+1/((8+x)*(6+x)))^2/(1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(2+x)^2+1/(4+x)^2)+1/(5+x)^2-(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((1+x)*(4+x))+1/((5+x)*(2+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(4+x)^2)+1/(8+x)^2-(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((9+x)*(6+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))*(1/((5+x)*(1+x))+1/((7+x)*(3+x))+1/((6+x)*(2+x))+1/((8+x)*(4+x))+1/((9+x)*(5+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((5+x)*(2+x)))^2/(1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(2+x)^2+1/(4+x)^2)+1/(5+x)^2-(1/((5+x)*(1+x))+1/((7+x)*(3+x))+1/((6+x)*(2+x))+1/((8+x)*(4+x))+1/((9+x)*(5+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)-(1/((5+x)*(3+x))+1/((9+x)*(7+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))*(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((3+x)*(4+x))+1/((7+x)*(6+x))+1/((5+x)*(4+x)))*(1/((7+x)*(4+x))+1/((5+x)*(8+x))+1/((3+x)*(6+x))+1/((9+x)*(6+x))-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))*(1/((5+x)*(1+x))+1/((7+x)*(3+x))+1/((6+x)*(2+x))+1/((8+x)*(4+x))+1/((9+x)*(5+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((5+x)*(2+x)))/(1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(2+x)^2+1/(4+x)^2)-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))*(1/((5+x)*(1+x))+1/((7+x)*(3+x))+1/((6+x)*(2+x))+1/((8+x)*(4+x))+1/((9+x)*(5+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((8+x)*(6+x)))^2/(1/(7+x)^2+1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))*(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/((3+x)*(4+x))+1/((7+x)*(6+x))+1/((5+x)*(4+x)))^2/(1/(6+x)^2+1/(3+x)^2+1/(5+x)^2-(1/((3+x)*(2+x))+1/((5+x)*(6+x))+1/((3+x)*(4+x))+1/((1+x)*(2+x))+1/((5+x)*(4+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(2+x)^2+1/(4+x)^2)-(1/((5+x)*(3+x))+1/((4+x)*(2+x))+1/((3+x)*(1+x))+1/((6+x)*(4+x))+1/((7+x)*(5+x)))^2/(1/(1+x)^2+1/(3+x)^2+1/(5+x)^2+1/(2+x)^2+1/(4+x)^2)+1/(4+x)^2)+1/(9+x)^2");
  int t = cputime();
  w = may_rationalize (w);

  t = cputime () - t;
  if (verbose >= 2)
    printf ("%dms\n", t);
  may_keep (NULL);
  return t;
}



/* First Fermat test.
   On a Pentium 4 1.6 Ghz, Fermat 3.5.6 takes 28.3 secs.
   On a Pentium M 1.7 Ghz, MAY doesn't do it. (200,000,000,000 terms...)
*/
int
test_fermat (void)
{
  may_t s1, s2, s3, s4, res, x, y;
  int t0, t1;

  if (verbose >= 2) {
    printf ("fermat test 1 ...");
    fflush (stdout);
  }

  t0 = cputime ();
  s1 = may_parse_str ("(l2*g*a22^3-l1*g*a12*a21*a22^2-l2*n22*a21*a22^2+l2*n11*a21*a22^2+2*l2*g*a12*a22^2-l1*g*a12*a22^2+l2*g^2*a22^2-l2*n11*n22*a22^2+l2*n11*a22^2+l1*n22*a12*a21^2*a22-l1*n11*a12*a21^2*a22-l2*g*a21^2*a22-l1*g*a12^2*a21*a22-l2*g^2*a12*a21*a22-l1*g^2*a12*a21*a22+l2*n11*n22*a12*a21*a22+l1*n11*n22*a12*a21*a22-2*l2*n22*a12*a21*a22+l1*n22*a12*a21*a22+l2*n11*a12*a21*a22-2*l1*n11*a12*a21*a22-l2*g*a21*a22+l2*g*a12^2*a22-l1*g*a12^2*a22+l2*g^2*a12*a22-l1*g^2*a12*a22-l2*n11*n22*a12*a22+l1*n11*n22*a12*a22+l2*n11*a12*a22-l1*n11*a12*a22+l1*g*a12*a21^3+l1*g^2*a12^2*a21^2-l1*n11*n22*a12^2*a21^2+l1*n22*a12^2*a21^2-l2*g*a12*a21^2+2*l1*g*a12*a21^2-l2*g^2*a12^2*a21+l1*g^2*a12^2*a21+l2*n11*n22*a12^2*a21-l1*n11*n22*a12^2*a21-l2*n22*a12^2*a21+l1*n22*a12^2*a21-l2*g*a12*a21+l1*g*a12*a21)/(l2*g*a12*a21*a22^2-l1*g*a12*a21*a22^2-l2*n22*a21*a22^2+l1*n22*a21*a22^2+l1*l2*g*a12*a22^2-l1*g*a12*a22^2-l1*l2*n22*a22^2+l1*n22*a22^2+l2*n11*a12*a21^2*a22-l1*n11*a12*a21^2*a22-l2*g*a21^2*a22+l1*g*a21^2*a22-l1*l2*g*a12^2*a21*a22+2*l2*g*a12^2*a21*a22-l1*g*a12^2*a21*a22+l1*l2*n22*a12*a21*a22-2*l2*n22*a12*a21*a22+l1*n22*a12*a21*a22+l1*l2*n11*a12*a21*a22+l2*n11*a12*a21*a22-2*l1*n11*a12*a21*a22-l1*l2*g*a21*a22-l2*g*a21*a22+2*l1*g*a21*a22+l1*l2*g*a12^2*a22-l1*g*a12^2*a22-l1*l2*n22*a12*a22+l1*n22*a12*a22+l1*l2*n11*a12*a22-l1*n11*a12*a22-l1*l2*g*a22+l1*g*a22-l1*l2*n11*a12^2*a21^2+l2*n11*a12^2*a21^2+l1*l2*g*a12*a21^2-l2*g*a12*a21^2-l1*l2*g*a12^3*a21+l2*g*a12^3*a21+l1*l2*n22*a12^2*a21-l2*n22*a12^2*a21-l1*l2*n11*a12^2*a21+l2*n11*a12^2*a21+l1*l2*g*a12*a21-l2*g*a12*a21)");
  if (may_nan_p (s1))
    abort ();

 s2 = may_parse_str ("(l2*g*a12*a22^2-l2*n22*a22^2+l2*a22^2-l1*g*a12^2*a21*a22+l1*n22*a12*a21*a22+l2*n11*a12*a21*a22-l2*a12*a21*a22-l1*a12*a21*a22-l2*g*a21*a22+l2*g*a12^2*a22-l1*g*a12^2*a22-l2*n22*a12*a22+l1*n22*a12*a22+l2*a12*a22-l1*a12*a22-l1*n11*a12^2*a21^2+l1*a12^2*a21^2+l1*g*a12*a21^2+l2*n11*a12^2*a21-l1*n11*a12^2*a21-l2*a12^2*a21+l1*a12^2*a21-l2*g*a12*a21+l1*g*a12*a21)/(l2*g*a12*a22^2-g*a12*a22^2-l2*n22*a22^2+n22*a22^2-l1*g*a12^2*a21*a22+g*a12^2*a21*a22+l1*n22*a12*a21*a22-n22*a12*a21*a22+l2*n11*a12*a21*a22-n11*a12*a21*a22-l2*g*a21*a22+g*a21*a22+l2*g*a12^2*a22-g*a12^2*a22-l2*n22*a12*a22+n22*a12*a22+l2*n11*a12*a22-n11*a12*a22-l2*g*a22+g*a22-l1*n11*a12^2*a21^2+n11*a12^2*a21^2+l1*g*a12*a21^2-g*a12*a21^2-l1*g*a12^3*a21+g*a12^3*a21+l1*n22*a12^2*a21-n22*a12^2*a21-l1*n11*a12^2*a21+n11*a12^2*a21+l1*g*a12*a21-g*a12*a21)");
  if (may_nan_p (s2))
    abort ();

  s3 = may_parse_str ("(l2*p21*a22^3-l1*p21*a12*a21*a22^2-l2*p22*a21*a22^2+l2*p11*a21*a22^2+2*l2*p21*a12*a22^2-l1*p21*a12*a22^2-l2*p11*p22*a22^2+l2*p12*p21*a22^2+l2*p11*a22^2+l1*p22*a12*a21^2*a22-l1*p11*a12*a21^2*a22-l2*p12*a21^2*a22-l1*p21*a12^2*a21*a22+l2*p11*p22*a12*a21*a22+l1*p11*p22*a12*a21*a22-2*l2*p22*a12*a21*a22+l1*p22*a12*a21*a22-l2*p12*p21*a12*a21*a22-l1*p12*p21*a12*a21*a22+l2*p11*a12*a21*a22-2*l1*p11*a12*a21*a22-l2*p12*a21*a22+l2*p21*a12^2*a22-l1*p21*a12^2*a22-l2*p11*p22*a12*a22+l1*p11*p22*a12*a22+l2*p12*p21*a12*a22-l1*p12*p21*a12*a22+l2*p11*a12*a22-l1*p11*a12*a22+l1*p12*a12*a21^3-l1*p11*p22*a12^2*a21^2+l1*p22*a12^2*a21^2+l1*p12*p21*a12^2*a21^2-l2*p12*a12*a21^2+2*l1*p12*a12*a21^2+l2*p11*p22*a12^2*a21-l1*p11*p22*a12^2*a21-l2*p22*a12^2*a21+l1*p22*a12^2*a21-l2*p12*p21*a12^2*a21+l1*p12*p21*a12^2*a21-l2*p12*a12*a21+l1*p12*a12*a21)/(l2*p21*a12*a21*a22^2-l1*p21*a12*a21*a22^2-l2*p22*a21*a22^2+l1*p22*a21*a22^2+l1*l2*p21*a12*a22^2-l1*p21*a12*a22^2-l1*l2*p22*a22^2+l1*p22*a22^2+l2*p11*a12*a21^2*a22-l1*p11*a12*a21^2*a22-l2*p12*a21^2*a22+l1*p12*a21^2*a22-l1*l2*p21*a12^2*a21*a22+2*l2*p21*a12^2*a21*a22-l1*p21*a12^2*a21*a22+l1*l2*p22*a12*a21*a22-2*l2*p22*a12*a21*a22+l1*p22*a12*a21*a22+l1*l2*p11*a12*a21*a22+l2*p11*a12*a21*a22-2*l1*p11*a12*a21*a22-l1*l2*p12*a21*a22-l2*p12*a21*a22+2*l1*p12*a21*a22+l1*l2*p21*a12^2*a22-l1*p21*a12^2*a22-l1*l2*p22*a12*a22+l1*p22*a12*a22+l1*l2*p11*a12*a22-l1*p11*a12*a22-l1*l2*p12*a22+l1*p12*a22-l1*l2*p11*a12^2*a21^2+l2*p11*a12^2*a21^2+l1*l2*p12*a12*a21^2-l2*p12*a12*a21^2-l1*l2*p21*a12^3*a21+l2*p21*a12^3*a21+l1*l2*p22*a12^2*a21-l2*p22*a12^2*a21-l1*l2*p11*a12^2*a21+l2*p11*a12^2*a21+l1*l2*p12*a12*a21-l2*p12*a12*a21)");
  if (may_nan_p (s3))
    abort ();

  s4 = may_parse_str ("(l2*p21*a12*a22^2-l2*p22*a22^2+l2*a22^2-l1*p21*a12^2*a21*a22+l1*p22*a12*a21*a22+l2*p11*a12*a21*a22-l2*a12*a21*a22-l1*a12*a21*a22-l2*p12*a21*a22+l2*p21*a12^2*a22-l1*p21*a12^2*a22-l2*p22*a12*a22+l1*p22*a12*a22+l2*a12*a22-l1*a12*a22-l1*p11*a12^2*a21^2+l1*a12^2*a21^2+l1*p12*a12*a21^2+l2*p11*a12^2*a21-l1*p11*a12^2*a21-l2*a12^2*a21+l1*a12^2*a21-l2*p12*a12*a21+l1*p12*a12*a21)/(l2*p21*a12*a22^2-p21*a12*a22^2-l2*p22*a22^2+p22*a22^2-l1*p21*a12^2*a21*a22+p21*a12^2*a21*a22+l1*p22*a12*a21*a22-p22*a12*a21*a22+l2*p11*a12*a21*a22-p11*a12*a21*a22-l2*p12*a21*a22+p12*a21*a22+l2*p21*a12^2*a22-p21*a12^2*a22-l2*p22*a12*a22+p22*a12*a22+l2*p11*a12*a22-p11*a12*a22-l2*p12*a22+p12*a22-l1*p11*a12^2*a21^2+p11*a12^2*a21^2+l1*p12*a12*a21^2-p12*a12*a21^2-l1*p21*a12^3*a21+p21*a12^3*a21+l1*p22*a12^2*a21-p22*a12^2*a21-l1*p11*a12^2*a21+p11*a12^2*a21+l1*p12*a12*a21-p12*a12*a21)");
  if (may_nan_p (s4))
    abort ();

  if (0) {
    s1 = may_parse_str ("n1/d1");
    s2 = may_parse_str ("n2/d2");
    s3 = may_parse_str ("n3/d3");
    s4 = may_parse_str ("n4/d4");
  }
  if (0) {
    s1 = may_parse_str ("(n11+n12)/(d12+d22)");
    s2 = may_parse_str ("(n21+n22)/(d21+d22)");
    s3 = may_parse_str ("(n31+n32)/(d31+d32)");
    s4 = may_parse_str ("(n41+n42)/(d41+d42)");
  }

  res = may_parse_str ("-l1*l2*p11*p22*q1^2*q4^2+l2*p11*p22*q1^2*q4^2+l1*p11*p22*q1^2*q4^2-p11*p22*q1^2*q4^2+l1*l2*p12*p21*q1^2*q4^2-l2*p12*p21*q1^2*q4^2-l1*p12*p21*q1^2*q4^2+p12*p21*q1^2*q4^2+l1*n22*p11*p22*q1*q4^2-n22*p11*p22*q1*q4^2+l2*n11*p11*p22*q1*q4^2-n11*p11*p22*q1*q4^2-l2*p11*p22*q1*q4^2-l1*p11*p22*q1*q4^2+2*p11*p22*q1*q4^2-l1*n22*p12*p21*q1*q4^2+n22*p12*p21*q1*q4^2-l2*n11*p12*p21*q1*q4^2+n11*p12*p21*q1*q4^2+l2*p12*p21*q1*q4^2+l1*p12*p21*q1*q4^2-2*p12*p21*q1*q4^2+g^2*p11*p22*q4^2-n11*n22*p11*p22*q4^2+n22*p11*p22*q4^2+n11*p11*p22*q4^2-p11*p22*q4^2-g^2*p12*p21*q4^2+n11*n22*p12*p21*q4^2-n22*p12*p21*q4^2-n11*p12*p21*q4^2+p12*p21*q4^2+l1*l2*n11*p22*q1*q2*q3*q4-l2*n11*p22*q1*q2*q3*q4-l1*n11*p22*q1*q2*q3*q4+n11*p22*q1*q2*q3*q4-l1*l2*g*p21*q1*q2*q3*q4+l2*g*p21*q1*q2*q3*q4+l1*g*p21*q1*q2*q3*q4-g*p21*q1*q2*q3*q4-l1*l2*g*p12*q1*q2*q3*q4+l2*g*p12*q1*q2*q3*q4+l1*g*p12*q1*q2*q3*q4-g*p12*q1*q2*q3*q4+l1*l2*n22*p11*q1*q2*q3*q4-l2*n22*p11*q1*q2*q3*q4-l1*n22*p11*q1*q2*q3*q4+n22*p11*q1*q2*q3*q4+l1*g^2*p22*q2*q3*q4-g^2*p22*q2*q3*q4-l1*n11*n22*p22*q2*q3*q4+n11*n22*p22*q2*q3*q4+l1*n11*p22*q2*q3*q4-n11*p22*q2*q3*q4-l2*g*p21*q2*q3*q4+g*p21*q2*q3*q4-l1*g*p12*q2*q3*q4+g*p12*q2*q3*q4+l2*g^2*p11*q2*q3*q4-g^2*p11*q2*q3*q4-l2*n11*n22*p11*q2*q3*q4+n11*n22*p11*q2*q3*q4+l2*n22*p11*q2*q3*q4-n22*p11*q2*q3*q4-l1*l2*n11*p22*q1*q3*q4+l1*n11*p22*q1*q3*q4+l1*l2*p22*q1*q3*q4-l1*p22*q1*q3*q4+l1*l2*g*p21*q1*q3*q4-l1*g*p21*q1*q3*q4+l1*l2*g*p12*q1*q3*q4-l2*g*p12*q1*q3*q4-l1*l2*n22*p11*q1*q3*q4+l2*n22*p11*q1*q3*q4+l1*l2*p11*q1*q3*q4-l2*p11*q1*q3*q4-l1*g^2*p22*q3*q4+l1*n11*n22*p22*q3*q4-l1*n22*p22*q3*q4-l1*n11*p22*q3*q4+l1*p22*q3*q4-l2*g^2*p11*q3*q4+l2*n11*n22*p11*q3*q4-l2*n22*p11*q3*q4-l2*n11*p11*q3*q4+l2*p11*q3*q4-l1*n22*p11*p22*q1*q2*q4+n22*p11*p22*q1*q2*q4-l2*n11*p11*p22*q1*q2*q4+n11*p11*p22*q1*q2*q4+l2*n11*p22*q1*q2*q4-n11*p22*q1*q2*q4+l1*n22*p12*p21*q1*q2*q4-n22*p12*p21*q1*q2*q4+l2*n11*p12*p21*q1*q2*q4-n11*p12*p21*q1*q2*q4-l1*g*p21*q1*q2*q4+g*p21*q1*q2*q4-l2*g*p12*q1*q2*q4+g*p12*q1*q2*q4+l1*n22*p11*q1*q2*q4-n22*p11*q1*q2*q4-2*g^2*p11*p22*q2*q4+2*n11*n22*p11*p22*q2*q4-n22*p11*p22*q2*q4-n11*p11*p22*q2*q4+g^2*p22*q2*q4-n11*n22*p22*q2*q4+n11*p22*q2*q4+2*g^2*p12*p21*q2*q4-2*n11*n22*p12*p21*q2*q4+n22*p12*p21*q2*q4+n11*p12*p21*q2*q4-g*p21*q2*q4-g*p12*q2*q4+g^2*p11*q2*q4-n11*n22*p11*q2*q4+n22*p11*q2*q4+2*l1*l2*p11*p22*q1^2*q4-l2*p11*p22*q1^2*q4-l1*p11*p22*q1^2*q4-l1*l2*p22*q1^2*q4+l1*p22*q1^2*q4-2*l1*l2*p12*p21*q1^2*q4+l2*p12*p21*q1^2*q4+l1*p12*p21*q1^2*q4-l1*l2*p11*q1^2*q4+l2*p11*q1^2*q4-l1*n22*p11*p22*q1*q4-l2*n11*p11*p22*q1*q4+l2*p11*p22*q1*q4+l1*p11*p22*q1*q4+l1*n22*p22*q1*q4-l1*p22*q1*q4+l1*n22*p12*p21*q1*q4+l2*n11*p12*p21*q1*q4-l2*p12*p21*q1*q4-l1*p12*p21*q1*q4+l1*g*p21*q1*q4+l2*g*p12*q1*q4+l2*n11*p11*q1*q4-l2*p11*q1*q4+l1*l2*g^2*q2^2*q3^2-l2*g^2*q2^2*q3^2-l1*g^2*q2^2*q3^2+g^2*q2^2*q3^2-l1*l2*n11*n22*q2^2*q3^2+l2*n11*n22*q2^2*q3^2+l1*n11*n22*q2^2*q3^2-n11*n22*q2^2*q3^2-2*l1*l2*g^2*q2*q3^2+l2*g^2*q2*q3^2+l1*g^2*q2*q3^2+2*l1*l2*n11*n22*q2*q3^2-l2*n11*n22*q2*q3^2-l1*n11*n22*q2*q3^2-l1*l2*n22*q2*q3^2+l1*n22*q2*q3^2-l1*l2*n11*q2*q3^2+l2*n11*q2*q3^2+l1*l2*g^2*q3^2-l1*l2*n11*n22*q3^2+l1*l2*n22*q3^2+l1*l2*n11*q3^2-l1*l2*q3^2-l1*g^2*p22*q2^2*q3+g^2*p22*q2^2*q3+l1*n11*n22*p22*q2^2*q3-n11*n22*p22*q2^2*q3-l2*g^2*p11*q2^2*q3+g^2*p11*q2^2*q3+l2*n11*n22*p11*q2^2*q3-n11*n22*p11*q2^2*q3+l2*g^2*q2^2*q3+l1*g^2*q2^2*q3-2*g^2*q2^2*q3-l2*n11*n22*q2^2*q3-l1*n11*n22*q2^2*q3+2*n11*n22*q2^2*q3-l1*l2*n11*p22*q1*q2*q3+l2*n11*p22*q1*q2*q3+l1*l2*g*p21*q1*q2*q3-l2*g*p21*q1*q2*q3+l1*l2*g*p12*q1*q2*q3-l1*g*p12*q1*q2*q3-l1*l2*n22*p11*q1*q2*q3+l1*n22*p11*q1*q2*q3+l1*l2*n22*q1*q2*q3-l1*n22*q1*q2*q3+l1*l2*n11*q1*q2*q3-l2*n11*q1*q2*q3+l1*g^2*p22*q2*q3-l1*n11*n22*p22*q2*q3+l1*n22*p22*q2*q3+l2*g*p21*q2*q3+l1*g*p12*q2*q3+l2*g^2*p11*q2*q3-l2*n11*n22*p11*q2*q3+l2*n11*p11*q2*q3-l2*g^2*q2*q3-l1*g^2*q2*q3+l2*n11*n22*q2*q3+l1*n11*n22*q2*q3-l1*n22*q2*q3-l2*n11*q2*q3+l1*l2*n11*p22*q1*q3-l1*l2*p22*q1*q3-l1*l2*g*p21*q1*q3-l1*l2*g*p12*q1*q3+l1*l2*n22*p11*q1*q3-l1*l2*p11*q1*q3-l1*l2*n22*q1*q3-l1*l2*n11*q1*q3+2*l1*l2*q1*q3+g^2*p11*p22*q2^2-n11*n22*p11*p22*q2^2-g^2*p22*q2^2+n11*n22*p22*q2^2-g^2*p12*p21*q2^2+n11*n22*p12*p21*q2^2-g^2*p11*q2^2+n11*n22*p11*q2^2+g^2*q2^2-n11*n22*q2^2+l1*n22*p11*p22*q1*q2+l2*n11*p11*p22*q1*q2-l1*n22*p22*q1*q2-l2*n11*p22*q1*q2-l1*n22*p12*p21*q1*q2-l2*n11*p12*p21*q1*q2-l1*n22*p11*q1*q2-l2*n11*p11*q1*q2+l1*n22*q1*q2+l2*n11*q1*q2-l1*l2*p11*p22*q1^2+l1*l2*p22*q1^2+l1*l2*p12*p21*q1^2+l1*l2*p11*q1^2-l1*l2*q1^2");
  if (may_nan_p (res))
    abort ();

  if (verbose >= 3) {
    printf ("%dms (parsing)...", cputime () - t0);
    fflush (stdout);
  }

  /* Now replace */
  const char *const name[] = {"q1", "q2", "q3", "q4"};
  const may_t value[] = {s1, s2, s3, s4};
  x = may_subs_c (res, 0, 4, name, (void*) value);
  x = may_eval (x);
  if (verbose >= 3) {
    printf ("%dms (replace+eval)...", cputime () - t0);
    fflush (stdout);
  }

  /* Now expand */
  may_comdenom (&x, &y, x);
  if (verbose >= 3) {
    printf ("%dms (comdenom)...", cputime () - t0);
    fflush (stdout);
  }
  x = may_expand (x);

  t1 = cputime () - t0;
  if (verbose >= 2) {
    printf ("%dms...\n", t1);
    fflush (stdout);
  }
  if (!may_zero_p (x)) {
    printf ("Test failed. Should be 0.\n");
    printf ("x=%s\n\n"
            "y=%s\n\n",
            may_get_string (NULL, 0, x), may_get_string (NULL, 0, y));
    exit (2);
  }

  return t1;
}


/* To be able to dump may_t under GDB */
static void (*const dump)(may_t) = may_dump;

int
main (int argc, const char *argv[])
{
  int t = 0;

  printf ("%s\n", may_get_version ());
  may_kernel_start (0, 0);
  if (argc >= 2)
    verbose = atoi (argv[1]);

  may_kernel_info (stdout, "Start");

  // Test eval
  t += test_construct ();
  t += test_eval (100);
  t += test_eval (1000);
  t += test_eval (10000);
  t += test_eval (100000);
  t += test_eval (1000000);
  t += test_sum_x (20000);
  t += test_critical (5000);
  t += test_addnum (20000);
  t += test_sin_pi6 (20000);
  t += test_complex (1000000);
  t += test_evalf (100000);

  // Test expand
  t += test_expand (500);
  t += test_expand2 (3, 16);
  t += test_expand2 (4, 20);
  t += test_expand3 (20);
  t += test_univariate_multiply (1, 2, 1000);
  t += test_univariate_multiply (17, 42, 600);
  t += test_power_field (65000);
  t += test_pow_multivariate (500);
  t += test_expand4 (50);
  t += test_expand5 (5);
  t += test_expand6 (65536);

  // Test divide
  t += test_divide ("(1+x)^1000+1", "(1-x)^500", "x");
  t += test_divide ("(1+x)^1000+1", "x^3-5*x+17", "x");
  t += test_divide ("(1+x+y^2)^50+1", "(1-x)^25+y", "x");
  t += test_divide ("(1+x+y^2)^25+1", "x^3*y-5*x*y^42+17*y+1", "x");
  t += test_divide ("(1+x+y^2)^50+1", "(1-x)^25+y", "{x,y}");
  t += test_divide ("(1+x+y^2)^25+1", "x^3*y-5*x*y^42+17*y+1", "{x,y}");

  // Test GCD
  t += test_gcd ( "(1+2*x)^200*(x^3+2*x^2+1)", "(1+2*x)^42*(x^3-2*x+42)");
  t += test_gcd ( "(1+2*x)^200*(x^3+2*x^2+1)", "(1+2*x)^42*(x^3-2*x+42)+1");
  t += test_gcd ( "(1+2*x+y)^100*(x^3+2*x^2*y+1)", "(1+2*x+y)^42*(x^3-2*x+42)");
  t += test_gcd ( "(x^2-3*x*y+y^2)^4*(3*x-7*y+2)^5", "(x^2-3*x*y+y^2)^3*(3*x-7*y-2)^6");
  t += test_gcd ( "(7*y*x^2*z^2-3*x*y*z+11*(x+1)*y^2+5*z+1)^4*(3*x-7*y+2*z-3)^6", "(7*y*x^2*z^2-3*x*y*z+11*(x+1)*y^2+5*z+1)^3*(3*x-7*y+2*z+3)^6");
  // t += test_gcd ( "(1+2*x+y)^42*(x^3+2*x^2*y+1)", "(1+2*x+y)^17*(y^3-2*x+42)+1");
  t += test_gcd ( "(x^2-y^2)*(a+b)^10", "(x-y)*(a-c)^10");
  t += test_gcd ("(x-y)^50+a", "(x+y)^50");
  t += test_gcd ("-2107-7967*x+19271*x^50+551*x^49-39300*x^48+23685*x^51-47193*x^61+22470*x^63-11981*x^64+4427*x^65+12796*x^66-11319*x^67-44213*x^68-18278*x^69-8897*x^70-15766*x^71+10258*x^72+13882*x^73-23195*x^74-8704*x^75+4815*x^76-13598*x^77+13217*x^78-16323*x^79-13612*x^80-14833*x^62+25634*x^52+17615*x^54+14299*x^55-19193*x^56-4719*x^57-51084*x^58-14141*x^59+22254*x^60+61350*x^53-7982*x^81-5434*x^82+16274*x^83+7039*x^84+4819*x^85+13739*x^86+7595*x^87+13020*x^88-1874*x^89-11233*x^90-24214*x^91-11353*x^92-2585*x^93+22786*x^94+18677*x^95+16977*x^96+1096*x^97-9941*x^98-5735*x^99-6630*x^100-11236*x^35+21918*x^34-7645*x^33+12569*x^32+44195*x^42+7111*x^41-5940*x^40+8721*x^39+8*x^38+3951*x^37-16955*x^36+8166*x^44+1255*x^43-5677*x^45+2889*x^46-8483*x^47-6954*x^31-21427*x^30-20019*x^29-13763*x^28-8902*x^27-8764*x^26+11613*x^12+1365*x^11-5613*x^10+7489*x^9+3585*x^8-11942*x^15+17166*x^18-12107*x^17-25122*x^16+17961*x^14-3205*x^13+13609*x^20-11190*x^24+3638*x^25+7883*x^23-6332*x^22+511*x^21+31863*x^19-2029*x^4+2383*x^3-4956*x^2+1663*x^7-6356*x^6+4493*x^5",
                 "-2401-3773*x-9484*x^50-4086*x^49-31296*x^48-21634*x^51+18525*x^61-1327*x^63+17211*x^64+6097*x^65+4392*x^66+5639*x^67+26737*x^68+5985*x^69-2186*x^70+3352*x^71+5962*x^72+5413*x^73+15009*x^74+7176*x^75+23806*x^76-860*x^77+16368*x^78+9044*x^79+4566*x^80-7929*x^62-13086*x^52-84580*x^54-17160*x^55-4502*x^56-56940*x^57+5464*x^58-25712*x^59-41935*x^60+2429*x^53+12607*x^81-4635*x^82+1625*x^83+11692*x^84-22514*x^85+14489*x^86+6490*x^87-6581*x^88+5116*x^89-9671*x^90-18639*x^91-1692*x^92-9911*x^93-3458*x^94+2083*x^95-10129*x^96+6425*x^97+986*x^98+1890*x^99+6205*x^100+4746*x^35+14296*x^34+33685*x^33+23430*x^32-20474*x^42+14725*x^41-39167*x^40+11129*x^39+34455*x^38+19625*x^37+6082*x^36+7384*x^44-9814*x^43+11853*x^45-23402*x^46+24318*x^47+10912*x^31+17976*x^30-11869*x^29+11588*x^28+11503*x^27+15005*x^26-11632*x^12-17281*x^11+4819*x^10-17526*x^9+1700*x^8-9352*x^15+5662*x^18-11328*x^17+9618*x^16-3703*x^14+9162*x^13-25581*x^20+13776*x^24-1141*x^25-22249*x^23-3267*x^22-2752*x^21+17790*x^19+13366*x^4-4768*x^3+1366*x^2+2927*x^7+5018*x^6-6514*x^5");
  t += test_gcd ("-1368+2517*x-62928*x^500+126728*x^499-139637*x^498+27580*x^50+22746*x^49-2531*x^48-14087*x^51-5358*x^61+46791*x^63-7435*x^64-40697*x^65+13424*x^66+30705*x^67+9285*x^68-11869*x^69+16191*x^70-17766*x^71-60286*x^72+12097*x^73+9105*x^74-31654*x^75+9021*x^76-53005*x^77+19426*x^78+53086*x^79-25232*x^80+12673*x^62-31816*x^52-21377*x^54+618*x^55+39344*x^56+4398*x^57-61128*x^58-463*x^59-8004*x^60+6777*x^53+896*x^81-14671*x^82-28762*x^83-35402*x^84-5453*x^85+40572*x^86+24269*x^87-44167*x^88-22203*x^89-50138*x^90-26643*x^91+19350*x^92+19349*x^93+40256*x^94+13649*x^95-47903*x^96-41079*x^97-5645*x^98-4755*x^99-11582*x^100-40612*x^35+2800*x^34+2779*x^33-7942*x^32-69752*x^426+94597*x^425+47154*x^424+2012*x^423+31277*x^422+33408*x^421+84807*x^420+28863*x^419+68957*x^418+68073*x^417+59170*x^416+90828*x^415-16413*x^414-47850*x^413+139517*x^412-127726*x^411+5282*x^410+22398*x^409+77991*x^408+17422*x^407+142163*x^406+61882*x^405+89785*x^404+94572*x^403-2414*x^402-56973*x^401-101640*x^400-149488*x^434-98144*x^433+5076*x^432+40468*x^431+136215*x^430+151022*x^429+29277*x^428-186747*x^427-11950*x^438+46996*x^437-87072*x^436+50978*x^435-53867*x^439+34216*x^465-34398*x^464+186429*x^463+115912*x^462+50371*x^461-94614*x^460-6364*x^459+208358*x^458-37065*x^457-95153*x^456+134795*x^455-4809*x^454-55455*x^453-122445*x^452+43069*x^451-72494*x^450+79409*x^449-68950*x^448-16066*x^447-50587*x^446-102418*x^445-74453*x^444+167227*x^443-43709*x^442+19063*x^441-33367*x^440+65259*x^476+43375*x^475-62053*x^474+57480*x^473+16073*x^472+196982*x^471+29802*x^470-49897*x^469-9948*x^468+67615*x^467-19190*x^466-7500*x^42+18420*x^41-23816*x^40-4876*x^39+7627*x^38+16788*x^37-6120*x^36+5817*x^44+26641*x^43-20495*x^45-3115*x^497-60587*x^496-201311*x^495-46792*x^494+31565*x^493+22845*x^492-40022*x^491-106106*x^490-69754*x^489+16080*x^488+66696*x^487+188430*x^486+143305*x^485+3480*x^484+19886*x^483-69468*x^482+54910*x^481-6406*x^480+26650*x^479+64640*x^478-11401*x^477-2757*x^46+3794*x^47+45223*x^266+94576*x^265+42457*x^264-45418*x^263+54839*x^262+62286*x^261-43253*x^260+74164*x^259-22360*x^258+367*x^267+87386*x^297+1033*x^293-34984*x^292+15107*x^291+29227*x^290-60676*x^289+37316*x^288+67439*x^287+28665*x^286-30395*x^285-12669*x^284-118959*x^283-76727*x^282+40578*x^281+12478*x^280+139518*x^279-64661*x^278-70425*x^277-51423*x^276-75396*x^275-69331*x^274+14651*x^273+65687*x^272-33259*x^271-5551*x^270+26596*x^269-48554*x^268+30609*x^31+13234*x^30-24923*x^29-18067*x^28-12113*x^27-23210*x^26+89410*x^323+102031*x^322+57892*x^321-20669*x^320+60640*x^319-1516*x^318+2552*x^317-34122*x^333-49281*x^332-16718*x^331+96104*x^330-15874*x^329-42731*x^328-33928*x^327-29381*x^326+25084*x^325-91237*x^324-11027*x^334+21081*x^345-6588*x^344-85155*x^343+81994*x^342-27868*x^341-18945*x^340+94970*x^339+74905*x^338+84612*x^337-2698*x^336+24174*x^335-961*x^371-62269*x^370+77449*x^369-8992*x^368-45317*x^367+50793*x^366+100850*x^365+49600*x^364+418*x^363-38448*x^362+111756*x^361-59212*x^360+83470*x^359+38710*x^358+41701*x^357+4635*x^356+34246*x^355-47782*x^354+63743*x^353+14401*x^352-2942*x^351-15056*x^350+909*x^349-42355*x^348-27510*x^347-125124*x^346-25863*x^373+80195*x^372+52262*x^316+149353*x^315-55819*x^314+103411*x^313+158668*x^312-959*x^311+41724*x^310+62321*x^309+40000*x^308-48054*x^307-43199*x^306+111954*x^305+10527*x^304-22207*x^303-44732*x^302-62568*x^301-57920*x^300-5505*x^299+4394*x^298-18533*x^296-3782*x^295-68145*x^294+31258*x^399+2445*x^398-23728*x^397-17216*x^396-63084*x^395-4385*x^394-95754*x^393+63983*x^392-53024*x^391-44658*x^390-17576*x^389-41740*x^388+6318*x^387+6705*x^386-84829*x^385+53323*x^384+853*x^383+21916*x^382+57967*x^381-2589*x^380+71462*x^379-52141*x^378-7581*x^377-107109*x^376+58294*x^375-9185*x^374+16162*x^145-45541*x^144-5390*x^143+16722*x^142-30871*x^141+8858*x^140-35953*x^139-64934*x^138-6993*x^137-47839*x^136+34376*x^135+1714*x^134-55912*x^133+60713*x^132-66513*x^131+3055*x^130-52112*x^129-16474*x^128+68069*x^127-20608*x^126+796*x^125+37824*x^124-61304*x^123-43055*x^122-17539*x^121+680*x^120-18792*x^12+22675*x^11+23056*x^10+12449*x^9+769*x^8-70461*x^156-13959*x^155-3582*x^154+38179*x^153+509*x^152+11510*x^151+21141*x^150-23384*x^149-39509*x^148+91242*x^147+2943*x^146+33565*x^177-16919*x^176+54064*x^175-48549*x^174+61080*x^173-7615*x^172-781*x^171+37221*x^170+67845*x^169-16675*x^168+48196*x^167+30887*x^166+17310*x^165-135726*x^164+67870*x^163+21560*x^162+19914*x^161+11556*x^160+70610*x^159-47532*x^158+25493*x^157-4417*x^15+20459*x^18+40952*x^17+29850*x^16-13653*x^14-41563*x^13-45800*x^119+61140*x^118+8669*x^117+21847*x^116-79064*x^115-12970*x^114+14648*x^113-14101*x^112-30609*x^111+34569*x^110+18287*x^109-12075*x^107+7847*x^106+22346*x^105-23549*x^104-3279*x^103+44478*x^102-1076*x^101-48244*x^20+6836*x^201+1668*x^200-48353*x^199-25269*x^198-75510*x^197+50290*x^196+75028*x^195+15221*x^194+82848*x^193-7631*x^192-80664*x^191-88334*x^190+18015*x^189+84791*x^188+46134*x^187-4242*x^186-17111*x^185-95674*x^184+54257*x^183-64039*x^182+35669*x^181+17814*x^180-75848*x^179-42986*x^178-27965*x^228-94680*x^227+63443*x^226-78487*x^225+8800*x^224-58462*x^223-85978*x^222-20545*x^221+38209*x^220+42553*x^219+62882*x^218-78547*x^217+73749*x^216-53425*x^215+11205*x^214+32703*x^213+116004*x^212+8114*x^211+788*x^210+15187*x^209-9315*x^208-80863*x^207+20671*x^206-56079*x^205-15767*x^204+31905*x^203+78466*x^202-12265*x^234-35331*x^233-91707*x^232+109440*x^231-19376*x^230+26171*x^229+30379*x^24+63028*x^252+9160*x^251-2817*x^250+49242*x^249-119901*x^248-153407*x^247-11239*x^246+23438*x^245+26296*x^244-22390*x^243-11429*x^242+25379*x^241-106807*x^240+12136*x^239+15332*x^238+39203*x^237+15547*x^236-20061*x^235-20118*x^257+19291*x^256+101761*x^255-3630*x^254-7337*x^253+13929*x^25+21267*x^23-5025*x^22-16945*x^21-27666*x^19-2191*x^4+11365*x^3+7985*x^2-3553*x^7-18692*x^6-11338*x^5-21056*x^108+6546*x^501-191302*x^502-79510*x^503-65806*x^504+8111*x^505+47722*x^506+78838*x^507+101061*x^508+63259*x^509-103564*x^510-74851*x^544-174966*x^546-109470*x^547+160718*x^548+28397*x^549+21235*x^550-103816*x^551+15599*x^552-149336*x^553-98258*x^545+28210*x^554-6175*x^555+23644*x^556+94624*x^557-89878*x^558-113748*x^559-49343*x^560+91912*x^561+34420*x^562-50036*x^563+8764*x^564+17462*x^565-10665*x^566+4433*x^567+42567*x^568-39896*x^511+34392*x^512-1252*x^513+87985*x^514+126695*x^515+22222*x^516-142289*x^517+20542*x^518+77779*x^519-199249*x^520+102289*x^521+51103*x^522-13558*x^524+33990*x^525-1402*x^526+52050*x^527+39174*x^528-17971*x^529+20449*x^530+47878*x^531-52137*x^532-2594*x^533-43235*x^534+13613*x^535-50267*x^536-79596*x^537-92795*x^538-32565*x^539+54227*x^540-71137*x^541+32592*x^542-20287*x^543+129714*x^523-11030*x^616+166341*x^617-89703*x^618+45481*x^619-36199*x^620+51414*x^621-99641*x^622-42368*x^623+94675*x^624+88105*x^625+62598*x^626+137212*x^627+73363*x^628+56976*x^629-8849*x^630-89311*x^631-52982*x^632+35761*x^633+84848*x^634-26340*x^635-5634*x^636+40135*x^637-17527*x^638-139991*x^639-66912*x^640+155655*x^641+13263*x^642-56872*x^643-199551*x^644-50946*x^645+15816*x^646-40485*x^647-2615*x^648+80392*x^649+32051*x^650-38389*x^651-111528*x^652+34080*x^653-72650*x^654+61913*x^655-70919*x^656-78982*x^657-63906*x^658+4987*x^659+85398*x^660+92332*x^661+10581*x^662-9507*x^608-8820*x^609-54207*x^610-5942*x^611+59155*x^612+65755*x^613+10436*x^614+15343*x^615+12258*x^585-11797*x^586-6093*x^587-254632*x^588-96200*x^589+36506*x^590+36557*x^591-94285*x^592-20095*x^593-26461*x^594+72331*x^595+95209*x^569+45876*x^570+69083*x^571+124047*x^572-38339*x^573+219104*x^574+35058*x^575+25981*x^576-11144*x^577+64388*x^578+114*x^579-19025*x^580+34757*x^581-3375*x^582-10633*x^583+94099*x^584-132267*x^596-100986*x^597+45027*x^598+19888*x^599+2646*x^600-103029*x^601-132542*x^602+25637*x^603+26933*x^604+50175*x^605-70515*x^606+3540*x^607+33620*x^663-104162*x^664-73999*x^665-94129*x^666+94415*x^667+31830*x^668-5005*x^669+27726*x^670+44480*x^671+54227*x^672+50650*x^673-36618*x^674+73854*x^675-83478*x^676+83430*x^677-12361*x^678+15746*x^679-67222*x^680+25389*x^681-59494*x^682+51009*x^683+5516*x^684+26886*x^685+39793*x^686-124334*x^687-104667*x^688-22802*x^689-59040*x^690-35282*x^691+20231*x^692-10146*x^693+10496*x^694-70312*x^695-23451*x^696+25991*x^697+55521*x^698-113503*x^699-134070*x^700-113543*x^701-137007*x^702+61726*x^703-33606*x^704+53903*x^705-40448*x^706+4894*x^707-107277*x^708-86139*x^709-68962*x^710-16212*x^711+6491*x^712-79377*x^713-27934*x^714-7561*x^715-67725*x^716+45970*x^717+216735*x^718-137176*x^719-83295*x^720+5746*x^721-48014*x^722+39883*x^723+60457*x^724+49097*x^725-3439*x^726-11151*x^727-2207*x^728+70984*x^729-16530*x^730-66203*x^731-13069*x^732+56823*x^733-28636*x^734+35635*x^735+7024*x^736-13647*x^737-141812*x^738+3551*x^739+40067*x^740+106430*x^741-60146*x^742-84142*x^743-56336*x^744-35018*x^745-28114*x^746+48357*x^747+83943*x^748-622*x^749-91288*x^750-60778*x^751+35656*x^752-29322*x^753+72416*x^754-33714*x^755-53150*x^756-61948*x^757-26773*x^758-14013*x^759+44150*x^760+102179*x^761-45084*x^762-21220*x^763-48465*x^764+76913*x^765-44733*x^766-14166*x^767+55934*x^768+51115*x^769+46692*x^770-50553*x^771+30072*x^772+84302*x^773+105576*x^774+15638*x^775-83937*x^776+57*x^777+50156*x^778+40385*x^779+1895*x^780+72976*x^781+98109*x^782-17711*x^783+36796*x^784-11557*x^785-53085*x^786+33967*x^787-43234*x^788-7853*x^789+80450*x^790+148118*x^791-13683*x^792-47086*x^793-45213*x^794+7230*x^795-35*x^796+64324*x^797-41719*x^798-3774*x^799-87888*x^800-2764*x^801+34199*x^802+28067*x^803+57152*x^804+6485*x^805-10129*x^806-52006*x^807-79247*x^808-31185*x^809+16696*x^810+49617*x^811-12422*x^812-113676*x^813-45421*x^814-21624*x^815+71790*x^816+85697*x^817-31184*x^818-19922*x^819-42000*x^820-64032*x^821-127714*x^822+75365*x^823+51420*x^824-35002*x^825+1706*x^826+33387*x^827-9716*x^828-19761*x^829-25702*x^830-37203*x^831+7663*x^832-75159*x^833+5565*x^834-9322*x^835-31941*x^836+21325*x^837-2845*x^838-34434*x^839-10368*x^840+2185*x^841-27934*x^842-16267*x^843-41163*x^844+25895*x^845+40393*x^846+3941*x^847-21991*x^848-6784*x^849+18050*x^850-90653*x^851+60321*x^852+8423*x^853+13572*x^854-156301*x^855-21806*x^856+35085*x^857-15085*x^858+54547*x^859-47800*x^860-5254*x^861-81781*x^862-29440*x^863+22184*x^864-46117*x^865-59070*x^866+17927*x^867+33855*x^868-3151*x^869-29846*x^870-65956*x^871+52289*x^872-35287*x^873+54512*x^874-83059*x^875-37459*x^876-2672*x^877-8033*x^878+35411*x^879+15932*x^880+17557*x^881-102416*x^882-758*x^883+33596*x^884+88484*x^885+12872*x^886+2572*x^887+46754*x^888+1808*x^889+39711*x^890-17346*x^891+93893*x^892+27086*x^893+41025*x^894+55423*x^895+10557*x^896+15809*x^897-41601*x^898+15383*x^899+56744*x^900+48292*x^901+12460*x^902+47575*x^903+7133*x^904-4963*x^905-36508*x^906+40056*x^907+24227*x^908+21353*x^909+27255*x^910+38360*x^911+17237*x^912+10553*x^913+85724*x^914+41962*x^915+63340*x^916-14715*x^917-15090*x^918-10060*x^919+55422*x^920+39387*x^921+18064*x^922+49105*x^923+30516*x^924+67754*x^925-39611*x^926-1869*x^927+17979*x^928+56851*x^929+26558*x^930+20527*x^931+24521*x^932-9309*x^933+8971*x^934+14831*x^935+68598*x^936+33539*x^937+4457*x^938+20848*x^939+14657*x^940-7254*x^941-41834*x^942+22774*x^943+18000*x^944+48189*x^945+6981*x^946+26315*x^947+1341*x^948-16146*x^949-27103*x^950+21402*x^951+38473*x^952+10482*x^953-23425*x^954+19125*x^955+3086*x^956+5820*x^957+27199*x^958+15683*x^959+14548*x^960+16778*x^961-21663*x^962-10322*x^963-56927*x^964+19710*x^965-16079*x^966+2508*x^967-9465*x^968+20640*x^969-10146*x^970-22758*x^971-12289*x^972-19208*x^973-37003*x^974+2892*x^975-17318*x^976-2994*x^977-10592*x^978-11231*x^979+20238*x^980+23234*x^981-2207*x^982-11822*x^983-15862*x^984+2933*x^985-7984*x^986-9333*x^987+9463*x^988+21888*x^989+5011*x^990+12439*x^991+2736*x^992-17243*x^993-22202*x^994+6028*x^995+16742*x^996+8153*x^997-8041*x^998-941*x^999+11*x^1000",
                 "-6336-11784*x+4932*x^500+50975*x^499+97099*x^498-15185*x^50-35458*x^49-3044*x^48+13767*x^51+19234*x^61-17894*x^63+18735*x^64-19974*x^65-74185*x^66+24966*x^67+20533*x^68-4341*x^69+7991*x^70+55079*x^71-50140*x^72-62180*x^73+39666*x^74+26552*x^75+427*x^76+35692*x^77-22108*x^78+1224*x^79-29889*x^80-7251*x^62+43*x^52+19785*x^54-25981*x^55-17850*x^56-8708*x^57-5910*x^58-17200*x^59+33737*x^60-1968*x^53-27426*x^81+76764*x^82+37352*x^83-7468*x^84-32889*x^85-2395*x^86+34476*x^87-30999*x^88+26660*x^89-23550*x^90-11040*x^91-3042*x^92+55160*x^93+34254*x^94-40638*x^95-21061*x^96-40800*x^97+5796*x^98+60663*x^99+50901*x^100+22063*x^35-31440*x^34-27849*x^33+7755*x^32+110306*x^426-115497*x^425+20415*x^424+67595*x^423+28928*x^422+42735*x^421+9196*x^420+65649*x^419+14523*x^418-122033*x^417-60312*x^416-80286*x^415-68129*x^414-41662*x^413-99035*x^412+90631*x^411+18885*x^410+20285*x^409+103847*x^408+49*x^407-78236*x^406+146563*x^405+51046*x^404+28921*x^403+8513*x^402+93930*x^401+57512*x^400-151248*x^434-32796*x^433-6320*x^432-13153*x^431+39244*x^430+1207*x^429-125494*x^428+75594*x^427+71100*x^438-104332*x^437+33349*x^436+17541*x^435-36707*x^439+3468*x^465-37426*x^464+4923*x^463-68389*x^462+37190*x^461-153141*x^460+9355*x^459+58415*x^458-26337*x^457-33589*x^456-60647*x^455-96122*x^454+3772*x^453+48878*x^452+47805*x^451-9815*x^450-19456*x^449+41357*x^448+109367*x^447-70859*x^446-26327*x^445-56764*x^444+112709*x^443+84028*x^442-114405*x^441+13469*x^440+75154*x^476-13917*x^475+89884*x^474+40702*x^473-102123*x^472-52269*x^471-54844*x^470-31354*x^469+35506*x^468-26591*x^467+40325*x^466+25113*x^42-38860*x^41-32253*x^40-18775*x^39+24889*x^38-3913*x^37+9886*x^36+25961*x^44+20718*x^43+53782*x^45+148743*x^497-71348*x^496+38970*x^495-69447*x^494+119334*x^493+209256*x^492+16894*x^491+17755*x^490-13091*x^489+61269*x^488+60877*x^487-34956*x^486-3555*x^485+7309*x^484+44127*x^483+2004*x^482-77975*x^481+37224*x^480-53310*x^479-99789*x^478-8866*x^477-26350*x^46+2664*x^47-28517*x^266-9030*x^265-56243*x^264+2532*x^263-148743*x^262-47669*x^261-2217*x^260+76152*x^259+33687*x^258-76494*x^267+27600*x^297-78630*x^293-68075*x^292-10407*x^291-40497*x^290-53532*x^289+22767*x^288-919*x^287-26600*x^286-19633*x^285+19684*x^284-11088*x^283-44447*x^282+29042*x^281+49415*x^280-23335*x^279+34497*x^278-7428*x^277-9528*x^276+2950*x^275-12454*x^274+80274*x^273-11356*x^272-8544*x^271+52315*x^270-86635*x^269-11482*x^268+18471*x^31+2029*x^30+6484*x^29+11951*x^28-17347*x^27+7705*x^26+15541*x^323-46446*x^322-86117*x^321-7060*x^320+9858*x^319-102444*x^318+112000*x^317+62657*x^333-21036*x^332-47391*x^331-25301*x^330+1843*x^329-329*x^328+46388*x^327-45259*x^326+95490*x^325+22935*x^324-79440*x^334+52306*x^345-56880*x^344+61293*x^343-74447*x^342-18731*x^341+25963*x^340+37668*x^339+10946*x^338-6400*x^337-56723*x^336+18902*x^335-6560*x^371+67845*x^370+30644*x^369-33743*x^368-93463*x^367-15362*x^366+15340*x^365+46588*x^364-33466*x^363+73487*x^362-54280*x^361-173369*x^360-28330*x^359-52960*x^358+30366*x^357+20899*x^356+59310*x^355+85864*x^354+36605*x^353+40710*x^352-51058*x^351+7116*x^350+37369*x^349+98835*x^348+80705*x^347+62567*x^346-678*x^373+18456*x^372-65734*x^316+32304*x^315-47716*x^314-157705*x^313-60347*x^312+35891*x^311-52771*x^310-54138*x^309-62416*x^308-18320*x^307+42911*x^306-51152*x^305+42328*x^304-64533*x^303+37605*x^302+30404*x^301+32050*x^300+76679*x^299-99895*x^298+59164*x^296-106932*x^295+106931*x^294+24565*x^399+101405*x^398+76345*x^397+34939*x^396+84599*x^395-129439*x^394+127572*x^393+27839*x^392+7132*x^391+58203*x^390-42361*x^389-9644*x^388+17227*x^387-90659*x^386+56714*x^385+19616*x^384-9827*x^383-49144*x^382+51479*x^381+548*x^380+101139*x^379-51395*x^378-9396*x^377+40716*x^376+15155*x^375-85129*x^374+45539*x^145-49187*x^144-84561*x^143+14981*x^142-78934*x^141+38549*x^140+46695*x^139+30137*x^138+18715*x^137+7978*x^136-105944*x^135-38644*x^134-25305*x^133+69515*x^132+49336*x^131+38646*x^130+10128*x^129+7578*x^128-45907*x^127-34515*x^126+745*x^125+51674*x^124-13931*x^123+47163*x^122-39146*x^121-38791*x^120+12741*x^12+2341*x^11-19627*x^10-21728*x^9+2501*x^8-62303*x^156+99627*x^155+43811*x^154-35273*x^153-25451*x^152-71627*x^151+14225*x^150+10629*x^149+101280*x^148+30343*x^147-107015*x^146+41606*x^177-5407*x^176+1429*x^175-36378*x^174+59320*x^173-47308*x^172+99354*x^171-31347*x^170-61573*x^169-11818*x^168-45715*x^167-58723*x^166+12914*x^165-56185*x^164-7108*x^163-84181*x^162+24633*x^161+94005*x^160-126837*x^159-1879*x^158-9439*x^157+21163*x^15-8494*x^18-25839*x^17-1561*x^16-5662*x^14+3974*x^13+44403*x^119-33107*x^118-21042*x^117+33494*x^116-17715*x^115-53687*x^114-13088*x^113+140*x^112-15245*x^111-33628*x^110+13219*x^109+40524*x^107+67247*x^106-8625*x^105-59664*x^104-49230*x^103-51057*x^102+51292*x^101-9917*x^20+6081*x^201+46891*x^200-47454*x^199-62696*x^198+41710*x^197+18905*x^196+46484*x^195+39763*x^194+5584*x^193-80122*x^192-68819*x^191-61225*x^190+65582*x^189-76743*x^188+31261*x^187+24627*x^186-43917*x^185+29632*x^184-5935*x^183-78409*x^182+54977*x^181+7914*x^180+18263*x^179+36344*x^178+17764*x^228+39636*x^227+72602*x^226-37408*x^225+24660*x^224-19506*x^223-57444*x^222-18625*x^221+75768*x^220-54505*x^219-1311*x^218-24631*x^217-66612*x^216-123582*x^215-91923*x^214+35544*x^213-42258*x^212-21479*x^211+78630*x^210-69830*x^209-68336*x^208-7103*x^207+21333*x^206+26090*x^205-2844*x^204+22078*x^203+89181*x^202+121555*x^234+87427*x^233+39975*x^232+19237*x^231-13255*x^230-37761*x^229-6025*x^24+58779*x^252+93840*x^251+16027*x^250+137937*x^249-70759*x^248+29186*x^247-32044*x^246-7642*x^245+41519*x^244+2409*x^243-51568*x^242-25984*x^241-60334*x^240+30556*x^239-7279*x^238-58845*x^237-15424*x^236-51954*x^235+63439*x^257+48527*x^256+66625*x^255-86121*x^254-16328*x^253+21835*x^25+20507*x^23+28454*x^22-11264*x^21-2287*x^19+5863*x^4+2059*x^3-6862*x^2+5251*x^7+17382*x^6+5646*x^5+53693*x^108-2220*x^501-178171*x^502+35562*x^503+86592*x^504+115360*x^505-21838*x^506-5983*x^507+133517*x^508-62778*x^509+14689*x^510+21219*x^544+78574*x^546+70049*x^547-2225*x^548+6810*x^549+1272*x^550+54800*x^551-15350*x^552+14022*x^553-14642*x^545-12293*x^554+34987*x^555-80945*x^556+65048*x^557+4396*x^558+5987*x^559+59996*x^560-25921*x^561-20674*x^562-73724*x^563-86051*x^564+57444*x^565+30358*x^566-47115*x^567+83676*x^568+32647*x^511-56107*x^512+160040*x^513+37815*x^514+73084*x^515+19498*x^516+80908*x^517-69505*x^518+57795*x^519+44871*x^520+117699*x^521-17791*x^522+77150*x^524-47800*x^525-88448*x^526-124619*x^527-162383*x^528+108698*x^529-72699*x^530+62382*x^531-17169*x^532-15565*x^533-75333*x^534-106966*x^535-37986*x^536+44573*x^537-36631*x^538+42208*x^539-5662*x^540+57347*x^541+90315*x^542+154755*x^543+5844*x^523+25893*x^616+6367*x^617-70773*x^618+32464*x^619-10751*x^620+9236*x^621-45776*x^622-85970*x^623+60399*x^624+13210*x^625+22899*x^626-101228*x^627+67283*x^628-49406*x^629-93221*x^630-17611*x^631-81861*x^632-80851*x^633-40863*x^634+101734*x^635+84815*x^636+93493*x^637-93707*x^638+60197*x^639+34746*x^640+71174*x^641+116940*x^642+19311*x^643-38383*x^644+27902*x^645-90481*x^646+35257*x^647+70607*x^648+62710*x^649+86763*x^650+37765*x^651-24756*x^652+79081*x^653-36572*x^654+71500*x^655+10004*x^656+10307*x^657-26116*x^658+40391*x^659+29403*x^660+48382*x^661+40371*x^662+17964*x^608-66405*x^609+84023*x^610+108021*x^611-57828*x^612+17896*x^613-30899*x^614+5573*x^615+80902*x^585+40394*x^586-95297*x^587-39918*x^588-21629*x^589-102865*x^590+18055*x^591+17661*x^592+70475*x^593+18799*x^594-62172*x^595+6542*x^569-63042*x^570-61611*x^571-804*x^572+30421*x^573-827*x^574+84135*x^575-93535*x^576-36838*x^577-31079*x^578-10732*x^579-152986*x^580+25187*x^581-78780*x^582-104612*x^583-82337*x^584-32116*x^596-18344*x^597+111809*x^598-3677*x^599-73528*x^600-74039*x^601+60655*x^602-7301*x^603-52559*x^604-50970*x^605-147067*x^606-87111*x^607+68281*x^663+8767*x^664+66218*x^665-1593*x^666+51951*x^667-42881*x^668-58854*x^669-41820*x^670+121478*x^671-29364*x^672+67316*x^673-108952*x^674-63036*x^675-63736*x^676-60490*x^677+26749*x^678+26778*x^679-6816*x^680-56263*x^681-103299*x^682+56220*x^683-56912*x^684+105806*x^685+18461*x^686-208*x^687-135502*x^688+40890*x^689-78405*x^690+6671*x^691+101024*x^692+20806*x^693-73654*x^694-4110*x^695+34849*x^696+83609*x^697-91702*x^698-10490*x^699-49417*x^700+72413*x^701-115808*x^702+67997*x^703-28999*x^704-15903*x^705-18810*x^706-64148*x^707-51673*x^708+71240*x^709+11167*x^710-25747*x^711+38696*x^712+33736*x^713-78299*x^714-72221*x^715+16201*x^716+102446*x^717-79882*x^718-64009*x^719-18865*x^720+20202*x^721-83188*x^722+13949*x^723-58788*x^724+27369*x^725-127822*x^726-47406*x^727-96352*x^728+77832*x^729-7570*x^730-68492*x^731-70303*x^732-33187*x^733+11249*x^734-19185*x^735-24464*x^736-27739*x^737-91427*x^738-2873*x^739+112169*x^740+52050*x^741+49872*x^742-27512*x^743-68687*x^744-21489*x^745+48460*x^746+25736*x^747+19330*x^748+36807*x^749-111154*x^750-40184*x^751-85849*x^752+30258*x^753+64448*x^754+18727*x^755+19609*x^756-101909*x^757-110682*x^758-56025*x^759+15190*x^760+60986*x^761-25770*x^762+1145*x^763-29242*x^764-40822*x^765+27055*x^766+69747*x^767-41493*x^768-38067*x^769+10638*x^770-12425*x^771+32665*x^772-50799*x^773-24851*x^774-37999*x^775+81972*x^776+13712*x^777+21142*x^778+80045*x^779+10989*x^780-18919*x^781+20331*x^782+1981*x^783+81220*x^784+93490*x^785+86203*x^786-6184*x^787-34669*x^788+2625*x^789+36488*x^790+98399*x^791+55871*x^792-12580*x^793+29981*x^794-11495*x^795-4120*x^796-11600*x^797+120764*x^798+27277*x^799-12511*x^800-22539*x^801+65689*x^802+59797*x^803-42505*x^804+54730*x^805+81002*x^806+122534*x^807-34284*x^808+35794*x^809+23785*x^810+93547*x^811+81437*x^812-75417*x^813-14919*x^814+92631*x^815+39042*x^816+41033*x^817+9437*x^818-2449*x^819-73120*x^820+21390*x^821-4992*x^822+113498*x^823-2845*x^824-66107*x^825-32548*x^826+1570*x^827+30833*x^828+12646*x^829-11235*x^830-50766*x^831+89265*x^832-43042*x^833-1536*x^834+77853*x^835-45779*x^836-22918*x^837-52773*x^838-6208*x^839-26906*x^840+88824*x^841-31306*x^842+47841*x^843-50295*x^844+10093*x^845-44660*x^846-14687*x^847+54329*x^848-76237*x^849-10938*x^850-31387*x^851+52803*x^852+52921*x^853-73013*x^854+61459*x^855-54150*x^856-14971*x^857-79759*x^858+57004*x^859-6337*x^860+33853*x^861-75884*x^862-29897*x^863-24566*x^864-33404*x^865+20385*x^866-10007*x^867+30032*x^868+25466*x^869-132998*x^870-7827*x^871-42954*x^872+74252*x^873-68670*x^874+37229*x^875-30501*x^876+39224*x^877-41751*x^878+40396*x^879-11037*x^880-33488*x^881-44421*x^882-23593*x^883+15648*x^884+49701*x^885-36712*x^886+45061*x^887-23462*x^888+35585*x^889-47581*x^890-24803*x^891+16116*x^892-15421*x^893-72965*x^894+8773*x^895+4501*x^896+21015*x^897-947*x^898-10723*x^899-4722*x^900-30308*x^901-25646*x^902+11311*x^903-7746*x^904+63211*x^905-56647*x^906-72333*x^907+11126*x^908+19959*x^909+15870*x^910-27557*x^911+24503*x^912+23184*x^913-51436*x^914-78049*x^915+20443*x^916+16088*x^917-1045*x^918+826*x^919-34245*x^920+24401*x^921-43788*x^922+46959*x^923-48763*x^924-10333*x^925-27312*x^926-4637*x^927+6597*x^928-16296*x^929+59310*x^930-4379*x^931+6115*x^932-45278*x^933+16605*x^934-35307*x^935-32384*x^936+43345*x^937-6473*x^938+18593*x^939-54643*x^940+30580*x^941-43187*x^942-37941*x^943+16887*x^944+34660*x^945+57995*x^946-39793*x^947-15020*x^948-28081*x^949+29576*x^950-20978*x^951-2490*x^952+39058*x^953+31933*x^954+23666*x^955-45171*x^956+13372*x^957+33063*x^958+24160*x^959+14389*x^960+1634*x^961-7927*x^962-28327*x^963+14751*x^964-23861*x^965+68400*x^966+11323*x^967+490*x^968-29450*x^969-19011*x^970-14395*x^971+19216*x^972+24061*x^973+34798*x^974+485*x^975-11013*x^976-17932*x^977+4632*x^978+6966*x^979+19961*x^980+16933*x^981+2852*x^982-5265*x^983-12204*x^984-8039*x^985+25412*x^986+12248*x^987-4338*x^988+6385*x^989+6283*x^990-20229*x^991-19051*x^992+1166*x^993+19521*x^994-1835*x^995-6702*x^996+483*x^997+5937*x^998-7163*x^999-924*x^1000");
  t += test_gcd ("3772-5709*x-28359*x^500+38352*x^499-18303*x^498+6501*x^50+10696*x^49-1676*x^48+16891*x^51-13974*x^61+2724*x^63+12680*x^64-21913*x^65-15210*x^66+55129*x^67+29145*x^68-6600*x^69-24329*x^70-51150*x^71+5490*x^72+55870*x^73+33147*x^74-3221*x^75-28446*x^76-51612*x^77+26380*x^78+23396*x^79-23223*x^80+9822*x^62-397*x^52-11382*x^54-46309*x^55-15076*x^56+6892*x^57+23052*x^58+11385*x^59+4256*x^60+36833*x^53-10501*x^81+43971*x^82+54351*x^83-19414*x^84-27134*x^85-21321*x^86-7194*x^87+13114*x^88-34251*x^89+8102*x^90+47750*x^91+28785*x^92+20345*x^93-54449*x^94-30618*x^95-22047*x^96+25330*x^97+21323*x^98-51229*x^99+19729*x^100+31419*x^35-4737*x^34+7355*x^33-2656*x^32+8843*x^426+47785*x^425+127478*x^424-65458*x^423+14777*x^422-93396*x^421+27831*x^420-9922*x^419+16810*x^418+144200*x^417-80526*x^416-4056*x^415-32827*x^414+49112*x^413-18552*x^412-13979*x^411+37676*x^410-64354*x^409-45623*x^408-53796*x^407-24610*x^406-3991*x^405-39627*x^404+25959*x^403-79866*x^402+53410*x^401+55172*x^400+67052*x^434+8049*x^433+3640*x^432-93987*x^431+16358*x^430+14985*x^429-20187*x^428+113128*x^427+3132*x^438+3831*x^437-5700*x^436-86528*x^435-9143*x^439+55924*x^465+70350*x^464+17611*x^463+19650*x^462-79863*x^461-84278*x^460+136906*x^459-24018*x^458+44776*x^457-43225*x^456+13730*x^455-66923*x^454+557*x^453+43161*x^452-73774*x^451-109307*x^450+61123*x^449+90363*x^448+98922*x^447-80633*x^446-254040*x^445-97756*x^444+5701*x^443+67470*x^442-68553*x^441+119231*x^440+124811*x^476+44511*x^475+53616*x^474+41041*x^473+4456*x^472+107588*x^471-122749*x^470+55230*x^469-1297*x^468-35117*x^467+11114*x^466-8983*x^42-11404*x^41+13251*x^40+19727*x^39-27929*x^38-44657*x^37+15832*x^36+11783*x^44-6278*x^43+6518*x^45+48134*x^497+79525*x^496-15300*x^495+86888*x^494-43997*x^493-27314*x^492-89871*x^491-44102*x^490-74716*x^489+34059*x^488+30999*x^487+113820*x^486-32060*x^485-3682*x^484+4103*x^483-55391*x^482+8341*x^481-49165*x^480-2255*x^479-118791*x^478-26432*x^477-3074*x^46-24293*x^47+9203*x^1004+108531*x^1006-170733*x^1007+137815*x^1008-34901*x^1009+71957*x^1010+6609*x^1011+258825*x^1012-17820*x^1013-264807*x^1014+45603*x^1015-4574*x^1005-1531*x^266-59518*x^265-19012*x^264+37042*x^263+3240*x^262+39299*x^261-55226*x^260+6422*x^259-10560*x^258+572*x^267+6719*x^297+51112*x^293+3709*x^292-34008*x^291-52713*x^290-121445*x^289-44027*x^288-5940*x^287+90909*x^286+21978*x^285+4080*x^284-38973*x^283+112409*x^282-38524*x^281-23809*x^280-2037*x^279+22270*x^278+4409*x^277-60532*x^276-70052*x^275-53699*x^274+154594*x^273+48683*x^272+41489*x^271+1878*x^270-43757*x^269+39645*x^268+15419*x^31-16192*x^30-21601*x^29+1534*x^28-7519*x^27+14238*x^26-39941*x^323-31780*x^322-22881*x^321-2198*x^320-54769*x^319+50104*x^318-914*x^317-158477*x^1001+39083*x^1002-26063*x^1003-34432*x^333+5983*x^332+70101*x^331+20318*x^330+15141*x^329+39636*x^328+18292*x^327-29947*x^326+79278*x^325-36376*x^324-4022*x^334-136587*x^345-47677*x^344+21486*x^343-43856*x^342+89452*x^341+62819*x^340+22127*x^339-24183*x^338-24931*x^337-37297*x^336-22717*x^335+24158*x^371+61743*x^370-630*x^369+3383*x^368-19622*x^367+61229*x^366+44875*x^365-19515*x^364+27991*x^363-12830*x^362-18460*x^361-76651*x^360-6611*x^359-65339*x^358+10309*x^357+47377*x^356+84508*x^355-12478*x^354-64497*x^353+17316*x^352+32521*x^351-87943*x^350-127444*x^349+7935*x^348-40913*x^347+177*x^346-9800*x^373+2023*x^372+19512*x^316+11931*x^315-84661*x^314-62540*x^313-4764*x^312-10806*x^311-16306*x^310-9342*x^309+63974*x^308-23053*x^307+113676*x^306-43558*x^305-59091*x^304-70289*x^303-40940*x^302-36946*x^301+85407*x^300+8680*x^299-41616*x^298-3743*x^296+31165*x^295-1565*x^294-74282*x^399+29337*x^398-15980*x^397-49629*x^396+133307*x^395-7518*x^394-50435*x^393-23113*x^392+54828*x^391-17768*x^390-2277*x^389+724*x^388+69633*x^387-25669*x^386+171*x^385-104710*x^384+70340*x^383-83334*x^382-43544*x^381+174825*x^380-58610*x^379+167228*x^378-42215*x^377+56474*x^376+48887*x^375+12706*x^374-8167*x^145-66621*x^144+6152*x^143-4165*x^142-47645*x^141+39451*x^140-2214*x^139-12613*x^138+41137*x^137+14914*x^136+25127*x^135-16150*x^134-76923*x^133-27807*x^132+5674*x^131+75168*x^130-15446*x^129+5500*x^128+11130*x^127-62332*x^126-42856*x^125+5621*x^124-13655*x^123+62107*x^122+8349*x^121+20309*x^120+7130*x^12+20434*x^11+7337*x^10-8977*x^9-13452*x^8-68173*x^1141+47372*x^1142-231942*x^1143+45359*x^1144-157898*x^1145+65560*x^1146-148902*x^1147+33401*x^1148+114773*x^1149+74585*x^1150-93515*x^1151-95988*x^1152-85746*x^1153-31449*x^1154+157471*x^1155-8780*x^156-52122*x^155+17879*x^154+20354*x^153+58090*x^152-65204*x^151+22676*x^150+54725*x^149+15021*x^148-23181*x^147-20140*x^146-42410*x^1129+71619*x^1131-195565*x^1132+142818*x^1133-93833*x^1134-150829*x^1135+228298*x^1136-87939*x^1137-31006*x^1138+150076*x^1139-185060*x^1140-51029*x^1130+79635*x^177+56552*x^176+67995*x^175+10042*x^174-80454*x^173-53102*x^172+25053*x^171+2431*x^170+72879*x^169+91588*x^168-51685*x^167-3226*x^166-33678*x^165-60220*x^164+4336*x^163+17958*x^162+8478*x^161+39694*x^160+60346*x^159-15283*x^158-6787*x^157+10696*x^1120+149020*x^1122+29870*x^1123-142446*x^1124-80538*x^1125-6936*x^1126+90377*x^1127+52673*x^1128-5889*x^1121-3779*x^15-11408*x^1104+97852*x^1106+260701*x^1107+1442*x^1108+49634*x^1109-47931*x^1110+68848*x^1111-116047*x^1112-1581*x^1113+183424*x^1114+22708*x^1115+6439*x^1116-107011*x^1117-145565*x^1118+20132*x^1119-89669*x^1105-535*x^18+15016*x^17-6413*x^16-9199*x^14+3484*x^13-2990*x^1088+40382*x^1090-20652*x^1091+152655*x^1092+46400*x^1093-183303*x^1094+82692*x^1095-110453*x^1096+118708*x^1097-16580*x^1098+53882*x^1099-5461*x^1100+79320*x^1101+159818*x^1102-7067*x^1103-70122*x^1089-221371*x^1079+15087*x^1081-39068*x^1082-41962*x^1083-63552*x^1084-34364*x^1085+18261*x^1086+120837*x^1087-97570*x^1080+36867*x^119-31538*x^118-7032*x^117-74049*x^116+62052*x^115+55579*x^114+41445*x^113+379*x^112-47600*x^111+4584*x^110-25869*x^109+9167*x^107-4171*x^106+38904*x^105-26747*x^104-16900*x^103+8690*x^102+47003*x^101+6361*x^1064-53665*x^1066+57646*x^1067+94036*x^1068-70379*x^1069-133237*x^1070+31827*x^1071+88796*x^1072-47844*x^1073+35790*x^1074-69277*x^1075-91352*x^1076-81658*x^1077+25860*x^1078-87341*x^1065+7773*x^20+80313*x^1052-102235*x^1054-60705*x^1055+30655*x^1056-167319*x^1057+14090*x^1058-112744*x^1059+34068*x^1060-102207*x^1061+173533*x^1062+89156*x^1063-16638*x^1053+37995*x^201-33773*x^200+23415*x^199+64552*x^198-13113*x^197+34086*x^196-49503*x^195-22589*x^194-39747*x^193+23788*x^192+51879*x^191-29777*x^190+53904*x^189-16915*x^188-24947*x^187+17642*x^186+62668*x^185+14565*x^184+35660*x^183-46250*x^182-24078*x^181-17278*x^180-22247*x^179-57012*x^178-25559*x^1040-64643*x^1042+104681*x^1043+41973*x^1044-25866*x^1045+49961*x^1046-44259*x^1047+61174*x^1048-199351*x^1049+123733*x^1050+34746*x^1051-42741*x^1041-38917*x^228-40209*x^227+32318*x^226-57064*x^225+89327*x^224+65815*x^223-8175*x^222-67170*x^221+21236*x^220-77565*x^219-14414*x^218+40881*x^217+56568*x^216+15071*x^215+66634*x^214-156090*x^213-69952*x^212+118206*x^211+40694*x^210-59214*x^209+26797*x^208+43366*x^207-12466*x^206-24958*x^205+11469*x^204+12296*x^203-30047*x^202-46990*x^234-1411*x^233+20672*x^232+40589*x^231-9248*x^230+12722*x^229-133915*x^1030+105715*x^1032+17767*x^1033-76702*x^1034-65803*x^1035+20026*x^1036+275795*x^1037+125281*x^1038+4661*x^1039+22975*x^1031-16386*x^1016+188613*x^1018+94084*x^1019+3577*x^1020+89906*x^1021+90588*x^1022-4886*x^1023-153387*x^1024-40475*x^1025-63520*x^1026+91394*x^1027+114707*x^1028-82262*x^1029+56865*x^1017-24513*x^24-21640*x^252+8755*x^251-19887*x^250-28650*x^249+56761*x^248-16846*x^247+54834*x^246+71903*x^245-16471*x^244+34998*x^243-59454*x^242-9955*x^241-17793*x^240+25127*x^239+127293*x^238-29974*x^237+41413*x^236-35631*x^235-14937*x^257-9678*x^256-28865*x^255+24028*x^254-32150*x^253+20151*x^25-4361*x^23-5432*x^22+31181*x^21-28282*x^19-35034*x^1182+23575*x^1184-58263*x^1185+91496*x^1186+31196*x^1187+62859*x^1188-84164*x^1189-83652*x^1190-86346*x^1191+24041*x^1192+28363*x^1193+104292*x^1194+89002*x^1195+80874*x^1183+22587*x^1168+111641*x^1170+69869*x^1171-26287*x^1172+236866*x^1173-55009*x^1174+17663*x^1175+8311*x^1176+90051*x^1177-34843*x^1178-123177*x^1179-232628*x^1180-110058*x^1181+52497*x^1169-1970*x^1156+73512*x^1158-1709*x^1159-22672*x^1160+9068*x^1161-134160*x^1162-49441*x^1163-641*x^1164-127079*x^1165+158356*x^1166+15277*x^1167+113834*x^1157+46820*x^1235+22165*x^1237+33410*x^1238-183695*x^1239+59870*x^1240+89077*x^1241-4512*x^1242-63740*x^1243+69803*x^1244-74478*x^1245-35296*x^1246-275*x^1236+105466*x^1221+158175*x^1223-87286*x^1224+98839*x^1225-68694*x^1226+20835*x^1227-14507*x^1228-93623*x^1229-78118*x^1230-137301*x^1231-13679*x^1232-26095*x^1233+182238*x^1234+162966*x^1222-412*x^4+8978*x^3-1056*x^2+130409*x^1206+6417*x^1208-32230*x^1209-68803*x^1210-40592*x^1211+47040*x^1212+45496*x^1213-42936*x^1214-42339*x^1215+5492*x^1216-205850*x^1217-182585*x^1218+66375*x^1219-40473*x^1220+40293*x^1207-7238*x^7+3095*x^6+7061*x^5+3921*x^1196+204569*x^1198-3557*x^1199+30700*x^1200-49739*x^1201+5835*x^1202-89834*x^1203-37423*x^1204-84361*x^1205+161127*x^1197+70187*x^1271-104520*x^1273+31874*x^1274-226320*x^1275+113582*x^1276-115284*x^1277-65331*x^1278-66044*x^1279-54202*x^1280-89261*x^1281+99789*x^1282+13044*x^1272-136776*x^1257-165020*x^1259+17338*x^1260+31469*x^1261+130115*x^1262+168607*x^1263-90990*x^1264+17710*x^1265-158760*x^1266+34429*x^1267-28892*x^1268-2794*x^1269+123963*x^1270-13224*x^1258-43382*x^1247+71433*x^1249-15273*x^1250-50176*x^1251-20783*x^1252+24421*x^1253-2529*x^1254-34380*x^1255-20344*x^1256-21540*x^1248-132039*x^1305+146657*x^1307+119477*x^1308+1277*x^1309-90402*x^1310-95129*x^1311+93626*x^1312+160936*x^1313-75528*x^1314+82322*x^1315+17779*x^1316-103487*x^1306-74597*x^1293-98702*x^1294+58925*x^1295-34416*x^1296-42156*x^1297+124021*x^1298-6050*x^1299-78639*x^1300+107382*x^1301+20580*x^1302-119679*x^1303+41592*x^1304+53357*x^1283+75555*x^1285-46694*x^1286+88795*x^1287+1239*x^1288+20114*x^1289+1581*x^1290+67645*x^1291+238132*x^1292-72176*x^1284-20257*x^1359-74666*x^1360+2604*x^1361+26129*x^1362-35169*x^1363-132287*x^1364-20883*x^1365-3574*x^1366+30404*x^1367+149474*x^1343-138626*x^1345-10849*x^1346-81161*x^1347-148896*x^1348-21933*x^1349+39840*x^1350-177908*x^1351+44600*x^1352+73194*x^1353-85260*x^1354+34269*x^1355+123114*x^1356+21489*x^1357-815*x^1358-1077*x^1344+8158*x^108+31988*x^1329-20285*x^1331-8986*x^1332+59630*x^1333+107700*x^1334+56421*x^1335+9758*x^1336+172644*x^1337-134293*x^1338+14173*x^1339+18720*x^1340-61811*x^1341-65987*x^1342-65503*x^1330+12462*x^1317-184048*x^1319+32965*x^1320-46819*x^1321-209*x^1322-2948*x^1323+101885*x^1324-41379*x^1325-66364*x^1326+107942*x^1327+37094*x^1328+148505*x^1318-80984*x^1368+32475*x^1370+5078*x^1371+21709*x^1372-83960*x^1373-191966*x^1374+48705*x^1375-20940*x^1376+23082*x^1377-6121*x^1378+8872*x^1379-91005*x^1369-119664*x^1380+58402*x^1382-63857*x^1383-87352*x^1384-107980*x^1385+15885*x^1386+123831*x^1387+135606*x^1388-38873*x^1389-4685*x^1390-47037*x^1391-7065*x^1392-15583*x^1393+143237*x^1394+68669*x^1395-69730*x^1381+47201*x^1424-138409*x^1425+103097*x^1426+131274*x^1427+22354*x^1428+175706*x^1429+115585*x^1430+20221*x^1431-54607*x^1432+18800*x^1433-51821*x^1414-74583*x^1416+9799*x^1417+105575*x^1418+5408*x^1419+16855*x^1420+52172*x^1421+69694*x^1422+72953*x^1423+8779*x^1415+47181*x^1404-53993*x^1406+16590*x^1407-10556*x^1408-87898*x^1409+205365*x^1410-22463*x^1411+56012*x^1412-155635*x^1413+56690*x^1405-104882*x^1396-63765*x^1397+26661*x^1398+88515*x^1399-41012*x^1400+163752*x^1401-41618*x^1402-25399*x^1403+73255*x^1443-64409*x^1444-4721*x^1445-126264*x^1446+25646*x^1447-59289*x^1448-137530*x^1449+36910*x^1450-67417*x^1451+20221*x^1452-85250*x^1434-14300*x^1436-97086*x^1437+60371*x^1438+23171*x^1439+51501*x^1440+92476*x^1441+16275*x^1442-52931*x^1435+11295*x^1462+37647*x^1463+75218*x^1464+22232*x^1465+91136*x^1466-36028*x^1467-62348*x^1468+48546*x^1469-135732*x^1470-17265*x^1471+25902*x^1472+9253*x^1473-11738*x^1453+68705*x^1454-119140*x^1455+70981*x^1456+38984*x^1457+139660*x^1458-65507*x^1459-124315*x^1460-28742*x^1461+221881*x^1474+10933*x^1475-67032*x^1476+28173*x^1477+2923*x^1478-182384*x^1479+93126*x^1480+98952*x^1481-38335*x^1482-72046*x^1483-93583*x^1484-99644*x^1485-118124*x^1486-30826*x^1487-57534*x^1488-32657*x^1497+97119*x^1498-67139*x^1499-59119*x^1500-126920*x^1501+60643*x^1502+29259*x^1503-8296*x^1504-4821*x^1505+81299*x^1506+33438*x^1507-7588*x^1508-7028*x^1509-130809*x^1510+52915*x^1524+53495*x^1525+68308*x^1526-43206*x^1527+118892*x^1528-102939*x^1529-63495*x^1530+33111*x^1531+46014*x^1532-934*x^1533-53658*x^1534-84847*x^1535-86881*x^1536+148256*x^1537+15667*x^1538-117299*x^1511-41896*x^1513-72493*x^1514+65773*x^1515+205958*x^1516+121818*x^1517+48066*x^1518-561*x^1519-158643*x^1520+122624*x^1521+48701*x^1522-91012*x^1523-110982*x^1512-177866*x^1550+48176*x^1551-43624*x^1552+108213*x^1553-46901*x^1554+19235*x^1555+104*x^1556+15254*x^1557-95919*x^1539-141033*x^1540+35704*x^1541+19697*x^1542-46414*x^1543+139383*x^1544-202970*x^1545+118759*x^1546+129855*x^1547-12160*x^1548+90563*x^1549-7266*x^1588+46806*x^1589-76671*x^1590-1028*x^1591+19483*x^1592+40235*x^1593-99343*x^1594+21790*x^1595-166246*x^1596-100259*x^1597+69487*x^1598-67815*x^1573+22733*x^1574-117028*x^1575-12928*x^1576+2752*x^1577+52008*x^1578+66525*x^1579-23187*x^1580-103187*x^1581+39820*x^1582+54489*x^1583+68045*x^1584-42373*x^1585+10419*x^1586-108065*x^1587+32796*x^1558-63441*x^1559+143538*x^1560+33004*x^1561-2428*x^1562-13757*x^1563-31317*x^1564+128779*x^1565+3796*x^1566+34595*x^1567+56126*x^1568+58421*x^1569+48601*x^1570-16130*x^1571+55968*x^1572-15098*x^1599+12412*x^1600-42843*x^1601-90294*x^1602-6359*x^1603-52991*x^1604+5909*x^1605-47552*x^1606+51224*x^1607+35545*x^1608+38229*x^1609-40053*x^1610-29634*x^1611+6684*x^1612-93724*x^1637+71081*x^1638-70221*x^1639+28279*x^1640+9509*x^1641+55804*x^1642+57538*x^1643+68610*x^1644-11580*x^1645-21892*x^1646+633*x^1647+66105*x^1648-16366*x^1649-102202*x^1650+43656*x^1651-89787*x^1629-37155*x^1630-58225*x^1631+11238*x^1632-13398*x^1633+19155*x^1634+24376*x^1635-140638*x^1636+97953*x^1613-101281*x^1615-29172*x^1616-71069*x^1617+95174*x^1618+45171*x^1619+37926*x^1620-70713*x^1621-43688*x^1622+114370*x^1623-64258*x^1624+57254*x^1625-63693*x^1626-56855*x^1627-136634*x^1628+25408*x^1614+67920*x^1652+19256*x^1653-46277*x^1654-24689*x^1655+45677*x^1656+16433*x^1657-13950*x^1658+72472*x^1659+58799*x^1660-9785*x^501-5888*x^502+61389*x^503-25197*x^504-4004*x^505+29249*x^506-134638*x^507-41832*x^508-495*x^509-10492*x^510-113620*x^544-132458*x^546-163749*x^547+24663*x^548-61149*x^549-18439*x^550-10290*x^551+30028*x^552-97253*x^553+57036*x^545+26531*x^554-22914*x^555+63081*x^556-84444*x^557+78863*x^558-116701*x^559-20662*x^560+111802*x^561-13353*x^562-85759*x^563-75560*x^564+36369*x^565+32091*x^566+99921*x^567+134559*x^568+144626*x^511+118178*x^512+210*x^513+66320*x^514-13267*x^515-16965*x^516-62879*x^517-21432*x^518-36987*x^519-16464*x^520+69277*x^521-59763*x^522-90288*x^524+8192*x^525+179324*x^526+53175*x^527-122367*x^528-104461*x^529+106693*x^530-77530*x^531+22490*x^532+73917*x^533+52691*x^534-35800*x^535+12140*x^536-136683*x^537-88515*x^538+40345*x^539-195370*x^540+38999*x^541+14788*x^542+98350*x^543+111088*x^523+8653*x^1661+54098*x^1662-34384*x^1663-40183*x^1664+27882*x^1665+6357*x^1666+55138*x^1667+70684*x^1668+16621*x^1669-94412*x^1670+23915*x^1671+89634*x^1672+40753*x^1673+132760*x^1674-2284*x^1675-13095*x^616-28455*x^617+189915*x^618-65613*x^619+158005*x^620-64699*x^621+82588*x^622+28754*x^623-20416*x^624+44882*x^625+90691*x^626-42986*x^627+2520*x^628+191002*x^629+78819*x^630+68477*x^631+91962*x^632-78339*x^633-1356*x^634+1800*x^635+38498*x^636-28458*x^637-70594*x^638-63912*x^639-98960*x^640-6374*x^641-17755*x^642-54124*x^643+134627*x^644-45988*x^645+50302*x^646-28579*x^647-71419*x^648-195852*x^649+138465*x^650-3687*x^651+75696*x^652+155900*x^653-258594*x^654+94763*x^655-3358*x^656+38577*x^657+94759*x^658-47070*x^659+51513*x^660+71330*x^661+119247*x^662+69983*x^608-226*x^609-19711*x^610+10956*x^611-87470*x^612-13449*x^613-3841*x^614-33836*x^615-4560*x^585-106825*x^586+89782*x^587-46767*x^588-74424*x^589+256141*x^590+4048*x^591-27156*x^592-41459*x^593-84486*x^594+67323*x^595+68560*x^569-2017*x^570-13035*x^571-24271*x^572-3948*x^573+10487*x^574-49851*x^575-53359*x^576-78913*x^577+24172*x^578-107478*x^579+44498*x^580+109805*x^581+78227*x^582-44689*x^583-57413*x^584+80451*x^596+121583*x^597-13614*x^598+17903*x^599-215*x^600+6198*x^601+4632*x^602+51284*x^603+10690*x^604-49151*x^605-107304*x^606+65739*x^607+77793*x^663-66197*x^664-39685*x^665+84116*x^666-104216*x^667+51536*x^668-64446*x^669-47352*x^670-186*x^671+29490*x^672+74887*x^673+80207*x^674-14862*x^675-63628*x^676+44656*x^677-3847*x^678-78911*x^679+27515*x^680+41462*x^681+87322*x^682+26298*x^683-40767*x^684-68320*x^685+118888*x^686+46601*x^687-45762*x^688-55042*x^689+67869*x^690-122070*x^691+128219*x^692+57528*x^693-81615*x^694+58923*x^695-74242*x^696+56664*x^697-58956*x^698-53456*x^699+100279*x^700+112640*x^701+80402*x^702-191243*x^703-76677*x^704-27847*x^705-31388*x^706-61077*x^707-38319*x^708-65012*x^709+14347*x^710-67412*x^711+123251*x^712+12662*x^713+3566*x^714+35878*x^715+36012*x^716+243395*x^717+64964*x^718-105783*x^719+27976*x^720+117661*x^721-48333*x^722+34342*x^723+47987*x^724-124426*x^725-28904*x^726+183410*x^727+80678*x^728+66443*x^729+37955*x^730+83404*x^731-60527*x^732-157324*x^733+25967*x^734-202293*x^735-61898*x^736-24463*x^737+78032*x^738+15608*x^739-21575*x^740-39329*x^741+64599*x^742-33623*x^743+48044*x^744-51659*x^745+11947*x^746-55954*x^747-20610*x^748-54201*x^749+48944*x^750-42391*x^751-73375*x^752-161520*x^753-136669*x^754-5499*x^755+158508*x^756+103368*x^757+70760*x^758+10222*x^759-89330*x^760-169735*x^761-18822*x^762+10200*x^763+55385*x^764-13231*x^765+146373*x^766+7972*x^767-5299*x^768-77136*x^769+6645*x^770+176831*x^771+9969*x^772-127946*x^773-130011*x^774+25301*x^775+12528*x^776-18695*x^777-66321*x^778-34106*x^779-113743*x^780+140714*x^781-14757*x^782+14113*x^783+128818*x^784+152392*x^785+65474*x^786-98479*x^787-53732*x^788+109592*x^789+77860*x^790-47974*x^791-30223*x^792+42135*x^793-41154*x^794+4069*x^795-72486*x^796+23858*x^797+86305*x^798-21728*x^799+51398*x^800-176976*x^801-137196*x^802-19754*x^803+136207*x^804+38809*x^805+106397*x^806+31364*x^807-14424*x^808-53618*x^809-37309*x^810-19672*x^811-46512*x^812+109478*x^813+124434*x^814+115104*x^815-59930*x^816-16425*x^817+108349*x^818+76262*x^819+65766*x^820-46287*x^821+58850*x^822-64739*x^823+116925*x^824-78962*x^825+14564*x^826-180971*x^827-140224*x^828+104915*x^829-20768*x^830+67220*x^831+122144*x^832+34979*x^833+127798*x^834-238069*x^835-8469*x^836-60389*x^837-75016*x^838-130440*x^839-74854*x^840+32208*x^841+88750*x^842+29997*x^843-214107*x^844-176045*x^845-137016*x^846+75697*x^847+110543*x^848-62289*x^849-124460*x^850-89863*x^851+106474*x^852+77986*x^853+43966*x^854-44189*x^855-156129*x^856-97581*x^857+60379*x^858+63679*x^859+11151*x^860+141029*x^861-112536*x^862-26083*x^863-48123*x^864-82519*x^865+80913*x^866-20615*x^867+18016*x^868+27684*x^869-70460*x^870-203036*x^871+87876*x^872-64109*x^873-135250*x^874-29534*x^875+303407*x^876+86992*x^877-33781*x^878-102907*x^879+45967*x^880-48311*x^1676-64646*x^1677+8780*x^1678-89730*x^1679+51125*x^1680+81822*x^1681+5829*x^1682+3387*x^1683-33443*x^1684+175329*x^881-63359*x^882+23769*x^883-135962*x^884-23904*x^885-37301*x^886+39517*x^887+101687*x^888+107948*x^889-8832*x^890-45108*x^891+34153*x^892-8577*x^893+84005*x^894-67379*x^895-9038*x^896-3827*x^897-172510*x^898+101362*x^899+169576*x^900+24876*x^901+182757*x^902+85387*x^903-14477*x^904-56052*x^905-80169*x^906-60150*x^907-45676*x^908+80858*x^909-107578*x^910-2330*x^911+85851*x^912-78139*x^913-49522*x^914+9485*x^915+260565*x^916+134767*x^917+180078*x^918-140282*x^919-64830*x^920+4377*x^921-43434*x^922-63049*x^923+186766*x^924+52498*x^925+36028*x^926+29718*x^927+153541*x^928-59153*x^929-51844*x^930-112862*x^931+146100*x^932-40034*x^933+26904*x^934-56942*x^935-72181*x^936+67551*x^937+33926*x^938-192857*x^939+86413*x^940+3606*x^941+3781*x^942+3711*x^943-87022*x^944-109076*x^945+58634*x^946+13013*x^947+58183*x^948+27378*x^949-113845*x^950+168774*x^951-92553*x^952+77528*x^953+24096*x^954+16479*x^955-30812*x^956+26047*x^957-32313*x^958-278592*x^959+84184*x^960-60507*x^961-23547*x^962-79864*x^963-9112*x^964+9588*x^965-34316*x^966+111219*x^967+35212*x^968-65229*x^969+158992*x^970-125110*x^971+47058*x^972-91530*x^973-26010*x^974+92244*x^975-60469*x^976-100094*x^977-47095*x^978+46224*x^979+215068*x^980-76270*x^981-30793*x^982+6009*x^983-47525*x^984+5423*x^985+97496*x^986-131565*x^987+169560*x^988-131158*x^989+93491*x^990+1852*x^991+26252*x^992-128619*x^993-202621*x^994-2557*x^995+213624*x^996+60672*x^997-83272*x^998+7942*x^999+122561*x^1000-27291*x^1489-8592*x^1490+6150*x^1491-8865*x^1492+108535*x^1493-20367*x^1494-16513*x^1495+107297*x^1496-98664*x^1685+9722*x^1687-28409*x^1688-13978*x^1689+118740*x^1690-106619*x^1691-29670*x^1692-38566*x^1693-11332*x^1694-57837*x^1686+67876*x^1712-58331*x^1714+7991*x^1715-147312*x^1716-16161*x^1717+76222*x^1718-20732*x^1719+6341*x^1720+2050*x^1721-25480*x^1722+7674*x^1723-24607*x^1724-44277*x^1725+13030*x^1726-33340*x^1727+4883*x^1713+38934*x^1703+18449*x^1704-29136*x^1705+30522*x^1706+44960*x^1707-8529*x^1708+65451*x^1709+41755*x^1710-5445*x^1711+34159*x^1728+62633*x^1729+86540*x^1730-77408*x^1731+17503*x^1732-112968*x^1733+3339*x^1734+29835*x^1735+102070*x^1736+3236*x^1737+37910*x^1738-18715*x^1784-57665*x^1785-82227*x^1786+27480*x^1787-37686*x^1788+28245*x^1789+10527*x^1790+50912*x^1791-64473*x^1775+60918*x^1776+24120*x^1777-26807*x^1778+18400*x^1779-89895*x^1780+2121*x^1781+9126*x^1782+35679*x^1783-7445*x^1767+22839*x^1768+19340*x^1769+27238*x^1770+24559*x^1771+45436*x^1772-2973*x^1773-13668*x^1774+21987*x^1752-7819*x^1753+63519*x^1754+60971*x^1755-98790*x^1756-6248*x^1757-12079*x^1758-55636*x^1759+43510*x^1760+35120*x^1761+111626*x^1762-564*x^1763+84316*x^1764-55917*x^1765-46318*x^1766+25824*x^1739-132938*x^1740-20887*x^1741-16132*x^1742-19132*x^1743-40618*x^1744+39119*x^1745-37971*x^1746-66805*x^1747+75969*x^1748-41685*x^1749-120856*x^1750+20209*x^1751+47829*x^1792-34439*x^1793-122213*x^1794-24829*x^1795-38969*x^1796+81353*x^1797+25175*x^1798-54500*x^1799+10957*x^1800+22748*x^1801-11963*x^1816+61524*x^1817+37381*x^1818+59825*x^1819-32059*x^1820-86559*x^1821-3718*x^1822+84036*x^1823+57510*x^1824+80879*x^1825+68929*x^1802-44311*x^1803+13058*x^1804-100241*x^1805+4836*x^1806+45301*x^1807-10822*x^1808+34468*x^1809+33225*x^1810-24522*x^1811+86596*x^1812+18151*x^1813-40104*x^1814-19875*x^1815-21231*x^1899-16720*x^1900-19181*x^1901-3057*x^1902+10632*x^1903+2741*x^1904+17912*x^1905+32509*x^1906-5047*x^1907+34868*x^1908-37072*x^1909-9945*x^1910+41040*x^1887-30315*x^1888+20144*x^1889-19847*x^1890-24625*x^1891-44139*x^1892+997*x^1893+23330*x^1894+14500*x^1895+51087*x^1896+19796*x^1897+32818*x^1898+14123*x^1875+9914*x^1876+15795*x^1877+45084*x^1878+22305*x^1879-41277*x^1880-32434*x^1881-11991*x^1882-704*x^1883+5954*x^1884-9443*x^1885-42617*x^1886+20887*x^1866-13298*x^1867+30041*x^1868-13534*x^1869+13807*x^1870-40806*x^1871+27390*x^1872+18161*x^1873+20333*x^1874-26728*x^1856+5573*x^1857+5636*x^1858+27291*x^1859-69151*x^1860-36663*x^1861-49539*x^1862+40517*x^1863-1380*x^1864+4822*x^1865-32427*x^1845+12606*x^1847+10723*x^1848+26182*x^1849-8954*x^1850-55776*x^1851-35072*x^1852+47230*x^1853-26335*x^1854-37773*x^1855-17214*x^1846-56942*x^1837+11442*x^1838-35038*x^1839+85186*x^1840-78238*x^1841+35190*x^1842+4035*x^1843-18615*x^1844-35891*x^1826+10667*x^1828-41201*x^1829-31474*x^1830+61512*x^1831+61579*x^1832+12666*x^1833+42192*x^1834-24041*x^1835-32367*x^1836-32948*x^1827+18855*x^1911-1529*x^1912+5514*x^1913+15533*x^1914-22568*x^1915-65496*x^1916+34125*x^1917+89771*x^1918+6914*x^1919+43189*x^1920-26659*x^1921-125496*x^1922+15192*x^1923-23943*x^1924-10198*x^1925-878*x^1926+21440*x^1927-7283*x^1928+43857*x^1929-803*x^1930-78652*x^1931-47887*x^1932-3595*x^1933-22661*x^1934+18410*x^1935+36991*x^1936-35960*x^1937-9177*x^1986-3056*x^1987-16887*x^1988-20881*x^1989-1059*x^1990-9252*x^1991-14908*x^1992-958*x^1993-4721*x^1994-16166*x^1995+11316*x^1996-2240*x^1997-10151*x^1998+25542*x^1974-1747*x^1976-17483*x^1977+4908*x^1978+3478*x^1979-3193*x^1980+30064*x^1981-18902*x^1982-19807*x^1983+17281*x^1984-10653*x^1985+21095*x^1975+15336*x^1959-12465*x^1961+18084*x^1962+22248*x^1963+7094*x^1964-1501*x^1965+34695*x^1966-35351*x^1967+25917*x^1968+42286*x^1969-15233*x^1970+40899*x^1971+10744*x^1972+6608*x^1973+28438*x^1960+39008*x^1951-43361*x^1952+26564*x^1953+34353*x^1954-24810*x^1955-4036*x^1956-7263*x^1957-22273*x^1958+10734*x^1938+3234*x^1939-5544*x^1940+18333*x^1941-26450*x^1942-3306*x^1943+1177*x^1944+24109*x^1945+3854*x^1946-31826*x^1947+7050*x^1948-28189*x^1949-27719*x^1950+6902*x^1999-2523*x^2000+6564*x^1695+166138*x^1696+88892*x^1697-70451*x^1698-158624*x^1699+28331*x^1700-12514*x^1701+88490*x^1702",
                 "-3680-4456*x+90816*x^500+35952*x^499+89870*x^498+752*x^50-12289*x^49-23435*x^48-17241*x^51-14723*x^61+6711*x^63+6503*x^64+22558*x^65+6863*x^66-46805*x^67+12318*x^68+19740*x^69+18027*x^70-937*x^71+8515*x^72-31468*x^73-45349*x^74-18459*x^75+12369*x^76+80664*x^77-306*x^78+227*x^79+5232*x^80+431*x^62+368*x^52+8678*x^54+20343*x^55+2013*x^56+4246*x^57-9906*x^58-17455*x^59-23042*x^60-15119*x^53-2597*x^81-3565*x^82-69014*x^83-17348*x^84-6767*x^85+11358*x^86+38549*x^87-18590*x^88-4627*x^89-4552*x^90-21171*x^91-16020*x^92-11018*x^93+35396*x^94+9435*x^95+42563*x^96+12154*x^97-44781*x^98-14640*x^99+12351*x^100+6263*x^35+3104*x^34-9478*x^33+20397*x^32+65067*x^426-28804*x^425-13785*x^424-22548*x^423-53184*x^422+14376*x^421-45257*x^420+10559*x^419-34416*x^418+101839*x^417-73815*x^416+28979*x^415-6977*x^414+6469*x^413+161564*x^412+90476*x^411-74268*x^410-3418*x^409-4925*x^408+8878*x^407+37405*x^406-133800*x^405+88358*x^404+37413*x^403+165902*x^402-33451*x^401+531*x^400-68561*x^434-36683*x^433-35691*x^432-22457*x^431+85080*x^430+27475*x^429-174244*x^428+64722*x^427-9452*x^438+52058*x^437+121930*x^436-90242*x^435+25541*x^439+8828*x^465-6237*x^464-29599*x^463+63744*x^462+26785*x^461+11147*x^460-46981*x^459-33202*x^458+75746*x^457-17667*x^456-33876*x^455+97186*x^454+52099*x^453+46929*x^452+99487*x^451-46197*x^450-70827*x^449+55981*x^448-110001*x^447+28442*x^446-14336*x^445+101296*x^444-20615*x^443+41444*x^442+44460*x^441-61876*x^440+23078*x^476-47718*x^475-12433*x^474+51298*x^473-53107*x^472-87012*x^471+159685*x^470-81269*x^469-172650*x^468-106750*x^467-8213*x^466-325*x^42+19881*x^41-6598*x^40-21981*x^39+2415*x^38-24610*x^37-17071*x^36+10732*x^44+12739*x^43+13865*x^45+133403*x^497-39686*x^496+94869*x^495-49546*x^494-36052*x^493-68993*x^492+67934*x^491-42113*x^490-31818*x^489+61066*x^488-112881*x^487-101480*x^486-23605*x^485+13221*x^484-22606*x^483+15143*x^482+53746*x^481+39127*x^480+176772*x^479+8443*x^478-86496*x^477+13773*x^46+13980*x^47+60273*x^1004-33060*x^1006-53145*x^1007+48235*x^1008+32886*x^1009+6984*x^1010+196602*x^1011+194816*x^1012+106181*x^1013-52616*x^1014+44083*x^1015+62867*x^1005+18730*x^266-103812*x^265+52035*x^264-28278*x^263+14922*x^262-24591*x^261-40270*x^260-87032*x^259+77605*x^258-38946*x^267+26196*x^297+36037*x^293+34332*x^292-30486*x^291-37045*x^290-20689*x^289-35642*x^288+90660*x^287-52277*x^286+54346*x^285-69694*x^284+50016*x^283+19063*x^282+61818*x^281-5714*x^280-22201*x^279+13581*x^278-68724*x^277+109641*x^276-93071*x^275-85136*x^274-40943*x^273+39641*x^272+61448*x^271-3759*x^270+91134*x^269-17711*x^268+19716*x^31+21003*x^30-549*x^29-13938*x^28+5136*x^27-32520*x^26+19366*x^323+79200*x^322-64670*x^321+23107*x^320+132715*x^319+46506*x^318-23611*x^317-128284*x^1001-13043*x^1002+71880*x^1003-72036*x^333+8024*x^332+2340*x^331-45317*x^330-31855*x^329-12566*x^328-34668*x^327-22004*x^326-19187*x^325-24293*x^324+61218*x^334+45586*x^345-5488*x^344+56838*x^343+10007*x^342+72494*x^341+8939*x^340-162576*x^339+14426*x^338-81724*x^337-45823*x^336+3189*x^335+26428*x^371-10871*x^370+17587*x^369+78741*x^368+40184*x^367-44367*x^366+145169*x^365-86323*x^364-54360*x^363-110599*x^362+7269*x^361-23484*x^360+16140*x^359+47237*x^358-59367*x^357+45830*x^356-55093*x^355-24347*x^354-12995*x^353+115231*x^352+36370*x^351-78074*x^350+18605*x^349-38905*x^348+19389*x^347-104066*x^346-78723*x^373+5062*x^372-17762*x^316-45825*x^315-43359*x^314+114095*x^313-16904*x^312+2467*x^311+94825*x^310-27607*x^309-58789*x^308+44540*x^307+36850*x^306-18507*x^305+18651*x^304-25070*x^303-68235*x^302+127130*x^301-86275*x^300-74417*x^299-43912*x^298-105277*x^296+83863*x^295+65041*x^294+12828*x^399-244588*x^398+4343*x^397+127499*x^396+43442*x^395-12826*x^394+50419*x^393-2428*x^392+5739*x^391+30428*x^390+51839*x^389+19437*x^388-29748*x^387-23482*x^386-86231*x^385-11595*x^384+52981*x^383+61910*x^382+56111*x^381-32479*x^380+5959*x^379-3510*x^378+47191*x^377-153495*x^376+18020*x^375-5107*x^374-16073*x^145-4727*x^144-18872*x^143+17994*x^142-45330*x^141+7214*x^140+79505*x^139-32905*x^138+38722*x^137-79692*x^136-9231*x^135+43793*x^134+32906*x^133+49039*x^132-23502*x^131-55229*x^130+11723*x^129+12571*x^128+9801*x^127+80*x^126-32045*x^125+36988*x^124+3384*x^123-39070*x^122-64115*x^121-50109*x^120-3545*x^12-849*x^11+3345*x^10+15261*x^9+4997*x^8+10369*x^1141-55733*x^1142+52731*x^1143-3263*x^1144-63120*x^1145+18000*x^1146-48692*x^1147+66406*x^1148-124290*x^1149+251127*x^1150+51041*x^1151+33806*x^1152+145567*x^1153+109542*x^1154-95615*x^1155+1510*x^156+13725*x^155+45548*x^154-4919*x^153+45083*x^152-12940*x^151-64211*x^150+1240*x^149+12272*x^148-55419*x^147-37252*x^146-38607*x^1129+62454*x^1131-129923*x^1132-229619*x^1133-237689*x^1134+145082*x^1135+59857*x^1136+95792*x^1137+145423*x^1138+49136*x^1139-12171*x^1140+131166*x^1130-88457*x^177-54635*x^176-44215*x^175+88757*x^174+51411*x^173-36202*x^172-17022*x^171-48857*x^170-5317*x^169-33756*x^168+12414*x^167-57648*x^166-1480*x^165-13698*x^164-31810*x^163+55991*x^162-46012*x^161-35693*x^160-1773*x^159+15532*x^158+20570*x^157-116968*x^1120-53038*x^1122-7002*x^1123-33218*x^1124-88707*x^1125+2355*x^1126-191541*x^1127-37708*x^1128+177525*x^1121-2592*x^15+17222*x^1104+2753*x^1106+64278*x^1107+105091*x^1108-162075*x^1109-18575*x^1110+874*x^1111-136441*x^1112-101399*x^1113+7252*x^1114+83800*x^1115+1222*x^1116-50296*x^1117-34961*x^1118+16008*x^1119-98528*x^1105-14166*x^18+9425*x^17+12994*x^16-3839*x^14-2358*x^13-19959*x^1088-109851*x^1090+198140*x^1091-136200*x^1092-185377*x^1093-75600*x^1094-29783*x^1095-113437*x^1096-83899*x^1097+81773*x^1098+71733*x^1099-29277*x^1100+245957*x^1101+191194*x^1102-12133*x^1103+146491*x^1089-158523*x^1079-58797*x^1081+155238*x^1082-48738*x^1083+21262*x^1084-116604*x^1085+40583*x^1086-134403*x^1087+678*x^1080+47410*x^119+49053*x^118+1160*x^117-9866*x^116+11978*x^115-14776*x^114-21298*x^113-6347*x^112-17162*x^111+25763*x^110-3905*x^109-6034*x^107+18332*x^106+11922*x^105+43090*x^104-9949*x^103+1666*x^102-25860*x^101+12734*x^1064+187796*x^1066+84630*x^1067-89242*x^1068-70552*x^1069-25560*x^1070+9474*x^1071+3857*x^1072-33195*x^1073+43056*x^1074-49544*x^1075+25784*x^1076+256441*x^1077-116785*x^1078+5188*x^1065-2882*x^20+182619*x^1052+10653*x^1054+77233*x^1055+21948*x^1056+2802*x^1057-18048*x^1058-147846*x^1059+56958*x^1060-17373*x^1061-94678*x^1062-157200*x^1063+83660*x^1053-3988*x^201-2422*x^200-23575*x^199-7858*x^198+17828*x^197-5741*x^196-28292*x^195+5731*x^194+75423*x^193+34509*x^192+7788*x^191-43123*x^190-8073*x^189+39991*x^188+75946*x^187-34425*x^186-65547*x^185+4456*x^184-36751*x^183-66083*x^182-16378*x^181+33512*x^180+45321*x^179+61718*x^178+116104*x^1040-94890*x^1042+62418*x^1043-77773*x^1044+80774*x^1045-17169*x^1046+66366*x^1047-104325*x^1048+81526*x^1049-191284*x^1050+45699*x^1051-227149*x^1041-104385*x^228+6420*x^227+120162*x^226-35314*x^225+21282*x^224+53374*x^223+42038*x^222+36193*x^221-79400*x^220-85426*x^219-48265*x^218+104987*x^217-40482*x^216-14313*x^215+36267*x^214+88659*x^213+29832*x^212-4569*x^211-60191*x^210+4532*x^209+3995*x^208-13939*x^207+16405*x^206-73393*x^205-50340*x^204+38169*x^203+25359*x^202+18664*x^234+3771*x^233+80025*x^232-7319*x^231-12541*x^230-71938*x^229+2645*x^1030-117203*x^1032-23567*x^1033+156185*x^1034-19077*x^1035-106498*x^1036+8836*x^1037-91916*x^1038+119870*x^1039-79348*x^1031-31830*x^1016+77703*x^1018+81164*x^1019-147008*x^1020-65830*x^1021-16529*x^1022-99176*x^1023+217225*x^1024+21312*x^1025-133777*x^1026+88713*x^1027-1817*x^1028-182475*x^1029+45311*x^1017-2462*x^24-39720*x^252+2854*x^251+4520*x^250+64365*x^249+21585*x^248+33270*x^247+13349*x^246-43708*x^245+2825*x^244-74777*x^243-18552*x^242+34656*x^241-50977*x^240+16453*x^239-116180*x^238-39345*x^237+95056*x^236+48041*x^235+33891*x^257+112422*x^256-21633*x^255-40052*x^254-11861*x^253-11717*x^25+20077*x^23+19259*x^22-5138*x^21-13491*x^19-14915*x^1182-46221*x^1184+178218*x^1185-65241*x^1186-44946*x^1187-54714*x^1188+22367*x^1189-1901*x^1190-102724*x^1191-17027*x^1192+57298*x^1193+211092*x^1194+14713*x^1195-8444*x^1183+518*x^1168-78847*x^1170-20614*x^1171+59290*x^1172-23399*x^1173+16329*x^1174-78773*x^1175+12000*x^1176-56430*x^1177+56167*x^1178+105680*x^1179+21921*x^1180+44956*x^1181+51312*x^1169-101982*x^1156-55418*x^1158+157376*x^1159+8704*x^1160-59319*x^1161+6459*x^1162-88212*x^1163-108093*x^1164+319887*x^1165+14785*x^1166-94138*x^1167-96266*x^1157-112534*x^1235+71385*x^1237+168038*x^1238+31764*x^1239+61453*x^1240-131329*x^1241+149060*x^1242+151845*x^1243+69823*x^1244+714*x^1245+37844*x^1246+15037*x^1236+148580*x^1221+74472*x^1223-47471*x^1224-136713*x^1225+158201*x^1226+62431*x^1227-127465*x^1228-59010*x^1229+181019*x^1230-9171*x^1231+39722*x^1232+60048*x^1233-156587*x^1234-89467*x^1222-172*x^4-1093*x^3+6080*x^2-92515*x^1206-25108*x^1208-14169*x^1209-45979*x^1210-46013*x^1211+7284*x^1212-12090*x^1213-127760*x^1214+4981*x^1215+6136*x^1216-24159*x^1217+17993*x^1218+89001*x^1219-47050*x^1220-185020*x^1207+2931*x^7-14656*x^6-4053*x^5+122916*x^1196-16700*x^1198-101963*x^1199-25020*x^1200-57138*x^1201+144133*x^1202+65118*x^1203-91713*x^1204+131327*x^1205+7820*x^1197-96371*x^1271+38917*x^1273-135482*x^1274+19585*x^1275-2123*x^1276+65286*x^1277+10079*x^1278+58366*x^1279+76815*x^1280+80436*x^1281+19932*x^1282-25293*x^1272+39471*x^1257-212787*x^1259+140198*x^1260-9353*x^1261-119558*x^1262-96765*x^1263+64906*x^1264-27120*x^1265-2100*x^1266+144343*x^1267+20188*x^1268-108481*x^1269+19309*x^1270-15533*x^1258-39072*x^1247+11512*x^1249-25039*x^1250+45795*x^1251-97515*x^1252-63183*x^1253-80283*x^1254-16194*x^1255-19962*x^1256-35524*x^1248-5350*x^1305-97996*x^1307+53493*x^1308+75947*x^1309+220551*x^1310-49130*x^1311+98339*x^1312-72630*x^1313+60620*x^1314+48427*x^1315+147951*x^1316-61015*x^1306+17495*x^1293+148146*x^1294+40370*x^1295-47447*x^1296+111730*x^1297-273775*x^1298+5127*x^1299-109025*x^1300+83060*x^1301-1091*x^1302-6675*x^1303-70816*x^1304-158254*x^1283+121287*x^1285-186229*x^1286+32934*x^1287-31649*x^1288+42470*x^1289+98917*x^1290-138365*x^1291+51235*x^1292+50448*x^1284-55133*x^1359-96162*x^1360+96511*x^1361-118334*x^1362+159186*x^1363-104697*x^1364+90790*x^1365-125180*x^1366+84748*x^1367-64906*x^1343+13333*x^1345+87464*x^1346-40091*x^1347-155928*x^1348-119251*x^1349-155553*x^1350+121376*x^1351+104489*x^1352-40264*x^1353+54653*x^1354+3782*x^1355+87200*x^1356+64608*x^1357-52751*x^1358+102895*x^1344-69875*x^108-18613*x^1329-90360*x^1331+25089*x^1332-205554*x^1333+56938*x^1334+29713*x^1335-9502*x^1336+33087*x^1337+22707*x^1338+55662*x^1339+51719*x^1340-9627*x^1341+76158*x^1342+84902*x^1330+177938*x^1317+25903*x^1319+1501*x^1320-11476*x^1321+52396*x^1322-102447*x^1323+23875*x^1324+1574*x^1325-91930*x^1326-36606*x^1327+90034*x^1328-8078*x^1318+21753*x^1368+22829*x^1370-4795*x^1371+165261*x^1372-35510*x^1373+71955*x^1374+4881*x^1375+130088*x^1376-40450*x^1377+116141*x^1378+84466*x^1379+32169*x^1369+80827*x^1380+53319*x^1382-133042*x^1383-3305*x^1384-102311*x^1385+6352*x^1386-50572*x^1387-147936*x^1388+127240*x^1389-3340*x^1390+185305*x^1391-81145*x^1392+38278*x^1393-160768*x^1394+24584*x^1395-37089*x^1381+165877*x^1424-52129*x^1425-45052*x^1426-124617*x^1427-171797*x^1428-15858*x^1429+19250*x^1430+62358*x^1431-57573*x^1432-56662*x^1433-22089*x^1414+35272*x^1416-127456*x^1417-47197*x^1418-94596*x^1419+123192*x^1420+55734*x^1421+77951*x^1422+46648*x^1423-119949*x^1415-18905*x^1404+60208*x^1406+46849*x^1407-4300*x^1408+14569*x^1409-147529*x^1410+10027*x^1411+70823*x^1412-2221*x^1413+3075*x^1405+120604*x^1396-95001*x^1397+102172*x^1398+51189*x^1399-27620*x^1400+25340*x^1401-17374*x^1402+17002*x^1403-66237*x^1443-66224*x^1444-98000*x^1445+171647*x^1446-21651*x^1447+43080*x^1448+124465*x^1449+168864*x^1450-17467*x^1451+62615*x^1452+43857*x^1434-34758*x^1436-23551*x^1437-36539*x^1438+13596*x^1439-61723*x^1440-79016*x^1441-2171*x^1442-32417*x^1435-51033*x^1462+23915*x^1463+47158*x^1464+82665*x^1465+23208*x^1466-1831*x^1467-43839*x^1468+8059*x^1469+119049*x^1470+8711*x^1471+42200*x^1472+92844*x^1473+66448*x^1453-218575*x^1454-74780*x^1455+58814*x^1456+73923*x^1457-17636*x^1458+93531*x^1459-11662*x^1460-175113*x^1461+47585*x^1474-17252*x^1475+8178*x^1476+79467*x^1477-84585*x^1478-26804*x^1479-32158*x^1480+75913*x^1481-89191*x^1482+188319*x^1483+70452*x^1484-50954*x^1485+69918*x^1486+113907*x^1487+34517*x^1488+83571*x^1497+85949*x^1498-247608*x^1499-46548*x^1500+19412*x^1501-74603*x^1502+118319*x^1503+96932*x^1504-20110*x^1505-135324*x^1506-30568*x^1507+20009*x^1508-24079*x^1509+153871*x^1510-69228*x^1524-28701*x^1525-61333*x^1526+66093*x^1527-2673*x^1528+10945*x^1529+2339*x^1530+33812*x^1531-68299*x^1532-3368*x^1533+12768*x^1534-81216*x^1535-10389*x^1536-31347*x^1537-18227*x^1538+117380*x^1511-32801*x^1513-8552*x^1514-63364*x^1515+29644*x^1516+37835*x^1517+75344*x^1518-38856*x^1519-39531*x^1520+78780*x^1521-15956*x^1522+49047*x^1523+74272*x^1512+120526*x^1550+65584*x^1551-62545*x^1552-33793*x^1553-97687*x^1554+10824*x^1555+58098*x^1556-63042*x^1557-10930*x^1539-113984*x^1540-2055*x^1541-89021*x^1542-87328*x^1543-138388*x^1544+135989*x^1545-92308*x^1546+86075*x^1547+5451*x^1548+34081*x^1549-12473*x^1588-52676*x^1589-32281*x^1590+36772*x^1591+22071*x^1592+48469*x^1593-29854*x^1594+115444*x^1595-26226*x^1596+74911*x^1597-14286*x^1598+18895*x^1573-47025*x^1574+135410*x^1575+33848*x^1576-24823*x^1577+18379*x^1578-77018*x^1579+116293*x^1580+32649*x^1581+38605*x^1582+822*x^1583-72621*x^1584+115965*x^1585+29434*x^1586-33152*x^1587+51429*x^1558-44077*x^1559-55219*x^1560+2240*x^1561+52128*x^1562-64105*x^1563-85187*x^1564-55672*x^1565-90450*x^1566+51451*x^1567-78319*x^1568+82512*x^1569-50738*x^1570+137898*x^1571-82033*x^1572-39331*x^1599+91032*x^1600+6344*x^1601+31867*x^1602-36543*x^1603+34121*x^1604-68923*x^1605+41496*x^1606+45439*x^1607+40997*x^1608+171507*x^1609+42067*x^1610-24489*x^1611+4816*x^1612-14877*x^1637+10433*x^1638-51050*x^1639-37017*x^1640-37143*x^1641-27287*x^1642+29492*x^1643+19333*x^1644-26916*x^1645-76982*x^1646+46763*x^1647-33148*x^1648+4704*x^1649+622*x^1650-104304*x^1651+17052*x^1629-15573*x^1630-35968*x^1631+39588*x^1632-41682*x^1633+46070*x^1634+2619*x^1635-69441*x^1636+71876*x^1613+145218*x^1615+6627*x^1616-30147*x^1617-40961*x^1618+10109*x^1619-37978*x^1620+24149*x^1621+57121*x^1622+31673*x^1623-28458*x^1624-47319*x^1625-36018*x^1626-21103*x^1627+79843*x^1628+92938*x^1614+20229*x^1652+114517*x^1653-56343*x^1654-44644*x^1655+24712*x^1656-15887*x^1657-79744*x^1658-29330*x^1659-50913*x^1660+8421*x^501+5136*x^502-27010*x^503-64069*x^504-52711*x^505-32600*x^506+33993*x^507+167034*x^508+129755*x^509-32472*x^510+59947*x^544+7937*x^546-35673*x^547-15447*x^548+68154*x^549-55218*x^550+6219*x^551+98545*x^552-36571*x^553-4929*x^545-56313*x^554-40840*x^555+2867*x^556-97093*x^557-38448*x^558+4290*x^559+28168*x^560+41991*x^561-102589*x^562-51695*x^563-17834*x^564-9504*x^565-80785*x^566-177711*x^567+156010*x^568-4501*x^511-28382*x^512+51267*x^513+113855*x^514+1280*x^515+9783*x^516-63583*x^517-15498*x^518-136174*x^519+21510*x^520+39599*x^521+86153*x^522+41382*x^524+63484*x^525-106729*x^526-49276*x^527-27021*x^528+57882*x^529-194749*x^530+71294*x^531-158601*x^532-15191*x^533+69321*x^534-13128*x^535+75661*x^536+21039*x^537+183716*x^538+106275*x^539-69993*x^540+4056*x^541-114975*x^542+63631*x^543-71494*x^523-70201*x^1661+125995*x^1662+4926*x^1663+96989*x^1664-7632*x^1665+13797*x^1666+2191*x^1667+82407*x^1668-16258*x^1669-1033*x^1670+24154*x^1671+14282*x^1672-82389*x^1673-21777*x^1674-22420*x^1675+10859*x^616-17844*x^617-162745*x^618-4073*x^619+70326*x^620+13353*x^621+95035*x^622+153534*x^623-71825*x^624-106868*x^625-168861*x^626+20905*x^627-43047*x^628-16969*x^629-66325*x^630-41710*x^631+138328*x^632+120510*x^633-14015*x^634+40697*x^635+157658*x^636-10032*x^637+85758*x^638+7937*x^639+53937*x^640-89298*x^641+74374*x^642+38697*x^643+200437*x^644+63412*x^645-111216*x^646-12689*x^647+43856*x^648-95230*x^649-108263*x^650-145687*x^651-8069*x^652-218044*x^653-40356*x^654-105418*x^655+149860*x^656+7125*x^657+28013*x^658+23164*x^659-19361*x^660-43065*x^661-68103*x^662+35525*x^608-3240*x^609+3187*x^610-99527*x^611+1194*x^612+161969*x^613-161481*x^614+24773*x^615-96039*x^585+82470*x^586-82744*x^587-121573*x^588-34899*x^589+107979*x^590+56176*x^591+22343*x^592+84404*x^593-74474*x^594-31579*x^595-29062*x^569+14573*x^570+83569*x^571+18414*x^572+79850*x^573-36487*x^574-92495*x^575-24990*x^576-24621*x^577+127735*x^578+15386*x^579+77070*x^580-9084*x^581+33484*x^582+29983*x^583-25767*x^584-7399*x^596+29375*x^597-4935*x^598+44674*x^599-3424*x^600-96916*x^601+7447*x^602-125032*x^603+41684*x^604+109116*x^605-13157*x^606+164645*x^607+64299*x^663-71567*x^664-51171*x^665-161989*x^666+72335*x^667+112796*x^668-6796*x^669+123732*x^670-116574*x^671+63104*x^672-110676*x^673-44265*x^674-15848*x^675+53867*x^676+36713*x^677-34907*x^678-13855*x^679-61195*x^680-16022*x^681-106208*x^682-36666*x^683+22987*x^684+143069*x^685+758*x^686+142529*x^687-129752*x^688-34448*x^689-701*x^690+208348*x^691-98208*x^692-56498*x^693-6621*x^694-46551*x^695-65012*x^696-50843*x^697-19027*x^698+22833*x^699+65457*x^700+42080*x^701+5932*x^702+111168*x^703+45928*x^704-82466*x^705+1306*x^706+106852*x^707-166435*x^708+73317*x^709-246868*x^710-34015*x^711+115230*x^712+61499*x^713+141860*x^714-1436*x^715+13116*x^716+97303*x^717+4677*x^718+127349*x^719-8341*x^720-40846*x^721-80584*x^722-143590*x^723-122991*x^724-201*x^725+118580*x^726+157950*x^727-45619*x^728-128412*x^729-134565*x^730+70202*x^731+125605*x^732-46547*x^733+81030*x^734+127368*x^735-51996*x^736-8532*x^737-44716*x^738+234233*x^739+12798*x^740+30071*x^741+18244*x^742-126790*x^743-174694*x^744-243956*x^745+115764*x^746-31119*x^747+70464*x^748-11845*x^749-151968*x^750-26735*x^751-60185*x^752+82005*x^753+128779*x^754+271963*x^755-86985*x^756-253090*x^757-48050*x^758-101357*x^759+129340*x^760+41763*x^761+53666*x^762+5824*x^763+85316*x^764-78241*x^765-183418*x^766-48918*x^767-55974*x^768+27645*x^769+77708*x^770+13291*x^771-63126*x^772-130765*x^773+38333*x^774+112330*x^775+85262*x^776+69336*x^777-119199*x^778+109469*x^779-138288*x^780+140112*x^781+64826*x^782+194909*x^783-83694*x^784-19245*x^785+10478*x^786-45688*x^787-151788*x^788-37544*x^789-42021*x^790+6891*x^791-84865*x^792+76969*x^793-95857*x^794+91731*x^795-154571*x^796+28909*x^797-47726*x^798+306060*x^799-31586*x^800+92643*x^801-69376*x^802+104359*x^803-41307*x^804-121697*x^805+46313*x^806-88099*x^807-24232*x^808+89970*x^809+127376*x^810+97867*x^811+105675*x^812+61013*x^813+95494*x^814-157168*x^815+59789*x^816-34522*x^817-49791*x^818+33701*x^819+46579*x^820-17078*x^821-120221*x^822+64415*x^823+71648*x^824-134071*x^825+173907*x^826-997*x^827-8076*x^828-99607*x^829+20681*x^830-97146*x^831-187798*x^832+145694*x^833+125343*x^834+46797*x^835-21492*x^836+15711*x^837+67805*x^838+22789*x^839-194601*x^840-93878*x^841-38870*x^842+129517*x^843+63748*x^844+42272*x^845+121927*x^846-48660*x^847+5933*x^848-57982*x^849+106329*x^850+86009*x^851-45875*x^852-10676*x^853-82832*x^854-111746*x^855-78240*x^856-91287*x^857-43776*x^858-104183*x^859+11261*x^860+130803*x^861+83288*x^862+21301*x^863-646*x^864+35312*x^865-125107*x^866+122724*x^867-101384*x^868+181114*x^869-76139*x^870+161553*x^871-12825*x^872+131625*x^873-12299*x^874-13158*x^875-146709*x^876+99759*x^877+104888*x^878-268913*x^879-141725*x^880-60181*x^1676+109930*x^1677+27589*x^1678-113124*x^1679-48937*x^1680-89572*x^1681-9691*x^1682+7172*x^1683-69*x^1684-120640*x^881-4395*x^882+26085*x^883-135325*x^884-61091*x^885+45141*x^886-207834*x^887+73531*x^888-183274*x^889+141774*x^890-153812*x^891+99395*x^892-160928*x^893-107080*x^894+106788*x^895-7071*x^896-37885*x^897+288255*x^898-128187*x^899-97931*x^900-104368*x^901-48435*x^902-179762*x^903+55858*x^904+86872*x^905-102687*x^906+83165*x^907-104949*x^908+10038*x^909-166171*x^910+237233*x^911-20153*x^912+151331*x^913-41070*x^914-61238*x^915-86835*x^916+55908*x^917-222072*x^918+75702*x^919+211649*x^920+165625*x^921-23446*x^922-9582*x^923+39364*x^924+30526*x^925-79572*x^926-66738*x^927-27090*x^928+184853*x^929+100842*x^930-150624*x^931-15814*x^932+61251*x^933-15448*x^934+62354*x^935-65163*x^936-28681*x^937+88907*x^938+105263*x^939+279460*x^940+101170*x^941+21482*x^942-127930*x^943-18227*x^944+105618*x^945+143108*x^946-81088*x^947+7145*x^948-128272*x^949+34233*x^950-42953*x^951-111170*x^952+35124*x^953-59374*x^954-68688*x^955+41785*x^956-80457*x^957-48648*x^958+223566*x^959-114526*x^960-94333*x^961-78000*x^962+16648*x^963+25087*x^964-83297*x^965+57755*x^966-132336*x^967+65761*x^968+58508*x^969+220969*x^970-69853*x^971+125297*x^972-169390*x^973-162899*x^974+36998*x^975-34992*x^976-122501*x^977-31433*x^978-109701*x^979-152151*x^980+168254*x^981-2830*x^982-42805*x^983-158721*x^984-86847*x^985-318671*x^986+103642*x^987-50610*x^988+47842*x^989+124777*x^990-34601*x^991+66171*x^992+61123*x^993+101347*x^994-165170*x^995-161517*x^996-60281*x^997-152963*x^998+56567*x^999-4168*x^1000-71221*x^1489-107555*x^1490+87999*x^1491-41746*x^1492+7382*x^1493-10596*x^1494+1113*x^1495+61476*x^1496-6900*x^1685+6802*x^1687+12120*x^1688+48484*x^1689-7628*x^1690-51861*x^1691-5657*x^1692-64354*x^1693-64422*x^1694+7937*x^1686-607*x^1712+53443*x^1714-79668*x^1715-1620*x^1716+92596*x^1717+64943*x^1718+231*x^1719+68027*x^1720-47330*x^1721-19707*x^1722-18944*x^1723+40223*x^1724+49412*x^1725+18359*x^1726+39068*x^1727-44002*x^1713-4706*x^1703+53824*x^1704+95228*x^1705-60792*x^1706+47096*x^1707-39448*x^1708-94257*x^1709+45128*x^1710+56*x^1711-90823*x^1728+61351*x^1729+15458*x^1730-907*x^1731+92035*x^1732+21361*x^1733+121421*x^1734+65992*x^1735+37326*x^1736+63451*x^1737+22041*x^1738+34914*x^1784+8612*x^1785-2905*x^1786-118604*x^1787-37956*x^1788-7549*x^1789-6294*x^1790+47907*x^1791+16465*x^1775-68638*x^1776-2765*x^1777-10939*x^1778+37528*x^1779-57538*x^1780-24247*x^1781-35202*x^1782-57856*x^1783-65725*x^1767+45747*x^1768-66456*x^1769-76130*x^1770+52733*x^1771+7266*x^1772+25009*x^1773-764*x^1774-58065*x^1752-74263*x^1753-59075*x^1754+67205*x^1755+23191*x^1756-1565*x^1757+12864*x^1758-50214*x^1759+6933*x^1760-94005*x^1761-65161*x^1762-22588*x^1763+119724*x^1764-18183*x^1765+32153*x^1766-23606*x^1739+71865*x^1740+51913*x^1741+79911*x^1742-40739*x^1743+41978*x^1744-26046*x^1745-16183*x^1746+47520*x^1747-8044*x^1748+120039*x^1749-18895*x^1750+40921*x^1751-9847*x^1792-15055*x^1793+32057*x^1794-43483*x^1795-42539*x^1796-41820*x^1797-23484*x^1798+15461*x^1799+22132*x^1800+82432*x^1801+25527*x^1816-83869*x^1817-17527*x^1818+85505*x^1819-5800*x^1820+17391*x^1821+41348*x^1822+461*x^1823+16691*x^1824+19838*x^1825+18091*x^1802+61033*x^1803-530*x^1804-115789*x^1805-47630*x^1806+13760*x^1807+33557*x^1808-28275*x^1809-74110*x^1810-26387*x^1811-12903*x^1812+30333*x^1813-39368*x^1814-8473*x^1815-23933*x^1899-51823*x^1900-3035*x^1901+50495*x^1902-44226*x^1903+12508*x^1904-6948*x^1905-73637*x^1906-651*x^1907-722*x^1908+20389*x^1909+22980*x^1910-28495*x^1887-24009*x^1888+24862*x^1889+9655*x^1890-25932*x^1891+33590*x^1892+39979*x^1893-38238*x^1894+6462*x^1895+14701*x^1896-22598*x^1897+4923*x^1898-2221*x^1875+7531*x^1876+36871*x^1877-21480*x^1878-3897*x^1879+21021*x^1880-48542*x^1881+33144*x^1882+110052*x^1883-6163*x^1884-1591*x^1885+81347*x^1886+4395*x^1866-23349*x^1867+67926*x^1868-18095*x^1869+1107*x^1870+18393*x^1871-28209*x^1872-22600*x^1873+36164*x^1874+30349*x^1856-24139*x^1857-6057*x^1858+15941*x^1859-12863*x^1860+11693*x^1861+26482*x^1862+2211*x^1863-8037*x^1864+18636*x^1865+14850*x^1845+22925*x^1847+10519*x^1848+7899*x^1849-4722*x^1850-10054*x^1851-9616*x^1852+38445*x^1853+22868*x^1854-13441*x^1855+33973*x^1846+10077*x^1837+36357*x^1838-30799*x^1839-78920*x^1840+41014*x^1841-28239*x^1842+19605*x^1843-54230*x^1844-26021*x^1826-55873*x^1828+8297*x^1829+17562*x^1830+33240*x^1831+18826*x^1832-70821*x^1833-9167*x^1834+48424*x^1835-5225*x^1836+2771*x^1827-27383*x^1911-34538*x^1912+7568*x^1913-17959*x^1914-64204*x^1915-22950*x^1916-6578*x^1917+2284*x^1918-5942*x^1919+66828*x^1920-30198*x^1921-4650*x^1922+6451*x^1923-11764*x^1924+31544*x^1925+22321*x^1926-15441*x^1927+2127*x^1928+1450*x^1929+27166*x^1930+25349*x^1931-9647*x^1932-24448*x^1933-40679*x^1934+9923*x^1935-36281*x^1936-20777*x^1937+2344*x^1986+8894*x^1987+8156*x^1988+3102*x^1989+1364*x^1990+12440*x^1991-2665*x^1992-6778*x^1993+6847*x^1994+4666*x^1995-3287*x^1996-2653*x^1997+5105*x^1998-5425*x^1974+6119*x^1976+9617*x^1977-243*x^1978-4409*x^1979+2158*x^1980+5584*x^1981+2275*x^1982+28604*x^1983+17938*x^1984+8287*x^1985+6169*x^1975-1842*x^1959+4458*x^1961-1171*x^1962+3447*x^1963+1392*x^1964+26920*x^1965+266*x^1966-21029*x^1967+3789*x^1968-5547*x^1969+20446*x^1970+24776*x^1971+11929*x^1972+1414*x^1973-3566*x^1960-44816*x^1951-9302*x^1952+25631*x^1953-6040*x^1954+15749*x^1955+11047*x^1956-7874*x^1957-9827*x^1958-5108*x^1938-48102*x^1939+39656*x^1940+17668*x^1941-2796*x^1942+14260*x^1943-27*x^1944-67367*x^1945-54545*x^1946+7286*x^1947+22418*x^1948-626*x^1949+55836*x^1950-2542*x^1999+609*x^2000-90381*x^1695+69588*x^1696+3053*x^1697-22673*x^1698+77138*x^1699-79388*x^1700-37440*x^1701+46467*x^1702");

  // GCD Mod 181
  may_kernel_intmod (may_set_ui (181));
  printf ("Compute GCD in Z/181Z\n");
  t += test_gcd ( "(1+2*x)^400*(x^3+2*x^2+1)", "(1+2*x)^42*(x^3-2*x+42)");
  t += test_gcd ( "(1+2*x)^400*(x^3+2*x^2+1)", "(1+2*x)^42*(x^3-2*x+42)+1");
  t += test_gcd ( "(1+2*x+y)^100*(x^3+2*x^2*y+1)", "(1+2*x+y)^42*(x^3-2*x+42)");
  t += test_gcd ( "(x^2-3*x*y+y^2)^4*(3*x-7*y+2)^5", "(x^2-3*x*y+y^2)^3*(3*x-7*y-2)^6");
  // t += test_gcd ( "(7*y*x^2*z^2-3*x*y*z+11*(x+1)*y^2+5*z+1)^4*(3*x-7*y+2*z-3)^6", "(7*y*x^2*z^2-3*x*y*z+11*(x+1)*y^2+5*z+1)^3*(3*x-7*y+2*z+3)^6");
  may_kernel_intmod (NULL);

  // GCD Mod 43051
  may_kernel_intmod (may_set_ui (43051));
  printf ("Compute GCD in Z/43051Z\n");
  t += test_gcd ("-936639990+26248623452*x^47-30174373832*x^46-19627087954*x^45+23532819511*x^24-15331409214*x^23-5892518210*x^22-2954269379*x^28-35110566664*x^27-4846605217*x^26-31276301429*x^25-19509035058*x^44+18467021749*x^43+18517327396*x^42-3435911855*x^50-272899085*x^49+1530941050*x^48+4507833875*x+10918084347*x^21+7814495607*x^20+23471621465*x^19-11157075535*x^18-11580405260*x^41-12162845828*x^39+12293848678*x^10-14679334842*x^9+17276957972*x^8+6436149609*x^7-8608995580*x^3-178435747*x^2+21474090980*x^17-3966988360*x^16-1330154020*x^15-13410537172*x^14-31038290506*x^51+17469329915*x^52-15282350857*x^53-11990574398*x^54+4255578378*x^55+2134905268*x^56+38375274669*x^57+9183261708*x^58+23467669465*x^59-5872201799*x^60+44704228721*x^61+27682281779*x^62-17204774085*x^63-7544450778*x^64-319408644*x^65+36034512058*x^66-6744628155*x^67+1363904895*x^68+22219362387*x^69+4275663006*x^70+14038262112*x^71+25591410149*x^72+9385606040*x^73-4745283802*x^74+11598735101*x^75-2508504600*x^76+11912265375*x^77+6624766926*x^78-4828562936*x^79+16636451249*x^80-9654281719*x^81+1913801599*x^82+2030662033*x^83-5474416933*x^84+11196778577*x^85+512268187*x^86-1029159530*x^87+1890708440*x^88+142491112*x^89-1261023993*x^90+3580061526*x^91-967823912*x^92+2292551912*x^93-3890383043*x^94-7104898913*x^95+3692350529*x^96-933592128*x^97+1199649580*x^98-227816977*x^99-12217636980*x^40+26794929142*x^38-41046742100*x^37+8432066353*x^36-6214147095*x^35+741141010*x^100-6270990007*x^34-6863205889*x^33-38388107386*x^32-561107347*x^13-6663332709*x^12+5507703343*x^11+11614165303*x^31-4503859503*x^30+26050790733*x^29+1788729074*x^6+2123912816*x^5-2542140060*x^4",
                 "1882371920-30937311949*x^47+6616907060*x^46-7420079075*x^45-28689993421*x^24+5544486596*x^23+7149046134*x^22+29115114125*x^28-7204610489*x^27-214559874*x^26+3363043648*x^25-6061422988*x^44-85814533599*x^43-5548876327*x^42+20531692560*x^50-23673866858*x^49+1553111326*x^48-7521320830*x-19801882818*x^21+7056872613*x^20-8039690791*x^19+11049483001*x^18+21957440404*x^41+18566882873*x^39-1088265300*x^10+29943249139*x^9+3160532878*x^8+110919258*x^7+1584505491*x^3-2133682609*x^2+11700952246*x^17-12424469513*x^16-426278523*x^15+7561875663*x^14+19494168654*x^51-8174921854*x^52+18073788685*x^53-48441925739*x^54-3850452489*x^55-17753230977*x^56-44973929094*x^57+7332053562*x^58-18246235652*x^59+9980611956*x^60+6961020716*x^61-3323333429*x^62-20693764726*x^63+2577247520*x^64-22463329638*x^65-13528331424*x^66+440594870*x^67+778770195*x^68+24497174109*x^69-11578701274*x^70-217623403*x^71+16461882236*x^72-6001481047*x^73-5750277357*x^74-16262001424*x^75+13760397067*x^76+12678193001*x^77+29675566382*x^78+1889202286*x^79+12672890141*x^80+26252749471*x^81+25061328813*x^82+20393397488*x^83-6344703601*x^84-12986831645*x^85+10287410396*x^86+18964048763*x^87+9141433582*x^88+9742202475*x^89+1800283356*x^90+9411879011*x^91+13562767020*x^92-3883570155*x^93+1190623161*x^94+4638169279*x^95+10385086102*x^96+10263027507*x^97+5186524611*x^98+1309311388*x^99+2485463829*x^40-14414308862*x^38-2750892631*x^37-34230002076*x^36-21558061051*x^35+2789684836*x^100-9973372012*x^34+12038293769*x^33-2280622009*x^32-14841292646*x^13+4463462090*x^12+11039962159*x^11+20714527103*x^31+29530802947*x^30+4813620791*x^29+8841198971*x^6-4237029618*x^5-1135211878*x^4");
  t += test_gcd ("-96986421453*x^426-75230349764*x^425+128202397899*x^424+67687797741*x^423+3024931260*x^422+43589211963*x^421+17028560437*x^420+9502389896*x^419+4613252773*x^418+12194328527*x^417-57222260166*x^416+40667752907*x^415+93728947357*x^414+35662883623*x^47-14000072723*x^46-9724148606*x^45-368500426*x^174-66112223350*x^173-44075800265*x^172+7490450738*x^171+51779005621*x^170-64020260063*x^169-35711451392*x^192-13811036693*x^191+38422996693*x^190+78178593950*x^189-26765678624*x^188-36201354868*x^187+2322225247*x^24-4088975893*x^23-5491427592*x^22+66441957259*x^333+132975753131*x^332-25949991451*x^331-18528608186*x^330+34867243557*x^329-86291867505*x^328-23705975211*x^327+28864997607*x^326-29169139957*x^325-72081734233*x^324+16339897368*x^323-36285833017*x^322+38448532777*x^321+5689122392*x^320-11755215572*x^113+27585680289*x^112+42744050328*x^111-19842208161*x^110-15319380323*x^109-9901465464*x^108-5117699979*x^107-46666019657*x^106-9394133585*x^105-6971615995*x^104+6471181832*x^114-93545055409*x^347-52862770931*x^346-17176975268*x^345+102286955731*x^344-108335379176*x^343-49965250837*x^342+70929683596*x^341+43502762174*x^340-21623476978*x^339-48184795307*x^338+147075382031*x^337+2375821388*x^336+73656068270*x^335-36587627572*x^334-46411968297*x^477+117867456111*x^476+21597655248*x^475-96794140788*x^474+39647492898*x^473+34037992542*x^472+46480360486*x^471-83731284596*x^470-95857281423*x^469+6999736667*x^468-38822852477*x^467-130028027968*x^466-14942819105*x^465-44491532079*x^464-136904650366*x^450+127351380760*x^449-127679332542*x^448-43163228644*x^447+130723680563*x^446+20671214531*x^445+28594103126*x^444-20976472951*x^443+51300798407*x^442-136736402258*x^441-70334716366*x^440-15000956055*x^439-62750491292*x^438+80489844722*x^451-72300799704*x^373+85385725929*x^372-36185314264*x^371+40354846070*x^370+100343908392*x^369-64625422713*x^368+64635098245*x^367+148519100093*x^366-2548271547*x^365+81466612776*x^364+4505982224*x^363-27138600773*x^362+33493048106*x^361-24851820772*x^28-10244897195*x^27-4624820516*x^26-7148687043*x^25-31464114015*x^44+4702632945*x^43+7767686263*x^42+42336900887*x^399+3102221349*x^398-70334528471*x^397+64364012828*x^396+2888804578*x^395+128564212488*x^394+46187560735*x^393-22449464022*x^392-10778814882*x^391-12324659485*x^390-29896766926*x^389-81652748187*x^388+83789647837*x^387+117499172*x^50+12296030709*x^49-19192082610*x^48-75325243338*x^202-23708473101*x^201-26628963155*x^200+8750880100*x^199+51496310059*x^198-825543354*x^197-55510698265*x^196+360107673*x^195+63114288079*x^194+47841450181*x^193-64231677271*x^203-33846557625*x^500+32003634503*x^499-58125049485*x^498-201024576939*x^497-16961069022*x^496-6460857023*x^495+17540403418*x^494+110942113017*x^493+31743184966*x^492-25555053476*x^491+4508005212*x+54592680578*x^437-76412796528*x^436+1877940561*x^435-103704559046*x^434+67584435952*x^433-92743779936*x^432+16830509289*x^431+90333755240*x^430+7491824203*x^429-39217091446*x^428+9937722888*x^427-19900810770*x^281+99196779*x^280-16832864815*x^279+12499571427*x^278+45347289311*x^277+71352470823*x^276+43741436118*x^275+37615792835*x^274+46658948580*x^273+33324303635*x^272+73592650369*x^271-67395425230*x^270-9849537996*x^269+66141877639*x^268+123533283703*x^360+23749744933*x^359-158320760707*x^358-96354447711*x^357+74192188378*x^356-12722639577*x^355-5277964622*x^354+27541514016*x^353-15290636975*x^352-56026626672*x^351-83627404427*x^350+134971538669*x^349-70927614564*x^348-30866959351*x^21+23196788827*x^20+14806487974*x^19+7202161677*x^18-77560240113*x^168-97097519755*x^167-7146671571*x^166+74640588234*x^165+44466905554*x^164+54305760741*x^163-42912777561*x^162-53367072318*x^161-53826227844*x^160+58601558389*x^159+46328377750*x^158+50761112626*x^157+45872218888*x^156-2281730398*x^155+17638972322*x^41-18461568113*x^39+138681011610*x^186+37241543347*x^185+6568392823*x^184+51513027685*x^183-7992856706*x^182+76890726092*x^181+42349995803*x^180+56514811799*x^179+39745228095*x^178-5406867587*x^177-1951053982*x^176-5802216424*x^175-50792201214*x^463+6116297*x^462+6556570017*x^461+108055362490*x^460-12691470210*x^459+4182889988*x^458-43791168823*x^457+34631974035*x^456-19719579134*x^455+25960754189*x^454+98225606953*x^453+91031125129*x^452-24988409414*x^10-743558160*x^9-1699950291*x^8+2808610975*x^7-48769244040*x^154-96704849123*x^153+111227374137*x^152-14353055349*x^151-3530455648*x^150+67627044718*x^149+48938383630*x^148-36284076468*x^147-36888035781*x^146+21119495829*x^145-7910849264*x^144-12955332083*x^143-45900441635*x^142-13504842602*x^3+6990226732*x^2-104066823246*x^294-33495506282*x^293+14075627371*x^292+98874306790*x^291-60578180611*x^290+4520025730*x^289+11037329991*x^288-61725490177*x^287-81799020830*x^286-7490987671*x^285-53034900008*x^284-18832528068*x^283-41894083315*x^282+37239574099*x^217-19338020660*x^216-12240694841*x^215+43055454509*x^214+40264787273*x^213-5627707745*x^212+65168034480*x^211+60502488359*x^210-43821281361*x^209-66454240411*x^208-2909056156*x^207+28786587304*x^206-30816172728*x^205+33777871529*x^204-190549103953*x^490+76694110369*x^489-2193972225*x^488-15694566522*x^487-5687638790*x^486+108859908155*x^485-10632768957*x^484+59932596621*x^483-128319694751*x^482+9747184412*x^481-46212670973*x^480-241524000*x^479-109350192806*x^478-19351431004*x^17-10992374349*x^16+4266182503*x^15+1491492502*x^14-26936243598*x^51+9140784640*x^52-17235611114*x^53+51043001177*x^54+17375254851*x^55+29087076273*x^56+41034433851*x^57-3776695101*x^58+15602920815*x^59-33931973560*x^60-5394467773*x^61-33356572049*x^62+26987634076*x^63-13728305358*x^64-9103761879*x^65-7718466644*x^66-36314004427*x^67-5838889357*x^68-34997974374*x^69+18394398361*x^70+2693349321*x^71+26120420097*x^72+17127750642*x^73-13997783088*x^74-20592275154*x^75-32678406375*x^76-37626204617*x^77-48378652006*x^78+21054785420*x^79-29131926756*x^80+8395471553*x^81+7784927438*x^82+13009002197*x^83-6234144809*x^84-37572100716*x^85-6593230199*x^86+66776088000*x^87+78828918914*x^88+11091149924*x^89+10549817427*x^90+6921991220*x^91-10162442680*x^92-31105862688*x^93+29905246284*x^94+56641649019*x^95+2729368709*x^96-29516060802*x^97-60463907529*x^98-20589437573*x^99-76023724773*x^386-66294717897*x^385+127893777713*x^384-33654971374*x^383+67124743823*x^382-31169670568*x^381-62445748947*x^380-88066123018*x^379-38626212646*x^378-16479040877*x^377-38277841666*x^376-22265169452*x^375-94473640391*x^374-16848867764*x^40+97436811*x^38-11456803668*x^37+29134819428*x^36+19443677064*x^35+115142334273*x^308+56747688015*x^307+25168740472*x^306+13152212288*x^305-44950776707*x^304+76665344711*x^303+29576368959*x^302+32075973218*x^301-9029375348*x^300+6642245020*x^299+91687715671*x^298-51412655310*x^297+83980703440*x^296+46894388060*x^295-5884703744*x^141+2949366186*x^140-94344563203*x^139-49160316545*x^138-32150373572*x^137+12194208956*x^136+6964141193*x^135-33897455084*x^134+10261728181*x^133-55302581972*x^132+21255450769*x^131-36152255205*x^130-15653019958*x^129+35552230340*x^128+19003739378*x^244+108435248514*x^243+56090919989*x^242+69228227868*x^241+91831598024*x^240-17331067829*x^239+85583813531*x^238+23101424573*x^237-81857540502*x^236-36120008211*x^235+5734833290*x^234-18668026178*x^233-49074512287*x^232+17073277846*x^413+30247783286*x^412-165671844164*x^411-155653969476*x^410-187279578486*x^409+39531046142*x^408-27411804632*x^407+29944561569*x^406-111402223051*x^405-154854505413*x^404-40754859947*x^403-42933459998*x^402-49470282393*x^401+47851831012*x^400+14974328774*x^100+35782130492*x^267-46375317876*x^266+35773383758*x^265+20027063447*x^264+21061029248*x^263+39184671316*x^262-90547480531*x^261-66296134219*x^260-17351793087*x^259+32060807026*x^258+94792744023*x^257+49960952178*x^256-56558329066*x^255+24220170558*x^34+13174400153*x^33-32599593323*x^32+68703378390*x^127+7209203154*x^126+53371357578*x^125+29055746831*x^124+7953940336*x^123+58282173183*x^122+9572688731*x^121+18717721630*x^120-73554904502*x^119+3118816831*x^118+14413362310*x^117+5342856589*x^116+19553114613*x^115+21062039154*x^102-554311514*x^101-22619708092*x^103+61239109029*x^254-3920281128*x^253-78250174682*x^252-54555413490*x^251+115877792898*x^250+43662559749*x^249-120143669223*x^248+50903904162*x^247+11038501743*x^246-77319776228*x^245+32754289554*x^13+9140139237*x^12-5050784936*x^11-28608214851*x^319-115868216966*x^318+21037102483*x^317-40760242975*x^316-97849927912*x^315+97524668075*x^314+15492130497*x^313-109577284825*x^312-15523330582*x^311+53171617167*x^310+2760321123*x^309+11953881518*x^31-4644794034*x^30+8900679779*x^29+3038154286*x^6-1713528498*x^5-1682288772*x^4-48992402174*x^231-74205154963*x^230-12320683295*x^229+2580030624*x^228+64073843802*x^227+98985527674*x^226+27217405490*x^225-4798940390*x^224-40677318547*x^223+22320424589*x^222-40460537540*x^221-2484720173*x^220+91568547773*x^219-30440775257*x^218-20925478361*x^501-55379150825*x^502+41318859160*x^503-201803259151*x^504+24491832146*x^505+11945310232*x^506+16801030141*x^507-85919705432*x^508-204400811240*x^509+108827759662*x^510-7434778800*x^511+11930243788*x^512+52620104622*x^513+2896973876*x^514-62777820856*x^515-58602826569*x^516+133850541568*x^517-141993258526*x^518+23529758012*x^519-49454155126*x^520+48873695726*x^521-4147628780*x^522+37394108608*x^523+47603927637*x^524-24791421698*x^525+227326835801*x^526-14216866536*x^527+18576010707*x^528-21047691775*x^529+41343039994*x^530+54576723923*x^531-93293864449*x^532-33311213168*x^533-121363092547*x^534+33716301298*x^535-141040357184*x^536+51729666523*x^537+47937306583*x^538+59972281522*x^539-118977051593*x^540+67621432194*x^541+49510957189*x^542-18373445363*x^543+77268324211*x^544-133652596329*x^545-33419335873*x^546-427835316*x^547-53735861227*x^548-18820033537*x^549+39133250886*x^550-56649816582*x^551+39906553346*x^552-38346679560*x^553-21315694546*x^554-5783912402*x^555+27780723169*x^556-38733479148*x^557-105931482172*x^558-96741538387*x^559-49387589876*x^560+8867816401*x^561+49087289355*x^562+1454848177*x^563+33306828689*x^564-99246716946*x^565+37769469573*x^566+32186219432*x^567-11308309096*x^568+18668048060*x^569+49786290189*x^570+18415965909*x^571+3684081565*x^572-4772411978*x^573+29967501107*x^574+88125664232*x^575-45402703990*x^576-17923494443*x^577-66856091994*x^578+52497931423*x^579-38961224535*x^580-37418352186*x^581+49679845815*x^582-57132928134*x^583+59226638697*x^584-154438376054*x^585-33182700061*x^586-29726913945*x^587-69912710958*x^588-35066505635*x^589+12117333556*x^590+90618649713*x^591+25641508805*x^592-14360015792*x^593-2388909185*x^594-66943383926*x^595+74263398539*x^596+143519268530*x^597+69955222853*x^598+93655324812*x^599+95865179464*x^600+26117219984*x^601+125957153927*x^602+85643852758*x^603+42892120680*x^604-21713462866*x^605+14954939220*x^606+10043903038*x^607+75469410317*x^608-51837119306*x^609+15040917218*x^610+20377338352*x^611+31441679566*x^612-83915039746*x^613+26252571519*x^614-503634581*x^615+26287902963*x^616-93906462389*x^617-31633190464*x^618+89968162928*x^619-170991982775*x^620+83899910708*x^621+106560970300*x^622-44686742795*x^623+15837336057*x^624+40628901174*x^625+45734331636*x^626+33015259104*x^627+98460164072*x^628-42946149377*x^629-7628175548*x^630+19113794084*x^631-41069464410*x^632+14600883385*x^633-39765610904*x^634+15187751854*x^635-64373116881*x^636+57427180760*x^637-54359632537*x^638-64614552532*x^639+33179178357*x^640+48320365071*x^641-11056685372*x^642+38160121363*x^643-103617462479*x^644-2932820983*x^645+55149815103*x^646+2425431294*x^647-56577206559*x^648+16057758155*x^649+35778946395*x^650-48176618659*x^651-64696379262*x^652-22620871596*x^653+58425156881*x^654+44785934533*x^655+80551487292*x^656+54031151324*x^657-5670209695*x^658+17381996998*x^659-19820657597*x^660+21974450271*x^661-82269840114*x^662-37921867307*x^663+36456465218*x^664-83294486767*x^665-15913209110*x^666-41870399203*x^667+75486284992*x^668-69200721639*x^669+71593232147*x^670-24896238679*x^671+12917620475*x^672+83950160209*x^673-49426619038*x^674-75127058039*x^675+54011881154*x^676+37241246401*x^677-57745731856*x^678+139894949202*x^679+81749104336*x^680+19047569663*x^681-3263108647*x^682+27211133766*x^683-122451257494*x^684-8555594792*x^685+33253239722*x^686-107349740514*x^687+51092807913*x^688+4057189648*x^689-93256950011*x^690+55162355472*x^691-61816339842*x^692+16885083096*x^693-121751819826*x^694+50639115264*x^695+77247288431*x^696+22943813602*x^697+33215869090*x^698-58975325286*x^699-157199542515*x^700+87871148892*x^701-28766482795*x^702-29747811055*x^703-68956086160*x^704+65220950625*x^705-21662209987*x^706+37272639173*x^707+96801516469*x^708-71318240058*x^709+51218503786*x^710+89676365367*x^711-74236109343*x^712+73239144110*x^713+74363608320*x^714+9145207269*x^715-43125847627*x^716+45624962357*x^717-87121922253*x^718-16301512498*x^719-74699639223*x^720+10858510024*x^721-49718828989*x^722+69188784202*x^723-57283163802*x^724-25240946842*x^725-18848125183*x^726+30824101946*x^727+28131925069*x^728-4523237314*x^729-21708406770*x^730+16572200087*x^731+33901545283*x^732+36570946377*x^733-85342952665*x^734-11024354222*x^735-52996875075*x^736+2850866301*x^737+584390938*x^738-15576643293*x^739-19952462826*x^740+118150124588*x^741-30970777574*x^742-65402936216*x^743+59841029763*x^744-45851459883*x^745-27905618565*x^746+9407727592*x^747+74081843028*x^748-36908923676*x^749+33404180933*x^750-12010832118*x^751-27598133503*x^752-18631604127*x^753+110706117529*x^754-58260993047*x^755+92000371551*x^756-3544997753*x^757-24026345516*x^758-5326405854*x^759+78096319892*x^760-597358210*x^761-35334977036*x^762+27768304059*x^763-27110809247*x^764-73612582334*x^765+107609364617*x^766-59301141886*x^767+23970324440*x^768-29060060082*x^769+68777102577*x^770-132377413827*x^771+22754238015*x^772-32129043594*x^773-64999432548*x^774-19372762078*x^775+31184492389*x^776-10477627638*x^777+50023457084*x^778+47413007239*x^779-26371384083*x^780+60690219120*x^781+19239801238*x^782-83296732353*x^783+1369782155*x^784+39528998233*x^785+89322610585*x^786-14469114718*x^787+61444882143*x^788-27268007305*x^789-66782025565*x^790+72559382333*x^791-58375896558*x^792-59019888869*x^793+59537001845*x^794-7672146710*x^795+46071362979*x^796+43864876841*x^797-38389399338*x^798+12445498240*x^799+22445421945*x^800-19867191482*x^801+29695639763*x^802-439060343*x^803-14548926177*x^804+6493137958*x^805-26475885652*x^806+17199673294*x^807-14472466294*x^808-3972443858*x^809-6694412444*x^810-27845179245*x^811+69564423337*x^812-1242944956*x^813+71753847110*x^814-110162491247*x^815+28760590961*x^816-45033514586*x^817+20432605842*x^818-2254902164*x^819-18122426841*x^820+39792310652*x^821-16010785979*x^822-29809849055*x^823+9525854977*x^824+17249950273*x^825-10134672379*x^826+20583429199*x^827+24105975389*x^828+7878147881*x^829+7805909361*x^830+24860098133*x^831-5072168506*x^832+40045803265*x^833+42779485436*x^834-13432875572*x^835+88161293341*x^836+91355873384*x^837-1467257349*x^838+11158963816*x^839+9583583914*x^840+31122429769*x^841-40482720803*x^842-4081869019*x^843-57902352430*x^844+16964780747*x^845-29060953772*x^846-69504616419*x^847-61121483464*x^848-46713891301*x^849+44904999298*x^850-69604536083*x^851-11337020865*x^852+294505024*x^853-47318977509*x^854+3027572134*x^855+21339618482*x^856-15853149005*x^857+16285335552*x^858+40110182790*x^859-18341657161*x^860+49887789936*x^861-33552701768*x^862+46385081780*x^863-981469808*x^864+41200826158*x^865+10777507154*x^866-14950519144*x^867+40419646263*x^868-53741450130*x^869-22868517477*x^870+45151817598*x^871-19543438862*x^872+3955972510*x^873+54752188479*x^874+21656278056*x^875-37418558367*x^876+46148554703*x^877-47291859633*x^878-60826337453*x^879-95667516424*x^880-25153078038*x^881-110893670927*x^882+36745227210*x^883+24891033298*x^884+13329786568*x^885+1662662627*x^886+11971271338*x^887+17144987144*x^888-2615988799*x^889-28070812835*x^890+37844104025*x^891+4209715470*x^892+20489525484*x^893-34750326173*x^894-26930924040*x^895+28807856566*x^896-67232744129*x^897+21557992547*x^898-7466474096*x^899-29460379882*x^900+11599719084*x^901+67152781205*x^902+8214925141*x^903-36941792660*x^904+53285909119*x^905+25945946437*x^906-50737201760*x^907-3797518282*x^908-14516581381*x^909+17052064867*x^910+49668098832*x^911-25564508274*x^912-27386697676*x^913-11774383518*x^914-493156372*x^915+13331359952*x^916+18215828822*x^917-1703765835*x^918-25637155270*x^919+16201914729*x^920+58046441105*x^921-63259400427*x^922+67373094341*x^923-10917543441*x^924+12944714407*x^925-330113187*x^926+16734740522*x^927+2893980284*x^928-32842296951*x^929+36608286601*x^930-15595841*x^931+16778173120*x^932-19939580967*x^933-9051600178*x^934-35290902058*x^935+13004524714*x^936-41266776816*x^937+12920294247*x^938+1269376803*x^939-5513768295*x^940+12475046328*x^941+31918641624*x^942+66368453249*x^943-897770392*x^944+40506291484*x^945+6825383600*x^946-210725476*x^947+23275280063*x^948-5947164362*x^949-28808686423*x^950+4353896571*x^951+10244426548*x^952-7839372567*x^953+30532389165*x^954-3403582670*x^955+45380086128*x^956-13074314129*x^957+28545325753*x^958-5842784212*x^959-26959506281*x^960+6604365268*x^961-32138117276*x^962-15682414365*x^963-4133390557*x^964-12416495535*x^965-2929737653*x^966-19993300065*x^967+11247744076*x^968+3841401771*x^969+25671473626*x^970-7838123767*x^971+3182563094*x^972-23190705393*x^973+25687775853*x^974+4991068675*x^975+9883795932*x^976-9640729721*x^977-9525700462*x^978+18936689098*x^979-1080031439*x^980+21713668278*x^981-16017536235*x^982+10320999730*x^983-6460201416*x^984-1480416714*x^985+1153896503*x^986-9970475903*x^987-1255301536*x^988+7891254981*x^989+6703945740*x^990+6810250661*x^991+1856482568*x^992+8149237774*x^993+7008509831*x^994+6006570629*x^995+6726792746*x^996+4503145470*x^997+91649222*x^998+3092691860*x^999+2111130448*x^1000+7931022064",
                 "13695560229*x^426+16181971852*x^425-90237548124*x^424-71238644589*x^423-54046636289*x^422-56977010004*x^421+24652773038*x^420+20598565048*x^419+91475303480*x^418-84971121323*x^417-113672677125*x^416+175104489886*x^415+24909134379*x^414+6097498021*x^47-184800727*x^46-20666602129*x^45-11384394640*x^174-39167969192*x^173+32831872664*x^172-414425301*x^171+26088535929*x^170+35596493049*x^169+30563227841*x^192+11577650926*x^191-43191322527*x^190+12086295210*x^189+23955571873*x^188-25298685458*x^187-28444834807*x^24+7010388904*x^23-29006384328*x^22-84330871264*x^333+9836257003*x^332+37640357302*x^331+17905116770*x^330-130251115515*x^329-95978501346*x^328+19514981244*x^327-106652197296*x^326+35941298261*x^325+57388442425*x^324+44763228998*x^323+51086800775*x^322-87113853021*x^321+63617860368*x^320-65881485345*x^113-14717773249*x^112-30051082488*x^111-16699273079*x^110+52386582094*x^109-51477006365*x^108+86299667628*x^107+33339117727*x^106-11189599026*x^105+15726918*x^104-21825360566*x^114-31322461593*x^347+65284435509*x^346-17301191144*x^345+15734564143*x^344+23681004128*x^343+134055240076*x^342-63495561637*x^341+69119399187*x^340+15761017924*x^339+62192334766*x^338+76140614307*x^337+48592812810*x^336-78591910551*x^335+5387039994*x^334-20627179048*x^477+30863999178*x^476-81087236312*x^475-75933164205*x^474-33082131999*x^473+45384435920*x^472+116792836581*x^471+116973440762*x^470-58617873216*x^469+16707117530*x^468+70585482190*x^467-25245695981*x^466+21964655001*x^465+156326246149*x^464+50572629272*x^450+15562656594*x^449-31384475178*x^448+28806488822*x^447+98379780384*x^446-8577150468*x^445-241997040784*x^444+44292557638*x^443-41043057929*x^442+43426121636*x^441+73421002043*x^440-122245378325*x^439+666183816*x^438-140210723957*x^451+33590048221*x^373+175101192821*x^372+70736972554*x^371-92048656022*x^370-3495793191*x^369-86224980665*x^368-67443623327*x^367+32314536775*x^366-28883186212*x^365+5111742127*x^364+121792145390*x^363+37030667675*x^362-4097118072*x^361+22857776248*x^28-12136573216*x^27+20442780080*x^26+25846700052*x^25+13425491987*x^44-51594114984*x^43-4377477922*x^42-43041382453*x^399-57661186745*x^398-22359325055*x^397+20325429303*x^396+69035457782*x^395+87685373045*x^394+24365492495*x^393+29684393262*x^392+48430131525*x^391-695625227*x^390+20950623212*x^389-11707096914*x^388+27698204708*x^387-17709284679*x^50-23800618055*x^49+58495527083*x^48+14753967994*x^202-7917376913*x^201-7656165582*x^200+44410996825*x^199+27191475903*x^198-65422047389*x^197-14819333083*x^196-1269092109*x^195-23263288526*x^194+23503451557*x^193-45722872111*x^203+18051733352*x^500-125499139260*x^499-36395631156*x^498-59637301917*x^497+105481548989*x^496+75797493110*x^495+32239299835*x^494+11835972529*x^493+116574982204*x^492-51868336172*x^491-9265203432*x-39597713836*x^437+113015935234*x^436-153556796338*x^435-4853604252*x^434+97729740048*x^433-33063566031*x^432+126719186003*x^431-74175775603*x^430-42715005191*x^429+32714632165*x^428+76253488404*x^427-22147870587*x^281-17004010435*x^280-17534946529*x^279+39828373720*x^278+52099668150*x^277+55016917354*x^276-27029028714*x^275+1832097078*x^274-60942810739*x^273+47857879747*x^272-51278864730*x^271-5222780191*x^270-5898650096*x^269-21678273747*x^268-21425566464*x^360+39916245264*x^359-48631849103*x^358-15434842369*x^357-11776709827*x^356+21409992004*x^355+60368342836*x^354-70458747506*x^353+18437371892*x^352-51315223539*x^351-105763188874*x^350-41326464493*x^349+26384980460*x^348-6783761138*x^21+21334689185*x^20-12549566198*x^19+27317791211*x^18-21543723029*x^168-39565879160*x^167-85203918111*x^166-6431356158*x^165+7131456890*x^164+56067799364*x^163-65758483287*x^162+14185002785*x^161+4476126737*x^160+58098059740*x^159-15413944760*x^158-21400285509*x^157+39120970417*x^156+41014228126*x^155+13550191011*x^41+1778606821*x^39-41861155597*x^186-87150315050*x^185+60380468445*x^184+11245803100*x^183-107391972561*x^182-15954816780*x^181-55185898305*x^180-51517237471*x^179+1349603646*x^178+39890682017*x^177+73058970151*x^176-88252631077*x^175+8225838344*x^463-52845822946*x^462-84438483052*x^461-17729797211*x^460-98562969717*x^459-82399019651*x^458+104524956499*x^457-126683869629*x^456+21367403421*x^455+36131797666*x^454-137807476393*x^453-94740863919*x^452+1868123354*x^10+6382151694*x^9-14641898053*x^8+21502452426*x^7-4475582501*x^154+8381680030*x^153+1999803400*x^152-89024997066*x^151+38794061551*x^150-47355124077*x^149-43574475045*x^148+981109162*x^147+13077105992*x^146-92234719034*x^145-22003948923*x^144+65545791087*x^143-21673710856*x^142-3360829230*x^3-5213505618*x^2+12467729502*x^294-5587793663*x^293+6216249569*x^292+1442921644*x^291+37867507103*x^290-31492290101*x^289-14673167737*x^288+33015189422*x^287-2704565484*x^286+95174488990*x^285-54217068575*x^284+44084204773*x^283-36577262466*x^282+10376879211*x^217-20844169749*x^216-63109698641*x^215+15376432884*x^214-21978305305*x^213+13899584470*x^212+139241561028*x^211-44664654159*x^210+2599647517*x^209+76562835916*x^208+18920601168*x^207+45165432827*x^206-13822292860*x^205-25195536685*x^204+2953366208*x^490+62275626307*x^489+44809710519*x^488+111073061078*x^487+76178250211*x^486+4726676962*x^485-16307384*x^484+25134354985*x^483+35561572012*x^482-77940816336*x^481+14820209996*x^480-8713117577*x^479+138978861424*x^478-900952782*x^17-15341289507*x^16-12806716201*x^15+9443970355*x^14-5790259982*x^51-12187496620*x^52+14358577261*x^53+16962500513*x^54+25955497814*x^55-18109191113*x^56-47942729935*x^57-25726536005*x^58-22107331394*x^59-26777218680*x^60+9526384172*x^61-2329994611*x^62-797667779*x^63+9992192122*x^64-10517453590*x^65+79225602882*x^66+37973423762*x^67-14732143550*x^68-10707214185*x^69-20734263762*x^70+32036697598*x^71-15304882544*x^72-8663047826*x^73+15796475487*x^74-15625754764*x^75-1806722698*x^76-13992042720*x^77+24071578801*x^78-1460871729*x^79-35556467385*x^80+6278675758*x^81+28441859025*x^82-21446356551*x^83-22846651633*x^84+10847836941*x^85+4499336468*x^86+128532189*x^87+4158071015*x^88+5253407021*x^89+29777404725*x^90-26059465923*x^91-40927448548*x^92-25096489123*x^93+53387092350*x^94-3391739440*x^95+19640218031*x^96-55211392254*x^97-25873270895*x^98-9727303835*x^99+106230698908*x^386-15945119461*x^385-17935823635*x^384-53556374993*x^383-109640641868*x^382-59714904671*x^381-122191901431*x^380+9957955506*x^379+149517895199*x^378+48813595534*x^377+54994058521*x^376-43911659492*x^375-40228007707*x^374+374490216*x^40+8496344002*x^38+21128346388*x^37-492653491*x^36-26347609411*x^35+92580385617*x^308+5753392383*x^307+72418711676*x^306-36955820476*x^305-2443808280*x^304+51138244830*x^303+19081895493*x^302-777541237*x^301+42995124541*x^300+43654667628*x^299-26268273836*x^298-61864683651*x^297+64621393027*x^296-51924046581*x^295-25603791079*x^141-23477310146*x^140+4635174703*x^139-2883430101*x^138+565462517*x^137+29217704662*x^136+40502985228*x^135-5190429488*x^134-815731986*x^133-18342727081*x^132+14168171180*x^131+72498842324*x^130+19120698858*x^129+3448048300*x^128-42150729253*x^244-59646966806*x^243+25136726637*x^242-58312463169*x^241+13429294186*x^240-68028026326*x^239-1855231812*x^238+67042351401*x^237-24141943642*x^236-108693143246*x^235-42010041207*x^234+48918679823*x^233+72331711491*x^232-40303836579*x^413-8185236331*x^412-90945546891*x^411-105388424000*x^410+37156268112*x^409-38869414179*x^408+87460111563*x^407+70482270872*x^406-73257224213*x^405-67165510452*x^404-45616617911*x^403+47409747378*x^402-10941088271*x^401-72305864589*x^400+18141927617*x^100-27919403621*x^267-75002469491*x^266-45875064276*x^265-18838130679*x^264+15478573827*x^263+17341560325*x^262+54857986683*x^261+88131799297*x^260-76999127565*x^259-66198972699*x^258+48106377422*x^257-77285757762*x^256+1624330315*x^255+3841588365*x^34+30013218950*x^33-5340125031*x^32-37334201612*x^127-59895558088*x^126-16505068767*x^125+33919294466*x^124+40944166758*x^123+24536748508*x^122-51786214663*x^121+49057258571*x^120+5860208924*x^119-16197153571*x^118+20745014792*x^117+1859432439*x^116+1336798052*x^115+25319297625*x^102+15443752161*x^101+15888204012*x^103+5268537528*x^254+13437751905*x^253+55130661310*x^252-56945656856*x^251-53187848971*x^250+36792199923*x^249+65889482847*x^248+54676062204*x^247+9373444102*x^246+82778707527*x^245-1155729565*x^13+10818690967*x^12-3151299763*x^11-113435197911*x^319+36460320856*x^318+31533841659*x^317+59374192238*x^316+36714124360*x^315-46466864464*x^314+57939540988*x^313-117051043786*x^312+6908217352*x^311+119695096985*x^310-37288034834*x^309+32155729060*x^31+10298011464*x^30-10817352548*x^29-4579078917*x^6+1775874428*x^5+930697362*x^4-27139276488*x^231-12815380034*x^230+109060553835*x^229-3147654703*x^228-69338296876*x^227-2295143261*x^226+88918309431*x^225-52677961098*x^224-4339428106*x^223+39692513052*x^222+2368222659*x^221-12821485264*x^220+40810501984*x^219-3135618321*x^218-3478620156+121642846747*x^501-30077710643*x^502+135206627500*x^503+80788885407*x^504+4294142336*x^505+60113761961*x^506-52525711241*x^507-67661624614*x^508-19690589512*x^509+17662010567*x^510+46484264868*x^511-49055110297*x^512+105429732598*x^513-159310387634*x^514-34712567266*x^515+17939009679*x^516+10058720567*x^517+189648210622*x^518+178503051946*x^519+40612030324*x^520+69140480072*x^521-19608267751*x^522-132451377221*x^523-101549814781*x^524+50009385834*x^525-35218480373*x^526-108625790743*x^527+18136907263*x^528-179938816243*x^529-15427215352*x^530-67053449940*x^531+47014300252*x^532-119829185605*x^533-31934518648*x^534+44603663143*x^535-51498133528*x^536-3111728028*x^537+27910834213*x^538-26950819455*x^539-15441994785*x^540-109737641439*x^541+6588852171*x^542-38378954724*x^543+4250430490*x^544+21653035413*x^545-90519813297*x^546+218486022762*x^547+15816240641*x^548+27772488593*x^549-46868203601*x^550-692131251*x^551-21710665882*x^552-3789275125*x^553+35568459021*x^554-145437381161*x^555+104188652886*x^556+11967964256*x^557+32234710722*x^558+51264850939*x^559+19578428472*x^560-6653459610*x^561+78776159254*x^562-90902552066*x^563+32028754344*x^564-16371374875*x^565+43915617220*x^566-40979866127*x^567-113279807002*x^568+41056504620*x^569+87062936546*x^570+84540227360*x^571+176182414158*x^572+126129784646*x^573-18113497064*x^574+95632973430*x^575-61835186044*x^576-1094266284*x^577-25548526606*x^578-16286188041*x^579+23652588125*x^580+99748065654*x^581+89189400499*x^582-84281188422*x^583-24822510820*x^584-89019899561*x^585+17299853927*x^586+37447146519*x^587-52389168891*x^588+60307425654*x^589+105423385125*x^590-16151503989*x^591+51669035353*x^592+25896930669*x^593+68125207997*x^594-48076475979*x^595+49080185222*x^596-114085618440*x^597+62417918964*x^598-29972805919*x^599+102994192965*x^600+9470661387*x^601-65249001543*x^602+101392509369*x^603+13718716428*x^604+38937251889*x^605-65562594382*x^606+66443969710*x^607+18877176302*x^608-31358768559*x^609+82684433809*x^610-35507835802*x^611+123781293747*x^612+31262151912*x^613-38249975546*x^614+3925377499*x^615-9663279445*x^616+65505212623*x^617-38333318816*x^618-10947904682*x^619-33057241878*x^620+137158617657*x^621+82280949071*x^622-144548051021*x^623+26049388729*x^624+120972835647*x^625+22919074287*x^626+21809122232*x^627+38807820245*x^628-35626871745*x^629+61073211657*x^630+10263794194*x^631-87779372272*x^632+122409165273*x^633-71973533044*x^634-16220803574*x^635+190684171230*x^636+39466031010*x^637-28535534784*x^638-129018222849*x^639-83527842222*x^640+12242673643*x^641-69609384363*x^642+110212358649*x^643+165584710235*x^644+83720551042*x^645-46594433050*x^646-25255661563*x^647-32840172649*x^648+38437706003*x^649+24359981454*x^650+103816698072*x^651+35342491654*x^652+99935819866*x^653-76900534968*x^654+9187154790*x^655-21467901456*x^656+11271802560*x^657+2793048339*x^658+156900676156*x^659-71757864537*x^660+69068834560*x^661-21725605511*x^662+20565641172*x^663-17160005713*x^664-28846431158*x^665-63820679105*x^666+67344279329*x^667+17417509088*x^668-49749571514*x^669-149526619276*x^670+63726307333*x^671+47868397878*x^672-152430070*x^673-98024226774*x^674-5900709150*x^675+35049382555*x^676+45151856490*x^677-74725155328*x^678+85550840075*x^679-28035942909*x^680-26105359645*x^681+8366135944*x^682-61065262686*x^683-11480596600*x^684+66582786332*x^685+31934448191*x^686-4555392418*x^687+21122866405*x^688+98576196894*x^689-67653329405*x^690+23891408534*x^691-97842890368*x^692-38880790121*x^693+31989951820*x^694-85233731747*x^695+81476023995*x^696+13150343534*x^697+34515945612*x^698-28479873758*x^699+7200965291*x^700+22962247818*x^701+31245755201*x^702-7231787902*x^703+20078549142*x^704+112814365060*x^705-37906130331*x^706+119341166775*x^707-4419273787*x^708-136935403112*x^709-42320208584*x^710-76243411849*x^711-85094494474*x^712+62345894287*x^713+123330307888*x^714-28416874429*x^715-8930496064*x^716-27860939118*x^717-67479134742*x^718+63217142335*x^719-104983862154*x^720+63261675838*x^721+29568447967*x^722+5532882614*x^723+7122613368*x^724-75015246457*x^725+27623940192*x^726-71280327669*x^727+63881717913*x^728-64602087729*x^729+55116167817*x^730-37894641441*x^731+24570439656*x^732+60506453909*x^733-56693236822*x^734+65202309407*x^735+48657462670*x^736+7020063399*x^737-14972937697*x^738+14832651654*x^739+38685418056*x^740-120977459366*x^741+128129373977*x^742-93004438817*x^743+21708662393*x^744-42256337713*x^745-74943652142*x^746+13089216312*x^747-68161772735*x^748-9184732864*x^749-36881334485*x^750+12849929766*x^751-14616276010*x^752-117623139884*x^753+85738538127*x^754-39397879206*x^755-41203911145*x^756+17752041163*x^757-28528185854*x^758-75782937817*x^759+58967984992*x^760-46501383932*x^761-30160463545*x^762+9264824504*x^763-61240651988*x^764+46332296135*x^765-36374812215*x^766-33996335903*x^767+38748904012*x^768+16692416615*x^769-24682633336*x^770-101120634096*x^771-16383524365*x^772-9803993525*x^773-41138860891*x^774+6438402836*x^775+15150248922*x^776+37535375473*x^777+38243096134*x^778+42990687315*x^779+6806571489*x^780-98981551731*x^781+48378556460*x^782-20305038281*x^783-19970849999*x^784-32199627381*x^785-33726111417*x^786+26825654676*x^787+61472162609*x^788-52262572570*x^789+6245081045*x^790+37192923285*x^791-36188152777*x^792-37778625393*x^793+26152618527*x^794-18379411727*x^795-16240247039*x^796+45087181394*x^797-124867049408*x^798-107527500523*x^799-24928581030*x^800+19049432356*x^801-47425859030*x^802+94662514506*x^803-83733870803*x^804+24856672813*x^805-94427622673*x^806+243768097*x^807+12589141374*x^808+41959344927*x^809+18983184801*x^810+42147767843*x^811+69911314756*x^812+41515466302*x^813-25611533183*x^814+20697173226*x^815-23226268622*x^816-3460976233*x^817+19330241623*x^818+53475160447*x^819-2631042112*x^820-9676147438*x^821-48282043682*x^822-90120418937*x^823+48976788717*x^824-7320635080*x^825-31580257870*x^826-40998967997*x^827-9980407645*x^828+66749588012*x^829+7524200886*x^830+14940938828*x^831-28733285670*x^832+61208670468*x^833-46627324765*x^834-324835109*x^835-104056679115*x^836+29480632050*x^837-22586403838*x^838+40186158836*x^839+15807331652*x^840-32196192662*x^841+4141096053*x^842-23428399361*x^843-68414521546*x^844-39195126416*x^845+20150874189*x^846+70929847486*x^847+15213160735*x^848-209188024*x^849-55922045928*x^850+31518449658*x^851+9332299671*x^852-61000802604*x^853+42494294179*x^854+333778076*x^855+21847051495*x^856-53208254853*x^857+41428747135*x^858-13241351753*x^859-16608043286*x^860+9621250261*x^861+25806175298*x^862-21297665820*x^863-14032790352*x^864-18013819490*x^865+15757760316*x^866+17003024240*x^867-30013032879*x^868+30760011284*x^869-13804439291*x^870-51756563292*x^871+51909198130*x^872-29495505438*x^873+7110464993*x^874-36258305774*x^875-9149267274*x^876-30428902968*x^877-48039515073*x^878-34750047014*x^879+7876564124*x^880-45028260612*x^881+29426481275*x^882+36322178879*x^883+55350571320*x^884-72386726651*x^885+23215477084*x^886-95129391342*x^887-62068317572*x^888-5131982570*x^889+25120942005*x^890+47570343070*x^891-16931626769*x^892+32866462223*x^893+75548504697*x^894-48026152317*x^895-33748968072*x^896-1682061382*x^897+8180943332*x^898+19356108993*x^899+7145333263*x^900+22433305927*x^901+18401298176*x^902-46765206984*x^903-19742304183*x^904-48845002624*x^905-27099816961*x^906+23031415252*x^907-4460429941*x^908-14418681720*x^909-41227198785*x^910-10148524063*x^911+18686856429*x^912-97172567422*x^913-9574103314*x^914-18076030731*x^915-28358519392*x^916-16581350003*x^917+12090038057*x^918+44243258246*x^919-3986576854*x^920+11411306779*x^921+17162852247*x^922+6351778392*x^923-15260387550*x^924-29138592351*x^925-14800636505*x^926+9885625539*x^927-36480762358*x^928+844036464*x^929-7561956503*x^930-28235178974*x^931-15625555530*x^932+2051786154*x^933-31812159700*x^934+13415581956*x^935+5382068286*x^936+20152707873*x^937-46081228993*x^938+17160743960*x^939-4906718548*x^940-22981161630*x^941+39189470825*x^942-15015976743*x^943+36442762021*x^944+14159701449*x^945-20259808043*x^946+19582548075*x^947+7969503721*x^948+15803199913*x^949+9091493711*x^950-11663943215*x^951-5644414749*x^952-3135411920*x^953+2212086003*x^954-3254521457*x^955-56705145197*x^956+6408789788*x^957+10381413436*x^958-3057504701*x^959+12451307686*x^960+9518096173*x^961-15016723746*x^962+37406257148*x^963+808574925*x^964+8217276211*x^965-5548816739*x^966+8580073344*x^967-16411223685*x^968+7110424112*x^969-8924388605*x^970+20244682123*x^971+16962224189*x^972+21644034171*x^973+10349693065*x^974+14406123922*x^975+17009695542*x^976+10162045861*x^977+5660460802*x^978-1036608219*x^979-1885363228*x^980-5283058429*x^981-516240895*x^982-15317530637*x^983-11663251903*x^984+4064930580*x^985+7633085681*x^986+8471058375*x^987+20912594799*x^988+3688126507*x^989+11604012893*x^990+5171125016*x^991+7684753357*x^992-2336834864*x^993+2482449646*x^994-23487081*x^995+41345539*x^996-1908649318*x^997-1563161986*x^998-3391442566*x^999-1975737484*x^1000");
  t += test_gcd ("138233755629*x^426-22168686741*x^425-65314032186*x^424+46713467112*x^423+54066842891*x^422+82543269656*x^421+176235947107*x^420+23156315980*x^419-8714217205*x^418-7917439564*x^417-20923895097*x^416-96817317174*x^415+37671252734*x^414+16053780026*x^47+26648458420*x^46+254608847*x^45-11343106531*x^174-44222442938*x^173-10656079278*x^172-54688112379*x^171-72653860271*x^170+70185903368*x^169-66162378498*x^192-19340590695*x^191-74831870843*x^190-51340722881*x^189-65948469390*x^188-40940690738*x^187-9541570627*x^24+12358437331*x^23-3840010930*x^22-26103658089*x^333+16787990043*x^332-72471206553*x^331+38195377054*x^330+48906246421*x^329+29212932541*x^328+27943555500*x^327-126086829668*x^326+45642899604*x^325+152779000312*x^324-45599025206*x^323-86920511292*x^322+30167371152*x^321+27416748135*x^320-546541724*x^113+41265938666*x^112-24708554690*x^111-20961586765*x^110+16946301630*x^109+45088470222*x^108-5349150466*x^107+15365225017*x^106-7995302120*x^105-20232813599*x^104+10081791273*x^114-3865321947*x^347-87489963865*x^346-30639453185*x^345+3676613013*x^344-58165676134*x^343-68737381572*x^342-3907635479*x^341+14873150406*x^340-5503004922*x^339-78473587467*x^338-54025110093*x^337-29151845013*x^336-67735495295*x^335+73397803045*x^334-27801675473*x^477-14039747517*x^476-8375986438*x^475+84327817349*x^474+82430096016*x^473+85036210781*x^472+74675551911*x^471-31481207861*x^470+50505881655*x^469+57661203244*x^468+1422790738*x^467+46263290040*x^466+17837475246*x^465+109465309830*x^464+5141857548*x^450-85449781066*x^449+84234728669*x^448-88234927160*x^447-127641987837*x^446-92744124462*x^445-55229182896*x^444-58231651691*x^443-71614587111*x^442-19795619691*x^441+16444963184*x^440+7188954710*x^439-21800762926*x^438+858791107*x^451-75911542139*x^373-9360344384*x^372+121593378315*x^371-13873126064*x^370+26377931906*x^369+65223112057*x^368+54567161420*x^367+81605659307*x^366+50232494189*x^365+22085432850*x^364+68840804661*x^363-15408612740*x^362+86485074789*x^361-15439204155*x^28-6661325097*x^27+6410229219*x^26+30961129925*x^25+38724464689*x^44+13389600650*x^43-4221359244*x^42-1728086134*x^399-37933493961*x^398-14669037140*x^397+23734942521*x^396-58830055830*x^395+8467094098*x^394+77771292477*x^393-89683404862*x^392-78155773051*x^391-39478631291*x^390-67987409649*x^389-64950072499*x^388-12266565939*x^387+11491110518*x^50+23873449940*x^49+22044595358*x^48+14077914429*x^202-57100831007*x^201+13655668351*x^200-22624838544*x^199-51198193976*x^198+4929392620*x^197+61541561422*x^196+58408299147*x^195-2755337167*x^194-2544113778*x^193+38364208184*x^203-39653790505*x^1196+237290860955*x^1197+1445942111*x^1198-44203650162*x^1199+82332058439*x^1200-8702521180*x^1201+70840770500*x^1202+119211377409*x^1203+62594288529*x^1204-90021858665*x^1205-129281425167*x^1206+136510590125*x^1207-32382101108*x^1208+4377395620*x^1209-94124507363*x^1210-136821711247*x^1211-62766336520*x^1212+73042766437*x^1213+170448193742*x^1214-159429807450*x^1215+23801498853*x^1216+83841365444*x^1217-2786331945*x^1218-110101444536*x^1219+120943591409*x^1220-17972193177*x^1221-17917016958*x^1222+76059557227*x^1223-26278875051*x^1224+28452314938*x^1225+59268213718*x^1226-14918040641*x^1227+214442906160*x^1228-40652938342*x^1229+96990811343*x^1230-1760397639*x^1231-21403305270*x^1232+108438376045*x^1233+11163376014*x^1234-49795674343*x^1235-155428477820*x^1236-46317266895*x^1237-21181283092*x^1238+54803863270*x^1239+77990955099*x^1240+9021816223*x^1241-81330127566*x^1242-172066502236*x^1243+38125269764*x^1244+126728193806*x^1245+20491510600*x^1246+52585146599*x^1247-188911557303*x^1248-134043853649*x^1249+67603167253*x^1250-130154245559*x^1251-55153609001*x^1252+163170176229*x^1253-4743088670*x^1254+41209412802*x^1255-10129602317*x^1256+181535124040*x^1257+56212841612*x^1258+71426935568*x^1259-30429980124*x^1260-60832906417*x^1261+67431835761*x^1262+121532246986*x^1263+33159787171*x^1264+79034947915*x^1265+12461193342*x^1266-75977547043*x^1267-43756049561*x^1268+168122514802*x^1269-48398370299*x^1270+35964935382*x^1271+54347586872*x^1272+44590944224*x^1273+10445353165*x^1274-26132065869*x^1275-47859095692*x^1276-1718582779*x^1277-41783188995*x^1279+11909834976*x^1278-77982413527*x^500-73027923452*x^499-85982752234*x^498-88590919157*x^497+18625207103*x^496-19917282031*x^495-203961285889*x^494+92315048982*x^493-113685780846*x^492-35278695612*x^491-4343198835*x+8372388799*x^437-7765335812*x^436+122688463230*x^435+143085745466*x^434-15940623925*x^433+59911209151*x^432+41198459053*x^431+86637043974*x^430-19150403305*x^429-9590187908*x^428+72843435206*x^427-18969078712*x^281-45111690065*x^280-29657990745*x^279-29667542848*x^278-14810537128*x^277+38201688736*x^276-3959944314*x^275+75115374863*x^274+13874014995*x^273+9466913592*x^272+40932968214*x^271-124744392985*x^270+42790568189*x^269+14764851450*x^268-56964896169*x^360+107236900075*x^359+64188502464*x^358-19966311943*x^357-89292693652*x^356+37459485658*x^355+9602002258*x^354+8694265442*x^353+104669007441*x^352+25291836948*x^351-18712737803*x^350+72885504388*x^349-6701955331*x^348+16019376388*x^21-10276721887*x^20+9332762048*x^19-15679749591*x^18+15800007975*x^168-43019181710*x^167+44135505581*x^166+609100023*x^165+6765959027*x^164-13557946012*x^163+4278574066*x^162+6019496346*x^161+63349714834*x^160+36279456292*x^159-8605662788*x^158-16446890130*x^157-41810449045*x^156-7095450334*x^155-4077050532*x^41+21830008852*x^39-29092609442*x^186-12826386339*x^185+6695603221*x^184-49972209582*x^183-83136366951*x^182-50423891996*x^181-18169858920*x^180-78476319819*x^179-21436351653*x^178+7919620529*x^177+28970986638*x^176+79296839091*x^175+77264613819*x^463+116466004826*x^462+55542962016*x^461-13262967705*x^460+36244684198*x^459+83226762576*x^458-64390551357*x^457-18179417756*x^456-99881894932*x^455+75031794196*x^454+26324508605*x^453-26157248749*x^452-3352003325*x^10-12675513756*x^9-15441617775*x^8-5613718699*x^7+43860635066*x^154+27940358221*x^153-2134093560*x^152+54527866704*x^151+2777626391*x^150+38378887212*x^149+49841642562*x^148-30221044274*x^147+21827366162*x^146+17014456405*x^145+30271071817*x^144-27945878711*x^143+51399953423*x^142-57512493*x^3+2646183642*x^2-20412139833*x^294-93210725051*x^293-34887615044*x^292-47910215801*x^291-6350488002*x^290-13981009447*x^289-155911910590*x^288+34327352071*x^287-43586097508*x^286-23319014164*x^285-56002153978*x^284+81803548303*x^283-71820207962*x^282-10617748524*x^217-34889926744*x^216-43806949611*x^215+12004331776*x^214+29950034237*x^213-32867273114*x^212-34298658181*x^211+21233130475*x^210-15987453386*x^209-28840589378*x^208+1865221090*x^207+5310473345*x^206+21898421919*x^205-7574068856*x^204+69079520037*x^490-77180539242*x^489-3940662000*x^488-74380408842*x^487+46029555022*x^486+22962337328*x^485-121388716462*x^484-114696481860*x^483-4968867689*x^482+69556807304*x^481+57719690408*x^480+83880745421*x^479-90995723818*x^478+16426027446*x^17-9008826521*x^16+3125040990*x^15-10087115771*x^14+7507131425*x^1364+5491038077*x^1365+23299185767*x^1366+158679796477*x^1367+124242620006*x^1368-34138235836*x^1369-104792195268*x^1370+44072214558*x^1371+30242736127*x^1372+74669209902*x^1373-28102587847*x^1374-39469546681*x^1375+52533883313*x^1376-80940926697*x^1377+78402169540*x^1378+58161497695*x^1379-78127274957*x^1380+187195727863*x^1381+34839827097*x^1382+97631469873*x^1383-132261723396*x^1384-243403867184*x^1385+101905407375*x^1386-103768707126*x^1387+140665326183*x^1388+38246904662*x^1389+94463316851*x^1390+42616536629*x^1391+75720607770*x^1392+75120834886*x^1393+57978068569*x^1394+82557166839*x^1395+14150729258*x^1396+27266620054*x^1397-18339937355*x^1398+167325111479*x^1399-172482710865*x^1400+26706898368*x^1401+49778092097*x^1402-153703751430*x^1403-61886877598*x^1404-81585576994*x^1405+19946334906*x^1406-69565555266*x^1407+94052269487*x^1408-113507716080*x^1409-17501141543*x^1410-137588116092*x^1411+165758417577*x^1412+35485042515*x^1413+61433836565*x^1414+90286860422*x^1415+2551044222*x^1416-77880307583*x^1417-50298698782*x^1418+83096470495*x^1419+115239475320*x^1420+72164396736*x^1421+189899241723*x^1422-56373869311*x^1423+97019679656*x^1424-48247967357*x^1425+154619324085*x^1426-67236039222*x^1427-47410964957*x^1428+169855658430*x^1429+30322992840*x^1430-63052501875*x^1431+222129643467*x^1432+4192274277*x^1433-75770181677*x^1434+64933458610*x^1435+77098206606*x^1436-27720674116*x^1437-5749263490*x^1438+125660847820*x^1439-34531000515*x^1440-124220079488*x^1441-138169543755*x^1442+17069521947*x^1443+47935404425*x^1444+52181081902*x^1445+16289942255*x^1446-15900660870*x^51+32923650103*x^52+27554024107*x^53+18237379971*x^54+20628678703*x^55+1475901766*x^56-15828776045*x^57+38774503478*x^58-26045359531*x^59-23045701898*x^60-24089408197*x^61+34446523234*x^62+40957087639*x^63+11389117440*x^64+9820681224*x^65+16429339120*x^66+26545401357*x^67-32362862895*x^68-26680758766*x^69-13927308596*x^70+57526789510*x^71-2089874730*x^72+77834083653*x^73+1750624845*x^74-16803601217*x^75-60811888974*x^76-30511923805*x^77-49471168139*x^78+295087634*x^79-2230079022*x^80+8101489602*x^81+1848095682*x^82-13546237573*x^83-64824054906*x^84-1604410156*x^85-13212803439*x^86-28859271440*x^87-21647178880*x^88-2728763159*x^89+20896727129*x^90+17467604913*x^91+17068807003*x^92-6893543875*x^93+47303976287*x^94+24117835126*x^95-42839904766*x^96+73128945825*x^97+16819953882*x^98+23690754989*x^99+43674412612*x^386+125292875686*x^385-6868632145*x^384+6792231538*x^383+13345729255*x^382+100308206906*x^381-36410281681*x^380+151329173416*x^379+34770326061*x^378-31189291341*x^377+41062130384*x^376+12585820391*x^375-25741536606*x^374+7011683808*x^40+3801986823*x^38-13975966697*x^37+860658280*x^36+7772384392*x^35+70631237268*x^308+8038593583*x^307+9785270382*x^306+7562096391*x^305+18097750087*x^304-36486540217*x^303+21014018569*x^302-50758761074*x^301+31281420904*x^300+53709825490*x^299+52888377853*x^298-74951274347*x^297+12921546086*x^296-76415705347*x^295+15826680805*x^141-8143429483*x^140-1266967067*x^139-52609851275*x^138-127235369220*x^137-5562487640*x^136+15459500426*x^135-34261922713*x^134-26128935470*x^133-31766069313*x^132-46271941901*x^131+11210102504*x^130-66673301013*x^129-52689509960*x^128+60511652766*x^244-75008847608*x^243-20985226623*x^242+7192516942*x^241-161582525421*x^240-49134000625*x^239-9420319932*x^238-52466552215*x^237+20737248665*x^236+4518034715*x^235-86981206669*x^234-65413662872*x^233-55595224849*x^232+48838613905*x^413-89117174044*x^412-28371635419*x^411+58369364519*x^410-63080675673*x^409-30544354398*x^408-45382170441*x^407-18873560244*x^406+18026475289*x^405-50705159627*x^404+38696002656*x^403+526588856*x^402-21240649788*x^401-43406345490*x^400+74297814205*x^100+6264961896*x^267-1021895999*x^266+86122332454*x^265+38169747213*x^264+64147932916*x^263+58378689172*x^262+24226397033*x^261-29422870170*x^260-47943224531*x^259+26452750294*x^258+105345226861*x^257+43954658692*x^256-20434482832*x^255+8127775817*x^34+30199475755*x^33-12223507015*x^32+22756827882*x^127-32719507346*x^126-49802125175*x^125+60597935861*x^124-74864424168*x^123+13477644513*x^122+5355799358*x^121+10198619648*x^120+20797815942*x^119+1011263561*x^118-6797018716*x^117-9127452274*x^116+22527387258*x^115+10256659433*x^102+29285807465*x^101+7541734764*x^103+17761344886*x^254-17286568131*x^253+25433230102*x^252-40561054425*x^251+11031323376*x^250+23702567137*x^249-60198922511*x^248+28633857562*x^247+47422980942*x^246-9295089069*x^245-10089830249*x^13-23491786545*x^12+4224016794*x^11+81467257701*x^319+63655491886*x^318-66641279796*x^317+144469815422*x^316-77290195044*x^315+70923255017*x^314+15984076870*x^313+14434564477*x^312+36604017606*x^311+103558138654*x^310-51454408044*x^309-3209161334*x^31+15201977479*x^30+28627262797*x^29-906050188*x^6-10466791945*x^5-8215574614*x^4+6088611949*x^1280-155043352048*x^1281-118198059014*x^1282+141985007241*x^1283-20206827728*x^1284-22529445083*x^1285+3357709716*x^1286+20047180952*x^1287+20996464819*x^1288-23941814014*x^1289-95188363707*x^1290+40630147116*x^1291-14052413668*x^1292-21048701821*x^1293+100140633841*x^1294+167338922803*x^1295+33226558*x^1296-128357266656*x^1297+81105983581*x^1298+31392576204*x^1299-21641561406*x^1300+235587348445*x^1301+161935696996*x^1302-128703673247*x^1303-39346114561*x^1304+42054040866*x^1305-12956319862*x^1306+65064708855*x^1307+44115412714*x^1308-39733064653*x^1309+3355869893*x^1310-161862258823*x^1311+97885854896*x^1312-102623735650*x^1313-108185508417*x^1314+108464851653*x^1315-112704576169*x^1316+131875786827*x^1317+60270345360*x^1318+20669673912*x^1319+21026875646*x^1320+75295304489*x^1321+43782162551*x^1322-61132288089*x^1323+24355164025*x^1324+63346171944*x^1325+85327679328*x^1326+97712315102*x^1327+99189458373*x^1328-160846738551*x^1329-141296521216*x^1330+20245830585*x^1331-52143723845*x^1332-76834814333*x^1333+41461601925*x^1334+18631439711*x^1335-178840949428*x^1336-66799837449*x^1337+106991716450*x^1338-153373473969*x^1339-50991501172*x^1340+136263482777*x^1341+40392134464*x^1342-83367427546*x^1343+28165179356*x^1344+120448869294*x^1345-139735638019*x^1346+55812330303*x^1347+115565807139*x^1348+80171150937*x^1349-29320222386*x^1350+85845037273*x^1351-56994419699*x^1352-1430513579*x^1353+5782046987*x^1354-38239520543*x^1355-80217085657*x^1356-66662368035*x^1357+73040143027*x^1358+31583949065*x^1359+22409165365*x^1360-52189298813*x^1361-30600519487*x^1363-160877387612*x^1362-130745878790*x^1447-164421846143*x^1448+58826335534*x^1449-121078622350*x^1450-92870208218*x^1451+63205043793*x^1452-47504622824*x^1453-40792276413*x^1454+132243469959*x^1455-50122842754*x^1456-25457647727*x^1457+75460123980*x^1458+92360457423*x^1459-47123614245*x^1460+27742628194*x^1461+70699349259*x^1462-60815737692*x^1463-169591512815*x^1464+120636922587*x^1465+2874472932*x^1466-107608428068*x^1467-5720975123*x^1468+79093930048*x^1469-56584218802*x^1470-170994826684*x^1471+72209169257*x^1472-49673733343*x^1473-90870320816*x^1474+157355941752*x^1475+110866008554*x^1476+16569857191*x^1477-30490578506*x^1478+76530403441*x^1479-162248432265*x^1480+93640590463*x^1481-15744202469*x^1482+8788690056*x^1483-70664515415*x^1484+35129616345*x^1485-39685588353*x^1486-10030759315*x^1487+88476545624*x^1488-37767780482*x^1489+85460102492*x^1490-154368097388*x^1491-101122282432*x^1492-17958917193*x^1493+15758996309*x^1494-52478307769*x^1495-40381137773*x^1496-133030420549*x^1497-24719543246*x^1498-8096296417*x^1499+101004857673*x^1500+86200685962*x^1501-25972251955*x^1502-84505928738*x^1503+4038114310*x^1504-1730887646*x^1505+34280919324*x^1506-10546638656*x^1507+33864769034*x^1508+37325029980*x^1509+52990264954*x^1510+78875435896*x^1511-23552799978*x^1512+18021688980*x^1513-70918050095*x^1514+37377330996*x^1515+147115987122*x^1516+59886319925*x^1517+69247156063*x^1518+44264194675*x^1519-5321836395*x^1520+49605413611*x^1521+16106834226*x^1522-127288542421*x^1523+10517014560*x^1524-33826730451*x^1525+17362491215*x^1526+11259519069*x^1527-111850411663*x^1528+169788136728*x^1529-61627977383*x^1530-83949340540*x^1531-203179287438*x^1532+65741933623*x^1533-43116357571*x^1534-40480690150*x^1535-41131362243*x^1536+79032181440*x^1537-59212226430*x^1538+15954047280*x^1539+58552174868*x^1540-2763240020*x^1541-70634496472*x^1542+61309662059*x^1543-116329612672*x^1544-3218593493*x^1545+88470810821*x^1546+39987733363*x^1547-110861185999*x^1548+67527613693*x^1549-5618360677*x^1550+34760731651*x^1551+101313541809*x^1552+57123487946*x^1553-73138213965*x^1554-113152834893*x^1555+85732909325*x^1556+17860086653*x^1557-112504565378*x^1558+55883545171*x^1559-65178434517*x^1560-39660841716*x^1561-63794649779*x^1562+46366489306*x^1563-13439935357*x^1564+88339338684*x^1565-57150594822*x^1566+30622063640*x^1567-16625632272*x^1568-119377667973*x^1569+224593656399*x^1570+53193487159*x^1571+65126081701*x^1572+62970399092*x^1573+68191968832*x^1574-2532739610*x^1575+182827386055*x^1576+88816579011*x^1577-89385510567*x^1578+66143786656*x^1579+126309891941*x^1580+5804094044*x^1581+97890664428*x^1582+85726994348*x^1583-23154770060*x^1584-117108787907*x^1585+13971227562*x^1586+38219978507*x^1587-29292257027*x^1588-45584106398*x^1589+226543786384*x^1590-123525218087*x^1591-106279675553*x^1592+59009279185*x^1593-21501846244*x^1594-58215906325*x^1595+10168443133*x^1596+14253963923*x^1597-103607933374*x^1598-23820552704*x^1599+73298115017*x^1600-76073336462*x^1601+27722337160*x^1602+30665618287*x^1603-40718538083*x^1604-10845646905*x^1605+103841915855*x^1606-88690958814*x^1607+15639683979*x^1608+63587256763*x^1609+79584369720*x^1610-3856065377*x^1611+3277105804*x^1612+49374804337*x^1613+22208269344*x^1614-103803342247*x^1615-18126304778*x^1616-2637468237*x^1617+1194910718*x^1618+49396303997*x^1619-59067168846*x^1620-151244829011*x^1621-61107141831*x^1622+6516627549*x^1623-46932605166*x^1624+40009442716*x^1625+10816141849*x^1626-13881407844*x^1627+19588816684*x^1628-30762755573*x^1629-3498132458*x^1630-51828207675*x^1631-143896344594*x^1632+13062299884*x^1633+118213321141*x^1634+20450936554*x^1635-22769649400*x^1636+24410873563*x^1637+61409701263*x^1638+62682111645*x^1639-12555885646*x^1640+81231121875*x^1641-87265869575*x^1642-50101835778*x^1643+23208178290*x^1644+47909319630*x^1645-56149574095*x^1646-5387312186*x^1647+45155954494*x^1648-26506247987*x^1649-24655453114*x^1650+64438306450*x^1651+933006957*x^1652+88649043790*x^1653+33722720231*x^1654-67168346690*x^1655+20490156374*x^1656+58590799138*x^1657-35113882356*x^1658+86769469196*x^1659-31677323009*x^1660+68172444087*x^1661+30384983461*x^1662+65971494796*x^1663+29114405715*x^1664-72273691319*x^1665+8637409991*x^1666+102558776528*x^1667+25534632508*x^1668-150062568831*x^1669+149103774958*x^1670+76826656126*x^1671-9206966427*x^1672+47754890342*x^1673+143906292602*x^1674-47363760887*x^1675-100347436928*x^1676+49034963661*x^1677-30032825100*x^1678+1904447368*x^1679-16465929892*x^1680+115898668860*x^1681-15427200819*x^1682+78327171484*x^1683+40114906121*x^1684-115194215170*x^1685-11250164384*x^1686-61135885162*x^1687-25229902390*x^1688-80960554583*x^1689+46627919488*x^1690+12640135253*x^1691-12106872032*x^1692+657736289*x^1693-61068471661*x^1694-69864159279*x^1695-132350791169*x^1696+98340842400*x^1697-70012959047*x^1698-2783637838*x^1699+3610440908*x^1700-47500311360*x^1701-27795498744*x^1702-8059436836*x^1703+31022917786*x^1704+11882840379*x^1705+32303942074*x^1706+85260001288*x^1707+34515935101*x^1708-48862338609*x^1709-37609344377*x^1710+26243060316*x^1711-93968692092*x^1712+7263670481*x^1713+14186751888*x^1714+50504820421*x^1715-97325627322*x^1716-7505006522*x^1717-22987526264*x^1718-25885379938*x^1719-60493345579*x^1720-35418112892*x^1721+84502334154*x^1722-76073278035*x^1723-14606929933*x^1724+66548501808*x^1725+5528600957*x^1726-8215155843*x^1727-59490241426*x^1728-28466163634*x^1729-70616897676*x^1730-8577546165*x^1731-17443872540*x^1732+45985496045*x^1733-25689081146*x^1734+31463778961*x^1735-11994818364*x^1736+26963273177*x^1737-14658991498*x^1738+3071653886*x^1739+10415294559*x^1740-11371033244*x^1741-63444243140*x^1742+6028056803*x^1743+43009638114*x^1744+134916900978*x^1745+17343194494*x^1746+80265572665*x^1747-34809795749*x^1748-105843176239*x^1749-825331310*x^1750-4839163988*x^1751-62409624690*x^1752-20792219308*x^1753-97773424039*x^1754-40261609853*x^1755-43580213332*x^1756-35959090815*x^1757-38749485599*x^1758-36327596188*x^1759-37274972348*x^1760+40676817240*x^1761+83402203312*x^1762+68572573193*x^1763-13232334596*x^1764+26432741583*x^1765-5897475378*x^1766-61972444406*x^1767+58535013523*x^1768+19602260192*x^1769-8318743400*x^1770+7803001425*x^1771+76563637063*x^1772-113144033283*x^1773-71048872512*x^1774+37755540165*x^1775+19330521255*x^1776-56127401651*x^1777+18888644242*x^1778+31817399525*x^1779+10800209238*x^1780+42691003617*x^1781+33764982495*x^1782+46697603267*x^1783-25632574756*x^1784+33073816150*x^1785+60790438629*x^1786+52911597033*x^1787+36269227256*x^1788+24513405600*x^1789-62856298604*x^1790+40347138135*x^1791+47204525731*x^1792-44078709555*x^1793-70239814448*x^1794-16761046243*x^1795-40213099308*x^1796-70370190048*x^1797+88560337870*x^1798-43789585801*x^1799-26066270170*x^1800-12513783195*x^1801+40117642516*x^1802+1078125116*x^1803+51122854580*x^1804+49031149283*x^1805-14630551725*x^1806+83787049535*x^1807+64190880486*x^1808+72057188859*x^1809+10305597348*x^1810+79560003326*x^1811+27992723763*x^1812-40123535204*x^1813+6540213580*x^1814-12201813772*x^1815-62863210776*x^1816-9530516684*x^1817+26089647341*x^1818+8240990342*x^1819-16988726374*x^1820+25502485674*x^1821+21819935364*x^1822+7578747975*x^1823-7924234392*x^1824-4583916789*x^1825+27312267438*x^1826+27028394122*x^1827+36340332694*x^1828+34569493970*x^1829-24317197250*x^1830-73040883806*x^1831+119152928326*x^1832-27999217300*x^1833-66377712512*x^1834+19049296317*x^1835-1499967276*x^1836-101411288144*x^1837-9893623936*x^1838-12904600465*x^1839-65098811535*x^1840-10374614407*x^1841-63493256502*x^1842-20925111013*x^1843-43545631503*x^1844-56380343071*x^1845-78765502946*x^1846-61513189902*x^1847-53296444805*x^1848-22832568686*x^1849+35971486958*x^1850+15588623624*x^1851+53739674006*x^1852-27920615672*x^1853+20232297980*x^1854-24748814662*x^1855+6432159679*x^1856+14056122601*x^1857+16908873134*x^1858-54999071671*x^1859-3428715644*x^1860-19509913014*x^1861-2921305636+10285270533*x^1862+54449949829*x^1863-16556984981*x^1864-45632321047*x^1865-37192326359*x^1866-89167294735*x^1867+28988566970*x^1868+33580738542*x^1869+17847650506*x^1870-2590933698*x^1871+60479028111*x^1872-74174252419*x^1873-51005953957*x^1874+34076527326*x^1875-12501722418*x^1876+11997164778*x^1877+11106537537*x^1878-67544802654*x^1879-28559326960*x^1880+25496394971*x^1881-7743517594*x^1882-60700405990*x^1883-3306265111*x^1884-52601213422*x^1885-48504437343*x^1886-33628794516*x^1887-6175399721*x^1888-42427338898*x^1889+2438088321*x^1890-10879611090*x^1891-50114291787*x^1892+8139307516*x^1893+699677484*x^1894+27742148976*x^1895+2363683066*x^1896+23142955138*x^1897-13125418109*x^1898-8068365620*x^1899-26376527681*x^1900-16879977313*x^1901-13899647882*x^1902+55501082808*x^1903+20977999945*x^1904-41959603668*x^1905+42141731465*x^1906-65252748680*x^1907-5713058482*x^1908+5531788408*x^1909+1048150468*x^1910-34295154871*x^1911+43795245653*x^1912-4033095400*x^1913+72651409*x^1914+31984617590*x^1915+71930374776*x^1916+54517887941*x^1917+63333532478*x^1918+28984482038*x^1919-3608878013*x^1920+53698106501*x^1921-6222405514*x^1922-7430102368*x^1923+1402958879*x^1924+24008303304*x^1925-26827491519*x^1926-18648926565*x^1927-3823394849*x^1928+5733309563*x^1929-6299547658*x^1930-6962395749*x^1931-40969231686*x^1932-45285051134*x^1933+10564829658*x^1934-30748565479*x^1935+26388805425*x^1936-10363567687*x^1937-2246446609*x^1938+52290201133*x^1939-17786261909*x^1940+17176068018*x^1941+64179974707*x^1942-1578020234*x^1943-2645079212*x^1944+4997898669*x^1945+10466153459*x^1946-27279279576*x^1947+13143215693*x^1948+25305770043*x^1949-7792611978*x^1950-3367810548*x^1951+23045646458*x^1952+2489207763*x^1953-9625209614*x^1954+17800677290*x^1955+26661651216*x^1956+10046368551*x^1957+42990303916*x^1958+25595010235*x^1959+10610259914*x^1960+48515299215*x^1961+18912135820*x^1962-32456695901*x^1963+43323673755*x^1964-21642505920*x^1965-52393990137*x^1966-9367854579*x^1967-10325914747*x^1968-16523379626*x^1969+22693063102*x^1970+20643925200*x^1971-19137060196*x^1972-39466187671*x^1973-22674273729*x^1974-13174940089*x^1975-28945458193*x^1976+21790522131*x^1977+17410100411*x^1978-12046307125*x^1979-1835929094*x^1980+12380357301*x^1981+373817436*x^1982+14048331587*x^1983+20998406345*x^1984-20185931652*x^1985-17385658538*x^1986-6371722529*x^1987-11640878021*x^1988+11243150741*x^1989+19803816827*x^1990+548583054*x^1991+5438200196*x^1992-640602400*x^1993-9126628108*x^1994+4447085824*x^1995-398990035*x^1996-5395080531*x^1997-2441931417*x^1998-1200282319*x^1999-655038612*x^2000+5366597811*x^231+68706406617*x^230+17345409974*x^229-48143185759*x^228+44737410494*x^227-68056841842*x^226-19799043985*x^225-49804885873*x^224+47061365105*x^223-4032111915*x^222-3623332613*x^221-23047211828*x^220-33602755354*x^219-53260548964*x^218+11121679469*x^1001+211391200411*x^1002-121806627697*x^1003-7593859366*x^1004-68089674863*x^1005-93535081213*x^1006-100904889771*x^1007+205151348397*x^1008-26911086523*x^1009+20992481200*x^1010-26975210049*x^1011+9802944823*x^1012-99339217781*x^1013-28676624812*x^1014-106268955620*x^1015-28064424851*x^1016+149985720354*x^1017-10056890284*x^1018-25972392754*x^1019-60442506971*x^1020+24876839939*x^1021-140451184537*x^1022+139436860282*x^1023+102456153010*x^1024+143113275735*x^1025-43874834829*x^1026-120467763129*x^1027+82361748186*x^1028+70759866448*x^1029-132303541720*x^1030+62109348740*x^1031-2307679959*x^1032-43220551985*x^1033-3105105156*x^501-115056774450*x^502+112590551437*x^503+5074319239*x^504-65050446174*x^505+27525467715*x^506+67043788704*x^507+73109872394*x^508+4454612231*x^509-166827879565*x^510+48004509602*x^511+80073196338*x^512+84629877684*x^513+24953136452*x^514-13404546936*x^515-60152470306*x^516+103604258158*x^517+30378727630*x^518+96105639988*x^519+86931170303*x^520+1851307686*x^521-10704467449*x^522+48099516842*x^523+25278865072*x^524-21023665046*x^525-27596977843*x^526+65317392068*x^527+56996468517*x^528-23541114280*x^529-4528634244*x^530-49979244811*x^531-1372409230*x^532+108626443622*x^533+38697103934*x^534+5617360642*x^535-31315823926*x^536+6522344835*x^537-50005336880*x^538-96453605654*x^539+28390028376*x^540+167430623705*x^541-79419733246*x^542-24583477278*x^543+63017279331*x^544-12241800025*x^545-40444796471*x^546-198616484429*x^547-9239010524*x^548+39965042629*x^549-33945600302*x^550-12524550727*x^551+3446319229*x^552-45586186264*x^553+95379898956*x^554-105411519522*x^555+28743671574*x^556-56347937800*x^557+82076691560*x^558-74986185221*x^559-53554425965*x^560-4171382516*x^561+47050272907*x^562-141507918105*x^563-68329670995*x^564-21468094592*x^565-42005013644*x^566+1988928389*x^567+89304325452*x^568+46242523185*x^569-64871208337*x^570-10239983494*x^571-70479561912*x^572+56681858933*x^573+97804124796*x^574+171255081488*x^575-130052127051*x^576-14919774499*x^577+18415158122*x^578-14270012626*x^579-9826133185*x^580+10877406520*x^581-37520045371*x^582+172353597026*x^583+190781179637*x^584+86006600201*x^585-63038348575*x^586+36324085954*x^587+25668119488*x^588-75472073699*x^589+94604145076*x^590+31201682136*x^591-66618040256*x^592-45927957789*x^593-30451201238*x^594+124888776542*x^595+106197365428*x^596-46438347803*x^597-18424519559*x^598+2639003013*x^599-27907496745*x^600-18199794457*x^601-103880772629*x^602+23137884650*x^603-1512635630*x^604-147696867223*x^605+80829019035*x^606-161525424590*x^607-100434984435*x^608+29614936899*x^609-6137679862*x^610+60715360985*x^611+34868225692*x^612-58893614200*x^613-32696951034*x^614+29320270324*x^615-87130316336*x^616+113482677724*x^617-160405479943*x^618-123613848093*x^619+93085583175*x^620+6456833758*x^621+39419082730*x^622+68534416102*x^623-25600393374*x^624+90189542563*x^625+188413710505*x^626+47761747853*x^627+133884897114*x^628+103302592307*x^629+838529232*x^630+99793611131*x^631-30322268075*x^632+111356852748*x^633+43231984135*x^634+110500432225*x^635+84573167353*x^636+437111929*x^637+88828390775*x^638+150357932904*x^639+69321584552*x^640+4998543409*x^641+77507164385*x^642-5551641217*x^643+133533084525*x^644-148039019687*x^645+34184755625*x^646+58913273467*x^647-144075969804*x^648-81457644110*x^649-88064205997*x^650-76970723677*x^651-135555167488*x^652-4963071211*x^653-71129120*x^654-25524718409*x^655+63345713205*x^656+61571118817*x^657-107047528701*x^658-16064516713*x^659-22356065855*x^660-96298139547*x^661+117212160499*x^662+23069062197*x^663+67098662581*x^664+60095764808*x^665+4311023429*x^666-13452349998*x^667+91235201366*x^668-122795422087*x^669-9005240476*x^670-99246025367*x^671+90306081579*x^672-113597846449*x^673-31798545519*x^674-71234065451*x^675+20415132246*x^676+171318088198*x^677+76440873812*x^678+86723386966*x^679+60768372398*x^680+38646393133*x^681+46034963485*x^682+161267463461*x^683-95395669596*x^684+43300902681*x^685+1364577202*x^686+89232456559*x^687+20757990799*x^688+168914572153*x^689-37448321567*x^690-15404071512*x^691-673765572*x^692-22284471241*x^693-45418634390*x^694-51127164178*x^695-54766988245*x^696+23452651883*x^697+82054856051*x^698+28585810080*x^699-128474952409*x^700-66068244980*x^701-62286757834*x^702+10446524482*x^703-23350333885*x^704-49357457852*x^705-20412291318*x^706+72438491276*x^707-9712277514*x^708+99856984639*x^709+12531900104*x^710+15860752484*x^711+92139497336*x^712-22087999262*x^713-189651791840*x^714+431932231*x^715-79802249583*x^716+31582207314*x^717+71827172753*x^718+88606853762*x^719+121148429011*x^720-20480789552*x^721-85506256319*x^722-22747058943*x^723+22272625680*x^724+75106833715*x^725-87815085198*x^726-18573784977*x^727+51790194453*x^728-34926769490*x^729+156368605389*x^730-89883516474*x^731-106204257960*x^732+126409684589*x^733+99590987979*x^734+76044515259*x^735+134493643759*x^736+80505851731*x^737-7948293309*x^738+28390988236*x^739+158320452938*x^740+45025281010*x^741+79496671226*x^742-48239197544*x^743+78128671709*x^744-250719796024*x^745-161889144663*x^746+121290695138*x^747+89224446335*x^748-12017264890*x^749+8836137575*x^750-186287680017*x^751+79370288211*x^752+81231119965*x^753+45969331743*x^754-84293927522*x^755-175094284570*x^756-77777720697*x^757+96346229248*x^758-98800364838*x^759+13503370732*x^760-117936272490*x^761-187387525590*x^762+16390728965*x^763-41826835741*x^764-108395149961*x^765-107880166963*x^766-76149324709*x^767-88276194375*x^768+33191031138*x^769-204617761996*x^770+161285750732*x^771-48140097503*x^772+11077755135*x^773+12757803021*x^774-43152311232*x^775-271935037932*x^776+1831433875*x^777+65100048254*x^778+17917219486*x^779+104345191161*x^780-19736603553*x^781-36421925355*x^782-26813686138*x^783+263563040875*x^784+192297311726*x^785-95972724582*x^786+17848985752*x^787-84432409440*x^788+111804445366*x^789+245846106331*x^790+91609765067*x^791+63679570834*x^792+10523814205*x^793-106441552262*x^794+115586926862*x^795+677538038*x^796+6853856052*x^797-39814859093*x^798+15816033482*x^799+147593524975*x^800-73234648246*x^801-53137394100*x^802+58091667262*x^803+37495621759*x^804-217416974216*x^805-66396397229*x^806-91927898947*x^807+54149805354*x^808-123901425066*x^809-51475545581*x^810-186300013422*x^811-126705120470*x^812-189510422*x^813+11338404677*x^814+6325441981*x^815+91232083427*x^816-133780522659*x^817-86298399414*x^818-44135279153*x^819+76816814544*x^820-138195966434*x^821-75684218038*x^822-126386426657*x^823-44809130231*x^824-83490163212*x^825-34351203484*x^826-41719317981*x^827-120688936869*x^828-14489273602*x^829+106470084813*x^830+41964660903*x^831+33403043858*x^832+151505022397*x^833+87862913140*x^834+120445537076*x^835-98566276077*x^836+4697632177*x^837+44046773581*x^838+73352678462*x^839-11410306650*x^840+22742711623*x^841-51216881846*x^842-147550591892*x^843+26476191234*x^844+193153996499*x^845-35927538398*x^846-6194176598*x^847-221667022042*x^848-14015578041*x^849-5698749713*x^850+26363594400*x^851+14645715378*x^852+48470068979*x^853-138256378354*x^854-132853825554*x^855+57482839352*x^856+34735643796*x^857+86752172547*x^858+60835536589*x^859-63791336423*x^860+23481544832*x^861+378282280*x^862+23239795711*x^863+88713416598*x^864-48468401808*x^865+43928927469*x^866+11773049307*x^867-62470361270*x^868+177145273813*x^869-194358054372*x^870+18849309164*x^871-61004124650*x^872-89154730183*x^873-180073144042*x^874+10857839862*x^875-11488914470*x^876-105388656993*x^877+29610973464*x^878-179112409056*x^879+27490437080*x^880+6130882402*x^881-73717395879*x^882-24727395241*x^883+6427407478*x^884+27774977309*x^885+26171001560*x^886+285300862204*x^887+250920282226*x^888+69247036204*x^889-68353373325*x^890-19660487923*x^891-199313228024*x^892+185922127103*x^893-56927953551*x^894+60519873690*x^895+77728652690*x^896-105603952672*x^897-63339878187*x^898-74519679962*x^899-45764977904*x^900+17864107983*x^901+10133013854*x^902-144667843119*x^903+33769537229*x^904+116016157352*x^905-109345484017*x^906+104654868950*x^907-961106931*x^908+31965621838*x^909-6900915942*x^910+39708679388*x^911+165967464883*x^912-13498312464*x^913-167799799320*x^914-131936289889*x^915+89614618044*x^916+85016248695*x^917-79954038780*x^918-209591894768*x^919-71794807620*x^920-23283477500*x^921+12638220555*x^922-95752230735*x^923-116899891043*x^924-101215036532*x^925-119610153404*x^926+54021790022*x^927-21789255788*x^928-260168043271*x^929+34842031670*x^930-47465526644*x^931+79209496048*x^932-162752101811*x^933-175986001160*x^934+48825705322*x^935-136513818801*x^936-52160690022*x^937+72550039192*x^938+94214685087*x^939+198826572346*x^940+57618960407*x^941-89924832368*x^942+15802881411*x^943-62911879311*x^944+32899449993*x^945+126311503110*x^946+78846277994*x^947+141308676066*x^948+70641162803*x^949+5724759068*x^950-49633865841*x^951+95173521056*x^952+161015858353*x^953-29202803241*x^954-12952363057*x^955+133465863039*x^956-87901324933*x^957+69820941995*x^958+3949578988*x^959-8912902751*x^960-227471782047*x^961-4215427616*x^962-36853070177*x^963-115632004639*x^964-137399830344*x^965+37813828221*x^966-134999621522*x^967-81129702306*x^968+2906545160*x^969+183426496219*x^970-58439170144*x^971-77898139208*x^972+27989592415*x^973-114857219664*x^974-45221435974*x^975-39064281171*x^976-144100939667*x^977+96900598155*x^978-9683692867*x^979+32505144597*x^980+75767337062*x^981+1732485201*x^982+94598116663*x^983-104431472515*x^984+45112579763*x^985-8325398155*x^986-25719591452*x^987+137274743203*x^988+63077282203*x^989+49611802496*x^990-56530688200*x^991-187825062699*x^992+63000663541*x^993+60988168565*x^994-38033047686*x^995+130178588597*x^996+38679439189*x^997+32500163168*x^998+252301658883*x^999-2212947761*x^1000+85808058970*x^1034-19483972835*x^1035-41683691103*x^1036+87145067951*x^1037-137328803807*x^1038+73968879794*x^1039+36957534556*x^1040+130660430707*x^1041-166476549509*x^1042+107108386321*x^1043+22690853232*x^1044-107648941612*x^1045-864971871*x^1046-26346541667*x^1047+10871561131*x^1048+97767553492*x^1049-32532248732*x^1050-1904971938*x^1051-103073957349*x^1052+86313670653*x^1053+62536836*x^1054-42832050487*x^1055+121155288006*x^1056+26990131150*x^1057-214613212658*x^1058+105777147715*x^1059+36621520780*x^1060+132811335476*x^1061+103729451809*x^1062-38953804569*x^1063-92616845436*x^1064-16510771977*x^1065+58930939417*x^1066+37085549174*x^1067+24888041169*x^1068-66607238646*x^1069+124192560702*x^1070-68000929614*x^1071+81195252365*x^1072+125144994277*x^1073-237062572076*x^1074+53610096124*x^1075+225331588922*x^1076-17707032826*x^1077-89973693921*x^1078+19974317710*x^1079+102021238351*x^1080-93050149638*x^1081-98539649902*x^1082-94653473795*x^1083-44306474118*x^1084+38414544973*x^1085+135628798013*x^1086-70487053525*x^1087-72092697500*x^1088-192271505385*x^1089-62340659589*x^1090+29834574943*x^1091-22322653140*x^1092-34947157570*x^1093-24287759319*x^1094-47068473067*x^1095+154054328835*x^1096+164245063199*x^1097-51273349848*x^1098-196203282068*x^1099-24096070070*x^1100-99584984772*x^1101+98129847697*x^1102-65702953785*x^1103-10789632775*x^1104+44241978999*x^1105+37558102317*x^1106-9505736359*x^1107-25199280665*x^1108+74235442860*x^1109-54694524996*x^1110+2264402295*x^1111-128996845643*x^1112-71285266617*x^1113+67661071648*x^1114-54492210583*x^1115+22513724859*x^1116+34033473659*x^1117+43758233629*x^1118-41516781021*x^1119-42784639112*x^1120+123644068072*x^1121-14247218617*x^1122+83808200085*x^1123-183298893778*x^1124-46735100971*x^1125-62031928641*x^1126-99474744881*x^1127+17800004552*x^1128+106525151324*x^1129-251246407755*x^1130-25580077568*x^1131-81843377333*x^1132+26788118746*x^1133+19182872780*x^1134-59925937362*x^1135-50058300773*x^1136-101822550837*x^1137+657933280*x^1138+19564861148*x^1139+35046428744*x^1140+2682475585*x^1141-56992102077*x^1142+35860931273*x^1143+203789134219*x^1144-6727336873*x^1145-109903742856*x^1146+47719560439*x^1147+13527975161*x^1148+98260043934*x^1149-58366673275*x^1150-130127164351*x^1151+83455071177*x^1152-68823162825*x^1153-60225615746*x^1154-74042831871*x^1155-12014905342*x^1156+23388731515*x^1157-8626013014*x^1158+21953195679*x^1159-155980220910*x^1160-217972125487*x^1161+89682346870*x^1162-37658668031*x^1163+58027967881*x^1164-90378430633*x^1165-43341202030*x^1166+16523493943*x^1167-18906640960*x^1168+212117365642*x^1169-33986370441*x^1170+45185768856*x^1171+4763319487*x^1172+1371747234*x^1173+148876972098*x^1174+95191837796*x^1175-123809421852*x^1176+79843098120*x^1177-41267372840*x^1178-121155059136*x^1179-65482206391*x^1180-79113584784*x^1181+11116916638*x^1182-92738566955*x^1183+3388410239*x^1184+19005832198*x^1185+66751715230*x^1186-96920966455*x^1187-184353824811*x^1188+29600147939*x^1189-36474471319*x^1190-75094557509*x^1191-55155813058*x^1192-139796362527*x^1193-36990042600*x^1194-149308311610*x^1195",
                 "-12298243395*x^426-33246261919*x^425+14881216216*x^424+100717315398*x^423+32435341799*x^422+11306102069*x^421-42857343048*x^420-23027068074*x^419-121525059139*x^418+41062828633*x^417-9120088623*x^416+38127915034*x^415-53332525642*x^414+10667345234*x^47+20250868505*x^46+28797169308*x^45-43235989359*x^174+38758957270*x^173-29243855534*x^172-46185370245*x^171-47223973616*x^170-10368178271*x^169-10557528292*x^192+37487979771*x^191-16245699850*x^190-55729045478*x^189+36012407938*x^188+72614386131*x^187-15204240841*x^24-8711949900*x^23+14257174071*x^22+56377120052*x^333+14839742003*x^332-38325417931*x^331+54176364398*x^330-53620700530*x^329+17198633981*x^328-15450486288*x^327+15372974977*x^326+5921781285*x^325+81501682291*x^324-66626158040*x^323-20609502025*x^322-109523674165*x^321+56875705806*x^320-23244792061*x^113-19309284277*x^112-38916326672*x^111+44091306530*x^110-34827830974*x^109+32699097269*x^108-72721416508*x^107+17265636833*x^106-36617312032*x^105-41573561571*x^104+12279258834*x^114+82436036606*x^347-31261753091*x^346-44500371183*x^345-125274002688*x^344-31133631871*x^343+128460249050*x^342+100179005762*x^341-114828839354*x^340+108064336275*x^339+90994968967*x^338-44508032271*x^337+85772561126*x^336-14871801916*x^335-21281913217*x^334+17047025764*x^477-26275017578*x^476+101182365586*x^475+88231498193*x^474-59806242426*x^473+113427095614*x^472+116163017856*x^471+53708041635*x^470-154328984869*x^469+56803301458*x^468-35861674205*x^467-88315900409*x^466-33792693422*x^465-132786811376*x^464-109585745531*x^450+37453216778*x^449+2927086149*x^448-59663587909*x^447-99229589433*x^446+22434345521*x^445+3804407873*x^444-22843316212*x^443+15023409723*x^442+79744521756*x^441-89156003460*x^440-62409603681*x^439-22865453841*x^438-62845550343*x^451-10075612299*x^373+90241752514*x^372+3172648684*x^371-64532174127*x^370+72446697772*x^369-44304010100*x^368-94564984737*x^367-19761536748*x^366+26376102015*x^365-158229005056*x^364-62154676691*x^363-40346077174*x^362-33481542412*x^361+13571997673*x^28+8161213787*x^27+1023845397*x^26-16720514075*x^25-2383844588*x^44+8356875255*x^43-26827438588*x^42+44585618338*x^399+37434341256*x^398+48503836694*x^397+64340759602*x^396+70497118845*x^395+58332730505*x^394+37917909552*x^393-17951385993*x^392-72126324042*x^391+83367616252*x^390+54914454358*x^389-80690399086*x^388+3788723413*x^387+6657597500*x^50-27587838729*x^49-11015114987*x^48-62834670333*x^202+14506585098*x^201+6123240614*x^200-23983987723*x^199-8895963188*x^198+74318166429*x^197-59835752920*x^196-29296586194*x^195-28410809765*x^194-57005146105*x^193-58918383547*x^203+10630344160*x^1196-18556978555*x^1197-164043335170*x^1198-87839930075*x^1199+4152300286*x^1200+99517087469*x^1201-73797793712*x^1202-25260491411*x^1203+76114220378*x^1204+40133672638*x^1205-230680916222*x^1206+219293086650*x^1207+3638620539*x^1208-6652308741*x^1209+12775249723*x^1210+3251379936*x^1211-75721519451*x^1212-163406006228*x^1213-165491770209*x^1214+46054734359*x^1215-741970731*x^1216+99721318208*x^1217-48283084345*x^1218-201279475291*x^1219+57418480238*x^1220-134680981668*x^1221+34536313632*x^1222+10430882187*x^1223+70914285740*x^1224-108730001945*x^1225+184561606957*x^1226-50600427283*x^1227+27438909544*x^1228+3869947176*x^1229-99212395993*x^1230-122356266132*x^1231+190721719645*x^1232+10663679376*x^1233+171017341569*x^1234-61192181164*x^1235+100411947630*x^1236-85397438998*x^1237+106275225374*x^1238+3419807107*x^1239+36976923465*x^1240+57882249834*x^1241-107227057330*x^1242-45892223123*x^1243+39898058432*x^1244-25685770417*x^1245-15437084634*x^1246+79678067292*x^1247+53459571441*x^1248-9772387681*x^1249-8634944328*x^1250-103233454566*x^1251+80654434021*x^1252-30348898500*x^1253+31622090094*x^1254-59747674770*x^1255+50418039889*x^1256-131270697937*x^1257+73386317355*x^1258-33268294896*x^1259+220123546149*x^1260-47814110905*x^1261+31648485555*x^1262+50736345265*x^1263+12957979612*x^1264-12352680940*x^1265+8954284112*x^1266+4405063605*x^1267-66479748368*x^1268+98898741778*x^1269+137071640600*x^1270-13677831086*x^1271-32633620324*x^1272+100644420930*x^1273-101006678220*x^1274+57929446438*x^1275-46456850356*x^1276+119383219795*x^1277-68889453480*x^1279+27403268808*x^1278-9452745528*x^500+12505989143*x^499-88746500517*x^498+36522434736*x^497+105148848765*x^496-31099548526*x^495-29920704228*x^494-84722298862*x^493-14005980691*x^492+18668364817*x^491+2284051770*x-33852801349*x^437+24746815214*x^436+14725842338*x^435+71589114295*x^434+95344800351*x^433-103870206918*x^432-2292807366*x^431+122702353734*x^430-39127758310*x^429+71141989765*x^428+91775420843*x^427+29584402698*x^281+34879992632*x^280-7929148263*x^279-62511168551*x^278+75173708697*x^277+2210745302*x^276+47365447862*x^275+34770067078*x^274+15629493834*x^273+123387217284*x^272-83879189754*x^271+90474389247*x^270+20723731518*x^269-13648232822*x^268+135048430531*x^360+9779963494*x^359-79763997627*x^358+80167613186*x^357+59427033891*x^356-56617078458*x^355+70792836473*x^354-42721386790*x^353+18854232633*x^352-27799012434*x^351-7540762106*x^350+51000825517*x^349-61789935211*x^348+6364435339*x^21-8596138092*x^20-13862640657*x^19+12972909594*x^18+25640262624*x^168+73106364011*x^167+50996630476*x^166-46522919587*x^165-2318061818*x^164-46439765345*x^163+3018635654*x^162-51033321714*x^161-54420823241*x^160-25754428117*x^159-20957251811*x^158-56979883142*x^157-113069491386*x^156+78310111278*x^155+8791974299*x^41-20484038584*x^39+65329554945*x^186+63604634904*x^185+67548191757*x^184-46287362343*x^183+86853548227*x^182-78112812263*x^181-39054030995*x^180+70082669285*x^179+4991564873*x^178+35132330683*x^177+5220672794*x^176+27538233543*x^175+45604753836*x^463-72517449336*x^462-172648125640*x^461+77371325704*x^460+103318563842*x^459-47970420952*x^458-173692529*x^457-16267041286*x^456-39608578784*x^455-44014507517*x^454-40698179828*x^453+64997452090*x^452-16621185638*x^10-5839518904*x^9+7779380684*x^8+7805287316*x^7-931618705*x^154+77399210009*x^153-12286080400*x^152-15618003169*x^151-44221287122*x^150+44169698801*x^149-77234959789*x^148+19578529715*x^147-2795707215*x^146-11375091988*x^145+21793365806*x^144+31454951276*x^143+6091116508*x^142+7402060559*x^3-15638332394*x^2-16846517099*x^294-6573457577*x^293-36345200919*x^292+104115784551*x^291-32337698003*x^290+17593183564*x^289+90879035663*x^288+124574515327*x^287-2750093470*x^286+74836130434*x^285+28760033840*x^284+76932285409*x^283+70510978158*x^282+2131234223*x^217-47160319189*x^216-4500367537*x^215-10694873972*x^214-13052396086*x^213-119312329735*x^212-12249530649*x^211-58377372451*x^210+61312423521*x^209-62735937652*x^208-18739775224*x^207+19541226826*x^206+52687803759*x^205-48487381888*x^204+49838817064*x^490+103336940622*x^489-114398006939*x^488+40164473234*x^487+22188458973*x^486+102241404959*x^485-132777915090*x^484+77723628319*x^483+29176152754*x^482+116603267994*x^481-14526032223*x^480-45775708833*x^479-267391385*x^478+22100788607*x^17-11428840425*x^16-954947579*x^15+6057723695*x^14+63494717499*x^1364-10349348333*x^1365-11354317920*x^1366+90668431927*x^1367-92110654585*x^1368+231933389918*x^1369+19189606214*x^1370+37836457811*x^1371-41835147412*x^1372-64829306413*x^1373-99539521475*x^1374+14786388671*x^1375+213836900645*x^1376+22624850100*x^1377+4395435896*x^1378-93161939597*x^1379+89789272863*x^1380-63671202085*x^1381-18208723976*x^1382+102193294445*x^1383+42149570567*x^1384-57427709585*x^1385-101096944615*x^1386+62581117784*x^1387-58899807423*x^1388+115177108233*x^1389-19168792202*x^1390+38132618085*x^1391-50907816003*x^1392-23737421785*x^1393-65750929733*x^1394-8568961669*x^1395-37662793882*x^1396+101039333860*x^1397-47341688773*x^1398+9724870415*x^1399-15791202897*x^1400+13843025496*x^1401-128893801207*x^1402-32183995709*x^1403-139023659805*x^1404-42684304557*x^1405-14610977549*x^1406+57122422992*x^1407+128491635951*x^1408+3614885611*x^1409-36404761221*x^1410-112811070097*x^1411+153443879256*x^1412+164794102332*x^1413+134470678635*x^1414-88690316169*x^1415-74374605027*x^1416-90950545612*x^1417-104814317576*x^1418-177228350594*x^1419-11835540964*x^1420+20873445658*x^1421-195114255686*x^1422+87306904630*x^1423+59376189861*x^1424-117251826188*x^1425+57626371798*x^1426+17253066619*x^1427-65717447508*x^1428+38123913202*x^1429+64257781929*x^1430-110886945374*x^1431+70757769597*x^1432+10322610698*x^1433-48134918817*x^1434-56503764527*x^1435+117474984824*x^1436+33765540181*x^1437+107731007106*x^1438+96338855448*x^1439+8934843329*x^1440-30541434613*x^1441+72531211591*x^1442+30197093191*x^1443+72368177095*x^1444-76451969600*x^1445-92672018447*x^1446-1744298703*x^51+26882297486*x^52-18398383509*x^53+3005642217*x^54+3593805670*x^55-21740657388*x^56+6314837654*x^57-14388840020*x^58-48213795147*x^59-11593118081*x^60-1502891403*x^61+37639792348*x^62+4590584128*x^63+62090294921*x^64+31955540056*x^65-8921556943*x^66+21988615968*x^67-23025433031*x^68+38334620159*x^69+19779920702*x^70-16146115882*x^71+18104295332*x^72-20379887728*x^73+28122778533*x^74-22233760331*x^75+2601350324*x^76-23301155844*x^77+27813631959*x^78-37895927213*x^79-2648750595*x^80+24636697824*x^81+17895391597*x^82+8568029391*x^83+36393556404*x^84+17452747156*x^85-49612336092*x^86-10886997637*x^87-11130667420*x^88+29158789170*x^89-5400236199*x^90+77812065577*x^91-49643768511*x^92-13107221267*x^93-52311659384*x^94+34469782323*x^95-35571894501*x^96+53972502564*x^97+30760824828*x^98+100388019377*x^99+97252215701*x^386-116099602858*x^385+76103387516*x^384-11150909008*x^383-43760747755*x^382+30435548717*x^381+105763654230*x^380+62286795532*x^379+76050666216*x^378+37247029295*x^377+2903246935*x^376+17740821225*x^375-67645627457*x^374+1068927231*x^40-21411907744*x^38+30329881614*x^37-30031713761*x^36+18856879565*x^35+28958849017*x^308-11122674865*x^307+16515984422*x^306-44335554221*x^305+54476123312*x^304-20904234081*x^303+61391311771*x^302-16775061346*x^301+3210400498*x^300+56658482015*x^299-105337656712*x^298+33759085293*x^297+61939776273*x^296-41177733805*x^295-59566196142*x^141+18476269572*x^140-23088141060*x^139-11801865656*x^138+30847010853*x^137+21929475729*x^136+31313930007*x^135-2990246614*x^134-14929686587*x^133+38476911951*x^132-46205516843*x^131-13904775963*x^130+10524952653*x^129+40066745986*x^128+1970323384*x^244+1446777973*x^243+2391572443*x^242-89493321216*x^241+39101186256*x^240+98222226415*x^239-9239552468*x^238+5916325669*x^237+36005795144*x^236+45537168101*x^235-40607567937*x^234-3750879706*x^233+57951296798*x^232-7755123318*x^413-37469436811*x^412+70016616413*x^411-42829701116*x^410-40550876166*x^409-36147696900*x^408+65981234023*x^407-122609177512*x^406-32890978471*x^405+89421010177*x^404+24395518617*x^403+97415643004*x^402-61366468342*x^401-68686837546*x^400-24197772525*x^100-55496272864*x^267+11842466716*x^266+11739104212*x^265+45395994939*x^264-2462010012*x^263-46901504750*x^262+2194886178*x^261-43818646770*x^260-64456303482*x^259-42678178437*x^258+16930473337*x^257-79111479558*x^256+55486339504*x^255-6531034828*x^34+27035707153*x^33-29177734926*x^32-102519500337*x^127-25120666485*x^126-30579535492*x^125+9392730819*x^124-19261680565*x^123+9092366703*x^122-13808837057*x^121-10767506476*x^120+9387090911*x^119+5861529627*x^118+30570618150*x^117+9821369125*x^116+27953902351*x^115+25585821707*x^102-9147523101*x^101-43069458046*x^103-77197009243*x^254+41768972254*x^253-51107733143*x^252+2179732347*x^251-73246837376*x^250-76028238883*x^249-15944993585*x^248-122980270638*x^247+29301117006*x^246-10775309722*x^245+18396353043*x^13-16862760430*x^12+15452547818*x^11-10527226187*x^319+101533968365*x^318+45962612479*x^317-40854272816*x^316+29357583774*x^315-98334493728*x^314-26844181578*x^313+152535370585*x^312-48558849904*x^311-65465771182*x^310+48084068810*x^309-798955677*x^31+35715352173*x^30-41114886366*x^29-10576978086*x^6-11825376542*x^5+7286512470*x^4+18388806457*x^1280+161794076352*x^1281-28337710306*x^1282-17255976845*x^1283-76008915764*x^1284+55400148387*x^1285-60639021891*x^1286-101066145720*x^1287-123972185159*x^1288+87115039832*x^1289-155574783014*x^1290+23730856463*x^1291+309102172*x^1292-106791492726*x^1293-237381096018*x^1294+28548371390*x^1295-330246556*x^1296-29689811838*x^1297-189254819970*x^1298-30865123604*x^1299-44582502556*x^1300-111505655799*x^1301-44557592539*x^1302+53733507680*x^1303+20819197021*x^1304+1395361755*x^1305-49184624216*x^1306-40903280869*x^1307-93317035741*x^1308-28525136465*x^1309+46815641959*x^1310-37147646401*x^1311-171269226982*x^1312+55545463284*x^1313+26432914392*x^1314+63358274704*x^1315-21229429338*x^1316-128600950288*x^1317-42113338606*x^1318-140824193885*x^1319-2326798220*x^1320+64192104658*x^1321-51630561209*x^1322-49498217773*x^1323-90416752098*x^1324+83991720563*x^1325+150608820483*x^1326+65119613397*x^1327+20967841540*x^1328-95934246964*x^1329-177636442910*x^1330-104683025381*x^1331+46752411240*x^1332-71028555511*x^1333-80536733392*x^1334-60889544079*x^1335-22450515462*x^1336-61149950566*x^1337-23445736673*x^1338-50742295044*x^1339+41001013374*x^1340+94763809142*x^1341+77883336357*x^1342-53873424849*x^1343-85586877816*x^1344+147245896720*x^1345+103273157293*x^1346+49487544766*x^1347-129528420075*x^1348-61945581330*x^1349-76408360080*x^1350+84312569941*x^1351+3005072928*x^1352+21564682198*x^1353-99865781821*x^1354-96904486038*x^1355-67290910420*x^1356+121809010991*x^1357-201280052*x^1358-25659166014*x^1359-7854234250*x^1360+83496185253*x^1361-156457302131*x^1363-130043471771*x^1362-116623034666*x^1447+63340754284*x^1448+29056858824*x^1449-14463741969*x^1450+101462718016*x^1451-69374438503*x^1452-104332978401*x^1453+14930947674*x^1454+32883271359*x^1455+37930611868*x^1456-16920913941*x^1457-80230082425*x^1458+46610161083*x^1459+19972507802*x^1460-59510262751*x^1461+10916713060*x^1462+57866464903*x^1463-10394342923*x^1464-90970881156*x^1465+37858629616*x^1466-106679590218*x^1467-24913741790*x^1468+109967614373*x^1469+48890249817*x^1470+1848149679*x^1471-98549032445*x^1472+60720768502*x^1473+137842172780*x^1474+5243491906*x^1475-59750299327*x^1476+45228722812*x^1477+27935880244*x^1478+80537672690*x^1479-20295991643*x^1480-87845315347*x^1481+50218956865*x^1482-38373564442*x^1483-20957663060*x^1484+27711035106*x^1485+95016265056*x^1486+55710516985*x^1487+82619750454*x^1488+110859243542*x^1489+53435806482*x^1490-133528838222*x^1491+158330827120*x^1492-97083117015*x^1493+111387362388*x^1494+85479818296*x^1495-10482105703*x^1496-2572665530*x^1497+133068208484*x^1498+4146821725*x^1499+68330996935*x^1500-28065751920*x^1501+15974361469*x^1502-85929799702*x^1503-28686472019*x^1504+148667781296*x^1505-10084131821*x^1506-94793837277*x^1507-84791761144*x^1508-63678878999*x^1509+39694015413*x^1510+446659189*x^1511+95377871547*x^1512+9666212199*x^1513+161795671654*x^1514-91000484021*x^1515+145579875230*x^1516+81265093794*x^1517-998831496*x^1518-128803139300*x^1519+52770776790*x^1520-146403349418*x^1521+32587886089*x^1522-75251805801*x^1523+166805166779*x^1524-57035303250*x^1525+57521577337*x^1526+21285234901*x^1527+3027584246*x^1528+105899112243*x^1529+14781758392*x^1530-22683394179*x^1531-25210689640*x^1532+99935945643*x^1533-84021930147*x^1534+42259497554*x^1535+45907068922*x^1536+84279071229*x^1537-29539017121*x^1538+45338942887*x^1539-24806480906*x^1540+87977283203*x^1541+62516484079*x^1542+159450661656*x^1543-23655833088*x^1544-44112269864*x^1545+10606111566*x^1546-8499088313*x^1547+96101127142*x^1548-89377300943*x^1549-59851591087*x^1550-56214120248*x^1551-157507855204*x^1552-62249119108*x^1553-85445577597*x^1554-4401721329*x^1555-101536174643*x^1556-58119121629*x^1557-114925796667*x^1558+48574709630*x^1559-34280314464*x^1560+14766752770*x^1561-30236783234*x^1562-14509377591*x^1563-209749274080*x^1564-37393636602*x^1565-59883917907*x^1566+90592372530*x^1567-125327647858*x^1568-144372630004*x^1569+17757782785*x^1570+32530054069*x^1571-103160474054*x^1572+27004259534*x^1573+36592332602*x^1574-49900527049*x^1575+109450983286*x^1576+87454580007*x^1577-22429929394*x^1578-90955942668*x^1579-52085998619*x^1580+7542444291*x^1581-129095640958*x^1582-116117273631*x^1583-15362293016*x^1584-75422573875*x^1585+104080240756*x^1586+29567135838*x^1587-85533231646*x^1588-14007407713*x^1589-72167537141*x^1590+15578436550*x^1591+37099609795*x^1592+5366651746*x^1593-80006924851*x^1594+21429342288*x^1595+30269096496*x^1596-56303168087*x^1597-32192045909*x^1598-24090181314*x^1599-91201040971*x^1600-90424483245*x^1601-41789191466*x^1602+48796472891*x^1603+6667317240*x^1604-20054404612*x^1605+90717205115*x^1606-45574268144*x^1607+96577965375*x^1608-84937868700*x^1609+15254114769*x^1610-85324218128*x^1611-36100598429*x^1612+59140120545*x^1613+28348387068*x^1614-25936772983*x^1615+14152778910*x^1616+93013422289*x^1617+5325703012*x^1618+5299012928*x^1619-114456880810*x^1620+24631873118*x^1621+71187736583*x^1622+44454005446*x^1623-65564007677*x^1624-18686868545*x^1625-38320190467*x^1626-11068635951*x^1627+38623912436*x^1628+128145741026*x^1629-47444447640*x^1630-69911372525*x^1631+73982054085*x^1632+67625321661*x^1633-43780921866*x^1634+42499209425*x^1635+112438203531*x^1636-29637261064*x^1637+29717059942*x^1638+73064218527*x^1639+78354730927*x^1640-23583391619*x^1641+51406219011*x^1642-49806860311*x^1643-22338703465*x^1644-37561475541*x^1645+87296115958*x^1646-25055231815*x^1647-26132634595*x^1648+51665990532*x^1649-172652705651*x^1650-104663637871*x^1651+11737785837*x^1652+47284031389*x^1653-125440895392*x^1654-42350923347*x^1655+22245934791*x^1656+50062160311*x^1657+26993180274*x^1658+93916764416*x^1659+17723933407*x^1660-7709629970*x^1661-10925189482*x^1662-87095313733*x^1663-24424934930*x^1664+138103101710*x^1665+9725815469*x^1666-95975878178*x^1667+111443666516*x^1668-96028904366*x^1669-61862950969*x^1670+20517212289*x^1671+85165260123*x^1672-168755515234*x^1673-45383540058*x^1674-14246010186*x^1675+5321412602*x^1676-97837120785*x^1677+63051474234*x^1678-11622330015*x^1679-6813440517*x^1680-158000336988*x^1681-61045444490*x^1682-16113615735*x^1683+42790018648*x^1684-55289921194*x^1685-55354435735*x^1686+7663298003*x^1687-75007919562*x^1688+155177776611*x^1689+58476171975*x^1690+2632834366*x^1691+18899732593*x^1692+26511602610*x^1693+105064498377*x^1694+61127828457*x^1695+6326945505-959052659*x^1696-73433944554*x^1697+17547912794*x^1698-54354897407*x^1699-58838742934*x^1700+94679918763*x^1701+42039556955*x^1702-130770900613*x^1703-54147770285*x^1704+31508315675*x^1705+18763864996*x^1706-22723355698*x^1707+23015897095*x^1708-73252327021*x^1709-66778379186*x^1710-33519375139*x^1711+8936095442*x^1712-54146328075*x^1713+3330235304*x^1714-19390098322*x^1715+48324815570*x^1716-22141912841*x^1717-37722070562*x^1718+26839695493*x^1719+76090925762*x^1720+88225567190*x^1721+47256653156*x^1722+30627370862*x^1723-63714216871*x^1724-18205624350*x^1725+59802680427*x^1726+53057549555*x^1727+14174441194*x^1728+75765825464*x^1729+32963108753*x^1730-12499134875*x^1731+27273333256*x^1732+9234764883*x^1733-22990113287*x^1734-30534003983*x^1735+5390988324*x^1736+51965774003*x^1737+36123492542*x^1738+23495532313*x^1739+24538679309*x^1740+48466824303*x^1741+25810926063*x^1742-12309341239*x^1743+71863273789*x^1744+27521706644*x^1745+51703817418*x^1746+31047946433*x^1747-17225947158*x^1748+113616875883*x^1749+48119672693*x^1750+32640697894*x^1751+5838339810*x^1752+35494583363*x^1753+35996883701*x^1754+64454412007*x^1755+67672887170*x^1756+24852637977*x^1757+39599198948*x^1758+100612424867*x^1759+61066112206*x^1760-47730662897*x^1761-75220323201*x^1762-7637941778*x^1763-13208589131*x^1764+55098518018*x^1765+102817175067*x^1766-46374680712*x^1767+7922233525*x^1768+12354251760*x^1769-17497361436*x^1770-59990480700*x^1771-2475917864*x^1772-53137166580*x^1773-65697709393*x^1774+8377278120*x^1775-39634303915*x^1776+27494498566*x^1777-6623951999*x^1778-2860049814*x^1779+45174937843*x^1780+19218150182*x^1781+31009745617*x^1782-57914680622*x^1783-93027860213*x^1784-32466824482*x^1785-87664162092*x^1786-40426743792*x^1787+22191823745*x^1788-4385019263*x^1789+40619736886*x^1790-59537851776*x^1791+29407007656*x^1792+25083658856*x^1793+16109214404*x^1794-2628597691*x^1795+90593408292*x^1796+73529865188*x^1797+69070203891*x^1798-22715686937*x^1799-47275795580*x^1800+24349081011*x^1801+37519310529*x^1802-30466082586*x^1803+16849925300*x^1804+52411382847*x^1805-49758979601*x^1806+15679528686*x^1807+76574645775*x^1808+46732065431*x^1809-67248446805*x^1810-41574164595*x^1811-45948531070*x^1812-91525781500*x^1813-98636828248*x^1814-62341009634*x^1815-106737947357*x^1816+5944715919*x^1817+27959962466*x^1818-18281456838*x^1819+43419080798*x^1820+8238895067*x^1821+57794363933*x^1822-3885695019*x^1823-16620769333*x^1824+20617426129*x^1825+31772378717*x^1826-77659311932*x^1827-2614944050*x^1828-53541082178*x^1829+47415264521*x^1830+19297049465*x^1831-50744052035*x^1832-14699341644*x^1833-20185897392*x^1834+11304473461*x^1835-8417793205*x^1836+42208138630*x^1837+40010050281*x^1838+8580892377*x^1839+744941642*x^1840+13372194202*x^1841+14518343301*x^1842-12610498299*x^1843-19180304452*x^1844+25853574415*x^1845-39663241273*x^1846-51735769060*x^1847+28701486795*x^1848+34631767452*x^1849+30665603097*x^1850-31737130343*x^1851-11359635277*x^1852-53356680586*x^1853+34804124525*x^1854-29905007001*x^1855-16020507432*x^1856-11717124778*x^1857+34454112900*x^1858-29236460046*x^1859-27386914094*x^1860+48525768547*x^1861+28213658802*x^1862+8947787875*x^1863+57150247624*x^1864+8348708750*x^1865+8808792586*x^1866+25213586793*x^1867+56635111548*x^1868-9337443075*x^1869+21165923060*x^1870+18588461938*x^1871+30858411086*x^1872+3419917365*x^1873+77549765533*x^1874-7998236281*x^1875-19374386693*x^1876-35418234974*x^1877+38577714149*x^1878+15358209695*x^1879+34405664552*x^1880+4306560036*x^1881-60158002*x^1882-22204346895*x^1883+9575772188*x^1884+38275674214*x^1885+76935802774*x^1886-8688725759*x^1887+4641950168*x^1888+26752524534*x^1889+11976777389*x^1890+47203277801*x^1891+34427926908*x^1892-10684178562*x^1893+11127497799*x^1894+42062545265*x^1895-53825407401*x^1896+4989669095*x^1897+55453648034*x^1898+20983145705*x^1899-14958562665*x^1900+8355251080*x^1901+24031795321*x^1902-29739158452*x^1903+21980289986*x^1904-17205919827*x^1905+7346027701*x^1906+11569319363*x^1907+13614834645*x^1908-35975188961*x^1909+46603311768*x^1910+30120456597*x^1911-8478220385*x^1912-56827863625*x^1913+30296155243*x^1914-36287579039*x^1915-24672161998*x^1916+24469720179*x^1917-35478863962*x^1918-54234895242*x^1919-66565864759*x^1920-40947685264*x^1921-19971682562*x^1922+2385658644*x^1923-29296192350*x^1924+1423940493*x^1925-10597630015*x^1926-20525914191*x^1927+2929730657*x^1928+16463714177*x^1929+12681038897*x^1930-19145947289*x^1931-13466142524*x^1932-20222110298*x^1933-19251726839*x^1934-37002885076*x^1935-7802339043*x^1936-47523780291*x^1937+8321382372*x^1938+13992616571*x^1939+26068955056*x^1940-8086679459*x^1941+286270153*x^1942-2705340674*x^1943+255535813*x^1944-34493130463*x^1945-8359391510*x^1946+32075412687*x^1947+3157786517*x^1948-5134583707*x^1949+20294329376*x^1950-5719300052*x^1951-3610361046*x^1952+19714585416*x^1953+31029310975*x^1954+6525668645*x^1955-17551589763*x^1956+6326740399*x^1957+4078698282*x^1958-19314799201*x^1959-3581682397*x^1960-18696461472*x^1961-45787235498*x^1962-39343868284*x^1963-25552626240*x^1964-47676215824*x^1965-6535189504*x^1966-20440671693*x^1967-27850667563*x^1968-2284895619*x^1969+12350652323*x^1970+10616601145*x^1971+34114023739*x^1972+31816702425*x^1973+26974067096*x^1974+29215489795*x^1975+23576334729*x^1976+34095198361*x^1977+21863597299*x^1978+23856960019*x^1979+25344212108*x^1980+15345894525*x^1981+2452061679*x^1982-7870355413*x^1983-19430775349*x^1984-11433900994*x^1985-14519775288*x^1986-12549755734*x^1987+1850026837*x^1988-7355516784*x^1989-5810824702*x^1990+512283203*x^1991-246928108*x^1992+2728608*x^1993-1528915243*x^1994-3274467887*x^1995+111483392*x^1996+204389413*x^1997-1867708432*x^1998+274767516*x^1999-407080161*x^2000+9253740790*x^231+13104137650*x^230+34300405699*x^229-8555184450*x^228+43336832551*x^227-56284596497*x^226-25147990866*x^225+60729543880*x^224-33106841378*x^223+6879339359*x^222+66221993617*x^221+78495102233*x^220+59060525921*x^219-45581015054*x^218-32695677520*x^1001+5858837086*x^1002+11101676909*x^1003+54178409615*x^1004-102677317737*x^1005+49344188139*x^1006-51959708001*x^1007+105417515933*x^1008+46478254597*x^1009+128535749044*x^1010-94364594821*x^1011+33413669637*x^1012-45620718356*x^1013-100703155988*x^1014+182697936406*x^1015-4328691289*x^1016-10572913320*x^1017+140226913701*x^1018+53616253797*x^1019+20429764848*x^1020-12296179015*x^1021-28995067707*x^1022+60058804070*x^1023+946107762*x^1024-123343657863*x^1025-119844451014*x^1026-25586132061*x^1027+228044249670*x^1028-78424973402*x^1029+15544042868*x^1030-156169819256*x^1031+133624252178*x^1032+134097573058*x^1033+10968367799*x^501-18881216863*x^502+33402630296*x^503+76127322783*x^504+159555881935*x^505-49965731760*x^506+115209281005*x^507-76077580384*x^508-78116158821*x^509+117550323107*x^510+26979777455*x^511-155912045504*x^512+103479368669*x^513+57621022564*x^514-13401704476*x^515+45474459041*x^516-96550433410*x^517-33742521550*x^518-151057690994*x^519-56249009463*x^520-18145952124*x^521-34496211522*x^522-37529073802*x^523-60399419461*x^524+76579518486*x^525+9508878552*x^526-135885189822*x^527+46048035428*x^528-30487445718*x^529-109948921884*x^530+70391869697*x^531-98966950587*x^532-44645403425*x^533+5885105007*x^534-70206621840*x^535+132109982423*x^536+5785503183*x^537+43805605951*x^538-212103940095*x^539+2924498758*x^540-35677811233*x^541+95766897237*x^542+77061233199*x^543+150823590075*x^544+51669758591*x^545+16098118870*x^546+62776931078*x^547+37921177915*x^548-72391602956*x^549+103759653977*x^550+118007269422*x^551+74135478514*x^552+37851770609*x^553-27151771323*x^554+135262774533*x^555-148721705333*x^556-57897207026*x^557+213375944794*x^558+2012721198*x^559-180168011470*x^560-208947464842*x^561+5976005785*x^562-122773099578*x^563-66719865278*x^564-44726841245*x^565+48311917869*x^566-183072526346*x^567+76608052065*x^568-103253918977*x^569-105765112928*x^570-109313878463*x^571-83146150392*x^572-79800305957*x^573+12469814261*x^574-57834727129*x^575+31234388882*x^576-29973254616*x^577-27479417998*x^578+85884874224*x^579-149843470993*x^580+139085218821*x^581+36488567498*x^582-140867295982*x^583-81036140847*x^584-14734953476*x^585+68113949406*x^586+76145793396*x^587+44037201888*x^588-58467597905*x^589+57456355271*x^590-46396703755*x^591+218686220932*x^592-103656661497*x^593+27260907290*x^594-74244694267*x^595+75519577923*x^596-31958208950*x^597+56019215687*x^598-17061668425*x^599+105139109203*x^600+77415602565*x^601-108954493245*x^602+85140330302*x^603-79572683246*x^604+114764444877*x^605-45484289011*x^606-91760986422*x^607-94315977182*x^608+878602400*x^609+16704751100*x^610-45166613346*x^611-79452542049*x^612-64363757544*x^613-82091593450*x^614-66991607227*x^615-33107406460*x^616-84308321740*x^617+41248072469*x^618+151606030721*x^619-20963307199*x^620-76780636743*x^621-60506307359*x^622-65639868144*x^623+77307497014*x^624-120629395172*x^625-58920551146*x^626-72152119854*x^627-13784023413*x^628-29723302902*x^629-59103450945*x^630-42263718431*x^631+90530664298*x^632+2162738628*x^633-58826755300*x^634-28237680670*x^635-104924312614*x^636+100208929059*x^637-70312120136*x^638+77455696272*x^639-63510025164*x^640+13962573264*x^641+51758925013*x^642-19656476291*x^643-90116387134*x^644+41548179409*x^645+167043690167*x^646-13106079927*x^647-12639563298*x^648-31532919796*x^649-81943547371*x^650+2659468451*x^651-72797097105*x^652-17552611729*x^653-68871406970*x^654-78926317536*x^655-51871988682*x^656+10073360063*x^657+117133535357*x^658-39610605270*x^659-124819309047*x^660+186332751132*x^661-5951967439*x^662-48169310072*x^663+20768672093*x^664-91409373144*x^665+19524139200*x^666+127619304757*x^667-98128438655*x^668-4904617268*x^669-57989017792*x^670+73729690684*x^671+89333834959*x^672+99716011285*x^673+103503472300*x^674-84453399907*x^675+8258389068*x^676-9450340905*x^677+7822296540*x^678-78821470413*x^679-83133235896*x^680-80330275908*x^681+22914959031*x^682-152091289333*x^683-36005600658*x^684+127080114963*x^685-68043079905*x^686+49836751263*x^687-51469985682*x^688-6616590590*x^689-69655808220*x^690+115173723007*x^691-45634640258*x^692-21385988093*x^693-300962360849*x^694+58893380816*x^695+62393657508*x^696+48036552547*x^697-134579469584*x^698+140621673048*x^699+7912276958*x^700-21132961576*x^701+65127930334*x^702+131378452178*x^703-15011857309*x^704+30180971097*x^705-80521071636*x^706-11613585664*x^707+7782812135*x^708-19836259198*x^709+76867406939*x^710-192663376*x^711+115884985152*x^712-200807422928*x^713+24839446685*x^714+118564095011*x^715-16582885964*x^716+8953308047*x^717+2612524258*x^718+75668312401*x^719-103182809527*x^720+51780158587*x^721+161189143724*x^722-16275946972*x^723-181335131597*x^724-182505534903*x^725+59091110661*x^726-64607113177*x^727-2296140395*x^728-68790652631*x^729-103730693290*x^730-207388846791*x^731-14552975373*x^732-62805658879*x^733+18781202924*x^734-128986271973*x^735+31639320741*x^736-83104477502*x^737-9885651183*x^738+137707546857*x^739+97735595675*x^740+112807494432*x^741+68402438627*x^742+35895157342*x^743+12428002821*x^744-72588844028*x^745-79799800966*x^746+42629111745*x^747+195740197570*x^748-39606216107*x^749+13892037724*x^750-3213907310*x^751+143251925064*x^752+83337773036*x^753+55289116209*x^754+111071736278*x^755+47114731432*x^756+46017675542*x^757+106534303190*x^758+65005886163*x^759-75480068719*x^760-32231292099*x^761-187585046063*x^762+106574561203*x^763-46410997680*x^764+132837442879*x^765+78280061721*x^766+7680405380*x^767-42516357685*x^768+128713753555*x^769-9029278097*x^770+47246046167*x^771+187701638909*x^772+19911051190*x^773-30984914944*x^774-2484314352*x^775+7170418460*x^776-1284643600*x^777+89116539289*x^778-20001530400*x^779-90466507480*x^780-74012539600*x^781+57274308958*x^782-24727523350*x^783+128683569525*x^784-103054717034*x^785-142511386724*x^786+73690538474*x^787-89229469699*x^788-74963796774*x^789+71970802266*x^790+133757287893*x^791+187098918939*x^792+132291921645*x^793+107833456866*x^794+64137348734*x^795-45340423408*x^796-20368785860*x^797-35482501796*x^798+104548164657*x^799-225828818492*x^800+83058094351*x^801+197238886430*x^802+71570704333*x^803-14855122348*x^804-11330859909*x^805+73194349886*x^806-85465641976*x^807+195052602572*x^808+181032739569*x^809+15148060162*x^810-148378949751*x^811+51042083256*x^812-69786758*x^813+97712314954*x^814-101202436100*x^815-98106306738*x^816-80219160719*x^817-46257669081*x^818-168779267268*x^819+151005793010*x^820+97144323363*x^821+6301125373*x^822-127837257500*x^823+40902091406*x^824-101906053776*x^825-23279213783*x^826-73850280596*x^827+21474507157*x^828-3710280111*x^829+125929627807*x^830-65005088220*x^831+112378373235*x^832-130261726208*x^833-103928057603*x^834+88732873688*x^835+123408542087*x^836-113183605499*x^837-72717691074*x^838+78681040025*x^839+73703419129*x^840+127758960193*x^841+17259409183*x^842-17726090931*x^843+162215334739*x^844-51931981106*x^845-104999647147*x^846-38897884082*x^847+11139645386*x^848+117700946259*x^849+109990176394*x^850-71799635745*x^851-459245032*x^852-69367307377*x^853-2027522216*x^854-7461919795*x^855+129571738032*x^856+95090476040*x^857-95589373455*x^858+144520675739*x^859+8580136460*x^860-76481988297*x^861-80888061958*x^862+175337057530*x^863+39161149387*x^864+46724848451*x^865-25067481720*x^866+109073193366*x^867-20110082142*x^868-2065440519*x^869+196039832496*x^870+51287378325*x^871+226840092695*x^872-85044341352*x^873+36582038212*x^874+22454235962*x^875-37614234565*x^876-12154781489*x^877-92525662835*x^878+43210605729*x^879-29503086808*x^880+47536718905*x^881-119488430617*x^882+41802893764*x^883-36749803555*x^884+96596043730*x^885+46051300012*x^886+90602974436*x^887-53608972981*x^888-5306460651*x^889+98749285110*x^890+131701203007*x^891+25773458094*x^892-46977988482*x^893+6670499818*x^894-68968602211*x^895+21468836009*x^896-70795434629*x^897-151229340489*x^898-79365642073*x^899+89379856502*x^900+180727114848*x^901+19745037556*x^902+20006492259*x^903-68966001684*x^904+130265998317*x^905-117371414201*x^906-7030328681*x^907-56677471592*x^908+19528257927*x^909+44475269947*x^910+3470169206*x^911+22467959607*x^912+98656618915*x^913-13743860146*x^914+23552943848*x^915-138242276800*x^916-88277644417*x^917+47255373796*x^918-31977886816*x^919-115363640056*x^920+25243901225*x^921-105295249154*x^922-36136467784*x^923+52453236830*x^924+80777038439*x^925+44837049142*x^926-55254888309*x^927-68234489848*x^928-92165707190*x^929-179410896808*x^930-6303111717*x^931-80199610328*x^932-15380092970*x^933-33938724551*x^934+17931510779*x^935-96931631295*x^936-48687303542*x^937-19820879797*x^938+37877911542*x^939-143694146707*x^940+113317058619*x^941+20429285363*x^942-171716251844*x^943-84741152883*x^944-40418477950*x^945+78524492177*x^946+22451226106*x^947-92140592429*x^948-224175995509*x^949-94751427805*x^950-40689590157*x^951-160025438504*x^952+101043700206*x^953+174023744486*x^954-42827761752*x^955-14794443421*x^956+243752790124*x^957+61496900473*x^958-36377552247*x^959-58121282080*x^960+144994090046*x^961+92486830236*x^962-149072168741*x^963+50911986581*x^964+63775356240*x^965+97687261817*x^966-46280177159*x^967-7688043430*x^968-39472067128*x^969+73201769053*x^970-58880360399*x^971+170840419458*x^972-54292746092*x^973-103821886252*x^974-83322943096*x^975+76066200343*x^976+165315085358*x^977+5424131554*x^978-67459706942*x^979+159323933212*x^980-107491571445*x^981-55118253633*x^982-17278583408*x^983+19225845993*x^984-114988147351*x^985-79315655096*x^986+11088007172*x^987-60859418995*x^988-145858383656*x^989+113692045711*x^990-75018497310*x^991-365437868380*x^992+35092192414*x^993-110082408015*x^994+51647441793*x^995-184568312640*x^996+99653476797*x^997+38507247220*x^998+123929024786*x^999+172382798537*x^1000-15653697225*x^1034-40712974939*x^1035-84481983259*x^1036-183059486597*x^1037+306329739993*x^1038-86809010253*x^1039-193596563569*x^1040+77147957893*x^1041+71846054998*x^1042-37049094394*x^1043-44803582029*x^1044+15081867854*x^1045-216710094742*x^1046-47796798084*x^1047-96211690279*x^1048-189635981392*x^1049-88214456018*x^1050+63138650430*x^1051-141086932675*x^1052+179045473644*x^1053+152961766238*x^1054+42049851016*x^1055-12923309793*x^1056+77822981644*x^1057+49606124226*x^1058+64949849127*x^1059+139832839733*x^1060-79078674035*x^1061-63872053437*x^1062+5880968210*x^1063+77059684059*x^1064+118861193169*x^1065-67361913998*x^1066-170366309445*x^1067-34162286067*x^1068-69460395006*x^1069+97779302218*x^1070+216724014279*x^1071+58118563580*x^1072-206554378418*x^1073+3910530696*x^1074+9692241373*x^1075+250051138068*x^1076-24386215253*x^1077-109899509519*x^1078-3113733500*x^1079+71737845657*x^1080+65818722190*x^1081-42083406251*x^1082+114681977929*x^1083+111087402900*x^1084-36208556668*x^1085+67189586885*x^1086-95415909459*x^1087-39930935857*x^1088+61175159309*x^1089-119992012339*x^1090+85494280676*x^1091-141608604262*x^1092+17013318480*x^1093-38635221143*x^1094-48453647574*x^1095+21051206372*x^1096-158416995981*x^1097+70402137413*x^1098+49633932323*x^1099-86748492368*x^1100+104553685254*x^1101-63949144239*x^1102+419956495*x^1103+16552807277*x^1104-74328747961*x^1105+75400756459*x^1106+34917686845*x^1107+37471578275*x^1108-73580767536*x^1109+77174108880*x^1110+148715919329*x^1111-66428402497*x^1112+165172263241*x^1113-26783742086*x^1114+190088576166*x^1115+54515121646*x^1116+101483331390*x^1117-108689512350*x^1118+53249730955*x^1119+72222967172*x^1120-5526562207*x^1121-207861618362*x^1122+39979297711*x^1123-131526399820*x^1124+89213762352*x^1125-62167567794*x^1126-30249775241*x^1127-2196583404*x^1128+15230073656*x^1129+60729041258*x^1130+88532730048*x^1131+10217062509*x^1132-60718111019*x^1133-57474242537*x^1134+79431153433*x^1135+80845158176*x^1136+97779807512*x^1137-56322792891*x^1138-164156429512*x^1139-19878255222*x^1140+78282404374*x^1141+98799976154*x^1142-95492650246*x^1143+93751186963*x^1144-4498456358*x^1145-65869796255*x^1146+169995598057*x^1147-50203042871*x^1148+109603182180*x^1149-111413589428*x^1150+98314563037*x^1151+67616351563*x^1152+181622227355*x^1153+20284657424*x^1154-61079727667*x^1155+147106285121*x^1156-17573539453*x^1157-30144779725*x^1158-2161067542*x^1159+39349893244*x^1160-27707519713*x^1161+94277219419*x^1162+134402588238*x^1163+179705148708*x^1164-162624387842*x^1165-67400319401*x^1166+131747136365*x^1167-38125149979*x^1168-67302505083*x^1169-70487051743*x^1170+8837205881*x^1171+83316371332*x^1172+50807693411*x^1173-52447034078*x^1174-14301265462*x^1175-147487671006*x^1176-23113128499*x^1177+8300673447*x^1178+56955833747*x^1179-12038131262*x^1180+47065770458*x^1181-157303901005*x^1182+37559510536*x^1183-148257314211*x^1184+34849913846*x^1185+45007732808*x^1186-115806825639*x^1187-161598801109*x^1188-18347919490*x^1189+60028556357*x^1190+30303822679*x^1191+161206159164*x^1192-1888494396*x^1193-4658087012*x^1194-61288806540*x^1195");
  may_kernel_intmod (NULL);
  t += test_gcd2 (14, 7, 4, 4, 6, 8, 10, 17, 7, 11);

  t += test_diff ("x/(1+sin(x^(y+x^2)))^2");

  t += test_R1(17);
  t += test_R2(25);
  t += test_R6(100000);
  t += test_R7(100000);
  t += test_R8(100000);
  t += test_S2(500);
  t += test_S3(500);
  t += test_S4(500);
  t += test_series_tan (100);

  // Test rationalize
  t += test_rationalize (1);
  t += test_sum_rationalize (10);
  t += test_sum_rationalize2 (10);

 //t += test_fermat ();

  may_kernel_info (stdout, "End");
  printf ("Total time %dms\n", t);

  may_kernel_end ();
  return 0;
}
