#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include "may.h"

#undef numberof
#define numberof(x)  (sizeof (x) / sizeof ((x)[0]))

/* Definition of the internal operators */
static void
setjmp_handler (may_error_e e, const char *desc, const void *data)
{
  (void) desc;
  longjmp ( *(jmp_buf*)data, e);
}
static may_t help (may_t unused);
static int eval_count;
static may_t eval (may_t x) {
  unsigned long p;
  if (may_get_ui (&p, x) < 0)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  eval_count = p;
  return may_set_ui (0);
}
static may_t prec (may_t x) {
  unsigned long p;
  if (may_get_ui (&p, x) < 0)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  p = may_kernel_prec (p);
  return may_set_ui (p);
}
static may_t intmod (may_t x) {
  if (may_get_name (x) != may_integer_name)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  x = may_kernel_intmod (x);
  return x == 0 ? may_set_ui (0) : x;
}
static may_t domain (may_t d) {
  const char *name;
  if (may_get_str (&name, d) < 0)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  if (strcmp (name, "real") == 0)
    may_kernel_domain (MAY_REAL_D);
  else if (strcmp(name, "complex") == 0)
    may_kernel_domain (MAY_COMPLEX_D);
  else if (strcmp(name, "integer") == 0)
    may_kernel_domain (MAY_INTEGER_D);
  else
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  return may_set_ui (0);
}
static may_t mem (may_t unused) {
  (void) unused;
  may_kernel_info (stdout, "");
  return may_set_ui (0);
}
static may_t write (may_t x) {
  if (strcmp (may_get_name (x), may_list_name) != 0 || may_nops (x) != 2)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  const char *filename;
  if (may_get_str (&filename, may_op (x, 0)) != 0)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  FILE *f = fopen (filename, "w");
  if (f == NULL)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  may_out_string (f, may_op (x, 1));
  fclose (f);
  return may_set_ui (0);
}
static may_t read (may_t x) {
  const char *filename;
  if (may_get_str (&filename, x) != 0)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  FILE *f = fopen (filename, "r");
  if (f == NULL)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  may_t r;
  may_in_string (&r, f);
  fclose (f);
  return r;
}

static may_t rectform (may_t x) {
  may_t r, i;
  may_rectform (&r, &i, x);
  return may_set_cx (r, i);
}
static may_t subs (may_t x){
  if (strcmp (may_get_name (x), may_list_name) != 0 || may_nops (x) != 3)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  return may_replace (may_op (x, 0), may_op (x, 1), may_op (x, 2));
}
static may_t series (may_t x) {
  unsigned long p;
  if (strcmp (may_get_name (x), may_list_name) != 0 || may_nops (x) != 3
      || may_get_ui (&p, may_op (x, 2)) < 0)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  return may_series (may_op (x, 0), may_op (x, 1), p);
}
static may_t partfrac (may_t x) {
  if (may_nops (x) != 2)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  x = may_partfrac (may_op (x, 0), may_op (x, 1), NULL);
  if (x == NULL)
    may_error_throw (MAY_CANT_BE_CONVERTED_ERR, NULL);
  return x;
}
static may_t factor (may_t x) {
  if (may_get_name (x) == may_list_name && may_nops (x) != 2)
    return may_ratfactor (may_op (x, 0), may_op (x, 1));
  else
    return may_ratfactor (x, NULL);
}
static may_t approx (may_t x) {
  if (may_get_name (x) != may_list_name || may_nops (x) != 2)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  unsigned long n;
  if (may_get_ui (&n, may_op (x, 1))<0)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  return may_approx (may_op (x, 0), 10, n, GMP_RNDN);
}
static may_t degree (may_t x) {
  if (may_get_name (x) != may_list_name || may_nops (x) != 2)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  may_t var = may_op (x, 1);
  mpz_srcptr z;
  if (!may_degree (NULL, &z, NULL, may_op (x, 0), 1, &var))
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  return may_set_z (z);
}
static may_t collect (may_t x) {
  if (may_get_name (x) != may_list_name || may_nops (x) != 2)
    may_error_throw (MAY_INVALID_TOKEN_ERR, NULL);
  return may_collect (may_op(x, 0), may_op (x, 1));
}

