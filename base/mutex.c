/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#define HAVE_PTHREAD_H

#include <errno.h>
#include <string.h>

#include <errorcode.h>
#include <log.h>
#include <mem.h>
#include <mutex.h>
#include <sv.h>

Mutex mutex_create(void)
{
    Mutex mutex;

#ifdef HAVE_PTHREAD_H
    pthread_mutexattr_t attr;
#endif /* HAVE_PTHREAD_H */

#ifdef HAVE_PTHREAD_H
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    mutex.mutex = MALLOC(pthread_mutex_t);
    if ((errno = pthread_mutex_init(mutex.mutex, &attr))) {
        fatal("Error creating mutex: %s", errorcode_to_string(errno));
    }
    pthread_mutexattr_destroy(&attr);
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    InitializeCriticalSection(&(mutex->cs));
#endif /* HAVE_PTHREAD_H */

    return mutex;
}

void mutex_free(Mutex mutex)
{
#ifdef HAVE_PTHREAD_H
    pthread_mutex_destroy(mutex.mutex);
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    DeleteCriticalSection(&(mutex->cs));
#endif /* HAVE_PTHREAD_H */
}

void mutex_lock(Mutex mutex)
{
    int retval = 0;

    trace(THREAD, "Locking mutex");
#ifdef HAVE_PTHREAD_H
    errno = pthread_mutex_lock(mutex.mutex);
    if (errno) {
        retval = -1;
    }
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    EnterCriticalSection(&(mutex->cs));
#endif /* HAVE_PTHREAD_H */
    if (retval) {
        fatal("Error locking mutex: %s", errorcode_to_string(errno));
    }
    trace(THREAD, "Mutex locked");
}

/**
 * @return 0 if the mutex was successfully locked
 *         1 if the mutex was owned by another thread
 *         -1 If an error occurred.
 */
int mutex_try_lock(Mutex mutex)
{
    int retval;

    trace(THREAD, "Trying to lock mutex");
#ifdef HAVE_PTHREAD_H
    errno = pthread_mutex_trylock(mutex.mutex);
    switch (errno) {
    case 0:
        retval = 0;
        break;
    case EBUSY:
        retval = 1;
        break;
    default:
        retval = -1;
        break;
    }
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    retval = (!TryEnterCriticalSection(&mutex->cs)) ? 0 : 1;
#endif /* HAVE_PTHREAD_H */
    trace(THREAD, "Trylock mutex: %s", (retval) ? "Fail" : "Success");
    return retval;
}

void mutex_unlock(Mutex mutex)
{
    int retval = 0;

    trace(THREAD, "Unlocking mutex");
#ifdef HAVE_PTHREAD_H
    errno = pthread_mutex_unlock(mutex.mutex);
    if (errno) {
        retval = -1;
    }
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    EnterCriticalSection(&mutex->cs);
#endif /* HAVE_PTHREAD_H */
    if (retval && errno != EPERM) {
        panic("Error unlocking mutex: %s", errorcode_to_string(errno));
    }
    trace(THREAD, "Mutex unlocked");
}

/* ------------------------------------------------------------------------ */
/* -- C O N D I T I O N _ T ----------------------------------------------- */
/* ------------------------------------------------------------------------ */

void condition_free(Condition condition)
{
#ifdef HAVE_PTHREAD_H
    pthread_cond_destroy(condition.condition);
#endif /* HAVE_PTHREAD_H */
    if (!condition.borrowed_mutex) {
        mutex_free(condition.mutex);
    }
}

Condition condition_create()
{
    Condition condition = { 0 };
    condition.mutex = mutex_create();
    condition.borrowed_mutex = false;
#ifdef HAVE_PTHREAD_H
    condition.condition = MALLOC(pthread_cond_t);
    pthread_cond_init(condition.condition, NULL);
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    InitializeConditionVariable(&condition->condition);
#endif /* HAVE_PTHREAD_H */
    trace(THREAD, "Condition created");
    return condition;
}

Condition condition_create_with_borrowed_mutex(Mutex mutex)
{
    Condition condition = { 0 };
    condition.mutex = mutex;
    condition.borrowed_mutex = true;
#ifdef HAVE_PTHREAD_H
    condition.condition = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_cond_init(condition.condition, NULL);
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    InitializeConditionVariable(&condition->condition);
#endif /* HAVE_PTHREAD_H */
    trace(THREAD, "Condition created");
    return condition;
}

void condition_acquire(Condition condition)
{
    trace(THREAD, "Acquiring condition");
    mutex_lock(condition.mutex);
}

void condition_release(Condition condition)
{
    trace(THREAD, "Releasing condition");
    mutex_unlock(condition.mutex);
}

/**
 * @return 0 if the condition was successfully locked
 *         1 if the condition was owned by another thread
 *         -1 If an error occurred.
 */
int condition_try_acquire(Condition condition)
{
    trace(THREAD, "Trying to acquire condition");
    return mutex_try_lock(condition.mutex);
}

void condition_wakeup(Condition condition)
{
    int retval = 0;

    trace(THREAD, "Waking up condition");
#ifdef HAVE_PTHREAD_H
    errno = pthread_cond_signal(condition.condition);
    if (errno) {
        retval = -1;
    }
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    WakeConditionVariable(&condition->condition);
#endif /* HAVE_PTHREAD_H */
    mutex_unlock(condition.mutex);
    if (retval) {
        fatal("Error waking condition: %s", errorcode_to_string(errno));
    }
    trace(THREAD, "Condition woken up");
}

void condition_broadcast(Condition condition)
{
    int retval = 0;

    trace(THREAD, "Waking up condition");
#ifdef HAVE_PTHREAD_H
    errno = pthread_cond_broadcast(condition.condition);
    if (errno) {
        retval = -1;
    }
#elif defined(HAVE_INITIALIZECRITICALSECTION)
#error Provide pthread_cond_broadcast equivalent
    WakeConditionVariable(&condition->condition);
#endif /* HAVE_PTHREAD_H */
    mutex_unlock(condition.mutex);
    if (retval) {
        fatal("Error waking condition: %s", errorcode_to_string(errno));
    }
    trace(THREAD, "All threads sleeping on Condition woken up");
}

void condition_sleep(Condition condition)
{
    int retval = 0;

    trace(THREAD, "Going to sleep on condition");
#ifdef HAVE_PTHREAD_H
    errno = pthread_cond_wait(condition.condition, condition.mutex.mutex);
    if (errno) {
        retval = -1;
    }
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    SleepConditionVariableCS(&condition->condition, &condition->mutex->cs, INFINITE);
#endif /* HAVE_PTHREAD_H */
    if (retval) {
        fatal("Error sleeping on condition: %s", errorcode_to_string(errno));
    }
    trace(THREAD, "Woke up from condition");
}

/* ------------------------------------------------------------------------ */
