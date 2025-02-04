/*
 * Some functions and definitions to ease development on GNU-Linux systems.
 * Version 1.1
 *
 * config.h is the macro settings file. Used to define behavior and variables.
 * globldef.h is the global definition file. It should be the first file included in all other source files.
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#ifndef CTHREAD_H
#define CTHREAD_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "globldef.h"
#include <pthread.h>

//If thread is joinable, wait thread join, returns true.
//If thread is not joinable or an error occurs, it returns false.
//argument pp_value may be NULL.
extern bool cthread_wait(pthread_t cthread, void **pp_value);

//Force stop current thread.
//If thread is joinable, detach thread before stopping.
extern void cthread_stop(pthread_t cthread);

#endif //CTHREAD_H

