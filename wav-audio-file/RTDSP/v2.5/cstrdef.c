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

#include "cstrdef.h"

ssize_t cstr_getlength(const char *str)
{
	size_t len = 0u;

	if(str == NULL) return -1;

	while(str[len] != '\0') len++;

	return (ssize_t) len;
}

bool cstr_compare(const char *str1, const char *str2)
{
	size_t len1 = 0u;
	size_t len2 = 0u;

	if(str1 == NULL) return false;
	if(str2 == NULL) return false;

	len1 = (size_t) cstr_getlength(str1);
	len2 = (size_t) cstr_getlength(str2);

	if(len1 != len2) return false;

	for(len1 = 0u; len1 < len2; len1++) if(str1[len1] != str2[len1]) return false;

	return true;
}

bool cstr_tolower(char *str, size_t buffer_length)
{
	size_t stop_index = 0u;
	size_t n_char = 0u;

	if(str == NULL) return false;
	if(buffer_length < 1u) return false;

	str[buffer_length - 1u] = '\0'; /*Ensure the buffer is null terminated.*/

	stop_index = cstr_getlength(str);

	for(n_char = 0u; n_char < stop_index; n_char++)
		if((str[n_char] >= 0x41) && (str[n_char] <= 0x5a)) 
			str[n_char] = (char) (((unsigned int) str[n_char]) | 0x20);

	return true;
}

bool cstr_toupper(char *str, size_t buffer_length)
{
	size_t stop_index = 0u;
	size_t n_char = 0u;

	if(str == NULL) return false;
	if(buffer_length < 1u) return false;

	str[buffer_length - 1u] = '\0'; /*Ensure the buffer is null terminated.*/

	stop_index = cstr_getlength(str);

	for(n_char = 0u; n_char < stop_index; n_char++)
		if((str[n_char] >= 0x61) && (str[n_char] <= 0x7a))
			str[n_char] = (char) (((unsigned int) str[n_char]) & 0xdf);

	return true;
}

