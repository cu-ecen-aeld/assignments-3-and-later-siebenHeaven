#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    thread_func_args->thread_complete_success = false;

    usleep(thread_func_args->wait_to_obtain_ms);

    if (0 != pthread_mutex_lock(thread_func_args->mutex)) {
        printf("pthread_mutex_lock failed\n");
        return thread_param;
    }

    usleep(thread_func_args->wait_to_release_ms);

    if (0 != pthread_mutex_unlock(thread_func_args->mutex)) {
        printf("pthread_mutex_unlock failed\n");
        return thread_param;
    }

    thread_func_args->thread_complete_success = true;

    return thread_param;
}

/**
* Start a thread which sleeps @param wait_to_obtain_ms number of milliseconds, then obtains the
* mutex in @param mutex, then holds for @param wait_to_release_ms milliseconds, then releases.
* The start_thread_obtaining_mutex function should only start the thread and should not block
* for the thread to complete.
* The start_thread_obtaining_mutex function should use dynamic memory allocation for thread_data
* structure passed into the thread.  The number of threads active should be limited only by the
* amount of available memory.
* The thread started should return a pointer to the thread_data structure when it exits, which can be used
* to free memory as well as to check thread_complete_success for successful exit.
* If a thread was started succesfully @param thread should be filled with the pthread_create thread ID
* coresponding to the thread which was started.
* @return true if the thread could be started, false if a failure occurred.
*/
bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data* thread_param = malloc(sizeof(*thread_param));
    if (thread_param == NULL) {
        printf("malloc failed\n");
        return false;
    }

    thread_param->mutex = mutex;
    thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_param->wait_to_release_ms = wait_to_release_ms;

    if (0 != pthread_create(thread, NULL, threadfunc, (void*)thread_param)) {
        printf("pthread_create failed\n");
        return false;
    }

    return true;
}

