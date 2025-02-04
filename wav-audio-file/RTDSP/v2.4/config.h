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

#ifndef CONFIG_H
#define CONFIG_H

//__FILE64: File Size and Position Definition Macro
//If defined, file size and position will be 64 bits. off64_t and lseek64() will be used.
//Else, file size and position will be long sized. off_t and lseek() will be used.

#define __FILE64

#endif //CONFIG_H

