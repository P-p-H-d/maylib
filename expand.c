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

#define MAY_EXPAND_BASECASE_THRESHOLD 100

/* Returns the size of an expanded multinom (sum(a[i],i=1..n)^e)*/
static MAY_REGPARM unsigned long
multinom_size (unsigned long n, unsigned long e)
{
  MAY_ASSERT (n >= 1);
  if (e == 0 || n == 1)
    return 1;
  if (e == 1)
    return n;
  unsigned long s = 0, i;
  for (i = 0 ; i <= e ; i++)
    s += multinom_size (n-1, e-i);
  return s;
}

/* Compute the expanded multinomial of arg[n]. Basecase.
   It is the faster when there is no overlap in the computed results,
   ie the final size == @var{final}, or only one sum */
static may_t
expand_mul_basecase (const may_t arg[], may_size_t n, may_size_t final)
{
  may_t y;
  may_size_t i, j;

  MAY_ASSERT (final > 0);

  MAY_LOG_FUNC (("n=%d final=%d", (int)n,(int)final));

  /* We may get some product of rationals which may simplify
     into an int power sum. Search for some sum powered to rational */
  int there_is_some_sum_powered_to_rat = 0;
  for (i = 0 ; i < n ; i++) {
    may_iterator_t it;
    may_t c, b;
    if (may_sum_p (arg[i])) {
      for (may_sum_iterator_init (it, arg[i]) ;
           may_sum_iterator_end (&c, &b, it)  ;
           may_sum_iterator_next (it) ) {
        if (MAY_UNLIKELY (MAY_TYPE (b) == MAY_POW_T
                          && MAY_TYPE (MAY_AT (b, 0)) == MAY_SUM_T
                          && MAY_TYPE (MAY_AT (b, 1)) == MAY_RAT_T)) {
          /*  We need to check after each multiplication if
              we have to perform a further expand */
          there_is_some_sum_powered_to_rat = 1;
          goto break_all;
        }
      }
    }
  }
 break_all:

  y = MAY_NODE_C (MAY_SUM_T, final);
  for (i = 0 ; MAY_LIKELY (i < final); i++) {
    MAY_RECORD ();
    may_size_t cumul = i;
    may_t z = MAY_NODE_C (MAY_PRODUCT_T, n);
    for (j = 0; MAY_LIKELY (j < n); j++) {
      if (MAY_TYPE (arg[j]) != MAY_SUM_T)
        MAY_SET_AT (z, j, arg[j]);
      else {
        may_size_t nz = MAY_NODE_SIZE(arg[j]);
        MAY_SET_AT (z, j, MAY_AT (arg[j], cumul%nz));
        cumul /= nz;
      }
    }
    z = may_eval (z);
    /* If we may have to perform a further expand... */
    if (MAY_UNLIKELY (there_is_some_sum_powered_to_rat == 1)) {
      /* Check for some pos int powered sum integer whih may have appeared */
      may_iterator_t it;
      may_t c, b;
      for (may_product_iterator_init (it, z) ;
	   may_product_iterator_end (&c, &b, it) ;
	   may_product_iterator_next (it) ) {
	if (MAY_UNLIKELY (MAY_TYPE (b) == MAY_SUM_T
			  && mpz_sgn (MAY_INT (c)) > 0)) {
	  /* FIXME: I don't like this. Seems like an infinite recur call! */
	  z = may_expand (z);
	  break;
	}
      }
    }
    MAY_COMPACT (z);
    MAY_SET_FLAG (z, MAY_EXPAND_F);
    MAY_SET_AT (y, i, z);
  }
  return y;
}

/* Compute the expanded multinomial of 2 sums using a bintree
   to accumulate the result. Faster when there is overlap but sparse. */
