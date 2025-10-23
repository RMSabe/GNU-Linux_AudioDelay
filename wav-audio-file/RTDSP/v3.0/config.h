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

#ifndef CONFIG_H
#define CONFIG_H

/*
 * __FILE64: File Size and Position Definition Macro
 * If defined, file size and position will be 64 bits. off64_t and lseek64() will be used.
 * Else, file size and position will be long sized. off_t and lseek() will be used.
 */

#define __FILE64

/*
 * TEXTBUF_SIZE_CHARS
 * Define the size (in number of characters) of the global char[] textbuf buffer.
 */

#define TEXTBUF_SIZE_CHARS 1024U

#endif /*CONFIG_H*/

