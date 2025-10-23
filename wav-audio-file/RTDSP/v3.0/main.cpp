/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 3.0
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#include "globldef.h"
#include "filedef.h"
#include "delay.h"
#include "cstrdef.h"
#include "strdef.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "shared.hpp"

#include "AudioRTDSP.hpp"
#include "AudioRTDSP_i16.hpp"
#include "AudioRTDSP_i24.hpp"

#define PB_I16 1
#define PB_I24 2

AudioRTDSP *p_audio = NULL;
audiortdsp_pb_params_t pb_params;

int h_filein = -1;

extern void app_deinit(void);

extern bool filein_ext_check(void);
extern bool filein_open(void);
extern void filein_close(void);

extern int filein_get_params(void);
extern bool compare_signature(const char *auth, const uint8_t *buf);

int main(int argc, char **argv)
{
	int n_ret = 0;

	if(argc < 3)
	{
		std::cout << "Error: missing arguments\nThis executable requires 2 arguments: <output audio device id> <input audio file directory>\nThey must be in that order\n";
		return 1;
	}

	pb_params.audio_dev_desc = argv[1];
	pb_params.filein_dir = argv[2];

	if(!filein_ext_check())
	{
		std::cout << "Error: bad file extension\n";
		goto _l_main_error;
	}

	if(!filein_open())
	{
		std::cout << "Error: could not open file\n";
		goto _l_main_error;
	}

	n_ret = filein_get_params();

	if(n_ret < 0) goto _l_main_error;

	switch(n_ret)
	{
		case PB_I16:
			p_audio = new AudioRTDSP_i16(&pb_params);
			break;

		case PB_I24:
			p_audio = new AudioRTDSP_i24(&pb_params);
			break;
	}

	if(p_audio == NULL)
	{
		std::cout << "Error: audio object instance failed.\n";
		goto _l_main_error;
	}

	if(!p_audio->initialize())
	{
		std::cout << p_audio->getLastErrorMessage() << std::endl;
		goto _l_main_error;
	}

	if(!p_audio->runPlayback())
	{
		std::cout << p_audio->getLastErrorMessage() << std::endl;
		goto _l_main_error;
	}

	app_deinit();
	return 0;

_l_main_error:
	app_deinit();
	return 1;
}

void app_deinit(void)
{
	if(p_audio != NULL)
	{
		delete p_audio;
		p_audio = NULL;
	}

	filein_close();
	return;
}

void __attribute__((__noreturn__)) app_exit(int exit_code, const char *exit_msg)
{
	app_deinit();

	if(exit_msg != NULL) std::cout << "PROCESS EXIT CALLED\n" << exit_msg << std::endl;

	exit(exit_code);

	while(true) delay_ms(10);
}

bool filein_ext_check(void)
{
	size_t len = 0u;
	std::string usrinput = "";

	if(pb_params.filein_dir == NULL) return false;

	snprintf(textbuf, TEXTBUF_SIZE_CHARS, "%s", pb_params.filein_dir);

	cstr_tolower(textbuf, TEXTBUF_SIZE_CHARS);

	len = (size_t) cstr_getlength(textbuf);

	if(len >= 5u)
		if(cstr_compare(".wav", &textbuf[len - 4u])) 
			return true;

	while(true)
	{
		std::cout << "WARNING: chosen file does not have a \".wav\" extension.\nFile format might be incompatible with this application\nDo you wish to continue? (yes/no): ";
		usrinput = "";
		std::cin >> usrinput;
		usrinput = str_tolower(usrinput);

		if(!usrinput.compare("yes")) return true;
		if(!usrinput.compare("no")) return false;

		std::cout << "Error: invalid command entered\n\n";
	}

	return false;
}

bool filein_open(void)
{
	filein_close(); /*Clear previous file handle*/

	if(pb_params.filein_dir == NULL) return false;

	h_filein = open(pb_params.filein_dir, O_RDONLY);

	return (h_filein >= 0);
}

void filein_close(void)
{
	if(h_filein < 0) return;

	close(h_filein);
	h_filein = -1;
	return;
}

