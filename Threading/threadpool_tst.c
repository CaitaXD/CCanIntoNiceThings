#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "Threadpool.h"
#include "Monitor.h"
#include "VirtualAllocator.h"
#include "defer.h"

struct threadpool_t *pool;

void *hello(void *data [[maybe_unused]]) {
    threadpool_queue_task(pool, hello, nullptr);
    printf("Hello World!\n");
    return "Returned From Hello!";
}

int main(void) {
    pool = g_threadpool();
    for (int i = 0; i < 1; ++i) {
        auto const hello_task = threadpool_queue_task(pool, hello, nullptr);
        printf("%s\n", (char *) threadpool_wait_task(pool, hello_task));
    }

    threadpool_wait(pool);
    threadpool_shutdown(pool);
    return 0;
}
