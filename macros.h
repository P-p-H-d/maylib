/* This file is part of the MAYLIB libray.
   Copyright 2014 Patrick Pelissier

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

#ifndef __MAY_MACRO_H
#define __MAY_MACRO_H

// TODO: use m-lib instead

/* This is a collection of macros for c-preprocessing programming */

/* Basic handling of concatenation of symbols:
 * CAT, CAT3, CAT4
 */
#define CAT0(a, ...)         a ## __VA_ARGS__
#define CAT(a, ...)          CAT0(a, __VA_ARGS__)
#define CAT3_0(a, b, ...)    a ## b ## __VA_ARGS__
#define CAT3(a, b, ...)      CAT3_0(a ,b, __VA_ARGS__)
#define CAT4_0(a, b, c, ...) a ## b ## c ## __VA_ARGS__
#define CAT4(a, b, c, ...)   CAT3_0(a ,b, c, __VA_ARGS__)

/* Increment the number given in argument (from [0..29[) */
#define INC(x)          CAT(INC_, x)
#define INC_0 1
#define INC_1 2
#define INC_2 3
#define INC_3 4
#define INC_4 5
#define INC_5 6
#define INC_6 7
#define INC_7 8
#define INC_8 9
#define INC_9 10
#define INC_10 11
#define INC_11 12
#define INC_12 13
#define INC_13 14
#define INC_14 15
#define INC_15 16
#define INC_16 17
#define INC_17 18
#define INC_18 19
#define INC_19 20
#define INC_20 21
#define INC_21 22
#define INC_22 23
#define INC_23 24
#define INC_24 25
#define INC_25 26
#define INC_26 27
#define INC_27 28
#define INC_28 29
#define INC_29 OVERFLOW

/* Decrement the number given in argument (from [0..29[) */
#define DEC(x)          CAT(DEC_, x)
#define DEC_0 overflow
#define DEC_1 0
#define DEC_2 1
#define DEC_3 2
#define DEC_4 3
#define DEC_5 4
#define DEC_6 5
#define DEC_7 6
#define DEC_8 7
#define DEC_9 8
#define DEC_10 9
#define DEC_11 10
#define DEC_12 11
#define DEC_13 12
#define DEC_14 13
#define DEC_15 14
#define DEC_16 15
#define DEC_17 16
#define DEC_18 17
#define DEC_19 18
#define DEC_20 19
#define DEC_21 20
#define DEC_22 21
#define DEC_23 22
#define DEC_24 23
#define DEC_25 24
#define DEC_26 25
#define DEC_27 26
#define DEC_28 27
#define DEC_29 28

/* Return the nth argument:
   RETURN_1ST_ARG
   RETURN_2ND_ARG
   RETURN_27TH_ARG
 */
#define RETURN_1ST_ARG0(a, ...)     a
#define RETURN_1ST_ARG(...)         RETURN_1ST_ARG0(__VA_ARGS__)

#define RETURN_2ND_ARG0(a, b, ...)  b
#define RETURN_2ND_ARG(...)         RETURN_2ND_ARG0(__VA_ARGS__)

#define RETURN_27TH_ARG0(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,aa, ...)    aa
#define RETURN_27TH_ARG(...)        RETURN_27TH_ARG0(__VA_ARGS__)


/* Convert an integer or a symbol into 0 (if 0) or 1 (if not 0).
   1 if symbol unknown */
#define TOBOOL_0                    1, 0,
#define BOOL(x)                     RETURN_2ND_ARG(CAT(TOBOOL_, x), 1, useless)

/* Inverse 0 into 1 and 1 into 0 */
#define INV_0                       1
#define INV_1                       0
#define INV(x)                      CAT(INV_, x)

/* Perform a AND between the inputs */
#define AND_00                      0
#define AND_01                      0
#define AND_10                      0
#define AND_11                      1
#define AND(x,y)                    CAT3(AND_, x, y)

/* Perform a OR between the inputs */
#define OR_00                       0
#define OR_01                       1
#define OR_10                       1
#define OR_11                       1
#define OR(x,y)                     CAT3(OR_, x, y)

/* IF Macro :
    IF(42)(Execute if true, execute if false)
 */
#define IF_CAT(c)                   CAT(IF_, c)
#define IF_0(true_macro, ...)       __VA_ARGS__
#define IF_1(true_macro, ...)       true_macro
#define IF(c)                       IF_CAT(BOOL(c))

/* Return 1 if comma inside the argument list, 0 otherwise */
#define COMMA_P(...)                RETURN_27TH_ARG(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, useless)

/* Return 1 if the argument is 'empty', 0 otherwise.
    Handle: EMPTY_P(), EMPTY_P(x), EMPTY_P(()) and EMPTY_P(,) and EMPTY_P(f) with #define f() 2,3 */
