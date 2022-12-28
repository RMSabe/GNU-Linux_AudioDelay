#include <cerrno>
#include <fstream>
#include <iostream>
#include <string>

#define DSP_N_DELAY 240
#define DSP_N_FEEDBACK_LOOPS 20
#define DSP_FEEDBACK_POL_ALTERNATE 1
#define DSP_CYCLE_DIV_INC_ONE 1

#define DSP_16BIT2CH 0
//#define DSP_24BIT2CH 1

std::fstream audio_file;
char *file_dir = NULL;
char *audio_dev = NULL;
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
	audio_file.open(file_dir, std::ios_base::in);
	if(audio_file.is_open())
	{
		audio_file.seekg(0, audio_file.end);
		audio_file_size = audio_file.tellg();
		return true;
	}

	return false;
}

bool check_file_dir(void)
{
	unsigned int len = 0;
	while(file_dir[len] != '\0') len++;

	if(len < 5) return false;

	if(compare_signature(".wav", file_dir, (len - 4))) return true;
	if(compare_signature(".WAV", file_dir, (len - 4))) return true;

	return false;
}

bool check_cmd_help(const char *text)
{
	unsigned int len = 0;
	while(text[len] != '\0') len++;

	if(len != 6) return false;
	if(!compare_signature("help", text, 2)) return false;
	if(text[0] != '-' || text[1] != '-') return false;

	return true;
}

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		std::cout << "Error: missing arguments\n";
		return 0;
	}

	if(argc == 2)
	{
		if(check_cmd_help(argv[1])) std::cout << "This executable requires 2 arguments: <audio device id> <input file directory>\nThey must be in this order\n";
		else std::cout << "Error: missing arguments\n";

		return 0;
	}

	audio_dev = argv[1];
	file_dir = argv[2];

	if(!check_file_dir())
	{
		std::cout << "Error: invalid file name or extension\n";
		return 0;
	}

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

	cmd_line += file_dir;
	cmd_line += " ";
	cmd_line += audio_dev;
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