static may_t
expand_mul_two_sum (may_t a, may_t b)
{
  may_size_t i, j, j_start, na, nb;
  may_t num, sumnum;
  may_bintree_t tree = NULL;

  MAY_ASSERT (MAY_TYPE (a) == MAY_SUM_T);
  MAY_ASSERT (MAY_TYPE (b) == MAY_SUM_T);

  MAY_LOG_FUNC (("a=%Y b=%Y",a,b));

  na = MAY_NODE_SIZE(a);
  nb = MAY_NODE_SIZE(b);

  /* num is an accumulator used to compute everything */
  num = MAY_DUMMY;
  /* sumnum is the pure numerical term of the expanded product */
  sumnum = may_num_set (MAY_DUMMY, MAY_ZERO);

  /* If there is numerical terms handle it now */
  i = j_start = 0;
  if (MAY_PURENUM_P (MAY_AT (a, 0))) {
    may_t a_num = MAY_AT (a, 0);
    /* If we have a_num*b_num, do it separetely */
    if (MAY_PURENUM_P (MAY_AT (b, 0))) {
      sumnum = may_num_mul (sumnum, a_num, MAY_AT (b, 0));
      j_start = 1;
    }
    for (j = j_start; j < nb; j++) {
      may_t y = MAY_AT (b, j);
      if (MAY_TYPE (y) == MAY_FACTOR_T) {
        num = may_num_mul (num, a_num, MAY_AT (y, 0));
        tree = may_bintree_insert (tree, num, MAY_AT (y, 1));
      } else {
        tree = may_bintree_insert (tree, a_num, y);
      }
    }
    i = 1;
  }
  if (MAY_PURENUM_P (MAY_AT (b, 0))) {
    may_t b_num = MAY_AT (b, 0);
    /* Don't add the a[0]*b[0] twice */
    for (j = j_start; j < na; j++) {
      may_t y = MAY_AT (a, j);
      if (MAY_TYPE (y) == MAY_FACTOR_T) {
        num = may_num_mul (num, b_num, MAY_AT (y, 0));
        tree = may_bintree_insert (tree, num, MAY_AT (y, 1));
      } else {
        tree = may_bintree_insert (tree, b_num, y);
      }
    }
    j_start = 1;
  }

  /* Save intmod to disable it temporarly */
  may_t oldintmod = may_g.frame.intmod;

  /* Main loop without purenum */
  for (; i < na ; i++)
    for (j = j_start; j < nb; j++) {
      may_t aa, bb, z;
      may_t *pa, *pb;
      may_size_t pas, pbs, pzs;
      int reexpand_product = 0;

      aa = MAY_AT (a, i);
      bb = MAY_AT (b, j);
      MAY_ASSERT (!MAY_PURENUM_P (aa));
      MAY_ASSERT (!MAY_PURENUM_P (bb));

      /* Extract the num coefficient */
      if (MAY_LIKELY (MAY_TYPE (aa) == MAY_FACTOR_T)) {
        if (MAY_LIKELY (MAY_TYPE (bb) == MAY_FACTOR_T)) {
          num = may_num_mul (num, MAY_AT (aa, 0), MAY_AT (bb, 0));
          bb = MAY_AT (bb, 1);
        } else {
          num = may_num_set (num, MAY_AT (aa, 0));
        }
        aa = MAY_AT (aa, 1);
      } else if (MAY_TYPE (bb) == MAY_FACTOR_T) {
        num = may_num_set (num, MAY_AT (bb, 0));
        bb = MAY_AT (bb, 1);
      } else {
        num = may_num_set (num, MAY_ONE);
      }
      
      /* Extract the product iterators */
      if (MAY_LIKELY (MAY_TYPE (aa) == MAY_PRODUCT_T)) {
        pa = MAY_AT_PTR (aa, 0);
        pas = MAY_NODE_SIZE(aa);
      } else {
        pa  = &aa;
        pas = 1;
      }
      if (MAY_LIKELY (MAY_TYPE (bb) == MAY_PRODUCT_T)) {
        pb = MAY_AT_PTR (bb, 0);
        pbs = MAY_NODE_SIZE(bb);
      } else {
        pb = &bb;
        pbs = 1;
      }

      /* Merge the product */
      MAY_RECORD ();
      z = MAY_NODE_C (MAY_PRODUCT_T, pas+pbs);
      pzs = 0;

      for (;;) {
        /* Extract base from base^power */
        may_t base_b, base_a, expo_b, expo_a;
        int ii;
        if (MAY_TYPE (*pb) == MAY_POW_T
            && MAY_PURENUM_P (MAY_AT (*pb, 1))) {
          base_b = MAY_AT (*pb, 0);
          expo_b = MAY_AT (*pb, 1);
        } else {
          base_b = *pb;
          expo_b = MAY_ONE;
        }
        if (MAY_TYPE (*pa) == MAY_POW_T
            && MAY_PURENUM_P (MAY_AT (*pa, 1))) {
          base_a = MAY_AT (*pa, 0);
          expo_a = MAY_AT (*pa, 1);
        } else {
          base_a = *pa;
          expo_a = MAY_ONE;
        }
        /* Compare base */
        ii = may_identical (base_b, base_a);
        if (ii < 0) {
          MAY_SET_AT (z, pzs++, *pb);
          pbs--, pb++;
        } else if (ii > 0) {
          MAY_SET_AT (z, pzs++, *pa);
          pas--, pa++;
        } /* Same base: sum the exponents */
        else if (MAY_UNLIKELY (MAY_TYPE (base_a) == MAY_POW_T)) {
          /* Complicate case (but rare): performs a full eval */
          may_t w = may_eval (may_pow_c (base_a, may_add_c (expo_a, expo_b)));
          MAY_SET_AT (z, pzs++, w);
          /* Decrement both products */
          pas--, pa++;
          pbs--, pb++;
        } else {
          /* expo_a is an integer */
          may_t dest = may_num_add (MAY_DUMMY, expo_a, expo_b);
          if (MAY_UNLIKELY (may_num_zero_p (dest)))
            /* Nothing to do */ ;
          else if (MAY_UNLIKELY (may_num_one_p (dest)))
            MAY_SET_AT (z, pzs++, base_a);
          else {
            /* FIXME: inline it ? */
            may_g.frame.intmod = NULL;
            expo_a = may_num_simplify (dest);
            may_g.frame.intmod = oldintmod;
            /* Check for expression like
               (1-x)^(1^2)*(1-x)^(3^2)-->(1-x)^2 --> To further expand */
            if (MAY_UNLIKELY (MAY_TYPE (base_a) == MAY_SUM_T
                              && MAY_TYPE (expo_a) == MAY_INT_T)) {
              /* To reperform an expand before adding the term
                 into the bintree -*/
              reexpand_product = 1;
            }
            may_t w = MAY_NODE_C (MAY_POW_T, 2);
            MAY_SET_AT (w, 0, base_a);
            MAY_SET_AT (w, 1, expo_a);
            MAY_CLOSE_C (w, MAY_FLAGS (base_a),
                         MAY_NEW_HASH2 (base_a, expo_a));
            MAY_ASSERT (MAY_EVAL_P (w));
            MAY_SET_AT (z, pzs++, w);
          }
          /* Decrement both products */
          pas--, pa++;
          pbs--, pb++;
        }
        /* Check if end of merging */
        if (pas == 0) {
          memcpy (MAY_AT_PTR (z, pzs), pb, pbs*sizeof (may_t));
          pzs += pbs;
          break;
        } else if (pbs == 0) {
          memcpy (MAY_AT_PTR (z, pzs), pa, pas*sizeof (may_t));
          pzs += pas;
          break;
        }
      }
      MAY_ASSERT (pzs <= MAY_NODE_SIZE(z));

      MAY_NODE_SIZE(z) = pzs;
      if (MAY_LIKELY (pzs > 0)) {
        /* Clean up if only one term */
        if (MAY_UNLIKELY (pzs == 1))
          z = MAY_AT (z, 0);
        else
          MAY_CLOSE_C (z, MAY_EVAL_F|MAY_EXPAND_F,
                       may_node_hash (MAY_AT_PTR (z, 0), pzs));
        MAY_ASSERT (MAY_EVAL_P (z));
        MAY_COMPACT (z);
        /* If we have to reperform an expand due to algebraic dependency */
        if (MAY_UNLIKELY (reexpand_product == 1)) {
          z = may_expand (may_eval (may_mul_c (may_num_set(MAY_DUMMY, num), z)));
          may_iterator_t it;
          may_t num2, num3;
          for(num2 = may_sum_iterator_init (it, z);
              may_sum_iterator_end (&num3, &z, it) ;
              may_sum_iterator_next(it)) {
            /* Insert (num,z) into the tree */
            tree = may_bintree_insert (tree, num3, z);
          }
          sumnum = may_num_add (sumnum, sumnum, num2);
        } else {
          void *top_position = may_g.Heap.top;
          /* Insert (num,z) into the tree */
          tree = may_bintree_insert (tree, num, z);
          /* If no allocation were done, we can clean everything,
             since it has succesfully reused previous memory */
          if (top_position == may_g.Heap.top)
            MAY_CLEANUP();
        }
      } else {
        /* base have been canceled (For example. z * z^-1)
           Add num to the sumnum */
        sumnum = may_num_add (sumnum, sumnum, num);
        MAY_COMPACT (sumnum);
      }
    } /* for j */
  /* Compute constant term */
  sumnum = may_num_simplify (sumnum);
  sumnum = may_bintree_get_sum (sumnum, tree);
  MAY_ASSERT (MAY_EVAL_P (sumnum));
  MAY_ASSERT (may_recompute_hash (sumnum) == MAY_HASH (sumnum));
  return sumnum;
}

