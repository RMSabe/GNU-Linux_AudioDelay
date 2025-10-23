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

#include "cppthread.hpp"

bool cppthread_wait(std::thread *p_cppthread)
{
	if(p_cppthread == nullptr) return false;

	if(!(*p_cppthread).joinable()) return false;

	(*p_cppthread).join();
	return true;
}

void cppthread_stop(std::thread *p_cppthread)
{
	if(p_cppthread == nullptr) return;

	if((*p_cppthread).joinable()) (*p_cppthread).detach();

	(*p_cppthread).~thread();
	return;
}

