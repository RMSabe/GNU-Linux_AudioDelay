/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 2.5
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#include "globldef.h"
#include "filedef.h"
#include "delay.h"
#include "cstrdef.h"
#include "strdef.hpp"

#include <iostream>

#include "shared.hpp"

#include "AudioBaseClass.hpp"
#include "AudioRTDSP_i16.hpp"
#include "AudioRTDSP_i24.hpp"

#define RTDSP_I16 1
#define RTDSP_I24 2

AudioBaseClass *p_audio = NULL;

audio_rtdsp_params_t audio_params;

int h_file = -1;

std::string usrinput = "";

extern void app_deinit(void);

extern bool file_ext_check(void);
extern bool file_open(void);
extern void file_close(void);
extern int file_get_params(void);
extern bool compare_signature(const char *auth, const char *buf, size_t offset);

int main(int argc, char **argv)
{
	int n_ret = 0;

	if(argc < 3)
	{
		std::cout << "Error: missing arguments\nThis executable requires 2 arguments: <output audio device id> <input audio file directory>\nThey must be in that order\n";
		return 1;
	}

	audio_params.audio_dev_desc = argv[1];
	audio_params.filein_dir = argv[2];

	if(!file_ext_check())
	{
		std::cout << "Error: file format is not supported\n";
		goto _l_main_error;
	}

	if(!file_open())
	{
		std::cout << "Error: could not open file\n";
		goto _l_main_error;
	}

	n_ret = file_get_params();

	if(n_ret < 0) goto _l_main_error;

	switch(n_ret)
	{
		case RTDSP_I16:
			p_audio = new AudioRTDSP_i16(&audio_params);
			break;

		case RTDSP_I24:
			p_audio = new AudioRTDSP_i24(&audio_params);
			break;
	}

	if(!p_audio->initialize())
	{
		std::cout << p_audio->getLastErrorMessage() << std::endl;
		goto _l_main_error;
	}

	if(!p_audio->runDSP())
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

	return;
}

void __attribute__((__noreturn__)) app_exit(int exit_code, const char *exit_msg)
{
	if(exit_msg != NULL) std::cout << "PROCESS EXIT CALLED\n" << exit_msg << std::endl;

	app_deinit();
	exit(exit_code);

	while(true) delay_ms(1);
}

bool file_ext_check(void)
{
	size_t len = 0u;

	if(audio_params.filein_dir == NULL) return false;

	snprintf(textbuf, TEXTBUF_SIZE_CHARS, "%s", audio_params.filein_dir);

	cstr_tolower(textbuf, TEXTBUF_SIZE_CHARS);

	len = (size_t) cstr_getlength(textbuf);

	if(cstr_compare(".wav", &textbuf[len - 4u])) return true;

	while(true)
	{
		std::cout << "WARNING: File extension is not .wav\nFile format might be incompatible\nContinue? (YES/NO): ";
		usrinput = "";
		std::cin >> usrinput;
		usrinput = str_tolower(usrinput);

		if(!usrinput.compare("yes")) return true;
		if(!usrinput.compare("no")) return false;

		std::cout << "Error: invalid command entered\n\n";
	}

	return false;
}

bool file_open(void)
{
	if(audio_params.filein_dir == NULL) return false;

	h_file = open(audio_params.filein_dir, O_RDONLY);

	return (h_file >= 0);
}

void file_close(void)
{
	if(h_file < 0) return;

	close(h_file);
	h_file = -1;
	return;
}

int file_get_params(void)
{
	const size_t BUFFER_SIZE = 4096u;

	uint8_t *p_headerinfo = NULL;
	uint16_t *p_u16 = NULL;
	uint32_t *p_u32 = NULL;

	size_t bytepos = 0u;

	uint16_t bit_depth = 0u;

	p_headerinfo = (uint8_t*) std::malloc(BUFFER_SIZE);
	if(p_headerinfo == NULL)
	{
		std::cout << "Error: buffer allocation failed.\n";
		goto _l_file_get_params_error;
	}

	__LSEEK(h_file, 0, SEEK_SET);
	read(h_file, p_headerinfo, BUFFER_SIZE);
	file_close();

	if(!compare_signature("RIFF", (const char*) p_headerinfo, 0u))
	{
		std::cout << "Error: invalid file signature\n";
		goto _l_file_get_params_error;
	}

	if(!compare_signature("WAVE", (const char*) p_headerinfo, 8u))
	{
		std::cout << "Error: invalid format signature\n";
		goto _l_file_get_params_error;
	}

	bytepos = 12u;

	while(true)
	{
		if(bytepos > (BUFFER_SIZE - 8u))
		{
			std::cout << "Error: subchunk \"fmt \" not found\nFile might be corrupted\n";
			goto _l_file_get_params_error;
		}

		if(compare_signature("fmt ", (const char*) p_headerinfo, bytepos)) break;

		p_u32 = (uint32_t*) &p_headerinfo[bytepos + 4u];

		if(!(*p_u32))
		{
			std::cout << "Error: subchunk \"fmt \" not found\nFile might be corrupted\n";
			goto _l_file_get_params_error;
		}

		bytepos += (size_t) (*p_u32 + 8u);
	}

	p_u16 = (uint16_t*) &p_headerinfo[bytepos + 8u];

	if(p_u16[0] != 1u)
	{
		std::cout << "Error: audio format not supported\n";
		goto _l_file_get_params_error;
	}

	audio_params.n_channels = p_u16[1];

	p_u32 = (uint32_t*) &p_headerinfo[bytepos + 12u];

	audio_params.sample_rate = *p_u32;

	p_u16 = (uint16_t*) &p_headerinfo[bytepos + 22u];

	bit_depth = *p_u16;

	p_u32 = (uint32_t*) &p_headerinfo[bytepos + 4u];
	bytepos += (size_t) (*p_u32 + 8u);

	while(true)
	{
		if(bytepos > (BUFFER_SIZE - 8u))
		{
			std::cout << "Error: subchunk \"data\" not found\nFile might be corrupted\n";
			goto _l_file_get_params_error;
		}

		if(compare_signature("data", (const char*) p_headerinfo, bytepos)) break;

		p_u32 = (uint32_t*) &p_headerinfo[bytepos + 4u];

		if(!(*p_u32))
		{
			std::cout << "Error: subchunk \"data\" not found\nFile might be corrupted\n";
			goto _l_file_get_params_error;
		}

		bytepos += (size_t) (*p_u32 + 8u);
	}

	p_u32 = (uint32_t*) &p_headerinfo[bytepos + 4u];

	audio_params.audio_data_begin = (__offset) (bytepos + 8u);
	audio_params.audio_data_end = audio_params.audio_data_begin + ((__offset) *p_u32);

	std::free(p_headerinfo);
	p_headerinfo = NULL;
	p_u16 = NULL;
	p_u32 = NULL;

	switch(bit_depth)
	{
		case 16u:
			return RTDSP_I16;

		case 24u:
			return RTDSP_I24;
	}

	std::cout << "Error: audio format not supported\n";

_l_file_get_params_error:
	if(p_headerinfo != NULL) std::free(p_headerinfo);
	return -1;
}

bool compare_signature(const char *auth, const char *buf, size_t offset)
{
	if(auth == NULL) return false;
	if(buf == NULL) return false;

	if(auth[0] != buf[offset]) return false;
	if(auth[1] != buf[offset + 1u]) return false;
	if(auth[2] != buf[offset + 2u]) return false;
	if(auth[3] != buf[offset + 3u]) return false;

	return true;
}

