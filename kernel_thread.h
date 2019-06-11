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

#ifndef __MAY_THREAD_H__
#define __MAY_THREAD_H__

/* TODO: Support C11 thread when more libc will support it */
#include <pthread.h>

#if defined(__has_include)
# if __has_include(<stdatomic.h>)
# define MAY_USE_STDATOMIC
# endif
#elif  ( __STDC_VERSION__ >= 201112L )
# define MAY_USE_STDATOMIC
#endif

/* Define atomic add macro */
#if defined(MAY_USE_STDATOMIC)
# include <stdatomic.h>
# define MAY_ATOMIC_ATTR _Atomic
# define MAY_ATOMIC_ADD(var, value) atomic_fetch_add(&(var), (value))
#elif __GNUC__ >= 4
# define MAY_ATOMIC_ATTR
# define MAY_ATOMIC_ADD(var, value) __sync_fetch_and_add(&(var), (value))
#else
# define MAY_ATOMIC_ATTR volatile
# define MAY_ATOMIC_ADD(var, value) may_atomic_add(&(var), (value))
# define MAY_NEED_ATOMIC_ADD
#endif

/* Define Thread attribute */
#if  defined(MAY_USE_STDATOMIC)
# define MAY_THREAD_ATTR _Thread_local
#elif defined(_MSC_VER)
# define MAY_THREAD_ATTR __declspec( thread )
#else
# define MAY_THREAD_ATTR __thread
#endif

/* Define maximum number of helper threads */
#define MAY_MAX_THREAD 32

/* Define state of a thread */
typedef enum {
  TERMINATE_THREAD = -1,
  WAITING_FOR_DATA = 0,
  THREAD_RUNNING = 1
} may_workstate_t;

/* Define index of an array for threaded for */
typedef long may_int_t;

/* Define a block of synchro for multiples threads */
typedef struct {
  int num_spawn;            /* Number of spawned threads */
  int num_terminated_spawn; /* Number of terminated threads */
  union may_mark_s *mark;   /* Mark to give the memory after completion*/
} may_spawn_block_t[1];

/* Define the data transferred from a thread to another */
typedef struct may_data4for_s {
  void *data;               /* Input data for the 'for' loop variables */
  void *thread_reduced_var; /* Ptr to the reduced variable for thread usage*/
  may_int_t begin;          /* Initial value of the loop counter */
  may_int_t end;            /* Last excluded value of the loop counter */
} may_data4for_t;

/* Define the private data of a thread used for handling threads.
   Note: may_g variable is also private for a thread */
typedef struct {
  pthread_t       idx;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;
  may_workstate_t working;
  volatile int * num_spawn_ptr;
  union may_mark_s *mark;
  struct may_globals_s *may_g_ref;
  void * data;
  void (*func) (void *data);
  may_data4for_t data4for;
} may_mt_comm_t;

/* Define data for handling threads */
typedef struct {
  int is_initialized;
  int num_thread;
  size_t stack_size;
  pthread_mutex_t master_mutex;
  pthread_cond_t  master_cond;
  may_mt_comm_t comm[MAY_MAX_THREAD];
} may_mt_t;

/* Common variable for all threads */
extern may_mt_t may_mt_g;

/* TODO: Export may_spawn_* in may.h when the API is good enought
   Note: MACRO version are much more usefull */
extern void may_spawn_start(may_spawn_block_t, may_mark_t);
extern void may_spawn (may_spawn_block_t, void (*)(void *), void *);
extern void may_spawn_sync(may_spawn_block_t);
extern void may_thread_init(int,size_t);
extern int  may_thread_quit(void);
extern void may_spawn_for(may_mark_t,
                          may_int_t, may_int_t,
                          void (*)(void*), void *);
extern void may_spawn_for_reduce(may_mark_t, may_int_t, may_int_t,
                                 void (*)(void *),
                                 void (*)(int, const void *, void*),
                                 void *, size_t, void *);

/* Global Macro which expand its argument in case of thread engine */
#define MAY_DEF_IF_THREAD(...) __VA_ARGS__

