/*
 * Some functions and definitions to ease development on GNU-Linux systems.
 * Version 1.0
 *
 * config.h is the macro settings file. Used to define behavior and variables.
 * globldef.h is the global definition file. It should be the first file included in all other source files.
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#ifndef DELAY_H
#define DELAY_H

#include "globldef.h"
#include <time.h>

extern void delay_ms(clock_t time_ms);
extern void delay_us(clock_t time_us);

#endif //DELAY_H

