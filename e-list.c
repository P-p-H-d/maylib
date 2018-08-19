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

static may_t
list_eval (may_t l)
{
  unsigned long i, n;
  MAY_ASSERT (may_ext_p (l) == MAY_LIST_T);
  /* FIXME/TODO/BUG: Reeval may be called to reevaluate==> l is evaluated! */
  n = may_nops (l);
  for (i = 0; i < n; i++)
    may_ext_set_c (l, i, may_eval (may_op (l, i)));
  return l;
}

static unsigned long
list_add (unsigned long start, unsigned long end, may_pair_t *tab)
{
  unsigned long i, j, size;
  may_pair_t temp[end];
  may_t list;

  MAY_ASSERT (may_ext_p (tab[start].second) == MAY_LIST_T);
  size = may_nops (tab[start].second);
  list = may_ext_c (MAY_LIST_T, size);
  for (j = 0; j < start; j++)
    temp[j] = tab[j];
  for (i = 0; i < size ; i++) {
    for (j = start ; j < end;j++) {
      MAY_ASSERT (may_ext_p (tab[j].second) == MAY_LIST_T);
      if (MAY_UNLIKELY (may_nops (tab[j].second) != size))
        may_error_throw (MAY_DIMENSION_ERR, __func__);
      temp[j].first  = tab[j].first;
      temp[j].second = may_op (tab[j].second, i);
    }
    MAY_ASSERT (j <= numberof (temp));
    may_ext_set_c (list, i, may_eval (may_addmul_vc (end, temp)));
  }
  tab[0].first  = MAY_ONE;
  tab[0].second = may_eval (list);
  return 1;
}

static unsigned long
list_mul (unsigned long start, unsigned long end, may_pair_t *tab)
{
  unsigned long i, j, size;
  may_pair_t temp[end];
  may_t list;

  MAY_ASSERT (may_ext_p (tab[start].second) == MAY_LIST_T);
  size = may_nops (tab[start].second);
  list = may_ext_c (MAY_LIST_T, size);
  for (j = 0; j < start; j++)
    temp[j] = tab[j];
  for (i = 0; i < size ; i++) {
    for (j = start ; j < end;j++) {
      MAY_ASSERT (may_ext_p (tab[j].second) == MAY_LIST_T);
      if (MAY_UNLIKELY (may_nops (tab[j].second) != size))
        may_error_throw (MAY_DIMENSION_ERR, __func__);
      temp[j].first  = tab[j].first;
      temp[j].second = may_op (tab[j].second, i);
    }
    MAY_ASSERT (j <= numberof (temp));
    may_ext_set_c (list, i, may_eval (may_mulpow_vc (end, temp)));
  }
  tab[0].first  = MAY_ONE;
  tab[0].second = may_eval (list);
  return 1;
}

static may_t
list_pow (may_t base, may_t expo)
{
  int c = 2*(MAY_TYPE (base) == MAY_LIST_T) + (MAY_TYPE (expo) == MAY_LIST_T);
  may_size_t i, n;
  may_t y;

  MAY_ASSERT (c != 0);
  switch (c) {
  case 1:
    /* EXPO is a list */
    n = may_nops (expo);
    y = may_ext_c (MAY_LIST_T, n);
    for (i = 0; i < n; i++)
      may_ext_set_c (y, i, may_pow_c (base, may_op (expo, i)));
    break;
  case 2:
    /* base is a list */
    n = may_nops (base);
    y = may_ext_c (MAY_LIST_T, n);
    for (i = 0; i < n; i++)
      may_ext_set_c (y, i, may_pow_c (may_op (base, i), expo));
    break;
  case 3:
  default:
    MAY_ASSERT (c == 3);
    /* Expo and base are lists */
    n = may_nops (base);
    if (MAY_UNLIKELY (n != may_nops (expo)))
      may_error_throw (MAY_DIMENSION_ERR, __func__);
    y = may_ext_c (MAY_LIST_T, n);
    for (i = 0; i < n; i++)
      may_ext_set_c (y, i, may_pow_c (may_op (base, i), may_op (expo, i)));
    break;
  }

  return may_eval (y);
}

static may_t
list_constructor (may_t list)
{
  MAY_ASSERT (may_ext_p (list) == MAY_LIST_T);
  /* Nothing to do when parsing LIST(....) */
  return list;
}

static const may_extdef_t list_cb = {
  .name = "LIST",
  .priority = 100,
  .eval = list_eval,
  .add = list_add,
  .mul = list_mul,
  .pow = list_pow,
  .constructor = list_constructor
};

