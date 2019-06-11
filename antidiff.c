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

/* This antidiff algorithm is the very simple:
   it just looks for the expression inside a precomputed table */

/* First field is the 'ByteCode' filter: it is used to quickly match (or unmatch) the expression with the required expression
   Second field is the antidiff expression if the filter successed.
   Finally, it is some additional conditions about the variables used in the filter */
typedef struct {
  const char *ByteCode;
  const char *expression;
  may_antidiff_condition_e condition;
} may_antidiff_table_t;

/* Table of antidiff */
/*  BYTECODE syntax:
        A: Expect the variable A which is independent of X or affect it otherwise.
        B: Expect the variable B which is independent of X or affect it otherwise.
        C: Expect the variable C which is independent of X or affect it otherwise.
        D: Expect the variable D which is independent of X or affect it otherwise.
        X: Expect the variable X
        L [1-9]: Expect A+B*X^N with N is 1..9 (the next char) and A, B independent of X
        J [1-9]: Expect C+D*X^N with N is 1..9 (the next char) and C, D independent of X
        P: Expect a polynomial of the variable X.
	T: Expect A+B*X+C*X^2
        ^: Expect a power which has 2 arguments base then exponent
        0 .. 9: [09]* Positive integer. If a ',' follow the integer, it is a rationnal
           (ie 1,2 is interpreted 1/2).
        -: [09]* Negative integer. If a ',' follow the integer, it is a rationnal.
        F: [A-9A-Za-z]: Internal Function calls (One argument)
            0,1,2,3,4,5: cos,sin,tan,acos,asin,atan
            6,7,8,9,A,B: cosh,sinh,tanh,acosh,asinh,atanh
            C,D,E,F, G: exp, log, abs, sign, floor
        *: [23] Product of 2 or 3 terms
	$: Nothing (Just a separator to mark the end of a number due to bad grammar).
           Needed in some cases where the parsing is ambigous.
 */