int filein_get_params(void)
{
	const size_t BUFFER_SIZE = 4096u;
	size_t buffer_index = 0u;

	uint8_t *p_headerinfo = NULL;

	uint32_t u32 = 0u;
	uint16_t u16 = 0u;

	uint16_t bit_depth = 0u;

	p_headerinfo = (uint8_t*) malloc(BUFFER_SIZE);
	if(p_headerinfo == NULL)
	{
		std::cout << "filein_get_params: Error: memory allocate failed\n";
		goto _l_filein_get_params_error;
	}

	memset(p_headerinfo, 0, BUFFER_SIZE);

	__LSEEK(h_filein, 0, SEEK_SET);
	read(h_filein, p_headerinfo, BUFFER_SIZE);
	filein_close();

	if(!compare_signature("RIFF", p_headerinfo))
	{
		std::cout << "filein_get_params: Error: file format is not supported\n";
		goto _l_filein_get_params_error;
	}

	if(!compare_signature("WAVE", (const uint8_t*) (((size_t) p_headerinfo) + 8u)))
	{
		std::cout << "filein_get_params: Error: file format is not supported\n";
		goto _l_filein_get_params_error;
	}

	buffer_index = 12u;

	while(true)
	{
		if(buffer_index > (BUFFER_SIZE - 8u))
		{
			std::cout << "filein_get_params: Error: broken header (missing subchunk \"fmt \")\nFile is probably corrupted\n";
			goto _l_filein_get_params_error;
		}

		if(compare_signature("fmt ", (const uint8_t*) (((size_t) p_headerinfo) + buffer_index))) break;

		u32 = *((uint32_t*) (((size_t) p_headerinfo) + buffer_index + 4u));
		buffer_index += (size_t) (u32 + 8u);
	}

	if(buffer_index > (BUFFER_SIZE - 24u))
	{
		std::cout << "filein_get_params: Error: broken header (error on subchunk \"fmt \")\nFile is probably corrupted\n";
		goto _l_filein_get_params_error;
	}

	u16 = *((uint16_t*) (((size_t) p_headerinfo) + buffer_index + 8u));

	if(u16 != 1u)
	{
		std::cout << "filein_get_params: Error: audio encoding format is not supported\n";
		goto _l_filein_get_params_error;
	}

	pb_params.n_channels = *((uint16_t*) (((size_t) p_headerinfo) + buffer_index + 10u));
	pb_params.sample_rate = *((uint32_t*) (((size_t) p_headerinfo) + buffer_index + 12u));
	bit_depth = *((uint16_t*) (((size_t) p_headerinfo) + buffer_index + 22u));

	u32 = *((uint32_t*) (((size_t) p_headerinfo) + buffer_index + 4u));
	buffer_index += (size_t) (u32 + 8u);

	while(true)
	{
		if(buffer_index > (BUFFER_SIZE - 8u))
		{
			std::cout << "filein_get_params: Error: broken header (missing subchunk \"data\")\nFile is probably corrupted\n";
			goto _l_filein_get_params_error;
		}

		if(compare_signature("data", (const uint8_t*) (((size_t) p_headerinfo) + buffer_index))) break;

		u32 = *((uint32_t*) (((size_t) p_headerinfo) + buffer_index + 4u));
		buffer_index += (size_t) (u32 + 8u);
	}

	u32 = *((uint32_t*) (((size_t) p_headerinfo) + buffer_index + 4u));

	pb_params.audio_data_begin = (__offset) (buffer_index + 8u);
	pb_params.audio_data_end = pb_params.audio_data_begin + ((__offset) u32);

	free(p_headerinfo);
	p_headerinfo = NULL;

	switch(bit_depth)
	{
		case 16u:
			return PB_I16;

		case 24u:
			return PB_I24;
	}

	std::cout << "filein_get_params: Error: audio format is not supported\n";

_l_filein_get_params_error:

	filein_close();
	if(p_headerinfo != NULL) free(p_headerinfo);

	return -1;
}

bool compare_signature(const char *auth, const uint8_t *buf)
{
	if(auth == NULL) return false;
	if(buf == NULL) return false;

	if(auth[0] != ((char) buf[0])) return false;
	if(auth[1] != ((char) buf[1])) return false;
	if(auth[2] != ((char) buf[2])) return false;
	if(auth[3] != ((char) buf[3])) return false;

	return true;
}

