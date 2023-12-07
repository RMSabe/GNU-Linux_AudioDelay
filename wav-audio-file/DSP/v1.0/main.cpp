/*
 * Audio Delay File Generation
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#include <fstream>
#include <iostream>
#include <string>

#define BYTEBUF_SIZE 2048U

#define DSP_16BIT1CH 1
#define DSP_16BIT2CH 2
#define DSP_24BIT1CH 3
#define DSP_24BIT2CH 4

std::fstream filein;

char *filein_dir = NULL;
char *fileout_dir = NULL;

unsigned long data_begin = 0ul;
unsigned long data_end = 0ul;

unsigned int sample_rate = 0u;
unsigned short bit_depth = 0u;
unsigned short n_channels = 0u;

int n_delay = 0;
int n_feedback = 0;
int feedback_pol_alt = 0;
int cycle_div_inc_one = 0;

bool filein_dir_check(void);
bool filein_open(void);

int file_get_params(void);
bool compare_signature(const char *auth, const char *bytebuf, unsigned int offset);

int main(int argc, char **argv)
{
	if(argc < 7)
	{
		std::cout << "Error: missing arguments\n";
		std::cout << "This executable requires 6 arguments: ";
		std::cout << "<input audio file directory> <output audio file directory> <delay time> <feedback loops> <feedback pol alt> <cycle div inc one>\n";
		std::cout << "They must be in this order\nWARNING: output file will be overwritten if it already exists\n";
		return 0;
	}

	filein_dir = argv[1];
	fileout_dir = argv[2];

	try
	{
		n_delay = std::stoi(argv[3]);
		n_feedback = std::stoi(argv[4]);
		feedback_pol_alt = std::stoi(argv[5]);
		cycle_div_inc_one = std::stoi(argv[6]);
	}
	catch(...)
	{
		std::cout << "Error: one or more parameters have invalid values\n";
		return 0;
	}

	if(n_delay < 0 || n_feedback < 0 || feedback_pol_alt < 0 || cycle_div_inc_one < 0)
	{
		std::cout << "Error: negative values are invalid\n";
		return 0;
	}

	if(feedback_pol_alt > 1 || cycle_div_inc_one > 1)
	{
		std::cout << "Error: \"feedback pol alt\" and \"cycle div inc one\" may only accept two values: 1 or 0\n";
		return 0;
	}

	if(!filein_dir_check())
	{
		std::cout << "Error: file format is not supported\n";
		return 0;
	}

	if(!filein_open())
	{
		std::cout << "Error: could not open file\n";
		return 0;
	}

	int n_ret = file_get_params();
	if(n_ret < 0)
	{
		std::cout << "Error: audio format is not supported\n";
		return 0;
	}

	std::string cmd = "";

	switch(n_ret)
	{
		case DSP_16BIT1CH:
			cmd = "./dsp_16bit1ch.elf ";
			break;

		case DSP_16BIT2CH:
			cmd = "./dsp_16bit2ch.elf ";
			break;

		case DSP_24BIT1CH:
			cmd = "./dsp_24bit1ch.elf ";
			break;

		case DSP_24BIT2CH:
			cmd = "./dsp_24bit2ch.elf ";
			break;
	}

	cmd += filein_dir;
	cmd += ' ';
	cmd += std::to_string(data_begin);
	cmd += ' ';
	cmd += std::to_string(data_end);
	cmd += ' ';
	cmd += std::to_string(n_delay);
	cmd += ' ';
	cmd += std::to_string(n_feedback);
	cmd += ' ';
	cmd += std::to_string(feedback_pol_alt);
	cmd += ' ';
	cmd += std::to_string(cycle_div_inc_one);

	system(cmd.c_str());

	cmd = "./conv.elf ";

	cmd += fileout_dir;
	cmd += ' ';
	cmd += std::to_string(n_channels);
	cmd += ' ';
	cmd += std::to_string(bit_depth);
	cmd += ' ';
	cmd += std::to_string(sample_rate);

	system(cmd.c_str());

	return 0;
}

bool filein_dir_check(void)
{
	unsigned int len = 0u;
	while(filein_dir[len] != '\0') len++;

	if(len < 5u) return false;

	if(compare_signature(".wav", filein_dir, (len - 4u))) return true;
	if(compare_signature(".WAV", filein_dir, (len - 4u))) return true;

	return false;
}

bool filein_open(void)
{
	filein.open(filein_dir, std::ios_base::in);
	return filein.is_open();
}

int file_get_params(void)
{
	char *header_info = (char*) malloc(BYTEBUF_SIZE);
	unsigned short *pu16 = NULL;
	unsigned int *pu32 = NULL;
	unsigned int bytepos = 0u;

	filein.seekg(0);
	filein.read(header_info, BYTEBUF_SIZE);

	//Error: invalid chunk signature
	if(!compare_signature("RIFF", header_info, bytepos))
	{
		filein.close();
		free(header_info);
		return -1;
	}

	bytepos = 8u;

	//Error: invalid format signature
	if(!compare_signature("WAVE", header_info, bytepos))
	{
		filein.close();
		free(header_info);
		return -1;
	}

	bytepos = 12u;

	while(!compare_signature("fmt ", header_info, bytepos))
	{
		//Error: subchunk fmt not found
		if(bytepos > (BYTEBUF_SIZE - 256u))
		{
			filein.close();
			free(header_info);
			return -1;
		}

		pu32 = (unsigned int*) &header_info[bytepos + 4u];
		bytepos += (*pu32 + 8u);
	}

	pu16 = (unsigned short*) &header_info[bytepos + 8u];

	//Error: encoding format not supported
	if(pu16[0] != 1u)
	{
		filein.close();
		free(header_info);
		return -1;
	}

	n_channels = pu16[1];

	pu32 = (unsigned int*) &header_info[bytepos + 12u];
	sample_rate = *pu32;

	pu16 = (unsigned short*) &header_info[bytepos + 22u];
	bit_depth = *pu16;

	pu32 = (unsigned int*) &header_info[bytepos + 4u];
	bytepos += (*pu32 + 8u);

	while(!compare_signature("data", header_info, bytepos))
	{
		//Error: subchunk data not found
		if(bytepos > (BYTEBUF_SIZE - 256u))
		{
			filein.close();
			free(header_info);
			return -1;
		}

		pu32 = (unsigned int*) &header_info[bytepos + 4u];
		bytepos += (*pu32 + 8u);
	}

	pu32 = (unsigned int*) &header_info[bytepos + 4u];

	data_begin = (unsigned long) (bytepos + 8u);
	data_end = data_begin + ((unsigned long) *pu32);

	filein.close();
	free(header_info);

	if((bit_depth == 16u) && (n_channels == 1u)) return DSP_16BIT1CH;
	if((bit_depth == 16u) && (n_channels == 2u)) return DSP_16BIT2CH;
	if((bit_depth == 24u) && (n_channels == 1u)) return DSP_24BIT1CH;
	if((bit_depth == 24u) && (n_channels == 2u)) return DSP_24BIT2CH;

	return -1;
}

bool compare_signature(const char *auth, const char *bytebuf, unsigned int offset)
{
	if(auth[0] != bytebuf[offset]) return false;
	if(auth[1] != bytebuf[offset + 1u]) return false;
	if(auth[2] != bytebuf[offset + 2u]) return false;
	if(auth[3] != bytebuf[offset + 3u]) return false;

	return true;
}

