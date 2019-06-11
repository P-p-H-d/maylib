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

#if defined (MAY_WANT_THREAD)

#include <pthread.h>
#include "may-impl.h"

/* Define shared data between thread, needed to handle them */
may_mt_t may_mt_g;

/* Reset the globals of a thread.
   Needed to have a clean state for a thread which is started */
static void reset_may_global(void)
{
  memset (&may_g, 0, sizeof (may_g));
  may_heap_init(&may_g.Heap, may_mt_g.stack_size, 0, 0);
  /* FIXME: How to design this properly? */
  may_g.kara.threshold = 10;
  may_g.kara.tmpnum = MAY_DUMMY;
}

/* Code of the main function of each working thread */
static void *may_mt_thread_main (void *arg)
{
  may_mt_comm_t *mc = arg;

  pthread_mutex_lock(&mc->mutex);

  /* Save reference of personnal may_g variable */
  mc->may_g_ref = &may_g;

  /* Reset may_g variable (& allocate a new heap) */
  reset_may_global();

  while (1) {
    /* Wait for data to process... */
    /* If not working, sleep until wake up */
    if (mc->working <= WAITING_FOR_DATA) {
      /* Check for terminate condition */
      if (mc->working == TERMINATE_THREAD) {
        break;
      }
      /* Wait for next cycle */
      pthread_cond_wait(&mc->cond, &mc->mutex);
      continue;
    }
    pthread_mutex_unlock(&mc->mutex);

    /* Disable error handling and string cache ***after*** master thread
       set may_g.frame with its own may_g.frame */
    may_g.frame.next = NULL;
    may_g.frame.error_handler = NULL;
    may_g.frame.cache_set_str_i = 0;
    may_g.frame.cache_set_str_n = 0;

    /* Execute thread */
    (*mc->func) (mc->data);

    /* Save MAY stack thread within the heap */
    struct may_heap_s *heap = may_alloc(sizeof ( struct may_heap_s));
    memcpy (heap, &may_g.Heap, sizeof ( struct may_heap_s));

    /* Unwork thread */
    pthread_mutex_lock(&mc->mutex);
    mc->working = WAITING_FOR_DATA;

    /* Enter Signal terminaison block */
    pthread_mutex_lock (&may_mt_g.master_mutex);

    /* Chain the heap of this Thread into the list of the threads
       to be free after the compact operation of the master thread */
    struct may_heap_s *previous =  MAY_MARK_HEAP_TO_FREE(mc->mark);
    MAY_MARK_HEAP_TO_FREE(mc->mark) = heap;
    heap->next_heap_to_free = previous;

    /* Signal terminaison */
    *mc->num_spawn_ptr += 1;
    pthread_cond_broadcast(&may_mt_g.master_cond);
    pthread_mutex_unlock (&may_mt_g.master_mutex);

    /* Reset may_g variable (& allocate a new heap) */
    reset_may_global();
  }
  pthread_mutex_unlock(&mc->mutex);
  return NULL;
}


/* TODO: Support failure in initialization */
void may_thread_init(int num_thread, size_t size)
{
  int rc;
  MAY_ASSERT (1 <= num_thread && num_thread <= MAY_MAX_THREAD);

  /* Initialize global memory for MT handling */
  memset(&may_mt_g, 0, sizeof may_mt_g);
  may_mt_g.stack_size = size;

  /* Initialize global mutex */
  rc = pthread_mutex_init(&may_mt_g.master_mutex, NULL);
  if (rc != 0)
   abort();
  rc = pthread_cond_init(&may_mt_g.master_cond, NULL);
  if (rc != 0)
    abort();

  /* Initialize threads */
  may_mt_g.num_thread = num_thread - 1;
  for (int i = 0 ; i < num_thread-1; i++) {
    rc = pthread_mutex_init(&may_mt_g.comm[i].mutex, NULL);
    if (rc != 0)
      abort();
    rc = pthread_cond_init(&may_mt_g.comm[i].cond, NULL);
    if (rc != 0)
      abort();
    rc = pthread_create (&may_mt_g.comm[i].idx,
                         NULL, may_mt_thread_main, &may_mt_g.comm[i]);
    if (rc != 0)
      abort();
  }

  /* Wait for the threads to block, waiting for data to work on */
  for (int i = 0 ; i < num_thread-1; i++) {
    /* This is reported as a data race since it fails to see that
       this was initialised to NULL using the memset before and
       we are waiting for the thread to set it to non-null */
    while (may_mt_g.comm[i].may_g_ref == NULL)  {
      pthread_mutex_lock (&may_mt_g.master_mutex);
      pthread_mutex_unlock (&may_mt_g.master_mutex);
    }
  }

  may_mt_g.is_initialized = true;
}

