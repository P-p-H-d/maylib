#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/resource.h>

#include "may-impl.h"

// Non exporté.
// Faire un système générique.
struct may_mat_s {
  unsigned int row;
  unsigned int col;
  size_t alloc;
  may_t *data;
};
typedef struct may_mat_s may_mat_t[1];
typedef const struct may_mat_s *may_mat_srcptr;
typedef struct may_mat_s *may_mat_ptr;

/***************** INTERNAL INLINE FAST FUNCTIONS ******************************/

MAY_INLINE unsigned int may_mat_row (may_mat_srcptr c) { return c->row; }
MAY_INLINE unsigned int may_mat_col (may_mat_srcptr c) { return c->col; }

MAY_INLINE void may_mat_resize (may_mat_ptr c, unsigned int row, unsigned int col)
{
  size_t p_new = (size_t) row*col;
  if (c->alloc < p_new) {
    may_t zero = may_set_ui (0);
    c->data = may_realloc (c->data, c->alloc*sizeof(may_t),
                           p_new*sizeof(may_t));
    for (size_t i = c->alloc ; i < p_new; i++)
      (c->data)[i] = zero;
    c->alloc = p_new;
  }
  c->row = row;
  c->col = col;
}

MAY_INLINE void may_mat_init (may_mat_ptr c, unsigned int row, unsigned int col)
{
  c->alloc = c->row = c->col = 0;
  may_mat_resize (c, row, col);
}

MAY_INLINE void may_mat_copy (may_mat_ptr c, may_mat_srcptr a)
{
  if (a!=c) {
    size_t p_new = (size_t) a->row*a->col;
    may_mat_resize (c, a->row, a->col);
    for (size_t i = 0; i < p_new; i++)
      (c->data)[i] = may_copy_c ((a->data)[i], 1);
  }
}

MAY_INLINE void may_mat_set (may_mat_ptr c, may_mat_srcptr a)
{
  if (a!=c) {
    size_t p_new = (size_t) a->row*a->col;
    may_mat_resize (c, a->row, a->col);
    for (size_t i = 0; i < p_new; i++)
      (c->data)[i] = (a->data)[i];
  }
}

MAY_INLINE void may_mat_set_at (may_mat_ptr c, unsigned int ri, unsigned int ci, may_t val)
{
  MAY_ASSERT (ri < c->row);
  MAY_ASSERT (ci < c->col);
  (c->data)[(size_t) ri*c->col+ci] = val;
}

MAY_INLINE may_t may_mat_at (may_mat_srcptr c, unsigned int ri, unsigned int ci)
{
  MAY_ASSERT (ri < c->row);
  MAY_ASSERT (ci < c->col);
  return (c->data)[(size_t) ri*c->col+ci];
}

MAY_INLINE void may_mat_set_at0 (may_mat_ptr c, size_t i, may_t val)
{
  MAY_ASSERT (i < (size_t) c->row*c->col);
  (c->data)[i] = val;
}

MAY_INLINE may_t may_mat_at0 (may_mat_srcptr c, size_t i)
{
  MAY_ASSERT (i < (size_t) c->row*c->col);
  return (c->data)[i];
}

MAY_INLINE void may_mat_compact (may_mark_t mark, may_mat_ptr a)
{
  a->data =  may_compact_v (mark, (size_t) a->row*a->col, a->data);
}

MAY_INLINE void may_mat_apply1 (may_mat_ptr c, may_mat_srcptr a, may_t (*f)(may_t))
{
  size_t p_new = (size_t) a->row*a->col;
  may_mat_resize (c, a->row, a->col);
  for (size_t i = 0; i < p_new; i++)
    (c->data)[i] = (*f) ((a->data)[i]);
}

MAY_INLINE void may_mat_apply1y (may_mat_ptr c, may_mat_srcptr a, may_t y, may_t (*f)(may_t, may_t))
{
  size_t p_new = (size_t) a->row*a->col;
  may_mat_resize (c, a->row, a->col);
  for (size_t i = 0; i < p_new; i++)
    (c->data)[i] = (*f) ((a->data)[i], y);
}