static const may_antidiff_table_t may_antidiff_table[] = {
  { "X",      "x^2/2", NOTHING },

  { "^L1-1",  "1/b*ln(abs(x*b+a))", NOTHING },
  { "^L1C",   "(a+b*x)^(c+1)/(b*(c+1))", NOTHING},
  { "^^L1C-1","(a+b*x)^(-c+1)/(b*(-c+1))", NOTHING},
  { "*2X^L1-2", "ln(abs(a+b*x))/b^2+a/b^2/(a+b*x)", NOTHING},
  { "*2X^L1C", "(a+x*b)^(1+c)*(x*(c+1)*b-a)/(2+c)/(1+c)/b^2", NOTHING},

  // Functions
  { "F0L1",   "sin(a+b*x)/b", NOTHING},
  { "F1L1",   "-cos(a+b*x)/b", NOTHING},
  { "F2L1",   "-ln(cos(a+b*x))/b", NOTHING },
  { "F3L1",   "((a+b*x)*acos(a+b*x)-sqrt(1-(a+b*x)^2))/b", NOTHING},
  { "F4L1",   "((a+b*x)*asin(a+b*x)+sqrt(1-(a+b*x)^2))/b", NOTHING},
  { "F5L1",   "((a+b*x)*atan(a+b*x)-ln((a+b*x)^2+1)/2)/b", NOTHING},
  { "F6L1",   "sinh(a+b*x)/b", NOTHING},
  { "F7L1",   "cosh(a+b*x)/b", NOTHING},
  { "F8L1",   "log(cosh(a+b*x))/b", NOTHING},
  { "F9L1",   "-2*(1/(2*b))*sqrt(a^2+2*a*b*x+b^2*x^2-1)-4*(-(-b*a)*(-1/2/b))*(-1/2/abs(b))*ln(abs((sqrt(a^2+2*a*b*x+b^2*x^2-1)-abs(b)*x)*abs(b)-a*b))+x*ln(a+b*x+sqrt(a^2+2*a*b*x+b^2*x^2-1))", NOTHING},
  { "FAL1",   "-2*(1/(2*b))*sqrt(a^2+2*a*b*x+b^2*x^2+1)-4*(-(-b*a)*(-1/2/b))*(-1/2/abs(b))*ln(abs((sqrt(a^2+2*a*b*x+b^2*x^2+1)-abs(b)*x)*abs(b)-a*b))+x*ln(a+b*x+sqrt(a^2+2*a*b*x+b^2*x^2+1))", NOTHING},
  { "FBL1",   "1/2*(-(a-1)/b*ln(abs(x*b+a-1))-(-a-1)/b*ln(abs(x*b+a+1))+x*ln((1+a+b*x)/(-a-b*x+1)))", NOTHING},
  { "FCL1",   "exp(a+b*x)/b", NOTHING},
  { "FDL1",   "(-a-b*x+(a+b*x)*ln(a+b*x))/b", NOTHING},
  { "FEL1",   "a^2/(2*b)*sign(a+b*x)+(a*x+b*x^2/2)*sign(a+b*x)", NOTHING},
  { "FDFEL1", "x*ln(abs(b*x+a))-b*(x/b-a*log(a+b*x)/b^2)", NOTHING},
  { "FFL1",   "sign(a+b*x)*x", NOTHING},
  { "FGL1",   "floor(a+b*x)*x", NOTHING},
  { "^AX",    "1/(ln(a))*a^x", A_POSITIF},
  { "^L21,2", "x*sqrt(a+b*x^2)/2+a*ln(abs(sqrt(a+b*x^2)+SQRT(b)*x))/2/b^(1/2)", B_POSITIF},
  { "^L21,2", "x*sqrt(a+b*x^2)/2+a*atan((-b)^(1/2)*x/(a+b*x^2)^(1/2))/2/(-b)^(1/2)", B_NEGATIF},

  { "^F0L1-1", "ln(abs(1/cos(a+b*x)+tan(a+b*x)))/b", NOTHING},
  { "^F1L1-1", "ln(abs(tan((a+b*x)/2)))/b", NOTHING},
  { "^F2L1-1", "ln(sin(a+b*x))/b", NOTHING },
  { "*2F0L1^F1L1-1", "ln(sin(a+b*x))/b", NOTHING },

  // x^n * exp (a+b*x^2) (==> ERF function)
  { "FCL2",     "exp(a)/PI^(1/2)*erf((-b)^(1/2)*x)/2/(-b)^(1/2)", NOTHING},
  { "*2XFCL2",  "1/2/b*exp(a+b*x^2)", NOTHING},
  { "*2^X2FCL2","exp(a)/2*(x*exp(b*x^2)/b-PI^(1/2)*erf((-b)^(1/2)*x)/2/b/(-b)^(1/2))", NOTHING},
  { "*2^X3FCL2","1/2/b^2*exp(a+b*x^2)*(-1+b*x^2)", NOTHING},

  // Some rational functions
  // FIXME: Use a transformation to reduce the number of rules?
  { "*2X^L1-1", "x/b-a*ln(a+b*x)/b^2", NOTHING},
  { "*2X^L1-2", "ln(a+b*x)/b^2+a/(b^2*(a+b*x))", NOTHING},
  { "*2X^L1-3", "a/2/b^2/(a+b*x)^2-1/b^2/(a+b*x)", NOTHING},
  { "*2^X2^L1-1", "x^2/(2*b)-x*a/b^2+a^2*ln(a+b*x)/b^3", NOTHING},
  { "*2^X2^L1-2", "x/b^2-2*a*ln(a+b*x)/b^3-a^2/(b^3*(a+b*x))", NOTHING},
  { "*2^X2^L1-3", "ln(a+b*x)/b^3+2*a/b^3/(a+b*x)-a^2/2/b^3/(a+b*x)^2", NOTHING},
  { "*2^X3^L1-1", "x^3/3/b-x^2*a/2/b^2+a^2*x/b^3-a^3*ln(a+b*x)/b^4", NOTHING},
  { "*2^X3^L1-2", "x^2/2/b^2-2*x*a/b^3+3*a^2*ln(a+b*x)/b^4+a^3/b^4/(a+b*x)", NOTHING},
  { "*2^X3^L1-3", "x/b^3-3*a*ln(a+b*x)/b^4-3*a^2/b^4/(a+b*x)+a^3/2/b^4/(a+b*x)^2", NOTHING},
  { "*2^X-1^L1-1", "ln(x)/a-ln(a+b*x)/a", NOTHING},
  { "*2^X-1^L1-2", "ln(x)/a^2-ln(a+b*x)/a^2+1/a/(a+b*x)", NOTHING},
  { "*2^X-1^L1-3", "1/2/a/(a+b*x)^2+1/a^2/(a+b*x)+ln(x)/a^3-ln(a+b*x)/a^3", NOTHING},
  { "*2^X-2^L1-1", "b*ln(a+b*x)/a^2-1/x/a-b*ln(x)/a^2", NOTHING},
  { "*2^X-2^L1-2", "2*b*ln(a+b*x)/a^3-1/x/a^2-2*b*ln(x)/a^3-b/(a+b*x)/a^2", NOTHING},
  { "*2^X-2^L1-3", "3*b*ln(a+b*x)/a^4-1/x/a^3-3*b*ln(x)/a^4-2*b/(a+b*x)/a^3-b/2/(a+b*x)^2/a^2", NOTHING},
  { "*2^X-3^L1-1", "b^2*ln(x)/a^3+b/x/a^2-1/2/x^2/a-b^2*ln(a+b*x)/a^3", NOTHING},
  { "*2^X-3^L1-2", "-3*b^2*ln(a+b*x)/a^4-1/2/x^2/a^2+2*b/x/a^3+3*b^2*ln(x)/a^4+b^2/(a+b*x)/a^3", NOTHING},
  { "*2^X-3^L1-3", "-6*b^2*ln(a+b*x)/a^5-1/2/x^2/a^3+3*b/x/a^4+6*b^2*ln(x)/a^5+3*b^2/(a+b*x)/a^4+b^2/2/(a+b*x)^2/a^3", NOTHING},

  { "^L2-1",       "atan(b*x/SQRT(a*b))/SQRT(a*b)", ApB_POSITIF},
  { "^L2-1",       "-atanh(b*x/SQRT(-b*a))/SQRT(-b*a)", ApB_NEGATIF},
  { "*2X^L2-1",    "ln(abs(a+b*x^2))/2/b", NOTHING},
  { "*2^X2^L2-1",  "x/b-a*atan(b*x/SQRT(a*b))/b/SQRT(a*b)", ApB_POSITIF},
  { "*2^X2^L2-1",  "x/b-a*atanh(-b*x/SQRT(-a*b))/b/SQRT(-a*b)", ApB_NEGATIF},
  { "*2^X3^L2-1",  "x^2/2/b-1/2*a*ln(abs(a+b*x^2))/b^2", NOTHING},
  { "*2^X-1^L2-1", "ln(x)/a-ln(a+b*x^2)/2/a", NOTHING},
  { "*2^X-2^L2-1", "-1/x/a-b*atan(b*x/SQRT(a*b))/a/SQRT(a*b)", ApB_POSITIF},
  { "*2^X-2^L2-1", "-1/x/a-b*atanh(-b*x/SQRT(-a*b))/a/SQRT(-a*b)", ApB_NEGATIF},
  { "*2^X-3^L2-1", "-b*ln(x)/a^2+b*ln(a+b*x^2)/2/a^2-1/2/x^2/a", NOTHING},

  { "^L2-2",       "x/2/a/(a+b*x^2)+atan(b*x/SQRT(a*b))/2/a/SQRT(a*b)", ApB_POSITIF},
  { "^L2-2",       "x/2/a/(a+b*x^2)+atanh(-b*x/SQRT(-a*b))/2/a/SQRT(-a*b)", ApB_NEGATIF},
  { "*2X^L2-2",    "-1/2/b/(a+b*x^2)", NOTHING},
  { "*2^X2^L2-2",  "atan(b*x/SQRT(a*b))/2/b/SQRT(a*b)-x/2/b/(a+b*x^2)", NOTHING},
  { "*2^X3^L2-2",  "ln(a+b*x^2)/2/b^2+a/2/b^2/(a+b*x^2)", NOTHING},
  { "*2^X-1^L2-2", "ln(x)/a^2-ln(a+b*x^2)/2/a^2+1/2/a/(a+b*x^2)", NOTHING},
  { "*2^X-2^L2-2", "-3/2*b*atan(b*x/SQRT(a*b)))/a^2/SQRT(a*b)-b*x/2/a^2/(a+b*x^2)-1/x/a^2", NOTHING},
  { "*2^X-3^L2-2", "b*ln(a+b*x^2)/a^3-2*b*ln(x)/a^3-b/2/a^2/(a+b*x^2)-1/2/x^2/a^2", NOTHING},

  { "^L3-1",       "ln(x+(a/b)^(1/3))/3/b/(a/b)^(2/3)-ln(x^2-x*(a/b)^(1/3)+(a/b)^(2/3))/6/b/(a/b)^(2/3)+atan((2*x/(a/b)^(1/3)-1)/3^(1/2))/3^(1/2)/b/(a/b)^(2/3)", NOTHING},
  { "*2X^L3-1",    "-ln(x+(a/b)^(1/3))/3/b/(a/b)^(1/3)+ln(x^2-x*(a/b)^(1/3)+(a/b)^(2/3))/6/b/(a/b)^(1/3)+atan((2*x/(a/b)^(1/3)-1)/3^(1/2))/3^(1/2)/b/(a/b)^(1/3)", NOTHING},
  { "*2^X2^L3-1",  "ln(a+b*x^3)/3/b", NOTHING},

  { "^L4-1",       "(a*b^3)^(1/4)*(-2*sqrt(2))*a*b/(((-2*sqrt(2))*a*b)^2+(b*a*sqrt(2)*2)^2)*ln(x^2+(-(sqrt(2)))*(a*b^3)^(1/4)/b*x+(-(-a)/b)^(1/4)*(-(-a)/b)^(1/4))+4*sqrt(2)*(a*b^3)^(1/4)/(16*a*b)*atan((x-sqrt(2)/2*(-(-a)/b)^(1/4))/(sqrt(2)/2*(-(-a)/b)^(1/4)))+(a*b^3)^(1/4)*2*sqrt(2)*a*b/((2*sqrt(2)*a*b)^2+(b*a*sqrt(2)*2)^2)*ln(x^2+sqrt(2)*(a*b^3)^(1/4)/b*x+(-(-a)/b)^(1/4)*(-(-a)/b)^(1/4))+4*sqrt(2)*(a*b^3)^(1/4)/(16*a*b)*atan((x-(-sqrt(2)/2)*(-(-a)/b)^(1/4))/(sqrt(2)/2*(-(-a)/b)^(1/4)))", NOTHING},
  { "*2X^L4-1",    "1/2/SQRT(a*b)*atan(2*b*x^2/2/SQRT(a*b))", NOTHING},
  { "*2^X2^L4-1",  "((a*b^3)^(1/4))^3*2*sqrt(2)*a*b^3/((2*sqrt(2)*a*b^3)^2+(b^3*a*sqrt(2)*2)^2)*ln(x^2+(-(sqrt(2)))*(a*b^3)^(1/4)/b*x+(-(-a)/b)^(1/4)*(-(-a)/b)^(1/4))+4*sqrt(2)*((a*b^3)^(1/4))^3/(16*a*b^3)*atan((x-sqrt(2)/2*(-(-a)/b)^(1/4))/(sqrt(2)/2*(-(-a)/b)^(1/4)))+(-(((a*b^3)^(1/4))^3))*2*sqrt(2)*a*b^3/((2*sqrt(2)*a*b^3)^2+(b^3*a*(-2*sqrt(2)))^2)*ln(x^2+sqrt(2)*(a*b^3)^(1/4)/b*x+(-(-a)/b)^(1/4)*(-(-a)/b)^(1/4))+4*sqrt(2)*((a*b^3)^(1/4))^3/(16*a*b^3)*atan((x-(-sqrt(2)/2)*(-(-a)/b)^(1/4))/(sqrt(2)/2*(-(-a)/b)^(1/4)))", NOTHING},
  { "*2^X3^L4-1",  "1/4/b*ln(abs(4*x^4/4*b+a))", NOTHING},

  { "^T-1",   "2*atan((2*c*x+b)/sqrt(4*a*c-b^2))/sqrt(4*a*c-b^2)", B2_m_4AC_NEGATIF},
  { "^T-1",   "ln((2*c*x-sqrt(b^2-4*a*c)+b)/(2*c*x+sqrt(b^2-4*a*c)+b))/sqrt(b^2-4*a*c)+b)", B2_m_4AC_POSITIF},
  { "*2X^T-1", "ln(abs(c*x^2+b*x+a))/2/c-b*atan((2*c*x+b)/sqrt(4*a*c-b^2))/c/sqrt(4*a*c-b^2)", B2_m_4AC_NEGATIF},
  { "*2X^T-1", "ln(abs(c*x^2+b*x+a))/2/c-b*ln((2*c*x-sqrt(b^2-4*a*c)+b)/(2*c*x+sqrt(b^2-4*a*c)+b))/sqrt(b^2-4*a*c)+b)/2/c", B2_m_4AC_POSITIF},

  // TODO: Precompute diff (f(x),x,-1): It is useless to recall the function (twice!) if we already know the answer
  { "*2F0L1P", "p*diff(cos(a+b*x),x,-1)-diff(diff(p,x)*diff(cos(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2F1L1P", "p*diff(sin(a+b*x),x,-1)-diff(diff(p,x)*diff(sin(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2F2L1P", "p*diff(tan(a+b*x),x,-1)-diff(diff(p,x)*diff(tan(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2F3L1P", "p*diff(acos(a+b*x),x,-1)-diff(diff(p,x)*diff(acos(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2F4L1P", "p*diff(asin(a+b*x),x,-1)-diff(diff(p,x)*diff(asin(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2F5L1P", "p*diff(atan(a+b*x),x,-1)-diff(diff(p,x)*diff(atan(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2F6L1P", "p*diff(cosh(a+b*x),x,-1)-diff(diff(p,x)*diff(cosh(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2F7L1P", "p*diff(sinh(a+b*x),x,-1)-diff(diff(p,x)*diff(sinh(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2F8L1P", "p*diff(tanh(a+b*x),x,-1)-diff(diff(p,x)*diff(tanh(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2F9L1P", "p*diff(acosh(a+b*x),x,-1)-diff(diff(p,x)*diff(acosh(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2FAL1P", "p*diff(asinh(a+b*x),x,-1)-diff(diff(p,x)*diff(asinh(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2FBL1P", "p*diff(atanh(a+b*x),x,-1)-diff(diff(p,x)*diff(atanh(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2FCL1P", "p*diff(exp(a+b*x),x,-1)-diff(diff(p,x)*diff(exp(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2FDL1P", "diff(p,x,-1)*log(a+b*x)-diff(diff(p,x,-1)*b/(a+b*x),x,-1)", NOTHING }, // Integration by parts (Reverse term)
  { "*2FEL1P", "p*diff(abs(a+b*x),x,-1)-diff(diff(p,x)*diff(abs(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2FFL1P", "p*diff(sign(a+b*x),x,-1)-diff(diff(p,x)*diff(sign(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2FGL1P", "p*diff(floor(a+b*x),x,-1)-diff(diff(p,x)*diff(floor(a+b*x),x,-1),x,-1)", NOTHING }, // Integration by parts
  { "*2^L1CP", "p*diff((a+b*x)^c,x,-1)-diff(diff(p,x)*diff((a+b*x)^c,x,-1),x,-1)", NOTHING }, // Integration by parts

  // sqrt (TODO: Add special rules if b<0)
  { "*2X^L11,2", "2*(a+b*x)^(3/2)*(-2*a+3*b*x)/(15*b^2)", NOTHING},
  { "*2^X2^L11,2", "2*(a+b*x)^(3/2)*(8*a^2-12*a*x*b+15*x^2+b^2)(105*b^3)", NOTHING},
  { "*2^X-1^L11,2", "2*(a+b*x)^(1/2)-2*a^(1/2)*atanh((a+b*x)^(1/2)/a^(1/2))", NOTHING},
  { "*2^X-2^L11,2", "-((a+b*x)^(1/2)*a^(1/2)+atanh((a+b*x)^(1/2)/a^(1/2))*x*b)/(x*a^(1/2))", NOTHING},
  { "*2X^L1-1,2", "2*(a+b*x)^(1/2)*(b*x-2*a)/(3*b^2)", NOTHING },
  { "*2^X2^L1-1,2", "2*(a+b*x)^(1/2)*(8*a^2-4*a*x*b+3*x^2*b^2)/(15*b^3)", NOTHING},
  { "*2^X-1^L1-1,2", "-2*atanh((a+b*x)^(1/2)/a^(1/2))/a^(1/2)", NOTHING},
  { "*2^X-2^L1-1,2", "(-(a+b*x)^(1/2)*a^(1/2)+atanh((a+b*x)^(1/2)/a^(1/2))*x*b)/(a^(3/2)*x)", NOTHING},
  { "^L21,2", "(x*(a+b*x^2)^(1/2)*b^(1/2)+a*ln(b^(1/2)*x+(a+b*x^2)^(1/2)))/(2*b^(1/2))", NOTHING },
  { "^L2-1,2", "asinh(SQRT(b)*x/SQRT(a))/SQRT(b)", B_POSITIF },
  { "^L2-1,2", "-asin(b*x/SQRT(a)/SQRT(-b))/SQRT(-b)", B_NEGATIF },
  { "*2^X-1^L2-1,2", "-ln((2*a+2*a^(1/2)*(a+b*x^2)^(1/2))/x)/a^(1/2)", NOTHING},
  { "*2^X-1^L21,2",  "(a+b*x^2)^(1/2)-a^(1/2)*ln((2*a+2*a^(1/2)*(a+b*x^2)^(1/2))/x)", NOTHING},
  { "*2X^L2-1,2",    "(a+b*x^2)^(1/2)/b", NOTHING },
  { "*2X^L21,2",     "(a+b*x^2)^(3/2)/(3*b)", NOTHING },
  { "*2^X2^L21,2",   "x*(a+b*x^2)^(3/2)/4/b-a*x*(a+b*x^2)^(1/2)/8/b-a^2*ln(b^(1/2)*x+a(a+b*x^2)^(1/2))/8/b^(3/2)", NOTHING},
  { "*2^X2^L2-1,2",  "x*(a+b*x^2)^(1/2)/2/b-a*ln(abs(b^(1/2)*x+(a+b*x^2)^(1/2)))/2/b^(3/2)", B_POSITIF},
  { "*2^X2^L2-1,2",  "x*(a+b*x^2)^(1/2)/2/b+a*atan((-b)^(1/2)*x/(a+b*x^2)^(1/2))/2/(-b)^(3/2)", B_NEGATIF},
  { "*2^X-2^L21,2",  "-(a+b*x^2)^(3/2)/a/x+b*x*(a+b*x^2)^(1/2)/a+b^(1/2)*ln(b^(1/2)*x+(a+b*x^2)^(1/2))", NOTHING},
  { "*2^X-2^L2-1,2", "-(a+b*x^2)^(1/2)/x/a", NOTHING},
  { "*2^X3^L21,2",   "(a+b*x^2)^(3/2)*(-2*a+3*b*x^2)/15/b^2", NOTHING},
  { "*2^X3^L2-1,2",  "(a+b*x^2)^(1/2)*(-2*a+b*x^2)/3/b^2", NOTHING},
  { "*2^X-3^L21,2",  "b*(a+b*x^2)^(1/2)/2/a-(a+b*x^2)^(3/2)/2/a/x^2-b*ln((2*a+2*a^(1/2)*(a+b*x^2)^(1/2))/x)/2/a^(1/2)", NOTHING},
  { "*2^X-3^L2-1,2", "b*ln((2*a+2*a^(1/2)*(a+b*x^2)^(1/2))/x)/2/a^(3/2)-(a+b*x^2)^(1/2)/2/a/x^2", NOTHING},
  { "*2^X-4^L21,2",  "-(a+b*x^2)^(3/2)/3/x^3/a", NOTHING},

  { "^*2X^L1-1$1,2", "sqrt(x/(a+b*x))*(sqrt(b)*sqrt(x)*(a+b*x)-a*sqrt(a+b*x)*log(2*(sqrt(b)*sqrt(a+b*x)+b*sqrt(x))))/b^(3/2)/sqrt(x)", B_POSITIF},
  //"2*a*(ln((2*b*sqrt(x/(a+b*x))-2*sqrt(b))/(2*b*sqrt(x/(a+b*x))+2*sqrt(b)))/4/b^(3/2)-sqrt(x/(a+b*x))/(2*b^2*x/(a+b*x)-2*b))", B_POSITIF},
  { "^*2X^L1-1$1,2", "-2*a*(atan(b*sqrt(x/(a+b*x))/sqrt(-b))/2/(-b)^(3/2)-sqrt(x/(a+b*x))/(2*b^2*x/(a+b*x)-2*b))", B_NEGATIF},

  // trig
  { "^F1L1-2", "-1/b/tan(a+b*x)", NOTHING},
  { "^F0L1-2", "tan(a+b*x)/b", NOTHING},
  { "*2F1L1^F0L1-2", "1/b/cos(a+b*x)", NOTHING},
  { "*2F2L1^F0L1-1", "1/b/cos(a+b*x)", NOTHING},
  { "*2^F12^F0L1-1", "-sin(a+b*x)/b+ln(1/cos(a+b*x)+tan(a+b*x))/b", NOTHING},
  { "*2F1L1F2L1",    "-sin(a+b*x)/b+ln(1/cos(a+b*x)+tan(a+b*x))/b", NOTHING},
  { "*2F0L1^F1L1-2", "-1/b/sin(a+b*x)", NOTHING},
  { "*2^F2L1-1^F1L1-1", "-1/b/sin(a+b*x)", NOTHING},
  { "*2^F0L1-1^F1L1-1", "ln(abs(tan(a+b*x)))/b", NOTHING},
  { "*2^F0L1-2^F1L1-1", "1/b/cos(a+b*x)+ln(1/sin(a+b*x)-1/tan(a+b*x))/b", NOTHING},
  { "*2^F0L1-1^F1L1-2", "-1/b/sin(a+b*x)+ln(1/cos(a+b*x)+tan(a+b*x))/b", NOTHING},
  { "*2^F0L1-2^F1L1-2", "1/b/cos(a+b*x)/sin(a+b*x)-2/tan(a+b*x)", NOTHING},
  { "^F2L12",  "tan(a+b*x)/b-atan(tan(a+b*x))/b", NOTHING},

  // Product of trig per exp
  { "*2FCL1F1J1", "exp(a+b*x)*(b*sin(c+d*x)-d*cos(c+d*x))/(b^2+d^2)", NOTHING },
  { "*2FCL1F0J1", "exp(a+b*x)*(b*cos(c+d*x)+d*sin(c+d*x))/(b^2+d^2)", NOTHING },
  { "*3FCL1F1J1X", "exp(a+b*x)/(b^2+d^2)^2*((b^3*x-b^2+b*d^2*x+d^2)*sin(c+d*x)-d*(b^2*x-2*b+d^2*x)*cos(c+d*x))", NOTHING},
  { "*3FCL1F0J1X", "exp(a+b*x)/(b^2+d^2)^2*(d*(b^2*x-2*b+d^2*x)*sin(c+d*x)+(b^3*x-b^2+b*d^2*x+d^2)*cos(c+d*x))", NOTHING},

  { "*2FCL1F7J1", "exp(a+b*x)*(b*sinh(c+d*x)-d*cosh(c+d*x))/(b^2-d^2)", B_NEQ_D},
  { "*2FCL1F6J1", "exp(a+b*x)*(b*cosh(c+d*x)-d*sinh(c+d*x))/(b^2-d^2)", B_NEQ_D},
  { "*3FCL1F7J1X", "exp(a+b*x)/(b^2-d^2)*((-b^2*d*x+2*b*d+d^3*x)*cosh(c+d*x)+(b^3*x-b^2-b*d^2*x-d^2)*sinh(c+d*x))", B_NEQ_D},
  { "*3FCL1F6J1X", "exp(a+b*x)/(b^2-d^2)*(d*(-b^2*x+2*b+d^2*x)*sinh(c+d*x)+(b^3*x-b^2-b*d^2*x-d^2)*cosh(c+d*x))", B_NEQ_D},

  { "*2FCL1F7L1", "(exp(2*a+2*b*x)-2*a-2*b*x)/4/b", NOTHING},
  { "*2FCL1F6L1", "(exp(2*a+2*b*x)+2*a+2*b*x)/4/b", NOTHING},
  { "*3FCL1F7L1X", "((2*b*x-1)*exp(2*a+2*b*x)-2*b^2*x^2)/8/b^2", NOTHING},
  { "*3FCL1F6L1X", "((2*b*x-1)*exp(2*a+2*b*x)+2*b^2*x^2)/8/b^2", NOTHING},

  { "*2FCL1F8L1", "(exp(a+b*x)-2*atan(exp(a+b*x)))/b", NOTHING},

  { "*2F0L1F6J1", "1/(b^2+d^2)*(cosh(c)*(d*sinh(d*x)*cos(a+b*x)+b*cosh(d*x)*sin(a+b*x))+sinh(c)*(b*sinh(d*x)*sin(a+b*x)+d*cosh(d*x)*cos(a+b*x)))", NOTHING},
  { "*2F1L1F7J1", "1/(b^2+d^2)*(cosh(c)*(d*cosh(d*x)*sin(a+b*x)-b*sinh(d*x)*cos(a+b*x))+sinh(c)*(d*sinh(d*x)*sin(a+b*x)-b*cosh(d*x)*cos(a+b*x)))", NOTHING},

  { "*2F0L1F7J1", "1/(b^2+d^2)*(sinh(c)*(d*sinh(d*x)*cos(a+b*x)+b*cosh(d*x)*sin(a+b*x))+cosh(c)*(b*sinh(d*x)*sin(a+b*x)+d*cosh(d*x)*cos(a+b*x)))", NOTHING},
  { "*2F1L1F6J1", "1/(b^2+d^2)*(sinh(c)*(d*cosh(d*x)*sin(a+b*x)-b*sinh(d*x)*cos(a+b*x))+cosh(c)*(d*sinh(d*x)*sin(a+b*x)-b*cosh(d*x)*cos(a+b*x)))", NOTHING},

  // Hyperbolic trig
  { "^F8L12", "-tanh(a+b*x)/b-ln(tanh(a+b*x)-1)/2/b+ln(tanh(a+b*x)+1)/2/b", NOTHING},

  // Log
  { "^FDL12", "ln(a+b*x)^2*a/b+ln(a+b*x)^2*x-2*ln(a+b*x)*a/b-2*ln(a+b*x)*x+2*a/b+2*x", NOTHING},
  { "*2FDL1^X-2", "b*ln(b*x)/a-ln(a+b*x)/x-b*ln(a+b*x)/a", NOTHING}, // FIXME: B not null!
  { "*2FD*2BX^X-1", "log(b*x)^2/2", NOTHING},
  { "FDL2", "x*log(a+b*x^2)+2*SQRT(a)*atanh(SQRT(-b)*x/SQRT(a))/SQRT(-b)-2*x", B_LIKELY_NEGATIF},
  { "FDL2", "x*ln(a+b*x^2)+2*SQRT(a)*atan(SQRT(b)*x/SQRT(a))/SQRT(b)-2*x", NOTHING },
  { "*2FDL2X", "1/2*x^2*ln(a+b*x^2)+a*ln(a+b*x^2)/2/b-x^2/2", NOTHING},

  // tan(b*x+a)^3 / ^5 cos(b*x+a)^-3 sin(b*x+a)^-3
  {"^F2L13", "tan(b*x+a)^2*1/2/b-1/2/b*ln(tan(b*x+a)^2+1)", NOTHING },
  {"^F2L15", "(tan(b*x+a)^4*b-2*tan(b*x+a)^2*b)*1/4/b^2+1/2/b*ln(tan(b*x+a)^2+1)", NOTHING },
  {"^F0L1-3", "-b*1/4/b^2*ln(abs(-cos(b*x+a)/b*b-1))+b*1/4/b^2*ln(abs(-cos(b*x+a)/b*b+1))-cos(b*x+a)/b/2/(-(-cos(b*x+a)/b)^2*b^2+1", NOTHING },
  {"^F1L1-3", "-b*1/4/b^2*ln(abs(sin(b*x+a)/b*b-1))+b*1/4/b^2*ln(abs(sin(b*x+a)/b*b+1))+sin(b*x+a)/b/2/(-(sin(b*x+a)/b)^2*b^2+1)", NOTHING}

};