/* Extract the degree and the maximum coefficient
   of an integer univariate polynomial */
static unsigned long
get_degree_max (mpz_srcptr *coeff, may_t a)
{
  unsigned long deg;
  mpz_ptr z;
  may_size_t i, n;

  MAY_ASSERT (MAY_TYPE (a) == MAY_SUM_T
              && MAY_NODE_SIZE(a) >= 2);
  deg = 1;
  n = MAY_NODE_SIZE(a);
  if (MAY_PURENUM_P (MAY_AT (a, 0)))
    z = MAY_INT (MAY_AT (a, 0));
  else
    z = MAY_INT (MAY_ONE);
  for (i = MAY_PURENUM_P (MAY_AT (a, 0)); i < n; i++) {
    may_t term = MAY_AT (a, i);
    if (MAY_LIKELY (MAY_TYPE (term) == MAY_FACTOR_T)) {
      if (mpz_cmpabs (MAY_INT (MAY_AT (term, 0)), z) > 0)
        z = MAY_INT (MAY_AT (term, 0));
      term = MAY_AT (term, 1);
    }
    if (MAY_LIKELY (MAY_TYPE (term) == MAY_POW_T)) {
      MAY_ASSERT (MAY_TYPE (MAY_AT (term, 1)) == MAY_INT_T);
      MAY_ASSERT (mpz_fits_ulong_p (MAY_INT (MAY_AT (term, 1))));
      unsigned long k = mpz_get_ui (MAY_INT (MAY_AT (term, 1)));
      if (k > deg)
        deg = k;
    }
  }
  *coeff = z;
  return deg;
}

