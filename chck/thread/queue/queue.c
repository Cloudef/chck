#include "queue.h"
#include "overflow/overflow.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#if HAS_VALGRIND
#  include <valgrind/helgrind.h>
#else
#  define VALGRIND_HG_DISABLE_CHECKING(x, y) ;
#  define VALGRIND_HG_ENABLE_CHECKING(x, y) ;
#endif

static bool
creator_thread(const struct chck_tqueue *tqueue, const char *function)
{
   if (tqueue->threads.self != pthread_self()) {
      fprintf(stderr, "chck: Function '%s' should be only called from same thread where tqueue was created in.\n", function);
      abort();
      return false;
   }
   return true;
}
#define creator_thread(x) creator_thread(x, __FUNCTION__)

static void*
get_data(struct chck_tasks *tasks, size_t index)
{
   assert(tasks && tasks->buffer);

   if (index >= tasks->qsize)
      return NULL;

   return tasks->buffer + (index * tasks->msize);
}

static void*
on_thread(void *arg)
{
   assert(arg);
   struct chck_tasks *tasks = arg;

   while (true) {
      // XXX: We could use seperate lock here for dequeueing, and have own lock for insertion.
      pthread_mutex_lock(&tasks->mutex);

      if (!tasks->cancel && !tasks->tcount)
         pthread_cond_wait(&tasks->notify, &tasks->mutex);

      if (tasks->cancel)
         break;

      if (!tasks->tcount) {
         pthread_mutex_unlock(&tasks->mutex);
         continue;
      }

      assert(tasks->tcount > 0);
      bool *processed = &tasks->processed[tasks->thead];
      void *data = get_data(tasks, tasks->thead);
      void (*work)() = tasks->work;
      tasks->thead = (tasks->thead + 1) % tasks->qsize;
      tasks->tcount -= 1;

      pthread_mutex_unlock(&tasks->mutex);

      // We only may read race against these. That's okay.
      // The user should not meddle with the input outside of the callbacks.
      // And tqueue won't touch the item when worker is working on it.
      VALGRIND_HG_DISABLE_CHECKING(data, tasks->msize);

      work(data);
      *processed = true;

      VALGRIND_HG_ENABLE_CHECKING(data, tasks->msize);

      if (tasks->fd >= 0)
         write(tasks->fd, (uint64_t[]){1}, sizeof(uint64_t));
   }

   pthread_mutex_unlock(&tasks->mutex);
   return NULL;
}

static void
stop(struct chck_tqueue *tqueue)
{
   assert(tqueue);

   if (!tqueue->threads.running)
      return;

   pthread_mutex_lock(&tqueue->tasks.mutex);
   tqueue->tasks.cancel = true;
   pthread_cond_broadcast(&tqueue->tasks.notify);
   pthread_mutex_unlock(&tqueue->tasks.mutex);

   for (size_t i = 0; i < tqueue->threads.count; ++i)
      pthread_join(tqueue->threads.t[i], NULL);

   memset(tqueue->threads.t, 0, sizeof(pthread_t) * tqueue->threads.count);
   tqueue->threads.running = false;
}

static bool
start(struct chck_tqueue *tqueue)
{
   assert(tqueue);

   if (tqueue->threads.running)
      return true;

   tqueue->tasks.cancel = false;

   for (size_t i = 0; i < tqueue->threads.count; ++i) {
      if (pthread_create(&tqueue->threads.t[i], NULL, on_thread, &tqueue->tasks) != 0)
         return false;
   }

   tqueue->threads.running = true;
   return true;
}

bool
chck_tqueue_add_task(struct chck_tqueue *tqueue, void *data, useconds_t block)
{
   assert(tqueue && data);

   if (!tqueue->threads.running && !start(tqueue))
      return false;

   const size_t next = (tqueue->tasks.tail + 1) % tqueue->tasks.qsize;
   assert(next < tqueue->tasks.qsize);

   bool ret = false;
   while (true) {
      pthread_mutex_lock(&tqueue->tasks.mutex);

      if (tqueue->tasks.count >= tqueue->tasks.qsize) {
         if (block) {
            pthread_mutex_unlock(&tqueue->tasks.mutex);

            if (tqueue->threads.self == pthread_self())
               chck_tqueue_collect(tqueue);

            usleep(block);
            continue;
         }
         break;
      }

      if (tqueue->tasks.cancel)
         break;

      void *ptr = get_data(&tqueue->tasks, tqueue->tasks.tail);
      memcpy(ptr, data, tqueue->tasks.msize);
      tqueue->tasks.processed[tqueue->tasks.tail] = false;
      tqueue->tasks.tail = next;
      tqueue->tasks.count += 1;
      tqueue->tasks.tcount += 1;

      pthread_cond_signal(&tqueue->tasks.notify);
      ret = true;
      break;
   }

   pthread_mutex_unlock(&tqueue->tasks.mutex);
   return ret;
}