/* Define the function which are recognized by the filter (Using the name of the function as a match).
   This corresponds to the filters F1 to FG in the table above. */
static const char *const FunctionTable[] = {
  may_cos_name, may_sin_name, may_tan_name, may_acos_name, may_asin_name, may_atan_name,
  may_cosh_name, may_sinh_name, may_tanh_name, may_acosh_name, may_asinh_name, may_atanh_name,
  may_exp_name, may_log_name, may_abs_name, may_sign_name, may_floor_name
};

/* Name of the variables A, B, C, D and X in the expression colummn of may_antidiff_table */
static const char *const name[] = {
  "a", "b", "c", "d", "x", "p", "SQRT"
};

/* Lists all the permutations of {0, 1} and {0, 1, 2} */
static const unsigned char permu_two[] = { 0, 1, 1, 0 };
static const unsigned char permu_three[] = { 0, 1, 2, 0, 2, 1, 1, 0, 2, 1, 2, 0, 2, 0, 1, 2, 1, 0 };

/* Short cuts */
#define A (may_g.antidiff.A)
#define B (may_g.antidiff.B)
#define C (may_g.antidiff.C)
#define D (may_g.antidiff.D)
#define X (may_g.antidiff.X)
#define P (may_g.antidiff.P)

/* Special version of sqrt which combines its exponent (We must use SQRT in the ref table)
   It helps providing the natural solution for the antidiff of
   1/(a^2+x^2) --> atan(x/a)/a instead of atan(x/(a^2)^(1/2))/(a^2)^(1/2) */
