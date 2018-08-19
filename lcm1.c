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

/* Try to return an expression which has some factors all other
   expressions without doing any GCD computation.
   It isn't the Lowest Commun Multiple, but it is a commun multiple */
may_t
may_naive_lcm (may_size_t n, const may_t *tab)
{
  may_list_t list;
  may_t base, coeff, num;
  may_size_t i, j;

  MAY_ASSERT (n >= 2);

  MAY_LOG_FUNC (("n=%d tab[0]=%Y tab[1]=%Y", (int) n, tab[0], tab[1]));

  may_mark();

  /* Add all sub-term of the base view as monomial */
  may_list_init (list, 0);
  num = MAY_ONE;

  /* Main loop */
  for (i = 0; MAY_LIKELY (i<n); i++) {
    may_t el = tab[i];
    /* If the current element is a sum, we can analyze it first */
    if (MAY_UNLIKELY (may_sum_p (el))) {
      /* Try to get the naive_gcd of its elements to be a little bit more efficient*/
      el = may_naive_gcd (MAY_NODE_SIZE(el), MAY_AT_PTR (el, 0));
      if (may_purenum_p (el)) {
        num = may_num_lcm (num, el);
        el  = may_div (tab[i], el);
      } else {
        el = may_mul (el, may_divexact (tab[i], el));
      }
    }
    may_iterator_t it;
    for (coeff = may_product_iterator_init (it, el),
           num = may_num_lcm (num, coeff);
         may_product_iterator_end2 (&coeff, &base, it) ;
         may_product_iterator_next (it)) {
      /* Try to Find it in the list */
      for (j = 0; j < may_list_get_size (list); j+=2)
        if (may_identical (base, may_list_at (list, j)) == 0)
          break;
      if (j == may_list_get_size (list)) {
        /* Not found: add it in the list */
        may_list_push_back (list, base);
        may_list_push_back (list, coeff);
      } else {
        /* Found: update coeff keeping a majorant of both coeff */
        may_t tmp = may_sub (may_list_at (list, j+1), coeff);
        if (MAY_LIKELY(may_imminteger_p (tmp))) {
          /* We can get the max */
          if (MAY_UNLIKELY (may_num_neg_p (tmp)))
            may_list_set_at (list, j+1, coeff);
        } else {
          /* We can't get the max. Get a majorant */
          tmp = may_sub_c (may_add_c (may_list_at (list, j+1), coeff),
                           may_naive_gce (may_list_at (list, j+1), coeff));
          tmp = may_eval (tmp);
          may_list_set_at (list, j+1, tmp);
        }
      }
    } /* for iterator */
  } /* for each i*/

  /* Transform the list in a product */
  may_size_t sp = may_list_get_size (list);
  size_t size_tab = sp/2+1;
  may_t base_tab[size_tab];
  for (i = 0; i < sp; i+=2) {
    base_tab[i/2] = may_pow_c (may_list_at (list, i),
                               may_list_at (list, i+1));
  }
  base_tab[sp/2] = num;
  base = may_mul_vc (size_tab, base_tab);

  /* Return and keep evaluated result */
  base = may_eval (base);
#ifdef MAY_WANT_ASSERT
  /* Check that the returned LCM is divided by everything */
  for (may_size_t i = 0; i < n; i++) {
    may_t tmp = may_divexact (base, tab[i]);
    MAY_ASSERT (tmp != NULL);
  }
#endif
  return may_keep (base);
}

/* The real LCM: Compute it from the GCD */
may_t
may_lcm (unsigned long n, const may_t tab[])
{
  unsigned long i;
  may_t lcm;

  MAY_LOG_FUNC (("n=%d tab[0]=%Y tab[1]=%Y", (int) n, tab[0], tab[1]));

  MAY_ASSERT (n >= 1);

  lcm = tab[0];
  may_mark();
  for (i = 1; i < n; i++) {
    may_t temp[2] = {lcm, tab[i] };
    may_t gcd = may_gcd (2, temp);
    may_t div =  may_divexact (tab[i], gcd);
    MAY_ASSERT (div != NULL);
    lcm = may_mul (lcm, div);
  }
  return may_keep (lcm);
}