/* Evaluate 'a' which is an univariate polynomial over the pure integer  at 2^n in 'z' */
static void
evaluate_at_power2 (mpz_t z, may_t a, unsigned long n, unsigned long deg)
{
  mpz_t temp;
  may_size_t m;
  may_t *p;
  MAY_ASSERT (MAY_TYPE (a) == MAY_SUM_T);

  /* Pre-reserve the size */
  mpz_set_ui (z, 1);
  mpz_mul_2exp (z, z, (deg + 1)* n);

  /* Init */
  mpz_set_ui (z, 0);

  m = MAY_NODE_SIZE(a);
  p = MAY_AT_PTR (a, 0);
  if (MAY_PURENUM_P (*p)) {
    mpz_add (z, z, MAY_INT (*p));
    p++;
    m--;
  }

  /* Loop */
  mpz_init (temp);
  for ( ; m != 0; m--, p++) {
    may_t term = *p, coeff = MAY_ONE;
    unsigned long deg = 1;
    if (MAY_LIKELY (MAY_TYPE (term) == MAY_FACTOR_T)) {
      coeff = MAY_AT (term, 0);
      term = MAY_AT (term, 1);
    }
    if (MAY_LIKELY (MAY_TYPE (term) == MAY_POW_T))
      may_get_ui (&deg, MAY_AT (term, 1));
    /* Add this new term */
    MAY_ASSERT (deg > 0 && n > 0);
    MAY_ASSERT (n < ULONG_MAX/deg);
    mpz_mul_2exp (temp, MAY_INT (coeff), n*deg);
    mpz_add (z, z, temp);
  }
  mpz_clear (temp);
}

/* Multiply 2 univariate integer polynomial 'a' and 'b' of variable 'v'
   using Kronecker tricks. */
static may_t
expand_univariate_poly (may_t a, may_t b, may_t v)
{
  mpz_t za, zb, two_n, two_n1;
  unsigned long dega, degb, n, i;
  mpz_srcptr maxa, maxb;
  may_t y, term;

  dega = get_degree_max (&maxa, a);
  degb = get_degree_max (&maxb, b);
  MAY_ASSERT (dega > 0);
  MAY_ASSERT (degb > 0);

  /* The largest coefficient in the result is :
     (1+min(dega,degb))*abs(maxa)*abs(maxb)
     + the sign.
     Take the power of 2 above. */
  n = 1 + MAY_SIZE_IN_BITS (1 + MIN (dega, degb))
    + mpz_sizeinbase (maxa, 2) + mpz_sizeinbase (maxb, 2);

  /* We MUST have n*max(dega,degba) < ULONG_MAX */
  if (MAY_UNLIKELY (n >= ULONG_MAX / MAX (dega, degb)))
    return NULL; /* Failed (Too big) */

  MAY_RECORD ();
  mpz_init (za);
  mpz_init (zb);

  /* Evaluate at 2^n, multiply them */
  evaluate_at_power2 (za, a, n, dega);
  evaluate_at_power2 (zb, b, n, degb);
  mpz_mul (za, za, zb);

  /* Extract the coefficient. Could be written faster. Does it worth it? */
  mpz_init (two_n);
  mpz_set_ui (two_n, 1);
  mpz_mul_2exp (two_n, two_n, n);
  mpz_init (two_n1);
  mpz_set_ui (two_n1, 1);
  mpz_mul_2exp (two_n1, two_n1, n-1);
  dega += degb+1;
  y = MAY_NODE_C (MAY_SUM_T, dega);
  for (i = 0; i < dega; i++) {
    mpz_fdiv_r_2exp (zb, za, n);
    if (mpz_cmp (zb, two_n1) > 0) {
      mpz_sub (zb, zb, two_n);
      mpz_add (za, za, two_n);
    }
    term = may_mul_c (may_set_z (zb), may_pow_c (v, MAY_ULONG_C (i)));
    MAY_SET_AT (y, i, term);
    mpz_fdiv_q_2exp (za, za, n);
  }
  MAY_RET_EVAL (y);
}

/* Return TRUE if there is only Pure INTEGER inside an univariate polynomial.
   Return 2 if there is only pure INTEGER or RATIONAL inside an univariate polynomial */