MAY_INLINE void may_mat_apply2 (may_mat_ptr c, may_mat_srcptr a, may_mat_srcptr b, may_t (*f)(may_t, may_t))
{
  size_t p_new = (size_t) a->row*a->col;
  if (a->row != b->row || a->col != b->col)
    may_error_throw (MAY_DIMENSION_ERR, NULL);
  
  may_mat_resize (c, a->row, a->col);
  for (size_t i = 0; i < p_new; i++)
    (c->data)[i] = (*f) ((a->data)[i], (b->data)[i]);
}

// Tout ce qui n'est pas inline ne peut pas accéder au champ de may_mat_t

void may_mat_dump (may_mat_srcptr a)
{
  unsigned int m, n, i, j;
  n = may_mat_row (a);
  m = may_mat_col (a);
  for (i = 0; i < n; i++) {
    if (i == 0)
      printf ("[[");
    else
      printf (" [");
    for (j = 0 ; j < m; j++) {
      may_out_string (stdout, may_mat_at(a, i, j));
      if (j != (m-1))
        printf (",");
    }
    if (i != (n-1))
      printf ("]\n");
    else
      printf ("]]\n");
  }
}

void may_mat_info (size_t *sparness, size_t *purenum, may_domain_e *domain,
                   may_mat_srcptr a)
{
  size_t p_new = (size_t) may_mat_row (a)*may_mat_col(a);
  size_t s = 0, num = 0;
  enum may_enum_t type = MAY_INT_T;

  /* Collect info */
  for( size_t i = 0; i < p_new; i++) {
    may_t ai = may_mat_at0(a, i);
    s += !!may_zero_p(ai);
    num += !!MAY_PURENUM_P (ai);
    type = MAX(type, MAY_TYPE(ai));
  }

  /* Return it to the caller */
  *sparness = s;
  *purenum  = num;
  switch (type) {
  case MAY_INT_T:
    *domain = MAY_INTEGER_D;
    break;
  case MAY_RAT_T:
    *domain = MAY_RATIONAL_D;
    break;
  case MAY_FLOAT_T:
    *domain = MAY_REAL_D;
    break;
  default:
    *domain = MAY_COMPLEX_D;
    break;
  }
}


void may_mat_eval (may_mat_ptr a, may_mat_srcptr b) {
  may_mat_apply1 (a, b, may_eval);
}
void may_mat_conj (may_mat_ptr a, may_mat_srcptr b) {
  may_mat_apply1 (a, b, may_conj_c);
  may_mat_eval (a, a);
}
void may_mat_real (may_mat_ptr a, may_mat_srcptr b) {
  may_mat_apply1 (a, b, may_real_c);
  may_mat_eval (a, a);
}
void may_mat_imag  (may_mat_ptr a, may_mat_srcptr b) {
  may_mat_apply1 (a, b, may_imag_c);
  may_mat_eval (a, a);
}

void may_mat_add_y (may_mat_ptr a, may_mat_srcptr b, may_t y) {
  may_mat_apply1y(a, b, y, may_add);
}
void may_mat_sub_y (may_mat_ptr a, may_mat_srcptr b, may_t y) {
  may_mat_apply1y(a, b, y, may_sub);
}
void may_mat_mul_y (may_mat_ptr a, may_mat_srcptr b, may_t y) {
  may_mat_apply1y(a, b, y, may_mul);
}

void may_mat_add(may_mat_ptr c, may_mat_srcptr a, may_mat_srcptr b) {
  may_mat_apply2 (c, a, b, may_add);
}
void may_mat_sub(may_mat_ptr c, may_mat_srcptr a, may_mat_srcptr b) {
  may_mat_apply2 (c, a, b, may_sub);
}


