#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stddef.h>
#include <stdbool.h>

#include "ArrayBuffer.h"
#include "Array.h"
#include "Allocator.h"
#include "pthread.h"
#include "defer.h"
#include "Monitor.h"

#ifndef unreachable
#   define unreachable() __builtin_unreachable()
#endif


#ifndef THREADPOOL_API
#  define THREADPOOL_API static inline
#endif

enum task_status {
    TASK_STATUS_CREATED,
    TASK_STATUS_QUEUED,
    TASK_STATUS_RUNNING,
    TASK_STATUS_COMPLETED,
    TASK_STATUS_FAILED
};

struct task_t {
    void * (*action)(void *data);

    void *result;
    void *data;
    enum task_status status;
};

DEFINE_ARRAY_BUFFER(work_queue_t, struct task_t);

DEFINE_ARRAY(threads_t, pthread_t);

struct threadpool_t {
    struct work_queue_t *work_queue;

    struct threads_t *threads;

    struct monitor_t monitor;
    bool shutdown;
};

#include <stdatomic.h>

static struct threadpool_t *_g_threadpool = nullptr;

THREADPOOL_API struct threadpool_t *g_threadpool();

THREADPOOL_API struct threadpool_t *threadpool_new(size_t thread_count, struct allocator_t *allocator);

THREADPOOL_API struct task_t *threadpool_queue_task(struct threadpool_t *pool, void *(*action)(void *data), void *data);

THREADPOOL_API void threadpool_wait(struct threadpool_t *pool);

THREADPOOL_API void threadpool_shutdown(struct threadpool_t *pool);

THREADPOOL_API void *threadpool_wait_task(struct threadpool_t *pool, const struct task_t *task);

THREADPOOL_API void *task_busy_wait(const struct task_t *task);

THREADPOOL_API int threadpool_processor_count();

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

int threadpool_processor_count() {
    #ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
    #else
    return sysconf(_SC_NPROCESSORS_ONLN);
    #endif
}

static void *threadpool_worker(void *data) {
    struct threadpool_t *pool = data;

    while (!pool->shutdown) {
        monitor_lock(&pool->monitor) {
            auto volatile const task = array_buffer_pop(&pool->work_queue);
            if (pool->work_queue->length == 0) {
                monitor_notify_all(&pool->monitor);
            }
            if (task) {
                monitor_break {
                    task->status = TASK_STATUS_RUNNING;
                    task->result = task->action(task->data);
                    task->status = TASK_STATUS_COMPLETED;
                    monitor_lock(&pool->monitor) {
                        monitor_notify_all(&pool->monitor);
                    }
                }
            }
        }
    }
    return nullptr;
}

struct threadpool_t *threadpool_new(const size_t thread_count, struct allocator_t *allocator) {
    struct threadpool_t *pool = alloc(allocator, sizeof(struct threadpool_t));
    *pool = (struct threadpool_t){
        .shutdown = false,
        .work_queue = (void *) array_buffer_new(struct task_t, 4, allocator),
        .threads = (void *) array_new(pthread_t, thread_count, allocator),
        .monitor = monitor_new(),
    };
    for (size_t i = 0; i < thread_count; ++i) {
        pthread_t *id = pool->threads->data + i;
        pthread_create(id, NULL, threadpool_worker, pool);
    }
    return pool;
}

struct threadpool_t *g_threadpool() {
    if (_g_threadpool != nullptr) return _g_threadpool;
    _g_threadpool = threadpool_new(threadpool_processor_count(), &default_allocator);
    return _g_threadpool;
}

void threadpool_wait(struct threadpool_t *pool) {
    if (pool->shutdown) return;
    monitor_lock(&pool->monitor) {
        monitor_wait(&pool->monitor, pool->work_queue->length == 0);
    }
}

void threadpool_shutdown(struct threadpool_t *pool) {
    if (pool->shutdown) return;
    pool->shutdown = true;

    for (size_t i = 0; i < pool->threads->length; ++i) {
        const pthread_t thread_id = pool->threads->data[i];
        pthread_join(thread_id, NULL);
    }

    const struct allocator_t *a = pool->work_queue->allocator;
    dealloc(a, pool->work_queue);
    pool->work_queue = nullptr;

    dealloc(a, pool->threads);
    pool->threads = nullptr;
    dealloc(a, pool);
}

struct task_t *threadpool_queue_task(struct threadpool_t *pool, void * (*action)(void *data), void *data) {
    if (pool->shutdown) return nullptr;

    monitor_lock(&pool->monitor) {
        const struct task_t task = {
            .action = action,
            .data = data,
            .status = TASK_STATUS_QUEUED
        };
        array_buffer_insert(&pool->work_queue, 0, task);
    }
    return pool->work_queue->data;
}

void *threadpool_wait_task(struct threadpool_t *pool, const struct task_t *volatile task) {
    if (task == nullptr) return nullptr;

    monitor_lock(&pool->monitor) {
        monitor_wait(&pool->monitor, task->status == TASK_STATUS_COMPLETED);
    }
    return task->result;
}

void *task_busy_wait(const struct task_t *task) {
    busy_wait(task->status == TASK_STATUS_COMPLETED) {
    }
    return task->result;
}

#endif //THREADPOOL_H