/* TODO: Check for too sparse too! */
static int
test_pureint (may_t a)
{
  may_size_t n;
  may_t *p;
  int retval = 1;

  MAY_ASSERT (MAY_TYPE (a) == MAY_SUM_T);

  n = MAY_NODE_SIZE(a);
  p = MAY_AT_PTR (a, 0);

  /* Check num if any */
  if (MAY_PURENUM_P (*p)) {
    if (MAY_UNLIKELY (MAY_TYPE (*p) != MAY_INT_T)) {
      if (MAY_UNLIKELY (MAY_TYPE (*p) != MAY_RAT_T))
        return 0;
      retval = 2;
    }
    p++;
    n--;
  }

  /* Check Loop */
  for ( ; n != 0; n--, p++) {
    may_t term = *p;
    if (MAY_LIKELY (MAY_TYPE (term) == MAY_FACTOR_T)) {
      if (MAY_UNLIKELY (MAY_TYPE (MAY_AT (term, 0)) != MAY_INT_T)) {
        if (MAY_TYPE (MAY_AT (term, 0)) != MAY_RAT_T)
          return 0;
        retval = 2;
      }
      term = MAY_AT (term, 1);
    }
    if (MAY_LIKELY (MAY_TYPE (term) == MAY_POW_T)) {
      if (MAY_UNLIKELY (MAY_TYPE (MAY_AT (term, 0)) != MAY_STRING_T
                        || MAY_TYPE (MAY_AT (term, 1)) != MAY_INT_T
                        || !mpz_fits_ushort_p (MAY_INT (MAY_AT (term, 1)))))
        return 0;
    } else if (MAY_UNLIKELY (MAY_TYPE (term) != MAY_STRING_T))
      return 0;
  }
  /* End */
  return retval;
}

/* This is a simpler version of comdenom */
static void
convert_poly_over_Q_to_Z (may_t *n, may_t *d, may_t p)
{
  may_iterator_t it;
  may_t num;
  may_t base, coeff;
  may_t n1;
  may_t d1;
  mpz_t dz;

  MAY_ASSERT (may_sum_p (p));
  MAY_RECORD();

  /* Compute the denominator: LCM */
  mpz_init_set_ui (dz, 1);
  for(num = may_sum_iterator_init (it, p) ;
      may_sum_iterator_end (&coeff, &base, it) ;
      may_sum_iterator_next (it))
    if (MAY_TYPE (coeff) == MAY_RAT_T)
      mpz_lcm (dz, dz, mpq_denref (MAY_RAT (coeff)));
  if (MAY_TYPE (num) == MAY_RAT_T)
    mpz_lcm (dz, dz, mpq_denref (MAY_RAT (num)));

  /* Multiply each term by the LCM */
  d1 = may_eval (MAY_MPZ_NOCOPY_C (dz));
  n1 = may_set_ui (0);
  for(num = may_sum_iterator_init (it, p) ;
      may_sum_iterator_end (&coeff, &base, it) ;
      may_sum_iterator_next (it))
    n1 = may_addinc_c (n1, may_mul_c (d1, may_sum_iterator_ref (it)));
  n1 = may_addinc_c (n1, may_mul_c (d1, num));
  n1 = may_eval (n1);

  MAY_COMPACT_2 (n1, d1);
  *n = n1;
  *d = d1;
}

/* Perform an expand of the different products in the case the resulted sum is big.
   Select which algorithms to performs.
   A. For each term of the product:
         Extract its coefficient (PURE INTEGER ?), its variable list, and its size
         And fill an array with these information.
   B. Sort the array by variable list / coeff / size (bigger is first).
   C. Sum of the elements of the same variable list:
       For each element with some vars of the array,
       if the next element is of the same variable,
       multiply both:
            If Univariate and not spare (#elem^2>degree) and coeff=INT  ==> Kronecker
            If Univariate and not spare (#elem^2>degree) and coeff<>INT ==> Karatsuba(TODO)
            Else                   ==> Accumultor (mul_two)
       Store the resul in the array and compact it.
   D. Sort the array by the number of terms of each element
   E. For each element:
         Multiply it with the next one using the Accumulator
   F. Finish the multiplication using the terms which weren't a sum.
*/
struct item {
  int pureint;
  may_t arg;
  may_t varlist;
  may_size_t size;
};

static int cmp_varlist (const void *a, const void *b) {
  struct item *pa = (struct item *)a, *pb = (struct item *)b;
  if (MAY_UNLIKELY (pa->size == 0))
    return 1;
  if (MAY_UNLIKELY (pb->size == 0))
    return -1;
  int i = may_identical (pa->varlist, pb->varlist);
  if (MAY_UNLIKELY (i != 0))
    return i;
  if (MAY_UNLIKELY (pa->pureint != pb->pureint))
    return pa->pureint < pb->pureint ? -1 : 1;
  return (pa->size > pb->size) ? -1 : (pa->size < pb->size);
}

static int cmp_size (const void *a, const void *b) {
  const struct item *pa = a, *pb = b;
  return (pa->size > pb->size) ? -1 : (pa->size < pb->size);
}