void may_elist_init (void)
{
  /* Don't need to create a global list_ext, since it is assumed
     list_ext is MAY_LIST_T (Since LIST extension is a mandatory
     extension ) */
  may_ext_t list_ext = may_ext_register (&list_cb, MAY_EXT_INSTALL);
  UNUSED (list_ext);
  MAY_ASSERT (list_ext != 0);
  MAY_ASSERT (list_ext == MAY_LIST_T);
}


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
#if 0

static may_ext_t modulo_ext;

/* Eval may reuse x */
static may_t
modulo_eval (may_t x)
{
  may_t a, c;

  /* Get the class */
  c = may_eval (may_op (x, 1));
  /* The modulo can be only an integer */
  assert (may_get_name (c) == may_integer_name);
  
  /* Z/cZ */
  may_t old = may_kernel_intmod (c);
  /* Reval the operand inside the new class */
  a = may_reeval (may_op (x, 0));
  may_kernel_intmod (old);

  /* Construct the result */
  may_ext_set_c (x, 0, a);
  may_ext_set_c (x, 1, c);
  return x;
}

/* Pb mod(xxx,5) + mod(xxx,7) --> ? */
/* */
static int
modulo_cmp (const void *a, const void *b)
{
  may_t ma = *(may_t*)a;
  may_t mb = *(may_t*)b;
  assert (may_ext_p (ma) == modulo_ext);
  assert (may_ext_p (mb) == modulo_ext);
  return may_identical (may_op (ma, 1), may_op (mb, 1));
}

/* Another way for the interface for sum */
/* IN:
   From 0 to start-1  : previous extensions (non modulos)
   From start to end-1: modulo extensions
   OUT:
   From 0 to (return_value-1): the updated table.
*/
static unsigned long
modulo_add (unsigned long start, unsigned long end, may_t *tab)
{
  unsigned long i, j, k;
  may_t c;

  /* Check if we have to sort the table because of different modulos */
  for (i = start+1 ; i< end ; i++) {
    assert (may_ext_p (tab[i]) == modulo_ext);
    if (may_identical (may_op (tab[i-1], 1), may_op (tab[i], 1)) != 0) {
      /* Sort the modulos because we can't add Z/5Z and Z/7Z (for example) */
      qsort (&tab[start], start-end, sizeof tab[0], modulo_cmp);
      break;
    }
  }

  /* Process them and update the table */
  j = k = 0;
  c = may_op (tab[start], 1);
  for ( i = 1; i <= end; i++)
    if (i == end
	|| (i >= start && may_identical (c, may_op (tab[i],1)) != 0)) {
      /* if there is only one term, we don't need to reevaluate it */
      if (i-j == 1) {
	tab[k++] = tab[j];
      } else {
	/* Construct the result */
	may_t a  = may_add_vc (i-j, &tab[j]);
	may_t nw = may_ext_c (modulo_ext, 2);
	may_ext_set_c (nw, 0, a);
	may_ext_set_c (nw, 1, c);
	/* Save the result */
	tab[k++] = may_eval (nw);
      }
      assert (k <= end);
      /* Prepare next iteration */
      j = i;
      c = i == end ? NULL : may_op (tab[i], 1);
    }
  /* Return the new number of valid entries in the table */
  return k;
}

static const may_extdef_t modulo_cb = {
  .name = "INTMOD",
  .priority = 20,
  .eval = modulo_eval,
  .add  = modulo_add
};

void modulo_init (void)
{
  modulo_ext = may_ext_register (&modulo_cb, MAY_EXT_INSTALL);
  if (modulo_ext == 0)
    abort ();
}

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

static may_ext_t simpbool_ext;

static may_t
simpbool_eval (may_t x)
{
  return x;
}

static unsigned long
simpbool_add (unsigned long start, unsigned long end, may_t tab[])
{
  return end;
}

static unsigned long
simpbool_mul (unsigned long start, unsigned long end, may_t tab[])
{
  return end;  
}

static const may_extdef_t simpbool_cb = {
  .name = "SIMPBOOL",
  .priority = 2,
  .eval = simpbool_eval,
  .add  = simpbool_add,
  .mul  = simpbool_mul
};

void simpbool_init (void)
{
  simpbool_ext = may_ext_register (&simpbool_cb, MAY_EXT_INSTALL);
  if (simpbool_ext == 0)
    abort ();
}

#endif