int may_thread_quit(void)
{
  int rc;
  int previous = may_mt_g.num_thread+1;
  UNUSED(rc);
  /* If the thread system has been initialized */
  if (may_mt_g.is_initialized != false) {
    for(int i = 0; i < may_mt_g.num_thread ; i++) {
      may_mt_comm_t *mc = &may_mt_g.comm[i];
      /* Request terminaison */
      rc = pthread_mutex_lock (&mc->mutex);
      MAY_ASSERT (rc == 0);
      /* No thread should be working when calling this function */
      MAY_ASSERT (mc->working == WAITING_FOR_DATA);
      mc->working = TERMINATE_THREAD; /* Request terminaison */
      pthread_cond_signal(&mc->cond);
      pthread_mutex_unlock (&mc->mutex);

      /* Join it to terminate it */
      rc = pthread_join(may_mt_g.comm[i].idx, NULL);
      MAY_ASSERT (rc == 0);

      /* mutex_destroy needs mutex to be unlocked */
      rc = pthread_mutex_destroy(&may_mt_g.comm[i].mutex);
      MAY_ASSERT (rc == 0);
      rc = pthread_cond_destroy(&may_mt_g.comm[i].cond);
      MAY_ASSERT (rc == 0);

      /* Free heap */
      may_heap_clear (&(may_mt_g.comm[i].may_g_ref->Heap));
    }
    /* mutex_destroy needs mutex to be unlocked */
    rc = pthread_mutex_destroy(&may_mt_g.master_mutex);
    MAY_ASSERT (rc == 0);
    rc = pthread_cond_destroy(&may_mt_g.master_cond);
    MAY_ASSERT (rc == 0);

    may_mt_g.is_initialized = false;
    may_mt_g.num_thread = 0;
  }
  return previous;
}

/* Start a new parallel job.
   The parallel job reference is block.
   Once the parallel job is finished, garbashed collect the memory
   when dealing with the next compact involving mark */
void may_spawn_start(may_spawn_block_t block, may_mark_t mark)
{
  block->num_spawn = 0;
  block->num_terminated_spawn = 0;
  block->mark = &mark[0];
}

/* Launch (or not) a new parallel job to compute func(data) */
void may_spawn (may_spawn_block_t block, void (*func)(void *), void *data)
{
  for(int i = 0; i < may_mt_g.num_thread ; i++) {
    may_mt_comm_t *mc = &may_mt_g.comm[i];

    /* If the thread is not working */
    if (mc->working == WAITING_FOR_DATA) {
      pthread_mutex_lock (&mc->mutex);
      /* If the thread is still not working */
      if (MAY_UNLIKELY (mc->working != WAITING_FOR_DATA)) {
        pthread_mutex_unlock (&mc->mutex);
        continue;
      }

      /* Setup data for the thread */
      mc->working = THREAD_RUNNING;
      mc->func = func;
      mc->data = data;
      mc->num_spawn_ptr = &block->num_terminated_spawn;
      mc->mark   = block->mark;
      block->num_spawn +=1;

      /* Setup frame of the thread:
         intmaxsize, prec, rnd_mode, etc... */
      /* FIXME: How to speed up this ?*/
      MAY_ASSERT (mc->may_g_ref != NULL);
      memcpy (&mc->may_g_ref->frame, &may_g.frame, sizeof may_g.frame);

      /* Signal to thread that some work are available */
      pthread_cond_signal(&mc->cond);
      pthread_mutex_unlock (&mc->mutex);
      return ;
    }
  }
  /* No thread available. Call the function ourself */
  (*func) (data);
}

void may_spawn_sync(may_spawn_block_t block)
{
  /* If the number of spawns is greated than the number
     of terminated spawns, some spawns are still working.
     So wait for terminaison */
  if (block->num_spawn > block->num_terminated_spawn) {
    pthread_mutex_lock (&may_mt_g.master_mutex);
    while (1) {
      if (block->num_spawn == block->num_terminated_spawn)
        break;
      pthread_cond_wait(&may_mt_g.master_cond, &may_mt_g.master_mutex);
    }
    pthread_mutex_unlock (&may_mt_g.master_mutex);
  }
}