static const char *const op_name[] = {
  "approx", "degree", "eval", "mem", "read", "write",
  "expand", "evalf", "evalr", "factor",
  "trig2exp", "exp2trig", "trig2tan2", "tan2sincos", "partfrac", "pow2exp",
  "rationalize", "normalsign", "rectform",
  "combine", "eexpand", "sign2abs", "texpand", "tcollect", "collect",
  "indets", "subs", "series", "sqrtsimp",
  "prec", "intmod", "domain", "help"
};
static const void *const op_func[] = {
  approx, degree, eval, mem, read, write,
  may_expand, may_evalf, may_evalr, factor,
  may_trig2exp, may_exp2trig, may_trig2tan2, may_tan2sincos, partfrac, may_pow2exp,
  may_rationalize, may_normalsign, rectform,
  may_combine, may_eexpand, may_sign2abs, may_texpand, may_tcollect, collect,
  may_indets, subs, series, may_sqrtsimp,
  prec, intmod, domain, help
};

static may_t help (may_t unused)
{
  unsigned int i;
  (void) unused;
  for (i = 0; i < numberof (op_name); i++)
    printf ("%s\t", op_name[i]);
  printf ("\n");
  return may_set_str ("_");
}

/*********************************************************/
static const char * const var_name[] = {
  "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z" };

static char *  update_variable (char *Buffer, int *save_it, may_t table[26])
{
  *save_it = 0;
  char *a = strstr (Buffer, ":=");
  if (a) {
    char *var = Buffer;
    *a = 0;
    a += 2;
    if (var[1] == 0 && var[0] >= 'a' && var[1] <= 'z') {
      *save_it = var[0] - 'a' + 1;
      if (*a == 0) {
        /* Delete it */
        table[*save_it-1] = NULL;
        *save_it = -1;
      }
    } else {
      printf ("WARNING: '%s' is not a one lower case letter variable. It won't be memorised\n", var);
    }
  } else
    a = Buffer;
  return a;
}
static may_t replace_variables (may_t x, may_t table[26])
{
  may_t r = may_subs_c(x, eval_count, numberof(var_name), var_name, (const void **) table);
  return may_eval (r);
}

int main (int argc, const char *argv[])
{
  char Buffer[200];
  jmp_buf err_buffer;
  int     err_code;
  may_t   r;
  may_mark_t original_mark, local_mark;
  may_t global_table[26];
  int save_it = 0;
  int quiet =  0;
  size_t size2alloc = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-q") == 0)
      quiet = 1;
    else if (strcmp (argv[i], "-m") == 0 && i != (argc-1)) {
      size2alloc = atoi (argv[i+1]) * (1UL << (sizeof (int)*5));
      i++;
    } else {
      fprintf (stderr, "Unkwown option: %s\n", argv[i]);
      exit (2);
    }
  }

  eval_count = 1;
  memset (global_table, 0, sizeof global_table);
  may_kernel_start (size2alloc, 0);
  may_kernel_prec (53);
  if (quiet == 0)
    printf ("Expression Evaluator\n"
            "Using %s\n"
            "Enter 'exit' to quit\n", may_get_version ());
  r       = may_set_ui (0);

  /* Register exception handler */
  err_code = setjmp(err_buffer);
  if (err_code == 0) {
    may_error_catch (setjmp_handler, &err_buffer);
    may_mark (original_mark);
    /* Loop until exit */
    while (1) {
      char *e;
      /* Read string and handle exit */
      if (quiet == 0) {
        printf ("$ ");
        fflush (stdout);
      }
      if (fgets (Buffer, sizeof Buffer, stdin) == 0)
        break;
      if ((e = strchr (Buffer, '\n')))
        *e = 0;
      if (strcmp (Buffer, "exit") == 0)
        break;
      /* Parsing and check if it is needed to save the variable latter */
      may_mark (local_mark);
      e = may_parse_c (&r, update_variable (Buffer, &save_it, global_table));
      if (*e != 0) {
	printf ("Syntax Error: '%s'.\n", e);
        may_compact (local_mark, NULL);
        continue;
      }
      /* Evaluation and take into account operator */
      r = replace_variables (r, global_table);
      r = may_subs_c(r, 1, numberof(op_name), op_name, op_func);
      r = may_eval (r);
      /* Display */
      e = may_get_string (NULL, 0, r);
      printf("%s\n", e);
      /* Save and compact */
      if (save_it > 0) {
        /* Save the new variable (garbage the memory before) */
        global_table[save_it-1] = may_compact (local_mark, r);
      } else if (save_it < 0) {
        /* A variable was deleted: garbash all the table */
        may_compact_v (original_mark, numberof (global_table), global_table);
      } else {
        /* Nothing to save except old globals which were compacted pass the local_mark */
        may_compact (local_mark, NULL);
      }
    }
    may_keep (original_mark, NULL);
    may_error_uncatch ();
  } else {
    may_kernel_info (stdout, "FATAL");
    printf("Exception %d caught: %s\n",
           err_code, may_error_what (err_code));
  }

  may_kernel_end ();
  exit (0);
}