static may_t special_sqrt (may_t a)
{
  /* The evaluation is mandatory as we may get a^2*1 as the parameter */
  a = may_eval (a);
  if (may_get_name (a) == may_pow_name)
    return may_pow_c (may_op (a, 0), may_mul_c (may_op (a, 1), may_set_si_ui (1, 2)));
  else
    return may_sqrt_c (a);
}

/* Compare e to the rational p/q */
static int cmp_si_ui (may_t e, long p, unsigned long q)
{
  int i;
  may_mark ();
  i = may_identical (e, may_set_si_ui (p, q));
  may_keep (NULL);
  return i;
}

/* Check if B^2-4*A*C is positive (val = 2) or negative (val = 4) */
static int check_B2_m_4AC (int val)
{
  if (A == 0 || B == 0 || C == 0)
    return 1;

  may_mark ();
  may_t tmp = may_eval (may_add_c (may_sqr_c (B),
				   may_mul_vac (may_set_si (-4), A, C, NULL)));
  int i = may_compute_sign (tmp);
  i = (i == 0) || ((i & val) == val);
  may_keep (NULL);
  return i;
}

/* Check if the condition for A & B matches the needed condition for the current pattern */
static int check_condition (void)
{
  switch (may_g.antidiff.condition) {
  case NOTHING:
    return 1;
  case A_POSITIF:
    return A == 0 || !may_purereal_p (A) || may_num_pos_p (A);
  case B_POSITIF:
    return B == 0 || !may_purereal_p (B) || may_num_pos_p (B);
  case B_NEGATIF:
    return B == 0 || !may_purereal_p (B) || may_num_neg_p (B);
  case ApB_POSITIF:
    return A == 0 || B == 0 || !may_purereal_p (A) || !may_purereal_p (B)
      || (may_num_pos_p (A) && may_num_pos_p (B)) || (may_num_neg_p (A) && may_num_neg_p (B));
  case ApB_NEGATIF:
    return A == 0 || B == 0 || !may_purereal_p (A) || !may_purereal_p (B)
      || (may_num_pos_p (A) && may_num_neg_p (B)) || (may_num_neg_p (A) && may_num_pos_p (B));
  case B2_m_4AC_POSITIF:
    return check_B2_m_4AC (2);
  case B2_m_4AC_NEGATIF:
    return check_B2_m_4AC (4);
  case B_NEQ_D:
    return B == 0 || D == 0 || may_identical (B, D) != 0;
  case B_LIKELY_NEGATIF:
    return B == 0
      || (may_purereal_p (B) && may_num_neg_p (B))
      || (MAY_TYPE (B) == MAY_FACTOR_T && may_num_neg_p (MAY_AT (B, 0))) ;
  default:
    return 0;
  }
}