/* Usage:
    MAY_SPAWN_BLOCK(block);
    MAY_SPAWN(block,(b,c,d), {
      a = func(b,c);
      a = func (a, d);
    }, (a));
    // Other computation in the main thread
    //...
    // Synchroniza all computations
    MAY_SPAWN_SYNC(block);
 */

/* Spawn the 'core' block computation into another thread if
   a worker thread is available. Compute it in the current thread otherwise.
   'block' shall be the initialised synchronised block for all threads.
   'input' is the list of input variables of the 'core' block.
   'output' is the list of output variables of the 'core' block.
   Output variables are only available after a synchronisation block. */
#define MAY_SPAWN(block, input, core, output)           \
  MAY_DEF_DATA(input, output)                           \
  MAY_DEF_SUBFUNC(input, output, core)                  \
  may_spawn ((block), MAY_SPAWN_SUBFUNC_NAME,           \
             &MAY_SPAWN_DATA_NAME)
#define MAY_SPAWN_STRUCT_NAME   MAY_CONCACT (data_s_, __LINE__)
#define MAY_SPAWN_DATA_NAME     MAY_CONCACT (data_, __LINE__)
#define MAY_SPAWN_SUBFUNC_NAME  MAY_CONCACT (subfunc_, __LINE__)
#define MAY_DEF_DATA(input,output)                \
  struct MAY_SPAWN_STRUCT_NAME {                  \
    MAY_DEF_DATA_INPUT input                      \
    MAY_DEF_DATA_OUTPUT output                    \
  } MAY_SPAWN_DATA_NAME = {                       \
    MAY_INIT_DATA_INPUT input                     \
    MAY_INIT_DATA_OUTPUT output                   \
  };
#define MAY_DEF_SINGLE_INPUT(var) typeof(var) var;
#define MAY_DEF_DATA_INPUT(...)                 \
  MAP(MAY_DEF_SINGLE_INPUT, __VA_ARGS__)
#define MAY_DEF_SINGLE_OUTPUT(var)              \
  typeof(var) *MAY_CONCACT(var, __ptr);
#define MAY_DEF_DATA_OUTPUT(...)                 \
  MAP(MAY_DEF_SINGLE_OUTPUT, __VA_ARGS__)
#define MAY_INIT_SINGLE_INPUT(var)              \
  .var = var,
#define MAY_INIT_DATA_INPUT(...)                \
  MAP(MAY_INIT_SINGLE_INPUT, __VA_ARGS__)
#define MAY_INIT_SINGLE_OUTPUT(var)              \
  .MAY_CONCACT(var, __ptr) = &var,
#define MAY_INIT_DATA_OUTPUT(...)                \
  MAP(MAY_INIT_SINGLE_OUTPUT, __VA_ARGS__)
#define MAY_DEF_SUBFUNC(input, output, core)            \
  auto void MAY_SPAWN_SUBFUNC_NAME(void *) ;            \
  void MAY_SPAWN_SUBFUNC_NAME(void *_data)              \
  {                                                     \
    struct MAY_SPAWN_STRUCT_NAME *_s_data = _data ;     \
    MAY_INIT_LOCAL_INPUT input                          \
      MAY_INIT_LOCAL_OUTPUT output                      \
      do { core } while (0);                            \
    MAY_PROPAGATE_LOCAL_OUTPUT output                   \
      };
#define MAY_INIT_SINGLE_LOCAL_INPUT(var)        \
  typeof(var) var = _s_data->var;
#define MAY_INIT_LOCAL_INPUT(...)               \
  MAP(MAY_INIT_SINGLE_LOCAL_INPUT, __VA_ARGS__)
#define MAY_INIT_SINGLE_LOCAL_OUTPUT(var)        \
  typeof(var) var;
#define MAY_INIT_LOCAL_OUTPUT(...)               \
  MAP(MAY_INIT_SINGLE_LOCAL_OUTPUT, __VA_ARGS__)