#define EMPTY_DETECT(...)           0, 1,
#define EMPTY_P_C1(...)             COMMA_P(EMPTY_DETECT __VA_ARGS__ () )
#define EMPTY_P_C2(...)             COMMA_P(EMPTY_DETECT __VA_ARGS__)
#define EMPTY_P_C3(...)             COMMA_P(__VA_ARGS__ () )
#define EMPTY_P(...)                AND(EMPTY_P_C1(__VA_ARGS__), INV(OR(OR(EMPTY_P_C2(__VA_ARGS__), COMMA_P(__VA_ARGS__)),EMPTY_P_C3(__VA_ARGS__))))

/* IF macro for empty arguments:
    IF_EMPTY(arguments)(action if empty, action if not empty) */
#define IF_EMPTY(...)               IF(EMPTY_P(__VA_ARGS__))

/* Return 1 if argument is "()" or "(x)" */
#define PARENTHESIS_DETECT(...)     0, 1,
#define PARENTHESIS_P(x)            RETURN_2ND_ARG(PARENTHESIS_DETECT x, 0, useless)

/* Necessary macros to handle recursivity */
/* Delay the evaluation by one level or two or three or ... */
#define DELAY0()
#define DELAY1(...)                  __VA_ARGS__ DELAY0 ()
#define DELAY2(...)                  __VA_ARGS__ DELAY1 (DELAY0) ()
#define DELAY3(...)                  __VA_ARGS__ DELAY2 (DELAY0) ()
#define DELAY4(...)                  __VA_ARGS__ DELAY3 (DELAY0) ()

/* Perform 3^5 evaluation */
#define EVAL(...)                   EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL1(...)                  EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL2(...)                  EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL3(...)                  EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
#define EVAL4(...)                  EVAL0(EVAL0(EVAL0(__VA_ARGS__)))
#define EVAL0(...)                  __VA_ARGS__

/* Example of use */
/* MAP(f,a, b, c) ==> f(a) f(b) f(c) */
#define MAP_L0_INDIRECT()           MAP_L0
#define MAP_L0(f, ...)              IF_NARGS_EQ1(__VA_ARGS__)( f(__VA_ARGS__) , MAP_L1(f, __VA_ARGS__))
#define MAP_L1(f, a, ...)           f(a) DELAY3(MAP_L0_INDIRECT) () (f, __VA_ARGS__)
#define MAP(f, ...)                 IF_EMPTY(__VA_ARGS__)( /* end */, EVAL(MAP_L0(f, __VA_ARGS__)))

/* NARGS: Return number of argument (don't work for empty arg) */
#define NARGS(...) RETURN_27TH_ARG(__VA_ARGS__, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, useless)

/* If the number of arguments is 1 */
#define IF_NARGS_EQ1(...)           IF(EQUAL(NARGS(__VA_ARGS__), 1))

/* NOTEQUAL(val1,val2) with val from [0 to 30[
   Return 1 or 0 if val1=val2
 */
#define NOTEQUAL_0_0 0
#define NOTEQUAL_1_1 0
#define NOTEQUAL_2_2 0
#define NOTEQUAL_3_3 0
#define NOTEQUAL_4_4 0
#define NOTEQUAL_5_5 0
#define NOTEQUAL_6_6 0
#define NOTEQUAL_7_7 0
#define NOTEQUAL_8_8 0
#define NOTEQUAL_9_9 0
#define NOTEQUAL_10_10 0
#define NOTEQUAL_11_11 0
#define NOTEQUAL_12_12 0
#define NOTEQUAL_13_13 0
#define NOTEQUAL_14_14 0
#define NOTEQUAL_15_15 0
#define NOTEQUAL_16_16 0
#define NOTEQUAL_17_17 0
#define NOTEQUAL_18_18 0
#define NOTEQUAL_19_19 0
#define NOTEQUAL_20_20 0
#define NOTEQUAL_21_21 0
#define NOTEQUAL_22_22 0
#define NOTEQUAL_23_23 0
#define NOTEQUAL_24_24 0
#define NOTEQUAL_25_25 0
#define NOTEQUAL_26_26 0
#define NOTEQUAL_27_27 0
#define NOTEQUAL_28_28 0
#define NOTEQUAL_29_29 0
#define NOTEQUAL(x,y) BOOL(CAT(CAT(NOTEQUAL_,x), CAT(_,y)))

/* ADD: Through recursivity */
#define ADD_L0_INDIRECT()           ADD_L0
#define ADD_L1(x,y)                 DELAY3(ADD_L0_INDIRECT) () (INC(x), DEC(y))
#define ADD_L0(x,y)                 IF(BOOL(y)) (ADD_L1(x,y) , x)
#define ADD(x,y)                    EVAL(ADD_L0(x,y))

/* SUB: Through recursivity */
#define SUB_L0_INDIRECT()           SUB_L0
#define SUB_L1(x,y)                 DELAY3(SUB_L0_INDIRECT) () (DEC(x), DEC(y))
#define SUB_L0(x,y)                 IF(BOOL(y)) (SUB_L1(x,y) , x)
#define SUB(x,y)                    EVAL(SUB_L0(x,y))

/* EQUAL(val1,val2) with val from [0 to 30[
   Return 1 if val1=val2 or 0
 */
#define EQUAL(x,y) INV(NOTEQUAL(x,y))

#endif

