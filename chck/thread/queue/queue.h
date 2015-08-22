#ifndef __chck_dispatch_h__
#define __chck_dispatch_h__

#include <chck/macros.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

struct chck_tqueue {
   struct chck_tasks {
      uint8_t *buffer;
      bool *processed;
      void (*work)();
      void (*callback)();
      void (*destructor)();
      size_t msize;
      size_t qsize;
      size_t head, thead, tail, count, tcount;
      pthread_mutex_t mutex;
      pthread_cond_t notify;
      int fd;
      bool cancel;
   } tasks;

   struct {
      pthread_t *t;
      pthread_t self;
      size_t count;
      bool running;
      bool keep_alive;
   } threads;
};

CHCK_NONULL bool chck_tqueue_add_task(struct chck_tqueue *tqueue, void *data, useconds_t block);
CHCK_NONULL size_t chck_tqueue_collect(struct chck_tqueue *tqueue);
CHCK_NONULL void chck_tqueue_set_fd(struct chck_tqueue *tqueue, int fd);
CHCK_PURE CHCK_NONULL int chck_tqueue_get_fd(struct chck_tqueue *tqueue);
CHCK_NONULL void chck_tqueue_set_keep_alive(struct chck_tqueue *tqueue, bool keep_alive);
CHCK_PURE CHCK_NONULL bool chck_tqueue_get_keep_alive(struct chck_tqueue *tqueue);
void chck_tqueue_release(struct chck_tqueue *tqueue);
CHCK_NONULLV(1, 5) bool chck_tqueue(struct chck_tqueue *tqueue, size_t nthreads, size_t qsize, size_t msize, void (*work)(), void (*callback)(), void (*destructor)());

#endif /* __chck_dispatch_h__ */