#define MAY_PROPAGATE_SINGLE_OUTPUT(var)        \
  *(_s_data->MAY_CONCACT(var, __ptr)) = var;
#define MAY_PROPAGATE_LOCAL_OUTPUT(...)         \
  MAP(MAY_PROPAGATE_SINGLE_OUTPUT, __VA_ARGS__)

/* Initialize a synchronisation block and select the mark to transfert the memory
   ownerchip of the memory allocated by the threads after the synchronization point */
#define MAY_SPAWN_BLOCK(_block, _mark)                              \
  may_spawn_block_t (_block) ;                                      \
  (_block)[0].num_spawn = 0 ;                                       \
  (_block)[0].num_terminated_spawn = 0;                             \
  (_block)[0].mark = &(_mark)[0];
/* NOTE: Can't set HEAP(mark) to NULL. One mark may have multiple block */

/* Synchronize all launched worker threads and continue
   when all the work of the worker threads is finished */
#define MAY_SPAWN_SYNC(_block)                                     \
  if ((_block)[0].num_spawn != (_block)[0].num_terminated_spawn)   \
    may_spawn_sync(_block)

/* Perform a for construction of the variable 'var', an integer,
   from 'begin' to 'end' (excluded) with the 'core' block.
   'in' shall be all the inputs of the blocks.
   All computation of the 'core' shall be ***fully*** independent of any other.
   'mark' is the mark to give ownership of the created memory, so that
   the compact operation of the mark will release the memory involved
   in threaded operation. */
/* Usage:
   MAY_SPAWN_FOR(mark, var, begin, end, (in), core)
   Example:
   MAY_SPAWN_FOR(mark, i, 0, 100000, (tab,tab2), { tab[i] = tab2[i]; });
 */
//Note: it may be better to spawn all threads, then use a global atomic counter
// for alls. In this case, the balancing between them will be better.
// The atomic addition is MUCH slower however.
#define MAY_SPAWN_FOR(_mark, _var, _begin, _end, _in, _core)            \
  MAY_DEF_DATA(_in, () )                                                \
  MAY_DEF_SUBFUNC_FOR(_var, _in, _core)                                 \
  if ((_end) - (_begin) > MAY_SPAWN_FOR_TH) {                           \
    may_spawn_for (_mark, _begin, _end,                                 \
                   MAY_SPAWN_SUBFUNC_NAME, &MAY_SPAWN_DATA_NAME);       \
  } else {                                                              \
    for(may_int_t (_var) = (_begin); (_var) < (_end); (_var)++) {       \
      _core ;                                                           \
    }                                                                   \
  }
#define MAY_DEF_SUBFUNC_FOR(_var, _input, _core)                \
  auto void MAY_SPAWN_SUBFUNC_NAME(void *) ;                    \
  void MAY_SPAWN_SUBFUNC_NAME(void *_data)                      \
  {                                                             \
    may_data4for_t *_f_data = _data;                            \
    const may_int_t _begin = _f_data->begin;                    \
    const may_int_t _end   = _f_data->end;                      \
    may_int_t _var;                                             \
    struct MAY_SPAWN_STRUCT_NAME *_s_data = _f_data->data ;     \
    MAY_INIT_LOCAL_INPUT _input                                 \
      MAY_ASSERT (_begin < _end);                               \
    for(_var = _begin; _var < _end; _var++)                     \
      { _core; }                                                \
  };

/* Perform a for loop construction of the variable 'var', an integer,
   from 'begin' to 'end' (excluded) with the 'core' block, in order
   to perform the reduce operation 'reduce_macro' over the for, and
   set the reduced variable in 'global_reduced_var' variable.
   'local_reduced_var' is the variable to be reduced for each loop
   iteration.
   'in' shall be all the inputs of the blocks.
   All computation a 'core' shall be ***fully*** independent of any other
   except for the reduced variable which is handled only through the
   reduced macro.
   The reduce macro shall be associative:
   reduce(reduce(a,b),c) = reduce(a, reduce(b, c))
   'mark' is the mark to give ownership of the created memory, so that
   the a compact operation of the mark will release the memory involved
   in threaded operation. */
