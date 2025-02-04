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

#include "cthread.h"

bool cthread_wait(pthread_t cthread, void **pp_value)
{
	pthread_attr_t cthread_attr;
	int cthread_detach_state = 0;

	if(pthread_getattr_np(cthread, &cthread_attr)) return false;
	if(pthread_attr_getdetachstate(&cthread_attr, &cthread_detach_state)) return false;

	if(cthread_detach_state != PTHREAD_CREATE_JOINABLE) return false;

	pthread_join(cthread, pp_value);
	return true;
}

void cthread_stop(pthread_t cthread)
{
	pthread_attr_t cthread_attr;
	int cthread_detach_state = 0;

	pthread_getattr_np(cthread, &cthread_attr);
	pthread_attr_getdetachstate(&cthread_attr, &cthread_detach_state);

	if(cthread_detach_state == PTHREAD_CREATE_JOINABLE) pthread_detach(cthread);

	pthread_cancel(cthread);
	return;
}

