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

#ifndef STRDEF_HPP
#define STRDEF_HPP

#include "globldef.h"
#include <string>

extern std::string str_tolower(std::string input);
extern std::string str_toupper(std::string input);

#endif /*STRDEF_HPP*/

