/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 2.4
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#include "globldef.h"
#include "filedef.h"
#include "cstrdef.h"
#include "strdef.hpp"

#include <iostream>

#include "AudioBaseClass.hpp"
#include "AudioRTDSP_16bit.hpp"
#include "AudioRTDSP_24bit.hpp"

#define RTDSP_16BIT 1
#define RTDSP_24BIT 2

AudioBaseClass *p_audio = nullptr;

audio_rtdsp_params_t audio_params;

int h_file = -1;

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
		return 1;
	}

	if(!file_open())
	{
		std::cout << "Error: could not open file\n";
		return 1;
	}

	n_ret = file_get_params();

	if(n_ret < 0) return 1;

	switch(n_ret)
	{
		case RTDSP_16BIT:
			p_audio = new AudioRTDSP_16bit(&audio_params);
			break;

		case RTDSP_24BIT:
			p_audio = new AudioRTDSP_24bit(&audio_params);
			break;
	}

	if(!p_audio->initialize())
	{
		std::cout << p_audio->getLastErrorMessage() << std::endl;
		delete p_audio;
		return 1;
	}

	if(!p_audio->runDSP())
	{
		std::cout << p_audio->getLastErrorMessage() << std::endl;
		delete p_audio;
		return 1;
	}

	delete p_audio;
	return 0;
}

bool file_ext_check(void)
{
	size_t len = 0u;

	if(audio_params.filein_dir == nullptr) return false;

	len = (size_t) cstr_getlength(audio_params.filein_dir);

	if(len < 5u) return false;

	if(cstr_compare(".wav", &audio_params.filein_dir[len - 4u])) return true;
	if(cstr_compare(".WAV", &audio_params.filein_dir[len - 4u])) return true;

	return false;
}

bool file_open(void)
{
	if(audio_params.filein_dir == nullptr) return false;

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
	char *header_info = nullptr;
	std::uint16_t *pu16 = nullptr;
	std::uint32_t *pu32 = nullptr;

	size_t bytepos = 0u;

	std::uint16_t bit_depth = 0u;

	header_info = (char*) std::malloc(BUFFER_SIZE);
	if(header_info == nullptr)
	{
		std::cout << "Error: buffer allocation failed.\n";
		return -1;
	}

	__LSEEK(h_file, 0, SEEK_SET);
	read(h_file, header_info, BUFFER_SIZE);
	file_close();

	if(!compare_signature("RIFF", header_info, 0u))
	{
		std::cout << "Error: file encoding not supported\n";
		std::free(header_info);
		return -1;
	}

	if(!compare_signature("WAVE", header_info, 8u))
	{
		std::cout << "Error: file encoding not supported\n";
		std::free(header_info);
		return -1;
	}

	bytepos = 12u;

	while(!compare_signature("fmt ", header_info, bytepos))
	{
		if(bytepos >= (BUFFER_SIZE - 256u))
		{
			std::cout << "Error: subchunk \"fmt \" not found\nFile might be corrupted\n";
			std::free(header_info);
			return -1;
		}

		pu32 = (std::uint32_t*) &header_info[bytepos + 4u];
		bytepos += (size_t) (*pu32 + 8u);
	}

	pu16 = (std::uint16_t*) &header_info[bytepos + 8u];

	if(pu16[0] != 1u)
	{
		std::cout << "Error: audio format not supported\n";
		std::free(header_info);
		return -1;
	}

	audio_params.n_channels = pu16[1];

	pu32 = (std::uint32_t*) &header_info[bytepos + 12u];

	audio_params.sample_rate = *pu32;

	pu16 = (std::uint16_t*) &header_info[bytepos + 22u];

	bit_depth = *pu16;

	pu32 = (std::uint32_t*) &header_info[bytepos + 4u];
	bytepos += (size_t) (*pu32 + 8u);

	while(!compare_signature("data", header_info, bytepos))
	{
		if(bytepos >= (BUFFER_SIZE - 256u))
		{
			std::cout << "Error: subchunk \"data\" not found\nFile might be corrupted\n";
			std::free(header_info);
			return -1;
		}

		pu32 = (std::uint32_t*) &header_info[bytepos + 4u];
		bytepos += (size_t) (*pu32 + 8u);
	}

	pu32 = (std::uint32_t*) &header_info[bytepos + 4u];

	audio_params.audio_data_begin = (__offset) (bytepos + 8u);
	audio_params.audio_data_end = audio_params.audio_data_begin + ((__offset) *pu32);

	std::free(header_info);

	switch(bit_depth)
	{
		case 16u:
			return RTDSP_16BIT;

		case 24u:
			return RTDSP_24BIT;
	}

	std::cout << "Error: audio format not supported\n";
	return -1;
}

bool compare_signature(const char *auth, const char *buf, size_t offset)
{
	if(auth == nullptr) return false;
	if(buf == nullptr) return false;

	if(auth[0] != buf[offset]) return false;
	if(auth[1] != buf[offset + 1u]) return false;
	if(auth[2] != buf[offset + 2u]) return false;
	if(auth[3] != buf[offset + 3u]) return false;

	return true;
}