/* Check if the expression matches A+B*X^N */
static int match_a_b_x_n (may_t e, int n)
{
  may_iterator_t it1, it2;
  may_t c, b, p, t, tmp_b, tmp_a;

  tmp_b = may_set_ui (0);

  /* Scan the expression to check if it looks like A+B*X^N */
  for (tmp_a = may_sum_iterator_init (it1, e) ;
       may_sum_iterator_end (&c, &t, it1)     ;
       may_sum_iterator_next (it1) ) {
    int found = 0;
    for (may_product_iterator_init (it2, t) ;
         may_product_iterator_end (&p, &b, it2)   ;
         may_product_iterator_next (it2) ) {
      if (may_identical (b, X) == 0) {
	found = 1;
        if (cmp_si_ui (p, n, 1) != 0)
          return 0; // Failure
      } else
        /* this term doesn't depend on X (yet!) */
        c = may_mulinc_c (c, may_product_iterator_ref (it2));
    }
    c = may_eval (c);
    if (!may_independent_p (c, X))
      return 0;
    if (found)
      tmp_b = may_addinc_c (tmp_b, c);
    else
      tmp_a = may_addinc_c (tmp_a, c);
  }
  tmp_a = may_eval (tmp_a);
  tmp_b = may_eval (tmp_b);

  /* Ok we now have successfully scanned e to be A+B*X^n with A=tmp_a and B=tmp_b */
  /* Now check that A and B match what they were (if they were already affected) */
  if (A && may_identical (A, tmp_a) != 0)
    return 0;
  if (B && may_identical (B, tmp_b) != 0)
    return 0;
  A = tmp_a;
  B = tmp_b;
  return check_condition ();
}

