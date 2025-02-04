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

#define TEXTBUF_SIZE_CHARS 256U
#define TEXTBUF_SIZE_BYTES TEXTBUF_SIZE_CHARS

extern char textbuf[TEXTBUF_SIZE_CHARS];

#endif //GLOBLDEF_H

