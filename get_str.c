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

/* Put a character */
#define PUT(c) do {                                             \
  if (MAY_UNLIKELY( may_g.Heap.top == may_g.Heap.limit ))       \
    may_throw_memory ();                                        \
  may_g.Heap.limit --;                                          \
  *(char*)may_g.Heap.limit = (c);                               \
} while (0)

/* Get the last put character */
#define GET() (*(char*)may_g.Heap.limit)

static void convert (may_t x, int level, int abs_sign);

/* Put a string */
static void
put_string (const char s[])
{
  while (*s)
    PUT (*s++);
}

/* Put a string and then a ( */
static void
put_string_and_bracket (const char s[])
{
  put_string (s);
  PUT ('(');
}

/* Check if it is a negative exponent */
static int
negative_expo_p (may_t x)
{
  may_t z;
  if (MAY_TYPE (x) != MAY_POW_T)
    return 0;
  z = MAY_AT (x, 1);
  switch (MAY_TYPE (z))
    {
    case MAY_INT_T:
    case MAY_RAT_T:
    case MAY_FLOAT_T:
      return MAY_NEG_P (z);
    case MAY_FACTOR_T:
      return MAY_NEG_P (MAY_AT (z, 0));
    default:
      return 0;
    }
}

static void
convert_for_convert2str (may_t x, int level)
{
  convert (x, level, 0);
}


/* abs_sign == 0 : display normal
   abs_sign == 1 : display absolute value
   abs_sign == 2 : Don't display { and } */