/* Check if the expression matches C+D*X^N */
static int match_c_d_x_n (may_t e, int n)
{
  /* Just use the other function by swapping variables */
  swap (A, C);
  swap (B, D);
  int i = match_a_b_x_n (e, n);
  swap (A, C);
  swap (B, D);
  return i;
}

/* Check if the expression matches A+B*X+C*X^2 */
static int match_trinome (may_t e)
{
  may_iterator_t it1, it2;
  may_t c, b, p, t, tmp_c, tmp_b, tmp_a;

  tmp_b = tmp_c = may_set_ui (0);

  /* Scan the expression to check if it looks like A+B*X+C*X^2 */
  for (tmp_a = may_sum_iterator_init (it1, e) ;
       may_sum_iterator_end (&c, &t, it1)     ;
       may_sum_iterator_next (it1) ) {
    int found = 0;
    for (may_product_iterator_init (it2, t) ;
         may_product_iterator_end (&p, &b, it2)   ;
         may_product_iterator_next (it2) ) {
      if (may_identical (b, X) == 0) {
	if (may_one_p (p))
	  found = 1;
	else if (cmp_si_ui (p, 2, 1) == 0)
	  found = 2;
	else
	  return 0; // Failure
      } else
        /* this term doesn't depend on X (yet!) */
        c = may_mulinc_c (c, may_product_iterator_ref (it2));
    }
    c = may_eval (c);
    if (!may_independent_p (c, X))
      return 0;
    if (found == 1)
      tmp_b = may_addinc_c (tmp_b, c);
    else if (found == 2)
      tmp_c = may_addinc_c (tmp_c, c);
    else
      tmp_a = may_addinc_c (tmp_a, c);
  }
  tmp_a = may_eval (tmp_a);
  tmp_b = may_eval (tmp_b);
  tmp_c = may_eval (tmp_c);

  /* Ok we now have successfully scanned e to be A+B*X+C*X^2 */
  /* Now check that A, B & C match what they were (if they were already affected) */
  if (A && may_identical (A, tmp_a) != 0)
    return 0;
  if (B && may_identical (B, tmp_b) != 0)
    return 0;
  if (C && may_identical (C, tmp_c) != 0)
    return 0;
  A = tmp_a;
  B = tmp_b;
  C = tmp_c;
  return check_condition ();
}

