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

#define MAX_OPERATOR_STACK 6
#define MAX_OPERANDE_STACK MAX_OPERATOR_STACK+1

#define SKIP_SPACE(s) while (*s==' ') s++;

static may_t
create_binary_tree (may_t x, may_t y)
{
  may_t r = MAY_NODE_C (MAY_BINARYTREE_T, 2);
  MAY_SET_AT (r, 0, x);
  MAY_SET_AT (r, 1, y);
  return r;
}

static MAY_REGPARM may_t
create_single_list (may_t x)
{
  may_t list = MAY_NODE_C (MAY_LIST_T, 1);
  MAY_SET_AT (list, 0, x);
  return list;
}

static MAY_REGPARM may_t
make_list (may_t x)
{
  return MAY_TYPE (x) == MAY_LIST_T ? x : create_single_list (x);
}

/* Transform a list from
      /\                           |     |
     /\          ---->          / / \ \  |
    /\                                   |
*/
static may_t
transform_binary_tree (may_t list)
{
  MAY_ASSERT (MAY_TYPE(list) == MAY_BINARYTREE_T);
  /* Pass 1: Count # of items */
  unsigned long count = 1;
  may_t x = list;
  do {
    MAY_ASSERT (MAY_NODE_SIZE(x) == 2);
    count++;            /* Count Right branch */
    x = MAY_AT(x, 0);   /* Go throught Left Branch */
  } while (MAY_TYPE(x) == MAY_BINARYTREE_T);

  /* Pass 2: Create the new list */
  may_t result = MAY_NODE_C(MAY_LIST_T, count);
  x = list;
  count--;
  do {
    MAY_SET_AT (result, count, MAY_AT(x, 1)); /* Set right branch */
    x = MAY_AT(x, 0);                         /* Go throught Left Branch */
    count--;
  } while (MAY_TYPE(x) == MAY_BINARYTREE_T);
  MAY_ASSERT (count == 0);
  MAY_SET_AT (result, 0, x);
  return result;
}

