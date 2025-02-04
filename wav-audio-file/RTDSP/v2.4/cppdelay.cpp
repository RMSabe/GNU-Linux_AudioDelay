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

#include "cppdelay.hpp"
#include <thread>
#include <chrono>

void cppdelay_ms(unsigned long time_ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(time_ms));
	return;
}

void cppdelay_us(unsigned long time_us)
{
	std::this_thread::sleep_for(std::chrono::microseconds(time_us));
	return;
}