/* Check if the expression matches a polynomial P of X */
static int match_polynomial (may_t e)
{
  mpz_srcptr deg[1];
  /* If we can get the degree of e, it is a polynomial :) */
  if (!may_ldegree (NULL, deg, NULL, e, 1, &X))
    return 0;
  /* Check if the lower degree is >= 0 */
  if (mpz_cmp_ui (deg[0], 0) < 0)
    return 0;
  /* Check if P was already afected */
  if (P && may_identical (P, e) != 0)
    return 0;
  /* Affect P */
  P = e;
  /* There is no condition on P */
  return 1;
}

static int fast_match (const char ** ByteCode, may_t f);

/* Check if the expression matches a product of n (=2 or 3) varables using the recursive ByteCode */
static int match_product (const char ** ByteCode, may_t f, int n)
{
  MAY_ASSERT (n >= 2 && n <= 3);
  if (!may_product_p (f))
    return 0;
  /* Decompose the product into each elements */
  may_t coeff, p, b;
  may_iterator_t it;
  may_t tab[3];
  int tab_n = 0;
  for ( coeff = may_product_iterator_init (it, f) ;
        may_product_iterator_end (&p, &b, it)     ;
        may_product_iterator_next (it) ) {
    if (tab_n >= n)
      return 0;
    tab[tab_n++] = may_product_iterator_ref (it);
  }
  if (!may_one_p (coeff)) {
    if (tab_n >= n)
      return 0;
    tab[tab_n++] = coeff;
  }
  if (tab_n != n)
    return 0;
  /* Check if the permutations agree */
  int permu_count = n == 2 ? 2 : 6;
  const unsigned char *permu_tab = n == 2 ? permu_two : permu_three ;
  const char * org_ByteCode = *ByteCode;
  /* For each permutation */
  may_t save_A = A, save_B = B, save_C = C, save_P = P;
  for (int permu = 0; permu < permu_count; permu++) {
    /* Restore original bytecode */
    *ByteCode = org_ByteCode;
    /* Check if this permutation match the expression */
    int success = 1;
    for (int i = 0; success && i < n; i++)
      success = fast_match (ByteCode, tab[permu_tab[i]]);
    /* Yes ? */
    if (success)
      return 1; /* Success! */
    /* No, so next permutation */
    permu_tab += n;
    /* Restore original value of A, B, C & P since they may have been partialy resolved! */
    A = save_A; B = save_B; C = save_C; P = save_P;
  }
  /* Failed to match */
  return 0;
}

/* Check if 'f' matches the ByteCode. ByteCode is another form for representing an expression, which is smaller than a real string, and faster to parse.
    EXAMPLE: "^X-1" "^XA" */
