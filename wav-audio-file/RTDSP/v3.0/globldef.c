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

#include "globldef.h"

char textbuf[TEXTBUF_SIZE_CHARS] = {'\0'};

size_t _get_closest_power2_round(size_t input)
{
	size_t _limit;
	size_t _max;
	size_t _size1;
	size_t _size2;

	if(!input) return 0u;

	if(_is_power2(input)) return input;

	_limit = _get_sizet_msb_value();
	_max = _get_sizet_max_value();

	if(input > _limit)
	{
		_size2 = _max - input;
		_size1 = input - _limit;

		if(_size1 < _size2) return _limit;
		return 0u;
	}

	_size2 = _get_closest_power2_ceil(input);
	_size1 = (_size2 >> 1);

	if((input - _size1) < (_size2 - input)) return _size1;

	return _size2;
}

size_t _get_closest_power2_floor(size_t input)
{
	size_t _limit;
	size_t _size;

	if(!input) return 0u;

	if(_is_power2(input)) return input;

	_limit = _get_sizet_msb_value();

	if(input > _limit) return _limit;

	_size = _get_closest_power2_ceil(input);
	_size = (_size >> 1);

	return _size;
}

size_t _get_closest_power2_ceil(size_t input)
{
	size_t _limit;
	size_t _size;

	if(!input) return 0u;

	if(_is_power2(input)) return input;

	_limit = _get_sizet_msb_value();

	if(input > _limit) return 0u;

	_size = 1u;

	while(true)
	{
		if(!(input/_size)) break;
		_size = (_size << 1);
	}

	return _size;
}

bool _is_power2(size_t input)
{
	size_t _limit;
	size_t _size;

	if(!input) return false;

	_limit = _get_sizet_msb_value();

	if(input == _limit) return true;
	if(input > _limit) return false;

	_size = 1u;

	while(_size < _limit)
	{
		if(_size == input) return true;
		_size = (_size << 1);
	}

	return false;
}

size_t _get_sizet_max_value(void)
{
	size_t _size;

	_size ^= _size; /*_size = 0x000...*/
	_size = ~_size; /*_size = 0xfff...*/

	return _size;
}

size_t _get_sizet_msb_value(void)
{
	size_t _size;

	_size ^= _size;		/*_size = 0x000...*/
	_size = ~_size;		/*_size = 0xfff...*/
	_size = (_size >> 1);	/*_size = 0x7ff...*/
	_size = ~_size;		/*_size = 0x800...*/

	return _size;
}

