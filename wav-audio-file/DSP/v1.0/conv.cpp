/*
 * Audio Delay File Generation.
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#include <fstream>
#include <iostream>
#include <string>

#define TEMPFILE_DIR ("temp.raw")

#define BYTEBUF_SIZE 4096U

std::fstream filein;
std::fstream fileout;

unsigned long filein_size = 0ul;
unsigned long filein_pos = 0ul;

unsigned long fileout_pos = 0ul;

char *fileout_dir = NULL;

unsigned int sample_rate = 0u;
unsigned short bit_depth = 0u;
unsigned short n_channels = 0u;

char *bytebuf = NULL;

bool fileout_create(void);
bool filein_open(void);
void file_close(void);

void fileout_write_header(void);
bool runtime_loop(void);

int main(int argc, char **argv)
{
	if(argc < 5)
	{
		std::cout << "Error: invalid runtime parameters\n";
		return 0;
	}

	fileout_dir = argv[1];

	n_channels = std::stoi(argv[2]);
	bit_depth = std::stoi(argv[3]);
	sample_rate = std::stoi(argv[4]);

	if(!fileout_create())
	{
		std::cout << "Error: could not create output WAV file\n";
		return 0;
	}

	if(!filein_open())
	{
		file_close();
		std::cout << "Error: could not open DSP file\n";
		return 0;
	}

	fileout_write_header();
	filein_pos = 0ul;

	bytebuf = (char*) malloc(BYTEBUF_SIZE);

	std::cout << "Converting to WAV...\n";
	while(runtime_loop());
	std::cout << "Done\n";

	file_close();
	free(bytebuf);

	return 0;
}

bool fileout_create(void)
{
	std::string cmd = "";

	fileout.open(fileout_dir, (std::ios_base::in | std::ios_base::out));
	if(fileout.is_open())
	{
		fileout.close();
		cmd = "rm ";
		cmd += fileout_dir;
		system(cmd.c_str());
	}

	cmd = "touch ";
	cmd += fileout_dir;
	system(cmd.c_str());

	fileout.open(fileout_dir, (std::ios_base::in | std::ios_base::out));
	return fileout.is_open();
}

bool filein_open(void)
{
	filein.open(TEMPFILE_DIR, std::ios_base::in);
	if(!filein.is_open()) return false;

	filein.seekg(0, filein.end);
	filein_size = filein.tellg();
	return true;
}

void file_close(void)
{
	if(filein.is_open()) filein.close();
	if(fileout.is_open()) fileout.close();

	return;
}

void fileout_write_header(void)
{
	char *header_info = (char*) malloc(44);
	unsigned short *pu16 = NULL;
	unsigned int *pu32 = NULL;

	header_info[0] = 'R';
	header_info[1] = 'I';
	header_info[2] = 'F';
	header_info[3] = 'F';

	pu32 = (unsigned int*) &header_info[4];
	*pu32 = 36u + ((unsigned int) filein_size);

	header_info[8] = 'W';
	header_info[9] = 'A';
	header_info[10] = 'V';
	header_info[11] = 'E';

	header_info[12] = 'f';
	header_info[13] = 'm';
	header_info[14] = 't';
	header_info[15] = ' ';

	pu32 = (unsigned int*) &header_info[16];
	*pu32 = 16u;

	pu16 = (unsigned short*) &header_info[20];
	pu16[0] = 1u;
	pu16[1] = n_channels;

	pu32 = (unsigned int*) &header_info[24];
	pu32[0] = sample_rate;
	pu32[1] = sample_rate*n_channels*bit_depth/8u;

	pu16 = (unsigned short*) &header_info[32];
	pu16[0] = n_channels*bit_depth/8u;
	pu16[1] = bit_depth;

	header_info[36] = 'd';
	header_info[37] = 'a';
	header_info[38] = 't';
	header_info[39] = 'a';

	pu32 = (unsigned int*) &header_info[40];
	*pu32 = (unsigned int) filein_size;

	fileout.seekg(0);
	fileout.write(header_info, 44);
	fileout_pos = 44ul;

	free(header_info);
	return;
}

bool runtime_loop(void)
{
	if(filein_pos >= filein_size) return false;

	filein.seekg(filein_pos);
	filein.read(bytebuf, BYTEBUF_SIZE);
	filein_pos += BYTEBUF_SIZE;

	fileout.seekg(fileout_pos);
	fileout.write(bytebuf, BYTEBUF_SIZE);
	fileout_pos += BYTEBUF_SIZE;

	return true;
}

