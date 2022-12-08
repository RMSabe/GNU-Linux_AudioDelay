#include <cerrno>
#include <fstream>
#include <iostream>
#include <string>

#include "rtdsp_params.h"

#define DSP_16BIT2CH 0
//#define DSP_24BIT2CH 1

std::fstream audio_file;
unsigned int audio_file_size = 0;
unsigned int audio_data_begin = 0;
unsigned int audio_data_end = 0;
unsigned int sample_rate = 0;

bool compare_signature(const char *auth, const char *bytebuf, unsigned int offset)
{
	if(bytebuf[offset] != auth[0]) return false;
	if(bytebuf[offset + 1] != auth[1]) return false;
	if(bytebuf[offset + 2] != auth[2]) return false;
	if(bytebuf[offset + 3] != auth[3]) return false;

	return true;
}

int audio_file_get_params(void)
{
	bool stereo = false;
	unsigned short bit_depth = 0;

	char *header_info = (char*) malloc(4096);
	unsigned short *pushort = NULL;
	unsigned int *puint = NULL;

	audio_file.seekg(0);
	audio_file.read(header_info, 4096);
	unsigned int byte_pos = 0;

	//Error Check: Invalid Chunk ID
	if(!compare_signature("RIFF", header_info, byte_pos)) return -1;

	//Error Check: Invalid Format Signature
	if(!compare_signature("WAVE", header_info, (byte_pos + 8))) return -1;

	byte_pos += 12;

	//Fetch "fmt " Subchunk
	while(!compare_signature("fmt ", header_info, byte_pos))
	{
		if(byte_pos >= 4088) return -1; //Error: Subchunk not found

		puint = (unsigned int*) &header_info[byte_pos + 4];
		byte_pos += (puint[0] + 8);
	}

	//Error Check: Encoding Type Not Supported
	pushort = (unsigned short*) &header_info[byte_pos + 8];
	if(pushort[0] != 1) return -1;

	stereo = (pushort[1] == 2);

	puint = (unsigned int*) &header_info[byte_pos + 12];
	sample_rate = puint[0];

	pushort = (unsigned short*) &header_info[byte_pos + 20];
	bit_depth = pushort[1];

	puint = (unsigned int*) &header_info[byte_pos + 4];
	byte_pos += (puint[0] + 8);

	//Fetch "data" Subchunk
	while(!compare_signature("data", header_info, byte_pos))
	{
		if(byte_pos >= 4088) return -1; //Error: Subchunk not found

		puint = (unsigned int*) &header_info[byte_pos + 4];
		byte_pos += (puint[0] + 8);
	}

	puint = (unsigned int*) &header_info[byte_pos + 4];
	audio_data_begin = (byte_pos + 8);
	audio_data_end = (audio_data_begin + puint[0]);

	free(header_info);

	if(stereo && (bit_depth == 16)) return DSP_16BIT2CH;
	//if(stereo && (bit_depth == 24)) return DSP_24BIT2CH;
	return -1;
}

bool open_audio_file(void)
{
	audio_file.open(WAVE_FILE_DIR, std::ios_base::in);
	if(audio_file.is_open())
	{
		audio_file.seekg(0, audio_file.end);
		audio_file_size = audio_file.tellg();
		return true;
	}

	return false;
}

int main(int argc, char **argv)
{
	if(!open_audio_file())
	{
		std::cout << "Error opening audio file\nError code: " << errno << "\nTerminated\n";
		return 0;
	}

	int n_return = audio_file_get_params();
	audio_file.close();

	if(n_return < 0)
	{
		std::cout << "Error: invalid audio file parameters\n";
		return 0;
	}

	std::string cmd_line = "";
	switch(n_return)
	{
		case DSP_16BIT2CH:
			cmd_line = "./rtdsp_16bit2ch ";
			break;

		/*case DSP_24BIT2CH:
			cmd_line = "./rtdsp_24bit2ch ";
			break;*/
	}

	cmd_line += WAVE_FILE_DIR;
	cmd_line += " ";
	cmd_line += AUDIO_DEV;
	cmd_line += " ";
	cmd_line += std::to_string(audio_data_begin);
	cmd_line += " ";
	cmd_line += std::to_string(audio_data_end);
	cmd_line += " ";
	cmd_line += std::to_string(sample_rate);
	cmd_line += " ";
	cmd_line += std::to_string((int) DSP_N_DELAY);
	cmd_line += " ";
	cmd_line += std::to_string((int) DSP_N_FEEDBACK_LOOPS);
	cmd_line += " ";
	cmd_line += std::to_string((int) DSP_FEEDBACK_POL_ALTERNATE);
	cmd_line += " ";
	cmd_line += std::to_string((int) DSP_CYCLE_DIV_INC_ONE);

	system(cmd_line.c_str());

	std::cout << "Terminated\n";
	return 0;
}