void may_mat_mul(may_mat_ptr c, may_mat_srcptr a, may_mat_srcptr b)
{
  unsigned int i, j, k, m, n, p;
  may_mark_t mark;
  may_mat_t temp;
  may_mat_ptr old_c ;

  m = may_mat_row (a);
  n = may_mat_col (b);
  p = may_mat_row (b);

  if (p != may_mat_col (a))
    may_error_throw (MAY_DIMENSION_ERR, NULL);

  may_mark (mark);

  /* Create a copy of c if c is a or b */
  if (c == a || c == b) {
    old_c = c;
    may_mat_init (temp, m, n);
    c = temp;
  } else {
    old_c = NULL;
    may_mat_resize (c, m, n);
  }

  /* FIXME: Optimization ?
     Perform a first pass to detect the sparness of A or B ? */
#if 0
  may_mat_set_y (c, may_set_ui (0));

  for (i = 0; i < m ; i++)
    for (j = 0; j < p ; j++) {
      may_t tmp = may_mat_at (a, i, j);
      if (may_zero_p (tmp))
        continue;
      for (unsigned int k = 0; k < n; k++) {
        may_t tmp2 = may_mul_c (tmp, may_mat_at (b, j, k));
        tmp2 = may_addinc_c (may_mat_at (c, i, k), tmp2);
        may_mat_set_at (c, i, k, tmp2);
      }
    }
#endif
  for (i = 0; i < m ; i++)
    for (j = 0 ; j < n ; j++) {
      may_mark_t mark2;
      may_mark (mark2);
      may_t s = may_set_ui(0);
      for (k = 0; k < p; k++)
	s = may_addinc_c (s, may_mul_c (may_mat_at (a, i, k), may_mat_at (b, k, j)));
      s = may_compact (mark2, may_eval (s));
      may_mat_set_at (c, i, j, s);
    }

  if (old_c ) {
    may_mat_set (old_c, c);
    may_mat_compact (mark, old_c);
  }
}

void
may_mat_extract (may_mat_ptr out, may_mat_srcptr a,
		 unsigned int minrow, unsigned int maxrow,
		 unsigned int mincol, unsigned int maxcol)
{
  may_mat_t temp;
  may_mat_ptr c;
  may_mark_t mark;
  unsigned int row = may_mat_row (a);
  unsigned int col = may_mat_col (a);

  if (maxrow >= row)
    maxrow = row-1;
  if (minrow >= row)
    minrow = row-1;
  if (minrow > maxrow)
    minrow = maxrow;
  if (maxcol >= col)
    maxcol = col-1;
  if (mincol >= col)
    mincol = col-1;
  if (mincol > maxcol)
    mincol = maxcol;

  row = maxrow-minrow+1;
  col = maxcol-mincol+1;

  may_mark (mark);
  if (a == out) {
    c = temp;
    may_mat_init (c, row, col);
  } else {
    c = out;
    may_mat_resize (c, row, col);
  }

  for (unsigned int i = 0; i < row; i++)
    for (unsigned int j = 0; j < col; j++)
      may_mat_set_at (c, i, j, may_copy_c (may_mat_at (a, minrow+i, mincol+j), 1));

  if (c != out) {
    may_mat_set (out, c);
    may_mat_compact (mark, out);
  }
}

void
may_mat_augment (may_mat_ptr out, may_mat_srcptr a, may_mat_srcptr b)
{
  unsigned int row, cola, colb;
  may_mat_ptr out2;
  may_mat_t temp;
  may_mark_t mark;
  
  row = may_mat_row (a);
  if (row != may_mat_row (b))
    may_error_throw (MAY_DIMENSION_ERR, NULL);
  cola = may_mat_col (a);
  colb = may_mat_col (b);
  
  may_mark (mark);
  out2 = out;
  if (out == a || out == b) {
    may_mat_init (temp, row, cola+colb);
    out2 = temp;
  } else
    may_mat_resize (out2,  row, cola+colb);

  for (unsigned int i = 0; i <  row ; i++) {
    for (unsigned int j = 0; j < cola; j++)
      may_mat_set_at (out2, i, j, may_mat_at (a, i, j));
    for (unsigned int j = 0; j < colb; j++)
      may_mat_set_at (out2, i, cola+j, may_mat_at (b, i, j));
  }
  
  if (out != out2) {
    may_mat_set (out, out2);
    may_mat_compact (mark, out);
  }
}

void
may_mat_fill_y (may_mat_ptr c, may_t b)
{
  size_t p_new = (size_t) may_mat_row(c)*may_mat_col(c);
  for (size_t i = 0; i < p_new; i++)
    may_mat_set_at0 (c, i, b);
}

int
may_mat_square_p (may_mat_srcptr a)
{
  return may_mat_row(a) == may_mat_col (a);
}

void
may_mat_identity (may_mat_ptr out, unsigned int row, may_t x)
{
  size_t i, p_new = (size_t) may_mat_row(out)*may_mat_col(out);

  may_mat_resize (out, row, row);
  for (i = 0; i < p_new; ) {
    may_mat_set_at0(out,i++,may_copy_c (x, 1));
    for (unsigned int j = 0; j < row; j++)
      may_mat_set_at0(out, i++, may_set_ui (0));
  }
}