void may_spawn_for(may_mark_t mark,
                   may_int_t begin, may_int_t end,
                   void (*func)(void*), void *data)
{
  MAY_SPAWN_BLOCK(block, mark);

  /* Compute number of cores */
  // FIXME: Preemption? Could it be bad?
  int nb_core_available = 1;
  for(int i = 0; i < may_mt_g.num_thread ; i++)
    nb_core_available += (may_mt_g.comm[i].working == WAITING_FOR_DATA);

  MAY_ASSERT (begin < end);
  const int nb_core_needed = (end-begin + MAY_SPAWN_FOR_TH-1)
    / (MAY_SPAWN_FOR_TH/2);
  MAY_ASSERT (nb_core_needed > 0);
  const int nb_core = MIN(nb_core_needed, nb_core_available);

  const may_int_t step = (end-begin) / nb_core;
  may_int_t var  = begin;

  for(int i = 0; i < may_mt_g.num_thread && var < end; i++) {
    may_mt_comm_t *mc = &may_mt_g.comm[i];
    if (mc->working == WAITING_FOR_DATA) {
      pthread_mutex_lock (&mc->mutex);
      /* If the thread is still not working */
      if (MAY_UNLIKELY (mc->working != WAITING_FOR_DATA)) {
        pthread_mutex_unlock (&mc->mutex);
        continue;
      }

      /* Setup data for the thread: handle the for from var to var+step */
      mc->working = THREAD_RUNNING;
      mc->func = func;
      mc->data4for.begin = var;
      mc->data4for.end   = var + step;
      mc->data4for.data  = data;
      mc->data = &mc->data4for.data;
      mc->num_spawn_ptr = &block->num_terminated_spawn;
      mc->mark   = block->mark;
      block->num_spawn +=1;

      /* Setup frame of the thread:
         intmaxsize, prec, rnd_mode, etc... */
      /* FIXME: How to speed up this ?*/
      memcpy (&mc->may_g_ref->frame, &may_g.frame, sizeof may_g.frame);

      /* Signal to thread that some work are available */
      pthread_cond_signal(&mc->cond);
      pthread_mutex_unlock (&mc->mutex);

      /* Next one */
      var += step;
    }
  }

  /* Finish the for loop with the remainder */
  may_data4for_t data4for;
  data4for.begin = var;
  data4for.end   = end;
  data4for.data  = data;
  (*func)(&data4for);

  MAY_SPAWN_SYNC(block);
}

void
may_spawn_for_reduce (may_mark_t mark, may_int_t begin, may_int_t end,
                      void (*compute_func)(void *),
                      void (*reduce_func)(int, const void *, void*),
                      void *data,
                      size_t size_global_reduced_var,
                      void *global_reduced_var_ptr)
{
  /* Nearly copy / paste of may_spawn_for
     except we have to set data->thread_reduced_var
     + the final reduction step */
  MAY_SPAWN_BLOCK(block, mark);

  /* Compute number of cores */
  // FIXME: Preemption? Could it be bad?
  int nb_core_available = 1;
  for(int i = 0; i < may_mt_g.num_thread ; i++)
    nb_core_available += (may_mt_g.comm[i].working == WAITING_FOR_DATA);

  MAY_ASSERT (begin < end);
  const int nb_core_needed = (end-begin + MAY_SPAWN_FOR_TH-1)
    / (MAY_SPAWN_FOR_TH/2);
  MAY_ASSERT (nb_core_needed > 0);

  const int nb_core = MIN(nb_core_needed, nb_core_available);

  /* Alloc the array of the reduced variable for each thread */
  char *thread_reduced_var_tab = may_alloc (nb_core * size_global_reduced_var);

  /* Compute the step of a thread */
  const may_int_t step = (end-begin) / nb_core;
  may_int_t var  = begin;
  int real_nb_core = 0;

  for(int i = 0; i < may_mt_g.num_thread && var < end; i++) {
    may_mt_comm_t *mc = &may_mt_g.comm[i];
    if (mc->working == WAITING_FOR_DATA) {
      pthread_mutex_lock (&mc->mutex);
      /* If the thread is still not working */
      if (MAY_UNLIKELY (mc->working != WAITING_FOR_DATA)) {
        pthread_mutex_unlock (&mc->mutex);
        continue;
      }

      /* Setup data for the thread: handle the for from var to var+step */
      mc->working = THREAD_RUNNING;
      mc->func = compute_func;
      mc->data4for.begin = var;
      mc->data4for.end   = var + step;
      mc->data4for.data  = data;
      mc->data4for.thread_reduced_var = thread_reduced_var_tab +
        (real_nb_core++ * size_global_reduced_var);
      mc->data = &mc->data4for.data;
      mc->num_spawn_ptr = &block->num_terminated_spawn;
      mc->mark   = block->mark;
      block->num_spawn +=1;

      /* Setup frame of the thread:
         intmaxsize, prec, rnd_mode, etc... */
      /* FIXME: How to speed up this ?*/
      memcpy (&mc->may_g_ref->frame, &may_g.frame, sizeof may_g.frame);

      /* Signal to thread that some work are available */
      pthread_cond_signal(&mc->cond);
      pthread_mutex_unlock (&mc->mutex);

      /* Next one */
      var += step;
    }
  }

  /* Finish the for loop with the remainder */
  may_data4for_t data4for;
  data4for.begin = var;
  data4for.end   = end;
  data4for.data  = data;
  data4for.thread_reduced_var = thread_reduced_var_tab +
    (real_nb_core++ * size_global_reduced_var);
  (*compute_func)(&data4for);

  MAY_SPAWN_SYNC(block);

  /* Final Reduction of the reduced data of all threads into the global one */
  (*reduce_func)(real_nb_core, thread_reduced_var_tab, global_reduced_var_ptr);
}


#if defined(MAY_NEED_ATOMIC_ADD)
unsigned int may_atomic_add(unsigned int *var, unsigned int inc)
{
  pthread_mutex_lock (&may_mt_g.master_mutex);
  unsigned int ret = *value;
  *value += inc;
  pthread_mutex_unlock (&may_mt_g.master_mutex);
  return ret;
}
#endif

#endif
