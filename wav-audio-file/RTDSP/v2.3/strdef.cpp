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

#include "strdef.hpp"

std::string str_tolower(std::string input)
{
	std::string output = "";
	size_t input_len = 0u;
	size_t n_char = 0u;

	input_len = (size_t) input.length();

	for(n_char = 0u; n_char < input_len; n_char++)
	{
		if((input[n_char] >= 0x41) && (input[n_char] <= 0x5a)) output += (char) (((unsigned int) input[n_char]) | 0x20);
		else output += input[n_char];
	}

	return output;
}

std::string str_toupper(std::string input)
{
	std::string output = "";
	size_t input_len = 0u;
	size_t n_char = 0u;

	input_len = (size_t) input.length();

	for(n_char = 0u; n_char < input_len; n_char++)
	{
		if((input[n_char] >= 0x61) && (input[n_char] <= 0x7a)) output += (char) (((unsigned int) input[n_char]) & 0xdf);
		else output += input[n_char];
	}

	return output;
}