/* Level: 0 -> +
   Level: 1 -> * .
   Level: 2 -> ^
   Level: 3 -> Func
   Level: 4 -> Int
*/
static void
convert (may_t x, int level, int abs_sign)
{
  void *top;
  const char *s;
  may_size_t i, n;
  mp_exp_t e;
  int dummy;

  switch (MAY_TYPE(x))
    {
    case MAY_INT_T:
      /* Save the memory pointer */
      top = may_g.Heap.top;
      s = mpz_get_str (NULL, may_g.frame.base, MAY_INT (x));
      MAY_ASSERT (s != NULL);
      if (s[0] == '-') {
        if (abs_sign)
          s++;
        else if (GET() == '+' && level <= 2)
          may_g.Heap.limit++; /* '+', '-' ==> '-' */
      }
      if (level > 3 && s[0] == '-')
        PUT ('(');
      put_string (s);
      if (level > 3 && s[0] == '-')
        PUT (')');
      may_g.Heap.top = top;
      break;

    case MAY_RAT_T:
      top = may_g.Heap.top;
      s = mpq_get_str (NULL, may_g.frame.base, MAY_RAT (x));
      MAY_ASSERT (s != NULL);
      if (s[0] == '-') {
        if (abs_sign)
          s++;
        else if (GET() == '+' && level <= 2)
          may_g.Heap.limit ++; /* '+', '-' ==> '-' */
      }
      if (level > 2)
	PUT ('(');
      put_string (s);
      if (level > 2)
	PUT(')');
      may_g.Heap.top = top;
      break;

    case MAY_FLOAT_T:
      top = may_g.Heap.top;
      s = mpfr_get_str (NULL, &e, may_g.frame.base, 0, MAY_FLOAT(x), MAY_RND);
      dummy = 0;
      MAY_ASSERT (s != NULL);
      if (s[0] == '-') {
        if (abs_sign) /* If abs, skip sign */
          s++;
        else {
          if (GET() == '+')
            may_g.Heap.limit ++; /* '+', '-' ==> '-' */
          else if (level > 3) {
            PUT ('(');
            dummy = 1;
          }
          PUT (*s++); /* Put sign */
        }
      }
      /* @NaN@ or @Inf@ or -@InF@ */
      if (s[0] == '@') {
        if (s[1] == 'N')
          put_string ("NAN");
        else
          put_string ("INFINITY");
        goto end_mpfr;
      }
      /* Check for Zero */
      if (mpfr_cmp_ui (MAY_FLOAT (x), 0) == 0) {
        put_string ("0.");
        goto end_mpfr;
      }
      /* Put mantissa */
      PUT(*s++);
      PUT('.');
      put_string (s);
      /* Remove optionnal ending zeros */
      while (GET() == '0')
	may_g.Heap.limit ++;
      /* Put exponent */
      e--; /* Fix, since we print X.XXXX rather than 0.XXXX */
      if (e != 0) {
        char temp[4*sizeof (mp_exp_t)];
        PUT (may_g.frame.base > 10 ? '@' : 'E');
        sprintf (temp, "%ld", (long) e);
        put_string (temp);
      }
    end_mpfr:
      may_g.Heap.top = top;
      if (dummy)
        PUT (')');
      break;

    case MAY_COMPLEX_T:
      /* 1. Special case I */
      if ((dummy = MAY_ZERO_P(MAY_RE(x))) && MAY_ONE_P(MAY_IM(x)))
	PUT('I');
      /* Check for -1*I = -I */
      else if (dummy && MAY_TYPE(MAY_IM(x)) == MAY_INT_T
	       && mpz_cmp_si (MAY_INT(MAY_IM(x)), -1) == 0) {
        if (GET() == '+')
          may_g.Heap.limit ++; /* '+', '-' ==> '-' */
        if (level > 2)
          PUT ('(');
        PUT ('-');
        PUT ('I');
        if (level > 2)
          PUT (')');
      }
      /* Check for pure imaginary */
      else if (dummy) {
        if (level > 2)
          PUT ('(');
        convert (MAY_IM(x), 1, 0);
        put_string ("*I");
        if (level > 2)
          PUT (')');
      }
      /* Generic complex */
      else {
        if (level > 1)
          PUT ('(');
        convert (MAY_RE(x), 1, 0);
        if (MAY_TYPE(MAY_IM (x)) == MAY_INT_T
            && mpz_cmp_si (MAY_INT(MAY_IM (x)), -1) == 0)
          PUT ('-');
        else {
          PUT ('+');
          if (!MAY_ONE_P(MAY_IM(x))) {
            convert (MAY_IM(x), 1, 0);
            PUT ('*');
          }
          }
        PUT ('I');
        if (level > 1)
          PUT (')');
      }
      break;

    case MAY_STRING_T:
      /* If float string, do not display the leading '#' */
      put_string (MAY_NAME(x)[0] == '#' ? MAY_NAME(x)+1 : MAY_NAME(x));
      break;

    case MAY_EXP_T:
    case MAY_LOG_T:
    case MAY_SIN_T:
    case MAY_COS_T:
    case MAY_TAN_T:
    case MAY_ASIN_T:
    case MAY_ACOS_T:
    case MAY_ATAN_T:
    case MAY_SINH_T:
    case MAY_COSH_T:
    case MAY_TANH_T:
    case MAY_ASINH_T:
    case MAY_ACOSH_T:
    case MAY_ATANH_T:
    case MAY_ABS_T:
    case MAY_SIGN_T:
    case MAY_FLOOR_T:
    case MAY_CONJ_T:
    case MAY_REAL_T:
    case MAY_IMAG_T:
    case MAY_ARGUMENT_T:
    case MAY_GAMMA_T:
      put_string_and_bracket (may_get_name(x));
      convert (MAY_AT(x,0), 0, 0);
      PUT (')');
      break;

    case MAY_MOD_T:
    case MAY_GCD_T:
      put_string_and_bracket (may_get_name (x));
      convert (MAY_AT (x,0), 0, 0);
      PUT (',');
      convert (MAY_AT (x, 1), 0, 0);
      PUT (')');
      break;

    case MAY_DIFF_T:
      put_string_and_bracket (may_diff_name);
      n = MAY_NODE_SIZE(x);
      for (i = 0; i < n; i++) {
        convert (MAY_AT (x, i), 1, 0);
        if (i != (n-1))
          PUT (',');
      }
      PUT (')');
      break;

    case MAY_FUNC_T:
      put_string_and_bracket (MAY_NAME(MAY_AT(x,0)));
      convert (MAY_AT(x, 1), 0, 2);
      PUT (')');
      break;

    case MAY_SUM_T:
      {
        n = MAY_NODE_SIZE(x);
        if (level > 1)
          PUT ('(');
        for (i = 0 ; i < n ; i++)
          {
            convert (MAY_AT(x, i), 1, 0);
            if (i != n-1)
	      PUT ('+');
          }
        if (level > 1)
	  PUT (')');
      }
      break;

    case MAY_FACTOR_T:
      dummy = (MAY_TYPE (MAY_AT (x,0)) == MAY_INT_T
	       && mpz_cmp_si (MAY_INT (MAY_AT (x,0)), -1) == 0);
      /* transforms '-1'*x in x */
      if (dummy && abs_sign)
	convert (MAY_AT(x,1), level, 0);
      else
	{
	  if (level > 2)
	    PUT ('(');
	  /* Transforms '-1'*x in '-x' */
	  if (dummy)
	    {
	      if (GET() == '+')
		may_g.Heap.limit ++; /* '+', '-' ==> '-' */
	      PUT ('-');
	    }
	  else
	    {
	      convert (MAY_AT (x, 0), 2, abs_sign);
	      PUT ('*');
	    }
	  convert (MAY_AT (x, 1), 2, 0);
	  if (level > 2)
	    PUT (')');
	}
      break;

    case MAY_PRODUCT_T:
      {
	int star = 0;
        may_size_t expo_negative = 0;

        n = MAY_NODE_SIZE(x);
        if (level > 2)
	  PUT ('(');
        for (i = 0; i < n ; i++)
          {
	    may_t z = MAY_AT(x, i);
	    if (negative_expo_p (z))
	      {expo_negative++; continue; }
	    if (star)
	      PUT ('*');
	    convert (z, 2, 0);
	    star = 1;
          }
	if (expo_negative)
	  {
	    if (GET() == '*')
              may_g.Heap.limit ++;
            else if (expo_negative == n)
	      PUT ('1');
	    PUT ('/');
	    if (expo_negative > 1)
	      PUT ('(');
	    star = 0;
	    for (i = 0; i < n ; i++)
	      {
		may_t z = MAY_AT (x, i);
		if (!negative_expo_p (z))
		  continue;
		if (star)
		  PUT ('*');
		convert (z, 2, 1);
		star = 1;
	      }
	    if (expo_negative > 1)
	      PUT (')');
	  }
        if (level > 2)
	  PUT (')');
      }
      break;

    case MAY_POW_T:
      /* Check if exponent is -1 */
      dummy = (MAY_TYPE (MAY_AT (x, 1)) == MAY_INT_T
	       && mpz_cmp_si (MAY_INT (MAY_AT (x, 1)), -1) == 0);
      /* transforms 'x^-1' in x */
      if (dummy && abs_sign)
	convert (MAY_AT(x, 0), level, 0);
      /* transforms x^-N in 1/x^N if not called by product */
      else if (!abs_sign && negative_expo_p (x)) {
        if (level > 2)
          PUT ('(');
        if (GET() == '*')
          may_g.Heap.limit ++;
        else
          PUT('1');
        PUT('/');
        convert (MAY_AT(x, 0), 3, 0);
        if (!dummy) {
          PUT ('^');
          convert (MAY_AT(x, 1), 3, 1);
        }
        if (level > 2)
          PUT (')');
      } else {
        if (level > 2)
          PUT ('(');
        convert (MAY_AT(x, 0), 4, 0);
        PUT ('^');
        convert (MAY_AT(x, 1), 3, abs_sign);
        if (level > 2)
          PUT (')');
      }
      break;

    case MAY_LIST_T:
      /* Can't put it as an extension since it is used to display
         the functions too (abs_sign is used to not displayed
         { and } for functions */
      if (abs_sign != 2)
	PUT ('{');
      n = MAY_NODE_SIZE(x);
      for ( i = 0 ; i < (n-1) ; i++) {
        convert (MAY_AT(x, i), 0, 0);
        PUT (',');
      }
      convert (MAY_AT(x, i), 0, 0);
      if (abs_sign != 2)
	PUT ('}');
      break;

    case MAY_RANGE_T:
      /* TODO: Find another form */
      put_string_and_bracket (may_range_name);
      mp_rnd_t old = may_kernel_rnd (GMP_RNDD);
      convert (MAY_AT(x, 0), 0, 0);
      PUT (',');
      may_kernel_rnd (GMP_RNDU);
      convert (MAY_AT(x, 1), 0, 0);
      may_kernel_rnd (old);
      PUT (')');
      break;

    default:
      if (MAY_EXT_P (x)) {
        void (*convert2str) (may_t,int,void (*)(may_t,int),void (*)(const char*));
        convert2str = MAY_EXT_GETX(x)->stringify;
        if (convert2str != 0) {
          (*convert2str) (x, level, convert_for_convert2str, put_string);
        } else {
          /* Print a generic way for the extensions */
          put_string_and_bracket (MAY_EXT_GETX(x)->name);
          n = MAY_NODE_SIZE(x);
          for ( i = 0 ; i < n ; i++) {
            convert (MAY_AT(x, i), 0, 0);
            if (i != (n-1))
              PUT (',');
          }
          PUT (')');
        }
      } else
        MAY_THROW (MAY_INVALID_TOKEN_ERR);
    }
  return ;
}

char     *
may_get_string (char out[], const size_t length, may_t x)
{
  char *top, *end;
  char *s, *b;
  size_t size;
  size_t length2;

  /* Convert the string */
  end = may_g.Heap.limit;
  MAY_TRY {
    PUT (0); /* Needed due to GET() macro */
    convert (x, 0, 0);
    s = may_g.Heap.limit;
  } MAY_CATCH
      s = may_g.Heap.limit+1;
  MAY_ENDTRY;

  /* Save the top position */
  top = may_g.Heap.top;

  /* Nullify string */
  *--s = 0;
  size = end - s;

  /* Restore original limit */
  may_g.Heap.limit = end;

  /* If create string */
  if (out == NULL) {
    b = top;
    length2 = end - top;
  } else {
    b = out;
    length2 = length;
  }
  /* Check overflow */
  if (size > length2)
    return NULL;

  /* Reverse the string */
  s = end - 1;
  while (*--s) *b++ = *s;
  *b++ = 0;

  /* Fix final stuff */
  if (out == NULL) {
    may_g.Heap.top = top + MAY_ALIGNED_SIZE ((size_t)(b-top));
    return top;
  }

  return out;
}