static may_t
expand_mul_heavy (const may_t arg[], may_size_t n)
{
  struct item tab[n+1];
  may_size_t i;
  may_t rat = NULL;

  MAY_LOG_FUNC (("n=%d", (int)n));

  /* Step A: analyze inputs */
  for (i = 0 ; i < n; i++) {
    tab[i].pureint = 0;
    tab[i].arg = arg[i];
    tab[i].varlist = may_indets (arg[i], MAY_INDETS_NUM);
    tab[i].size    = (MAY_TYPE (arg[i]) == MAY_SUM_T) ? MAY_NODE_SIZE(arg[i]) : 0;
    if (MAY_LIKELY (MAY_NODE_SIZE(tab[i].varlist) == 1 && tab[i].size > 0)) {
      tab[i].pureint = test_pureint (arg[i]);
      /* If it is an univariate polynomial over the rational,
         transform it to an univariate polynomial over the integer */
      if (MAY_UNLIKELY (tab[i].pureint == 2)) {
        may_t p, q;
        convert_poly_over_Q_to_Z (&p, &q, arg[i]);
        tab[i].arg = p;
        MAY_ASSERT (MAY_TYPE (q) == MAY_INT_T);
        rat = (rat == NULL) ? q : may_mul_c (rat, q);
      }
    }
  }
  
  /* If we have converted some entry from Q[X] to Z[X], we need to handle the extracted denominator */
  if (MAY_UNLIKELY (rat != NULL)) {
    tab[n].pureint = 0;
    tab[n].arg = may_eval (may_div_c (MAY_ONE, rat));
    tab[n].varlist = NULL;
    tab[n].size = 0;
    n++;
  }
  /* Step B */
  qsort (tab, n, sizeof (struct item), cmp_varlist);
#if 0
  for (i = 0; i < n ; i ++) {
    printf ("i=%d tab[].pureint=%d tab[].size=%d arg=", i, tab[i].pureint, tab[i].size);
    may_dump (tab[i].varlist);
  }
#endif

  /* Step C */
  for ( i = 0; i < (n-1) ; ) {
    if (MAY_UNLIKELY (tab[i].size == 0 || tab[i+1].size == 0))
      break;
    if (MAY_LIKELY (may_identical (tab[i].varlist, tab[i+1].varlist) == 0)) {
      /* Multiply both */
      may_t var = tab[i].varlist;
      may_t result = NULL;
      /* Univariate case: try Kronecker tip */
      if (MAY_NODE_SIZE(var) == 1
          && tab[i].pureint && tab[i+1].pureint
          && MAY_TYPE (MAY_AT (var, 0)) == MAY_STRING_T)
        result = expand_univariate_poly (tab[i].arg, tab[i+1].arg, MAY_AT (var, 0));
      /* If failed to multiply them using Kronecker tip for univariate, use Karatsuba */
      if (MAY_UNLIKELY (result == NULL))
        result = may_karatsuba (tab[i].arg, tab[i+1].arg, var);
      /* Otherwise use a classical basecase without any limits */
      if (MAY_UNLIKELY (result == NULL))
        result = expand_mul_two_sum (tab[i].arg, tab[i+1].arg);
#if defined(MAY_WANT_ASSERT)
      else {
        may_t result2 = expand_mul_two_sum (tab[i].arg, tab[i+1].arg);
        MAY_ASSERT (may_identical (result, result2) == 0);
      }
#endif
      /* Save the result */
      MAY_ASSERT (MAY_TYPE (result) == MAY_SUM_T);
      tab[i].arg  = result;
      tab[i].size = MAY_NODE_SIZE(result);
      /* Compact table */
      memmove (&tab[i+1], &tab[i+2], (char*)&tab[n]-(char*)&tab[i+1]);
      n--;
      continue;
    }
    i++;
  }
  /* Step D */
  qsort (tab, n, sizeof (struct item), cmp_size);
#if 0
  for (i = 0; i < n ; i ++) {
    printf ("i=%d tab[].pureint=%d tab[].size=%d arg=", i, tab[i].pureint, tab[i].size);
    may_dump (tab[i].varlist);
  }
#endif
  /* Step E */
  may_t accu = tab[0].arg;
  MAY_RECORD ();
  for (i = 1; i < n ; i++) {
    if (tab[i].size == 0)
      break;
    accu = expand_mul_two_sum (accu, tab[i].arg);
    MAY_COMPACT (accu);
  }
  MAY_ASSERT (MAY_TYPE (accu) == MAY_SUM_T);
  /* Step F */
  if (MAY_UNLIKELY (i < n)) {
    may_size_t j;
    n = n-i+1;
    may_t temp[n];
    temp[0] = accu;
    for (j = 1; j < n; j++)
      temp[j] = tab[i++].arg;
    accu = expand_mul_basecase (temp, n, MAY_NODE_SIZE(accu));
  }
  return accu;
}

/* Check if the terms of the sum are not too dependent,
   algebracaly speaking, ie expanded (sum)^N has nearly the same number
   of terms as sum. */
static int
test_algebra_dependency_p (may_t sum)
{
  may_size_t i,n,s;

  MAY_ASSERT (MAY_TYPE (sum) == MAY_SUM_T);
  /* Today, we only check if there is some integer powered to a fraction in the sum. */
  n = MAY_NODE_SIZE(sum);
  MAY_ASSERT (n >= 2);
  for (i = s = 0; i<n; i++){
    may_t term = MAY_AT (sum,i);
    if (MAY_PURENUM_P (term)
        || (MAY_TYPE (term) == MAY_POW_T
            && MAY_TYPE (MAY_AT (term, 0)) == MAY_INT_T
            && MAY_TYPE (MAY_AT (term, 1)) == MAY_RAT_T))
      s +=1;
  }
  /* FIXME: Need test to compute threshold*/
  return 4*(n-s) < n;
}