void
may_mat_make (may_mat_ptr out, unsigned int row, unsigned int col, may_t pattern)
{
  may_mark_t mark;
  may_mark (mark);
  may_mat_resize (out, row, col);
  may_t i_str = may_set_str ("i");
  may_t j_str = may_set_str ("j");
  for (unsigned int i = 1; i <= row; i++) {
    may_t pattern_i = may_replace (pattern, i_str, may_set_ui (i));
    for (unsigned int j = 1; j <= row; j ++) {
      may_t x = may_replace (pattern_i, j_str, may_set_ui (j));
      may_mat_set_at (out, i-1, j-1, x);
    }
  }
  may_mat_compact (mark, out);
}

void
may_mat_transpose (may_mat_ptr out, may_mat_srcptr a)
{
  unsigned int row = may_mat_row (a), col = may_mat_col (a);
  may_mat_ptr c;
  may_mat_t temp;
  may_mark_t mark;

  may_mark (mark);
  if (out == a) {
    may_mat_init (temp, col, row);
    c = temp;
  } else {
    may_mat_resize (out, col, row);
    c = out;
  }

  for(unsigned int i = 0; i < row; i++)
    for (unsigned int j = 0; j < col; j++)
      may_mat_set_at (c, j, i, may_mat_at (a, i, j));

  if (out != c) {
    may_mat_set (out, c);
    may_mat_compact(mark, out);
  }
}

may_t
may_mat_trace (may_mat_srcptr a)
{
  may_mark ();
  may_t s = may_set_ui (0);
  for (unsigned int i = 0; i < may_mat_row(a); i++)
    s = may_addinc_c (s, may_mat_at(a, i, i));
  return may_keep (may_eval (s));
}

/* Gauss Reduction:
   If notfull=0, Full Reduction.
   If notfull=1, Partial Reduction (faster) 
   Quite complicated due to symbolic matrix
   Returns 1 if an even number of rows were swapped
   Return -1 if an odd number of rows were swapped
   Return  0 if the matrix was singuler */
unsigned int
may_mat_gauss (may_mat_ptr out, may_mat_srcptr x, int notfull)
{
  unsigned int row = may_mat_row (x), col = may_mat_col (x);
  int sign = 1;
  may_t *row_ptr[row];
  may_mark_t mark;
  may_mat_t temp;

  MAY_ASSERT (notfull == 0 || notfull == 1);

  may_mark (mark);

  may_mat_init (temp, row, col);
  /* We can only handle evaluated arguments */
  may_mat_eval (temp, x);

  /* We will handle the matrix throught a row index
     to perform a fast swapping of the rows */
  for (unsigned int i = 0; i < row ;i++)
    row_ptr[i] = &((temp->data)[i*col]);
  
  /* Main Loop */
  unsigned int row_p = 0, compact_counter = 0;
  for (unsigned int col_p = 0 ; col_p < col && row_p < row - notfull; col_p++) {
    unsigned int p = -1;
    may_t pivot;
    size_t pivot_len = -1, new_len;
#if 0
    printf ("Next step:\n");
    for (unsigned int j = 0; j < row; j++)
      for (unsigned int k = 0; k < col; k++) {
        may_out_string (stdout, row_ptr[j][k]);
        printf ("%c", k != (col-1) ? ',' : '\n');
      }
#endif

    /* Search for a non-null pivot, and as small as possible  */
    for (unsigned int j = row_p ; j < row ; j++) {
      if (!may_zero_p (row_ptr[j][col_p])
          && pivot_len > (new_len = may_length (row_ptr[j][col_p]))) {
        /* Smaller non nul pivot */
        p = j;
        pivot = row_ptr[j][col_p];
        pivot_len = new_len;
      }
    }
    /* Swap Pivot if needed */
    if (p != row_p) {
      if (p == -1) {
        /* p==-1 ==> Singular matrix */
	sign = 0;
        /* Fill the column with 0 */
        for (unsigned int j = notfull*row_p; j < row_p; j++)
          row_ptr[j][col_p] = MAY_ZERO;
        /* Don't increment the pivot row */
        continue;
      }
      swap (row_ptr[row_p], row_ptr[p]);
      sign = -sign;
    }
    /* Reduce the lines */
    for (unsigned int j = notfull*row_p; j < row; j++) {
      /* Don't change the pivot line */
      if (MAY_UNLIKELY(j == row_p))
	continue;
      /* if row_ptr[j][i] is zero, the line is already simplified */
      may_t leading_i = row_ptr[j][col_p];
      if (MAY_UNLIKELY(may_zero_p (leading_i)))
        continue;
      /* Replace the line with the line - pivot_line * line[i] / pivot */
      may_t tmp = may_eval (may_neg_c (may_div_c (leading_i, pivot)));
      /* Fill up left hand side with 0 */
      row_ptr[j][col_p] = MAY_ZERO;
      /* Reduction */
      for (unsigned int k = col_p+1 ; k < col ; k++)
	row_ptr[j][k] = may_eval (may_add_c (row_ptr[j][k],
                                                 may_mul_c (tmp, row_ptr[p][k])));
    }
    /* Compact matrix for freeing memory.
       For a full gauss reduction of a matrix 1/(i+j-1)
            | WITHOUT | WITH
       TIME | 14425ms | 14473ms
       MEM  | 142M    | 34M           */
    if (++compact_counter >= 10) {
      may_mat_compact (mark, temp);
      compact_counter = 0;
    }
    /* Next Pivot line */
    row_p ++;
  }

  /* Clean remaining rows if any */
  for (unsigned int j = row_p+notfull; j < row; j++)
    for (unsigned int k = 0; k < col; k++)
      row_ptr[j][k] = MAY_ZERO;

  /* Rebuild the matrix according to the requested swapping */
  for (unsigned int i = 0; i < row ;i++)
    for (unsigned int j = 0; j < col ; j++)
      may_mat_set_at (out, j, i, row_ptr[j][i]);
  may_mat_compact (mark, out);

  return sign;
}