size_t
chck_tqueue_collect(struct chck_tqueue *tqueue)
{
   assert(tqueue);

   // For simplicity, we only allow collection on creator thread.
   // Collecting anywhere else does not make sense anyways.
   if (!tqueue || !creator_thread(tqueue))
      return 0;

   if (tqueue->tasks.fd >= 0) {
      char buf[sizeof(uint64_t)];
      read(tqueue->tasks.fd, buf, sizeof(buf));
   }

   pthread_mutex_lock(&tqueue->tasks.mutex);
   const size_t head = tqueue->tasks.head;
   const size_t count = tqueue->tasks.count;
   pthread_mutex_unlock(&tqueue->tasks.mutex);

   // Read races again fine here.
   // We can't enter inside until the thread is done with the item.
   size_t processed = 0;
   for (size_t i = head, x = 0; x < count; i = (i + 1) % tqueue->tasks.qsize, ++x) {
      if (!tqueue->tasks.processed[i])
         continue;

      void *data = get_data(&tqueue->tasks, i);
      VALGRIND_HG_DISABLE_CHECKING(data, tqueue->tasks.msize);

      if (tqueue->tasks.callback)
         tqueue->tasks.callback(data);

      if (tqueue->tasks.destructor)
         tqueue->tasks.destructor(data);

      memset(data, 0, tqueue->tasks.msize);
      VALGRIND_HG_ENABLE_CHECKING(data, tqueue->tasks.msize);

      tqueue->tasks.processed[i] = false;
      ++processed;
   }

   // We need to lock here however
   pthread_mutex_lock(&tqueue->tasks.mutex);
   assert(tqueue->tasks.count >= processed);
   tqueue->tasks.head = (tqueue->tasks.head + processed) % tqueue->tasks.qsize;
   tqueue->tasks.count -= processed;
   const size_t rcount = tqueue->tasks.count;
   pthread_mutex_unlock(&tqueue->tasks.mutex);

   if (!rcount)
      stop(tqueue);

   return rcount;
}

void
chck_tqueue_release(struct chck_tqueue *tqueue)
{
   // Allowed only on creator thread.
   if (!tqueue || (tqueue->threads.self && !creator_thread(tqueue)))
      return;

   stop(tqueue);
   pthread_mutex_destroy(&tqueue->tasks.mutex);
   pthread_cond_destroy(&tqueue->tasks.notify);

   if (tqueue->tasks.destructor) {
      for (size_t i = tqueue->tasks.head, x = 0; x < tqueue->tasks.count; i = (i + 1) % tqueue->tasks.qsize, ++x)
         tqueue->tasks.destructor(get_data(&tqueue->tasks, i));
   }

   if (tqueue->tasks.fd >= 0)
      close(tqueue->tasks.fd);

   free(tqueue->tasks.processed);
   free(tqueue->tasks.buffer);
   free(tqueue->threads.t);
   memset(tqueue, 0, sizeof(struct chck_tqueue));
}

void
chck_tqueue_set_fd(struct chck_tqueue *tqueue, int fd)
{
   assert(tqueue);

   // Allowed only on creator thread.
   if (!tqueue || !creator_thread(tqueue))
      return;

   if (tqueue->tasks.fd >= 0)
      close(tqueue->tasks.fd);

   tqueue->tasks.fd = dup(fd);
}

int
chck_tqueue_get_fd(struct chck_tqueue *tqueue)
{
   assert(tqueue);

   // Allowed only on creator thread.
   if (!tqueue || !creator_thread(tqueue))
      return -1;

   return tqueue->tasks.fd;
}

bool
chck_tqueue(struct chck_tqueue *tqueue, size_t nthreads, size_t qsize, size_t msize, void (*work)(), void (*callback)(), void (*destructor)())
{
   assert(tqueue && work && msize > 0);
   memset(tqueue, 0, sizeof(struct chck_tqueue));
   tqueue->tasks.fd = -1;

   if (!msize || !work)
      return false;

   if (!(tqueue->tasks.buffer = chck_calloc_of(qsize, msize)) ||
       !(tqueue->tasks.processed = chck_calloc_of(qsize, sizeof(bool))))
      return false;

   // We allow racy reads on this array.
   // However we don't allow racy writes.
   // Unfortunately some helgrind macros are unimplemented that would allow turning this off just for reads.
   VALGRIND_HG_DISABLE_CHECKING(tqueue->tasks.processed, qsize);

   if (pthread_mutex_init(&tqueue->tasks.mutex, NULL) != 0 ||
       pthread_cond_init(&tqueue->tasks.notify, NULL) != 0)
      goto fail;

   if (!(tqueue->threads.t = chck_calloc_of(nthreads, sizeof(pthread_t))))
      goto fail;

   tqueue->threads.self = pthread_self();
   tqueue->threads.count = nthreads;
   tqueue->tasks.msize = msize;
   tqueue->tasks.qsize = qsize;
   tqueue->tasks.work = work;
   tqueue->tasks.callback = callback;
   tqueue->tasks.destructor = destructor;
   return true;

fail:
   chck_tqueue_release(tqueue);
   return false;
}