static int fast_match (const char ** ByteCode, may_t f)
{
  long p, q = 1;
  int sign, n;

  /* Compare the byte code with the expression */
  switch (*(*ByteCode)++) {
    /* Dummy variable used to separate some numbers... */
  case '$':
    return fast_match(ByteCode, f);

    /* Does it match the variable? */
  case 'X':
    return may_identical (X, f) == 0;
    /* Does it match the product wildcard? */
  case 'A':
    if (A)
      return may_identical (A, f) == 0;
    if (!may_independent_p(f, X))
      return 0;
    A = f;
    return check_condition ();
    /* Does it match the sum wildcard? */
  case 'B':
    if (B)
      return may_identical (B, f) == 0;
    if (!may_independent_p(f, X))
      return 0;
    B = f;
    return check_condition ();
    /* Does it match the wildcard? */
  case 'C':
    if (C)
      return may_identical (C, f) == 0;
    if (!may_independent_p(f, X))
      return 0;
    C = f;
    return check_condition ();
    /* Does it match the wildcard? */
  case 'D':
    if (D)
      return may_identical (D, f) == 0;
    if (!may_independent_p(f, X))
      return 0;
    D = f;
    return check_condition ();

    /* Does it match an integer? */
  case '-':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    if (may_get_name (f) != may_integer_name && may_get_name (f) != may_rational_name)
      return 0;
    /* Integer: read it */
    p = strtol ((char*)(*ByteCode)-1, (char **) ByteCode, 10);
    /* Check for rational */
    if (**ByteCode == ',')
      q = strtol ((char*)(*ByteCode)+1, (char **) ByteCode, 10);
    /* Compare */
    return cmp_si_ui (f, p, q) == 0;

    /* Does it match a power  */
  case '^':
    return may_get_name (f) == may_pow_name && fast_match (ByteCode, may_op (f, 0)) && fast_match (ByteCode, may_op (f, 1));

    /* Does it match a function? */
  case 'F':
    sign = *(*ByteCode)++;
    n = (sign >= '0' && sign <= '9') ? sign-'0' : (sign >= 'A' && sign <= 'Z') ? sign-'A'+10 : sign-'a'+26+10;
    MAY_ASSERT (n >= 0 && n < (int)numberof( FunctionTable));
    if (may_get_name (f) != FunctionTable[n])
      return 0;
    return fast_match (ByteCode, may_op (f, 0));

    /* Does it match A+B*X^n ? */
  case 'L':
    n = (*(*ByteCode)++) - '0' + 0;
    MAY_ASSERT (n >= 1 && n <= 9);
    return match_a_b_x_n (f, n);
    /* Does it match C+D*X^n ? */
  case 'J':
    n = (*(*ByteCode)++) - '0' + 0;
    MAY_ASSERT (n >= 1 && n <= 9);
    return match_c_d_x_n (f, n);

    /* Does it match a trinome? */
  case 'T':
    return match_trinome(f);

    /* Does it match a polynomial? */
  case 'P':
    return match_polynomial (f);

    /* Does it match exactly a product? */
  case '*':
    n = (*(*ByteCode)++) - '0' + 0;
    return match_product (ByteCode, f, n);

    /* Does it match exactly a sum? */
  case '+':
  default:
    return 0;
  }
}

static may_t
antidiff (may_t f)
{
   /* If f is independent of X, return F*x */
   if ( may_independent_p (f, X))
     return may_mul (f, X);

   /* If f is a sum, return sum(integral(F[i])).
      We don't compute an integral but an antidiff, and we assume it exists */
   if (may_sum_p (f)) {
     may_t r, c, b;
     may_iterator_t it;
     /* TO PARALIZE */
     for (r = may_mul (may_sum_iterator_init (it, f), X) ;
          may_sum_iterator_end (&c, &b, it) ;
          may_sum_iterator_next (it) ) {
       b = antidiff (b);
       if (b == NULL)
         return NULL;
       r = may_addinc_c (r, may_mul_c (c, b));
     }
     return may_eval (r);
   }

   /* If f is a product, extract the term which is dependent of x, and the one which is not */
   may_t constant = may_set_ui (1);
   may_t base     = f;
   if (may_product_p (f)) {
     may_t e, b;
     may_iterator_t it;
     base = constant;
     for (constant = may_product_iterator_init (it, f) ;
          may_product_iterator_end (&e, &b, it)  ;
          may_product_iterator_next (it) ) {
       if (may_independent_p (b, X))
         constant = may_mulinc_c (constant, may_product_iterator_ref (it));
       else
         /* FIXME: Performs a naive factor ? */
         base     = may_mulinc_c (base, may_product_iterator_ref (it));
     }
     constant = may_eval (constant);
     base     = may_eval (base);
   }

   /* Lock for base in the precomputed antidiff table.
      If it matches, return the antidiff by replacing a,b,c,x by their values. */
   for (unsigned int i = 0; i < numberof (may_antidiff_table) ; i++) {
     const char *ByteCode = may_antidiff_table[i].ByteCode;
     /* Reinit the wildcard variables */
     A = B = C = D = P = 0;
     may_g.antidiff.condition = may_antidiff_table[i].condition;
     if (fast_match(&ByteCode, base)) {
       /* Note that the following evaluation may destroy the values of the global variables */
       const void *value[numberof(name)] = { A, B , C , D, X , P, (const void *)special_sqrt};
       may_t r;
       /* So don't eval the expression until we have replace the values of the global variables */
       may_parse_c (&r, may_antidiff_table[i].expression);
       MAY_ASSERT (r != NULL);
       r = may_subs_c (r, 1, numberof (name), name, value);
       /* Now the global variables may be reused */
       return may_mul (constant, may_eval (r));
     }
   }

   /* Not found */
   return NULL;
}

may_t
may_antidiff (may_t f, may_t var)
{
  MAY_LOG_FUNC (("f='%Y' var='%Y'", f, var));
  MAY_ASSERT (MAY_TYPE (var) == MAY_STRING_T);

  may_mark ();
  X = var;
  may_t f2, y;

  /* Protect against some memory errors (tcollect may generate huge expressions) */
  MAY_TRY {
    /* Try direct integration */
    y = antidiff (f);
    /* If it fails, try integration of the expanded / trig collect entry
       if the original expression contains some sin[h]/cos[h] */
    if (y == NULL && may_exp_p (f, MAY_SIN_EXP_P|MAY_COS_EXP_P|MAY_SINH_EXP_P|MAY_COSH_EXP_P)) {
      f2 = may_expand (may_tcollect (f));
      y = may_identical (f, f2) == 0 ? NULL : antidiff (f2);
    }
    /* If it fails, try integration of the partial fraction decomposition */
    if (y == NULL) {
      f2 = may_partfrac (f, var, may_ratfactor);
      if (f2 != NULL) {
	f2 = may_expand (f2);
	y = may_identical (f, f2) == 0 ? NULL : antidiff (f2);
      }
    }
  } MAY_CATCH { y = 0; }
  MAY_ENDTRY ;

  return may_keep (y);
}