/* Compute x^n using 'fast exponent' trick.
   It is a win only if there is some cancelations in the power */
static may_t
expand_pow_binary (may_t x, long n)
{
  long i, m;
  may_t r;
  may_t tab[3];

  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_ASSERT ((MAY_FLAGS (x) & MAY_EXPAND_F) == MAY_EXPAND_F);
  MAY_ASSERT (MAY_TYPE (x) == MAY_SUM_T);

  MAY_LOG_FUNC (("%Y",x));

  for (i = 0, m = n; m != 0; i++)
    m >>= 1;
  r = x;

  MAY_RECORD ();
  for (i -= 2; i >= 0; i--) {
    if (n & (1UL << i)) {
      tab[0] = r;
      tab[1] = r;
      tab[2] = x;
      r = expand_mul_basecase (tab, 3,
                               MAY_NODE_SIZE(r)*MAY_NODE_SIZE(r)*MAY_NODE_SIZE(x));
    } else {
      tab[0] = tab[1] = r;
      r = expand_mul_basecase (tab, 2,
                               MAY_NODE_SIZE(r)*MAY_NODE_SIZE(r) );
    }
    r = may_eval (r);
    MAY_COMPACT (r);
  }
  return r;
}

static MAY_REGPARM may_t
may_expand_recur (may_t x)
{
  may_t y;
  may_size_t i, n;

  /* Check if x is already expanded ( ~ 67% of the cases) */
  if (MAY_LIKELY (MAY_FLAGS (x) & MAY_EXPAND_F))
    return x;

  MAY_LOG_FUNC (("%Y",x));

  may_mark();
  switch (MAY_TYPE(x))
    {
    case MAY_PRODUCT_T: /* 5% of the cases, but the ones which are slow */
      {
	/* 1. Compute all the sub args and get the size of the result */
	n = MAY_NODE_SIZE(x);
	may_t *arg = may_alloc (n*sizeof *arg);
	may_size_t final = 1, nsum = 0;
        int isnew = 0;
        /* FIXME: Ne serait-il pas mieux de développer le produit, puis de développer chaque terme après (et pas avant) ? */
	for (i = 0; MAY_LIKELY (i < n); i++) {
          may_t zz = MAY_AT (x, i);
          arg[i] = may_expand_recur (zz);
          isnew |= (arg[i] != zz);
          if (MAY_TYPE(arg[i]) == MAY_SUM_T) {
            may_size_t size = MAY_NODE_SIZE(arg[i]);
            MAY_ASSERT (size >= 2);
            /* Handle potential overflow */
            if ((final > MAY_EXPAND_BASECASE_THRESHOLD
                 || size > MAY_EXPAND_BASECASE_THRESHOLD)
                && nsum > 0)
              final = 2*MAY_EXPAND_BASECASE_THRESHOLD;
            else
              final *= size;
            nsum ++;
          }
        }
        
	/* 2. Check special case when no expansion (which is likely: 99.5%) */
	if (MAY_LIKELY (final == 1)) {
          if (MAY_UNLIKELY (isnew)) {
	    y = MAY_NODE_C (MAY_PRODUCT_T, n);
            for (i = 0; MAY_LIKELY (i < n); i++)
              MAY_SET_AT (y, i, arg[i]);
          } else
            y = x;
          break;
	}

	/* 3. Alloc expanded result */
        if (MAY_LIKELY (nsum == 1 || final < MAY_EXPAND_BASECASE_THRESHOLD )) /* It is likely to be a simple expand */
          y = expand_mul_basecase (arg, n, final);
        /* Theses are very unlikely (0.000825% of the total expand calls!) BUT quite heavy in CPU time */
        else
          y = expand_mul_heavy (arg, n);
      }
      break;

    case MAY_POW_T: /* 4% of the cases, but the other ones which are slow*/
      /* (A + B) ^N --> Multinome */
      ; may_t base = may_expand_recur (MAY_AT(x, 0));
      if (MAY_UNLIKELY (MAY_TYPE (MAY_AT (x, 1)) == MAY_INT_T
                        && MAY_TYPE (base) == MAY_SUM_T
                        && mpz_fits_ushort_p (MAY_INT (MAY_AT (x, 1))) )) /* .024 */
	{
	  unsigned long expo;
          int success = may_get_ui (&expo, MAY_AT(x, 1));
          UNUSED (success);
          MAY_ASSERT (success == 0);
          n = MAY_NODE_SIZE(base);
          MAY_ASSERT (expo >= 2);
          MAY_ASSERT (n >= 2);
          if (MAY_UNLIKELY (expo == 2)) { /* special fast case: (sum ai) ^2 */
            /* FIXME: Overflow ? */
            may_size_t finalsize = n * (n+1) / 2, i, j, pos = 0;
            y = MAY_NODE_C (MAY_SUM_T, finalsize);
            for (i = 0; i < n; i++) {
              may_t z = may_pow_c (MAY_AT (base, i), MAY_TWO);
              MAY_SET_AT (y, pos++, z);
              for (j = i + 1; j < n; j++) {
                may_t z = may_mul_vac (MAY_TWO, MAY_AT (base, i),
                                       MAY_AT (base, j), NULL);
                MAY_SET_AT (y, pos++, z);
              }
            }
            MAY_ASSERT (pos == finalsize);
            /* Special case: (1+sqrt(5))^1000 */
          } else if (test_algebra_dependency_p (base)) {
            y = expand_pow_binary (base, expo);
            /* TODO: special case: (a+b)^N = sum(cNp(n,i)*a^i*b^(n-i),i=0..n) ??*/
          } else { /* Generic case */
            /* Create the fact tab */
            mpz_t fact[expo+1], temp;
            mpz_init_set_ui (fact[0], 1);
            for (i = 1 ; MAY_LIKELY (i <= expo); i++)
              mpz_init (fact[i]), mpz_mul_ui (fact[i], fact[i-1], i);
            mpz_init_set (temp, fact[expo]);
            /* We need to make a sum over all the ai such that sum(ai)=expo */
            unsigned int  a[n], s[n];  /* The ai and the sum */
            int  i, j;        /* Needs int, not unsigned type */
            may_t z, num;
            for (i = 0 ; MAY_LIKELY (i < (int) (n-1)); i++)
              a[i] = s[i] = 0;
            a[n-1] = s[n-1] = expo;
            /* Precompute the sum */
            unsigned long final_size = multinom_size (n, expo), pos = 0;
            y = MAY_NODE_C (MAY_SUM_T, final_size);
            /* Start Sum */
            while (1) {
	    begin_loop:
	      /* Compute Product( ai! ) */
	      mpz_set (temp, fact[a[0]]);
	      for ( i = 1 ; MAY_LIKELY (i < (int) n); i++)
		mpz_mul (temp, temp, fact[a[i]]);
	      mpz_divexact (temp, fact[expo], temp);
              num = may_set_z (temp);
	      /* Compute Product (xi ^ai) (Optimisation if ai=1 or ai=0) */
	      z = MAY_NODE_C (MAY_PRODUCT_T, n);
	      for (i = 0, j = 0 ; MAY_LIKELY (i < (int) n); i++) {
                if (a[i] == 1)
                  MAY_SET_AT(z, j++, MAY_AT (base, i));
                else if (a[i] != 0)
                  MAY_SET_AT (z, j++, may_pow_c (MAY_AT (base, i),
                                                 MAY_ULONG_C (a[i])));
              }
	      MAY_NODE_SIZE(z) = j;
	      z = may_mul_c (num, z);
	      MAY_SET_AT (y, pos++, z);
	      /* Next partition of ai */
	      i = n-2;
	      do {
		a[i]++; s[i]++;
		for (j = i+1; MAY_LIKELY (j <= (int) (n-2)); j++)
                  s[j] -= (a[i+1]-1);
		a[i+1] = 0;
		if (s[i] <= expo) {
                  MAY_ASSERT (s[n-1] >= s[n-2]);
                  a[n-1] = s[n-1] - s[n-2];
                  MAY_ASSERT (s[n-1] == expo);
                  goto begin_loop;
                }
		i--;
	      } while (i>=0);
	      break;
	    }
            MAY_ASSERT (pos == final_size);
          }
	  break;
	}
      /* else go down to the default handling */
      if (base != MAY_AT (x, 0)) {
        /* base has been expanded, so we have done something */
        y = MAY_NODE_C (MAY_POW_T, 2);
        MAY_SET_AT (y, 0, base);
        MAY_SET_AT (y, 1, MAY_AT (x, 1));
      } else
        y = x;
      break;

    case MAY_FACTOR_T:
      y = may_expand_recur (MAY_AT(x, 1));
      if (MAY_LIKELY(y != MAY_AT(x, 1))) {
        may_t z = MAY_NODE_C (MAY_FACTOR_T, 2);
        MAY_SET_AT(z, 0, MAY_AT(x, 0));
        MAY_SET_AT(z, 1, y);
        y = z;
      } else {
        y = x;
      }
      break;
    
    case MAY_SUM_T: /* 90((+5% of the previous cases) */
      n = MAY_NODE_SIZE(x);
      y = MAY_NODE_C (MAY_SUM_T, n);
      int rebuild = 1;
      for (i = 0 ; MAY_LIKELY (i < n); i++) {
	may_t xi = MAY_AT (x, i);
	may_t z = may_expand_recur (xi);
	MAY_SET_AT (y, i, z);
	rebuild &= (z == xi);
      }
      /* Check if something has changed, otherwise we keep x */
      if (MAY_LIKELY (rebuild))
	y = x;
      break;

      /* expand is not recursive by default */
    default:
      y = x;
      break;
    }
  y = may_eval (y);
  MAY_SET_FLAG (y, MAY_EXPAND_F);
  return may_keep (y);
}

may_t
may_expand (may_t x)
{
  MAY_ASSERT (MAY_EVAL_P (x));
  MAY_LOG_FUNC (("%Y",x));

  x = may_expand_recur (x);
  return x;
}
