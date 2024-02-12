/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#define HAVE_PTHREAD_H

#ifndef __MUTEX_H__
#define __MUTEX_H__

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif /* HAVE_PTHREAD_H */
#ifdef HAVE_INITIALIZECRITICALSECTION
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif /* HAVE_WINDOWS_H */
#endif /* HAVE_INITIALIZECRITICALSECTION */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mutex {
#ifdef HAVE_PTHREAD_H
    pthread_mutex_t *mutex;
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    CRITICAL_SECTION cs;
#endif /* HAVE_PTHREAD_H */
} Mutex;

typedef struct condition {
    Mutex mutex;
    bool  borrowed_mutex;
#ifdef HAVE_PTHREAD_H
    pthread_cond_t *condition;
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    CONDITION_VARIABLE condition;
#endif
} Condition;

extern Mutex     mutex_create(void);
extern void      mutex_free(Mutex mutex);
extern void      mutex_lock(Mutex mutex);
extern int       mutex_try_lock(Mutex mutex);
extern void      mutex_unlock(Mutex mutex);
extern Condition condition_create();
extern Condition condition_create_with_borrowed_mutex(Mutex mutex);
extern void      condition_free(Condition condition);
extern void      condition_acquire(Condition);
extern int       condition_try_acquire(Condition);
extern void      condition_release(Condition);
extern void      condition_wakeup(Condition);
extern void      condition_sleep(Condition);

#ifdef __cplusplus
}
#endif

#endif /* __MUTEX_H__ */
