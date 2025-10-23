/*
 * Some functions and definitions to ease development on GNU-Linux systems.
 * Version 1.3
 *
 * config.h is the macro settings file. Used to define behavior and variables.
 * globldef.h is the global definition file. It should be the first file included in all other source files.
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 *
 * GitHub Repository: https://github.com/RMSabe/GNU-Linux_Lib
 */

#ifndef CPPTHREAD_HPP
#define CPPTHREAD_HPP

#include "globldef.h"
#include <thread>

/*
 * If thread is joinable, wait thread join, returns true.
 * If thread is not joinable or an error occurs, it returns false.
 */

extern bool cppthread_wait(std::thread *p_cppthread);

/*
 * Force stop current thread.
 * If thread is joinable, detach thread before stopping.
 */

extern void cppthread_stop(std::thread *p_cppthread);

#endif /*CPPTHREAD_HPP*/