/* TODO:
   void  may_mat_divfree(may_mat_ptr out, may_mat_srcptr a, int notfull);
   void  may_mat_fracfree(may_mat_ptr out, may_mat_srcptr a, int notfull);
   void  may_mat_solve(may_mat_ptr x, may_mat_srcptr a, may_mat_srcptr b);
   void  may_mat_inverse(may_mat_ptr out, may_mat_srcptr a);
   void  may_mat_pow_si(may_mat_ptr out, may_mat_srcptr a, long n);
   may_t may_mat_det(may_mat_srcptr a);
   may_t may_mat_charpoly(may_mat_srcptr a);
   unsigned int may_mat_rank(may_mat_srcptr a);
   void  may_mat_diag(may_mat_ptr out, unsigned int num, may_t tab[num]);
*/

void (*dump) (may_t) = may_dump;
static int cputime (void)
{
  struct rusage rus;
  getrusage (0, &rus);
  return rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_usec / 1000;
}
int main (int argc, const char *argv[])
{
  may_kernel_start (0, 0);

  may_mat_t m, m1, m2;
  int t, size;

  size = argc >= 2 ? atoi (argv[1]) : 4;

  may_mat_init (m, 3, 3);
  may_mat_make (m, size, size, may_parse_str ("1/(i+j+1+x)"));
  printf ("Original:\n");
  may_mat_dump (m);
  
  may_mat_init (m1, 3, 3);
  may_mat_sub (m1, m, m);
  printf ("SUB:\n");
  may_mat_dump (m1);

  may_mat_add (m1, m, m);
  printf ("ADD:\n");
  may_mat_dump (m1); 

  may_mat_init (m2, 3, 3);
  t = cputime();
  may_mat_mul (m2, m, m);
  t = cputime() - t;
  printf ("MUL:\n");
  if (size < 10)
  may_mat_dump (m2);
  printf ("TIME= %dms\n", t);

  t = cputime();
  may_mat_gauss (m2, m2, 1);
  t = cputime() - t;
  printf("GAUSS1:\n");
  if (size < 10)
  may_mat_dump (m2);
  printf ("TIME= %dms\n", t);

  may_mat_mul (m2, m, m);
  t = cputime();
  may_mat_gauss (m2, m2, 0);
  t = cputime() - t;
  printf ("GAUSS0\n");
  if (size < 10)
  may_mat_dump (m2);
  printf ("TIME= %dms\n", t);

  may_kernel_end ();
}