/* Usage:
   MAY_SPAWN_FOR_REDUCE(mark, var, begin, end, (in), reduce_macro, global_reduced_var, local_reduced_var, core)
   Example to compute a sum of the elements of a sum:
   may_t *tab = .... ;
   may_t s = may_set_ui(0);
   MAY_SPAWN_FOR_REDUCE(mark, i, 0, n, (tab), MAY_REDUCED_ADD, s, tab[i], {});
   <==>
   for(int i = 0; i < n ; i++)
     s = may_addinc_c (s, tab[i]);
 */
#define MAY_SPAWN_FOR_REDUCE(_mark, _var, _begin, _end, _in,            \
                             _reduce_macro, _global_reduced_var, _local_out, \
                             _core)                                     \
    MAY_DEF_DATA(_in, () )                                              \
    MAY_DEF_SUBFUNC_FOR_REDUCE(_var, _in, _global_reduced_var,          \
                               _local_out, _reduce_macro, _core)        \
    if ((_end) - (_begin) > MAY_SPAWN_FOR_TH) {                         \
      may_spawn_for_reduce (_mark, _begin, _end,                        \
                            MAY_SPAWN_SUBFUNC_NAME,                     \
                            MAY_SPAWN_SUBFUNC2_NAME,                    \
                            &MAY_SPAWN_DATA_NAME,                       \
                            sizeof(_global_reduced_var),                \
                            &_global_reduced_var);                      \
    } else {                                                            \
      for(may_int_t (_var) = (_begin); (_var) < (_end); (_var)++) {     \
        _core ;                                                         \
        if ((_var) == (_begin)) _global_reduced_var = _local_out;       \
        else _reduce_macro(_global_reduced_var, _local_out);            \
      }                                                                 \
    }
#define MAY_SPAWN_SUBFUNC2_NAME  MAY_CONCACT (reduce__, __LINE__)
#define MAY_DEF_SUBFUNC_FOR_REDUCE(_var, _input,                        \
                                   _global_reduced_var, _local_out,     \
                                   _reduce_macro,                       \
                                   _core)                               \
    auto void MAY_SPAWN_SUBFUNC_NAME(void *) ;                          \
    void MAY_SPAWN_SUBFUNC_NAME(void *_data) {                          \
      may_data4for_t *_f_data = _data;                                  \
      const may_int_t _begin = _f_data->begin;                          \
      const may_int_t _end   = _f_data->end;                            \
      typeof(_global_reduced_var) *const _th_reduced_var =              \
        _f_data->thread_reduced_var;                                    \
      typeof(_global_reduced_var) _reduced_var;                         \
      struct MAY_SPAWN_STRUCT_NAME *_s_data = _f_data->data ;           \
      MAY_INIT_LOCAL_INPUT _input                                       \
        MAY_ASSERT (_begin < _end);                                     \
      for(may_int_t (_var) = _begin; (_var) < _end; (_var)++)           \
        { _core;                                                        \
          if ((_var) == _begin) _reduced_var = _local_out;              \
          else _reduce_macro(_reduced_var, _local_out); }               \
      *_th_reduced_var = _reduced_var;                                  \
    };                                                                  \
    auto void MAY_SPAWN_SUBFUNC2_NAME(const int n, const void *tab,     \
                                      void *global) ;                   \
    void MAY_SPAWN_SUBFUNC2_NAME(const int n, const void *v_tab,        \
                                 void *global) {                        \
      const typeof(_global_reduced_var) *_tab = v_tab;                  \
      typeof(_global_reduced_var) _reduced_var = _tab[0];               \
      for(int i = 1 ; i < n; i++)                                       \
        _reduce_macro(_reduced_var, _tab[i]);                           \
      *(typeof(_global_reduced_var) *)global = _reduced_var;            \
    };

/* Measure:
   REDUCE = + over long long ==> 10000
   REDUCE = GCD over may ==> 300
   More likely to have may operations with theses macros... */
#define MAY_SPAWN_FOR_TH 500

#endif
