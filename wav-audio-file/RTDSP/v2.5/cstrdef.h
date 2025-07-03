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

#ifndef CSTRDEF_H
#define CSTRDEF_H

#include "globldef.h"

extern ssize_t cstr_getlength(const char *str);
extern bool cstr_compare(const char *str1, const char *str2);

extern bool cstr_tolower(char *str, size_t buffer_length);
extern bool cstr_toupper(char *str, size_t buffer_length);

#endif /*CSTRDEF_H*/

