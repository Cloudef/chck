#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef __linux__
#  include <sys/eventfd.h>
#  include <poll.h>
#endif

#undef NDEBUG
#include <assert.h>

struct item {
   int a;
   int c;
};

static void
work(struct item *item)
{
   assert(item);
   item->c /= 5;
}

CHCK_PURE static void
callback(struct item *item)
{
   assert(item);
   assert((item->a == 1 && item->c == 2) || (item->a == 2 && item->c == 1));
}

CHCK_PURE static void
destructor(struct item *item)
{
   assert(item);
   assert((item->a == 1 && item->c == 2) || (item->a == 2 && item->c == 1));
}

int main(void)
{
   /* TEST: thread pools */
   {
      struct chck_tqueue tqueue;
      assert(chck_tqueue(&tqueue, 1, 2, sizeof(struct item), work, callback, destructor));

      struct item a = { 1, 10 };
      assert(chck_tqueue_add_task(&tqueue, &a, 0));

      struct item b = { 2, 5 };
      assert(chck_tqueue_add_task(&tqueue, &b, 0));

      while (chck_tqueue_collect(&tqueue));
      chck_tqueue_release(&tqueue);
   }

   /* TEST: thread pools, no collect */
   {
      struct chck_tqueue tqueue;
      assert(chck_tqueue(&tqueue, 1, 1, sizeof(struct item), work, NULL, destructor));
      chck_tqueue_set_keep_alive(&tqueue, true);

      struct item a = { 1, 10 };
      assert(chck_tqueue_add_task(&tqueue, &a, 100));

      struct item b = { 2, 5 };
      assert(chck_tqueue_add_task(&tqueue, &b, 100));

      usleep(100);
      chck_tqueue_release(&tqueue);
   }

#ifdef __linux__
   /* TEST: fd support */
   {
      struct chck_tqueue tqueue;
      assert(chck_tqueue(&tqueue, 1, 2, sizeof(struct item), work, callback, destructor));

      int fd;
      assert((fd = eventfd(0, EFD_CLOEXEC)) >= 0);
      chck_tqueue_set_fd(&tqueue, fd);
      close(fd);

      struct item a = { 1, 10 };
      assert(chck_tqueue_add_task(&tqueue, &a, 0));

      struct item b = { 2, 5 };
      assert(chck_tqueue_add_task(&tqueue, &b, 0));

      struct pollfd fds[1] = { { .fd = chck_tqueue_get_fd(&tqueue), .events = POLLIN } };
      while (true) {
         if (poll(fds, 1, -1) <= 0)
            continue;

         for (size_t i = 0; i < 1; ++i) {
            if (fds[i].revents & POLLIN) {
               if (!chck_tqueue_collect(&tqueue))
                  goto out;
            }
         }
      }

out:
      chck_tqueue_release(&tqueue);
   }
#endif

   /* TEST: throughput on single thread */
   {
      struct chck_tqueue tqueue;
      const size_t iters = 0xFFFFF;
      assert(chck_tqueue(&tqueue, 1, iters, sizeof(struct item), work, callback, destructor));

      for (size_t i = 0; i < iters / 2; ++i) {
         struct item a = { 1, 10 };
         assert(chck_tqueue_add_task(&tqueue, &a, 0));

         struct item b = { 2, 5 };
         assert(chck_tqueue_add_task(&tqueue, &b, 0));
      }

      while (chck_tqueue_collect(&tqueue)) usleep(1000);
      chck_tqueue_release(&tqueue);
   }

   return EXIT_SUCCESS;
}
