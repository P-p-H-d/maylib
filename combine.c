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

static int true_p (may_t x) { UNUSED (x); return 1; }

may_t
may_combine (may_t a, may_combine_flags_e flags)
{
  int (*functab[5])(may_t);
  may_t b;
  int max_recur = 0;
  may_mark_t mark;

  MAY_LOG_FUNC (("a='%Y' flag=%d", a, flags));

  may_mark (mark);

  /* Pass 1:
     a^b*a^c  --> a^(b+c) if 'a' is a positive real or force mode
     (a^b)^c  --> a^(b*c) if 'a' is a positive real and b a real, or c an integer, or Re(a)>0 et -2<=b<=2 or -1<b<1 or b=-1 and not(a<0) or force mode
     a^b*c^b  --> (a*c)^b fi 'a' and 'c' positive real or force mode
     a^b*(a^c)^d-->a^(b+c*d) if 'a' is a positive real and (b real or c integer) or force mode */
  may_t rule1a = may_parse_str ("$0^$1*$0^$2*$3");
  may_t rule1b = may_parse_str ("$0^($1+$2)*$3");
  may_t rule1a2 = may_parse_str ("$0^$1*$0*$2");
  may_t rule1b2 = may_parse_str ("$0^($1+1)*$2");
  may_t rule2a = may_parse_str ("($0^$1)^$2");
  may_t rule2b = may_parse_str ("$0^($1*$2)");
  may_t rule3a = may_parse_str ("$0^$1*$2^$1*$3");
  may_t rule3b = may_parse_str ("($0*$2)^$1*$3");
  may_t rule4a = may_parse_str ("$0^$1*($0^$2)^$3*$4");
  may_t rule4b = may_parse_str ("$0^($1+$2*$3)*$4");
  may_t rule5a = may_parse_str ("$0*($0^$2)^$3*$4");
  may_t rule5b = may_parse_str ("$0^(1+$2*$3)*$4");

  b = a;
  do {
    a = b;
    /* Rule 1.1 */
    functab[0] = (flags == MAY_COMBINE_FORCE ) ? true_p : may_nonneg_p;
    functab[1] = functab[2] = functab[3] = true_p;
    b = may_rewrite2 (b, rule1a, rule1b, 4, functab);
    b = may_rewrite2 (b, rule1a2, rule1b2, 3, functab);
    /* Rule 1.2 */
    if (flags == MAY_COMBINE_FORCE) {
      b = may_rewrite (b, rule2a, rule2b);
    } else {
      /* 'a' is a positive real and b a real */
      functab[0] = may_nonneg_p;
      functab[1] = may_real_p;
      b = may_rewrite2 (b, rule2a, rule2b, 4, functab);
      /* c an integer is not a useful rule. Skip it */
    }
    /* Rule 1.3 */
    functab[0] = (flags == MAY_COMBINE_FORCE ) ? true_p : may_nonneg_p;
    functab[1] = true_p;
    functab[2] = (flags == MAY_COMBINE_FORCE ) ? true_p : may_nonneg_p;
    b = may_rewrite2 (b, rule3a, rule3b, 4, functab);
    /* Rule 1.4 */
    if (flags == MAY_COMBINE_FORCE) {
      b = may_rewrite (b, rule4a, rule4b);
    } else {
      functab[0] = may_nonneg_p;
      functab[1] = true_p;
      functab[2] = may_real_p;
      functab[3] = true_p;
      functab[4] = true_p;
      b = may_rewrite2 (b, rule4a, rule4b, 5, functab);
      functab[2] = true_p;
      functab[3] = may_integer_p;
      b = may_rewrite2 (b, rule4a, rule4b, 5, functab);
    }
    /* Rule 1.5 */
    if (flags == MAY_COMBINE_FORCE) {
      b = may_rewrite (b, rule5a, rule5b);
    } else {
      functab[0] = may_nonneg_p;
      functab[1] = true_p;
      functab[2] = may_real_p;
      functab[3] = true_p;
      functab[4] = true_p;
      b = may_rewrite2 (b, rule5a, rule5b, 5, functab);
      functab[2] = true_p;
      functab[3] = may_integer_p;
      b = may_rewrite2 (b, rule5a, rule5b, 5, functab);
    }
    UNUSED (max_recur);
    MAY_ASSERT (max_recur++ < 1000);
  } while (may_identical (a, b) != 0);
  a = may_compact (mark, b);

  /* Pass 2:
     exp(a)*(exp(b)   --> exp (a+b)
     (exp x)^c        --> exp (x*c) (if c in Z or -PI < Im(x) <= PI) or force */
  rule1a = may_parse_str ("exp($0)*exp($1)*$2");
  rule1b = may_parse_str ("exp($0+$1)*$2");
  rule2a = may_parse_str ("exp($0)^$1");
  rule2b = may_parse_str ("exp($0*$1)");

  b = a;
  do {
    a = b;
    /* Rule 2.1 */
    b = may_rewrite (b, rule1a, rule1b);
    /* Rule 2.2 */
    functab[0] = functab[2] = functab[3] = true_p;
    functab[1] = (flags == MAY_COMBINE_FORCE ) ? true_p : may_real_p;
    b = may_rewrite2 (b, rule2a, rule2b, 4, functab);
    MAY_ASSERT (max_recur++ < 1000);
  } while (a!=b);
  a = may_compact (mark, b);

  /* Pass 3:
     a^log(b)	 --> exp(log(b)*log(a))
     a^b*exp(c)  --> exp(b*log(a)+c) if a positive real or force mode */
  rule1a = may_parse_str ("$0^log($1)");
  rule1b = may_parse_str ("exp(log($0)*log($1))");
  rule2a = may_parse_str ("$0^$1*exp($2)*$3");
  rule2b = may_parse_str ("exp($1*log($0)+$2)*$3");

  b = a;
  do {
    a = b;
    b = may_rewrite (b, rule1a, rule1b);
    if (flags == MAY_COMBINE_FORCE) {
      b = may_rewrite (b, rule2a, rule2b);
    } else {
      functab[0] = may_nonneg_p;
      functab[1] = true_p;
      functab[2] = true_p;
      functab[3] = true_p;
      b = may_rewrite2 (b, rule2a, rule2b, 4, functab);
    }
    MAY_ASSERT (max_recur++ < 1000);
  } while (a!=b);
  a = may_compact (mark, b);

  /* Pass 4:
     exp(x+k*PI/2)    --> I^floor(k)*exp(x+frac(k)*PI/2) if k RAT or INTEGER
     exp(c*log(a)+b)  --> a^c*exp(b) [Good idea?]
     exp(log(a)+b)    --> a*exp(b) [Good idea?]
     exp(a*log(b))    --> b^a  [Last case] */
  return a;
}

may_t
may_tcollect (may_t a)
{
  may_mark ();

  /* Convert sin/cos/tan to exp (tan throught sin/cos)
     Then combine it (to get the sign inside. Ex: sin/cos ) */
  a = may_trig2exp2 (a);
  may_t p, q;

  /* Separate quotient and reminder */
  may_comdenom (&p, &q, a);
  p = may_expand (p);
  p = may_combine (p, MAY_COMBINE_NORMAL);
  q = may_expand (q);
  q = may_combine (q, MAY_COMBINE_NORMAL);

  /* TODO: support a*cos(a)+b*sin(a) -> */
  p = may_normalsign (may_exp2trig (may_div (p, q)));
  return may_keep (may_rationalize (p));
}