char *
may_parse_c (may_t *dest, const char str[])
{
  /* We can have:
     + an operator: + - * / ^
     + a variable: starts with a letter
     + a function: starts with a lettre, finished by a '('
     + an int: starts with a number.
     + a float: starts with a number. Contains 'E' and '.'.
     + a matrix: starts with '['
     + a list: starts with '{'
     + a parenthesis: starts with '('

     We use:
       + a stack of operande
       + a stack of operator.

     Read a char:
     Phase 1:
      Skip SPACE
      Read negate operator, and skip it.
      If it is a lettre, scans for variable/function.
         If it is a variable, push it in the operand stack.
	 If it is a function, parse the arguments as a list - allow list-
	 and push it in the operand stack.
      If it is a number, scans for 'E' or '.' while there are digits, and then:
         Build Int and push in the operand stack.
	 Build Float and push in the operand stack.
      If it is a parenthesis, parse the sub-express - unallow list- and push in the operand stack.
      Else stop parsing (Pop last Operator before finishing).
      If there was a negate operator, negate the pushed operator
       (Well in fact, it is a little big more complicated ;) )

     Phase 2:
      Skip SPACE.
      If it is an operator, add it in the operator stack so that
         all pushed operators have bigger priority than this one.
	 If an already pushed operator has lower or equal priority
	 Then eval the operator (pop it) over the two pushed operands (pop them),
	 and push the new evaluated operand.
      Else stop parsing.

      Phase Finish:
        Finish by evaluating the pushed operators:
            While OpcRef>=1
               func = Opc[--OpcRef]
               op   = func( Op[--OpRef], Op[--OpRef])
               OpRef[OpRef++] = op
	    ASSERT (OpRef == 1)
*/
  static const struct {
    char code;
    char prio;
    may_t (*func)(may_t, may_t);
  } OpcTable[] = {
    /* {';', 0, create_binary_tree}, */
    {',', 1, create_binary_tree},
    {'+', 2, may_addinc_c},
    {'-', 2, may_sub_c},
    {'*', 3, may_mulinc_c},
    {'/', 3, may_div_c},
    {1,   4, may_mul_c}, /* Used to simulate the neg for a code which can't be present */
    {'^', 5, may_pow_c},
    {2,   6, may_mul_c} /* Used to simulate the neg for a code which can't be present */
  };
  mpfr_t f;
  mpz_t  z;
  may_t  Op[MAX_OPERANDE_STACK];
  int    OpcRef = 0, OpRef = 0;
  unsigned int j;
  char   Opc[MAX_OPERATOR_STACK];
  char   ch;

  mpz_init (z);
  mpfr_init (f);

  for (;;)
    {
      /* Phase 1: Add an operande */
      SKIP_SPACE (str);
      ch = *str;

      /* Check if operator negate before operande */
      if (MAY_UNLIKELY (ch == '-')) {
        Op[OpRef++] = MAY_MPZ_NOCOPY_C (MAY_INT (MAY_N_ONE));
        /* Usually -x has to be considered like multiply with a slighty higher priority
           and lesser than pow, except for x^-x where it has to be considered higher than pow */
	ch = (OpcRef == 0 || OpcTable[(int)Opc[OpcRef-1]].code != '^') ? 1 : 2;
        goto handle_operator;
      }

      /* Check if the operande is a number */
      if (isdigit (ch)) {
	const char *a = str + 1;
	while (isdigit (*a)) a++;
	if (*a == 'E' || *a == '.' || *a =='@') {
          if (may_g.frame.num_presimplify) {
            /* For float, it is easy due to strtofr */
            mpfr_strtofr (f, str, (char**) &a, 0, MAY_RND);
            if (a == str) /* Parsing failed */
              goto error_code;
            Op[OpRef++] = may_set_fr (f);
          } else {
            /* We need to create a MAY_STRING_T to avoid preconverting it
               But we need to parse it. We still uses MPFR to parse it,
               but we uses a float of prec 2 to go slighty faster */
            MPFR_DECL_INIT (f2, 2);
            mpfr_strtofr (f2, str, (char**) &a, 0, MAY_RND);
            if (a == str) /* Parsing failed */
              goto error_code;
            {
              unsigned long n = a - str + 2;
              char Buffer[n];
              Buffer[0] = '#';
              memcpy (&Buffer[1], str, n-2);
              Buffer[n-1] = 0;
              Op[OpRef++] = may_set_str (Buffer);
            }
          }
	} else {
	  /* For integer, we need to copy it in a buffer first */
	  unsigned long n = a-str+1;
	  char buffer[n];
	  memcpy (buffer, str, n);
	  buffer[n-1] = 0;
	  if (mpz_set_str (z, buffer, 10))
	    goto error_code;
	  Op[OpRef++] = MAY_MPZ_C (z);
	}
	str = a;
      }
      /* Check if the operande is a variable */
      else if (isalpha (ch) || ch == '$' || ch == '_') {
	const char *a = str+1;
	while (isalnum (*a) || *a=='_') a++;
	unsigned long n = a-str+1;
	char buffer[n];
	memcpy (buffer, str, n);
	buffer[n-1] = 0;
	str = a;
	SKIP_SPACE (str);
	/* Check if it is a function call or a variable */
	if (*str == '(') {
	  /* In case of function call, parse the sub arguments */
	  const char *new = may_parse_c (&Op[OpRef], str+1);
	  if (*new++ == ')') {
	    str = new;
            may_t temp = may_parse_extension (buffer, Op[OpRef]);
            if (temp == NULL)
              temp = may_func_c (buffer, Op[OpRef]);
            Op[OpRef] = temp;
	    OpRef++;
	  }
	  else
	    goto error_code;
	} else
	  /* It is a variable: add it in the operande buffer */
	  Op[OpRef++] = may_set_str (buffer);
      }
      /* Check if the operande is an expression inside ( ) */
      else if (ch == '(') {
	const char *new = may_parse_c (&Op[OpRef], str+1);
	if (*new++ == ')') {
	  str = new;
	  OpRef++;
	} else
	  goto error_code;
      }
      /* Check if it is a LIST  */
      else if (ch == '{') {
	const char *new = may_parse_c (&Op[OpRef], str+1);
	if (*new++ == '}') {
	  Op[OpRef] = make_list (Op[OpRef]);
	  str = new;
	  OpRef++;
	} else
	  goto error_code;
      }
      /* Check if it is a matrix */
      else if (ch == '[') {
	may_t l1, l2;
	const char *new;
	/* Read 1st Row */
	new = may_parse_c (&l1, str+1);
	l1 = make_list (l1);
	ch = *new++;
	/* Check if there are any others rows */
	if (MAY_UNLIKELY(ch == ']')) {
	  l1 = create_single_list (l1);
	  goto mat_end;
	} else if (MAY_UNLIKELY(ch != ';'))
	  goto error_code;
	/* Read other rows */
	do {
	  new = may_parse_c (&l2, new);
	  ch = *new++;
	  if (ch != ']' && ch != ';')
	    goto error_code;
	  l2 = make_list (l2);
	  l1 = create_binary_tree (l1, l2);
	} while (ch == ';');
	/* Build the matrix */
	l1 = transform_binary_tree (l1);
      mat_end:
	MAY_OPEN_C (l1, MAY_MAT_T);
	str = new;
	Op[OpRef++] = l1;
      }
      /* We can't parse the operande: invalid expression, or end
	 of previous expression (like ')' or ']' */
      else {
      error_code:
	if (OpcRef >= 1) {
	  OpcRef--; /* Pop last pushed operator */
	  str--;
	}
	break;
      }

      /* Check if there is a '!' operator */
      SKIP_SPACE (str); ch = *str;
      if (ch == '!') {
        Op[OpRef-1] = may_fact_c (Op[OpRef-1]);
        str++;
        SKIP_SPACE (str); ch =*str;
      }

      /* Phase 2: add an operator 'ch' */
    handle_operator:
      for (j = 0; j < numberof(OpcTable); j++) {
	if (ch == OpcTable[j].code) {
	  while (OpcRef >= 1
		 && OpcTable[j].prio <= OpcTable[(int)Opc[OpcRef-1]].prio) {
	    MAY_ASSERT (OpRef >= 2);
	    may_t arg2 = Op[--OpRef];
	    may_t arg1 = Op[--OpRef];
	    Op[OpRef++] = OpcTable[(int)Opc[--OpcRef]].func (arg1, arg2);
	  }
	  Opc[OpcRef++] = j;
	  str++;
	  break;
	}
      }
      if (j == numberof(OpcTable))
	break;

      MAY_ASSERT (OpcRef <= MAX_OPERATOR_STACK);
    } /* END FOR */

  /* Finish the parsing: Add all remaining Operator */
  while (OpcRef > 0) {
    MAY_ASSERT (OpRef >= 2);
    may_t arg2 = Op[--OpRef];
    may_t arg1 = Op[--OpRef];
    Op[OpRef++] = OpcTable[(int)Opc[--OpcRef]].func (arg1, arg2);
  }

  /* Clean up the returned expressions */
  if (OpRef != 1)
    Op[0] = MAY_NAN;
  if (MAY_TYPE (Op[0]) == MAY_BINARYTREE_T)
    Op[0] = transform_binary_tree (Op[0]);
  *dest = Op[0];
  return (char*) str;
}

may_t
may_parse_str (const char str[])
{
  may_t r;
  MAY_RECORD ();
  str = may_parse_c (&r, str);
  MAY_RET ( *str == 0 ? may_eval (r) : MAY_NAN);
}
