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

#ifndef GLOBLDEF_H
#define GLOBLDEF_H

#include "config.h"

#ifdef __FILE64
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#else
#ifdef _LARGEFILE64_SOURCE
#undef _LARGEFILE64_SOURCE
#endif
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define TEXTBUF_SIZE_BYTES TEXTBUF_SIZE_CHARS

extern char textbuf[];

/*
 * These functions are meant to be used mostly with buffer allocation sizes.
 * They will return the closest power of 2 value from the input value.
 * ..._round() will round the value to the closest power of 2
 * ..._floor() will return the closest power of 2 below input
 * ..._ceil() will return the closest power of 2 above input
 * if input is 0 or if function fails, it will return 0
 */

extern size_t _get_closest_power2_round(size_t input);
extern size_t _get_closest_power2_floor(size_t input);
extern size_t _get_closest_power2_ceil(size_t input);

/*This function returns true if input is a power of 2, false otherwise*/

extern bool _is_power2(size_t input);

/*
 * These functions return the maximum possible value of size_t type and the most significant bit value of size_t type respectivelly.
 * size_t is the same as unsigned long.
 * 
 * These functions are so minimal they might become inline functions in future versions...
 */

extern size_t _get_sizet_max_value(void);
extern size_t _get_sizet_msb_value(void);

#endif /*GLOBLDEF_H*/

