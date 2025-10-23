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

#include "delay.h"

void delay_ms(clock_t time_ms)
{
	time_ms *= (clock_t) 1024;
	delay_us(time_ms);
	return;
}

void delay_us(clock_t time_us)
{
	volatile clock_t start_time = 0;

	start_time = clock();
	while(clock() < (start_time + time_us));

	return;
}

