/* This file is part of the MAYLIB libray.
   Copyright 2007-2012 Patrick Pelissier

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

/* Define a generic BINTREE by overiding the numerical functions used in
   bintree */
#define may_bintree_size may_genbintree_size
#define may_bintree_get_sum may_genbintree_get_sum
#define may_bintree_insert may_genbintree_insert
#undef MAY_PURENUM_P
#define MAY_PURENUM_P(x) MAY_EVAL_P(x)
#undef MAY_CLOSE_C
#define MAY_CLOSE_C(a,b,c) (void)0
#define may_num_zero_p(x) (0)
#define may_num_one_p(x)  (0)
#define may_num_simplify(x) may_eval(x)
#define may_num_set(d,a) (a)
#define may_num_add(d,a,b) may_add_c(a,b)
#include "expand_bintree.c"

may_t
may_collect (may_t a, may_t x)
{
  MAY_LOG_FUNC (("a='%Y', x='%Y'", a, x));

  may_mark();

  /* Expand */
  a = may_expand (a);

  /* Collect */
  may_iterator_t it1, it2;
  may_t num, c, b, pseudo_num, pseudo_key, e2, b2;
  may_bintree_t t = NULL;
  for (num = may_sum_iterator_init (it1, a) ;
       may_sum_iterator_end (&c, &b, it1);
       may_sum_iterator_next (it1) ) {
    pseudo_num = c;
    pseudo_key = may_set_ui (1);
    for (may_product_iterator_init (it2, b);
         may_product_iterator_end (&e2, &b2, it2) ;
         may_product_iterator_next (it2) ) {
      if ((MAY_TYPE (x) == MAY_LIST_T ? may_independent_vp : may_identical) (b2, x) == 0)
        pseudo_key = may_mulinc_c (pseudo_key, may_product_iterator_ref (it2));
      else
        pseudo_num = may_mulinc_c (pseudo_num, may_product_iterator_ref (it2));
    }
    t = may_genbintree_insert (t, may_eval (pseudo_num), may_eval (pseudo_key));
  }
  a = may_genbintree_get_sum (num, t);

  return may_keep (may_eval (a));
}
