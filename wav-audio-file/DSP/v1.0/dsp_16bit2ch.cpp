/*
 * Audio Delay File Generation
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#define TEMPFILE_DIR ("temp.raw")

#define BUFFER_SIZE_FRAMES 32768U

#define BUFFER_SIZE_SAMPLES (2U*BUFFER_SIZE_FRAMES)
#define BUFFER_SIZE_BYTES (2U*BUFFER_SIZE_SAMPLES)

#define SAMPLE_MAX_VALUE (0x7fff)
#define SAMPLE_MIN_VALUE (-0x8000)

std::fstream filein;
std::fstream fileout;

unsigned long filein_pos = 0ul;
unsigned long fileout_pos = 0ul;

char *filein_dir = NULL;

unsigned long data_begin = 0ul;
unsigned long data_end = 0ul;

int dsp_n_delay = 0;
int dsp_n_cycles = 0;
bool feedback_pol_alt = false;
bool cycle_div_inc_one = false;

short *buffer_input_0 = NULL;
short *buffer_input_1 = NULL;
short *buffer_output = NULL;
short *buffer_dsp = NULL;

short *curr_in = NULL;
short *prev_in = NULL;

int n_frame = 0;

bool curr_buf_cycle = false;

bool fileout_create(void);
bool filein_open(void);
void file_close(void);
void buffer_malloc(void);
void buffer_free(void);

bool runtime_loop(void);
bool read_proc(void);
void write_proc(void);
void run_dsp(void);
void load_delay(void);

int main(int argc, char **argv)
{
	if(argc < 8)
	{
		std::cout << "Error: invalid runtime parameters\n";
		return 0;
	}

	filein_dir = argv[1];
	data_begin = std::stol(argv[2]);
	data_end = std::stol(argv[3]);
	dsp_n_delay = std::stoi(argv[4]);
	dsp_n_cycles = std::stoi(argv[5]) + 1;
	feedback_pol_alt = (std::stoi(argv[6]) & 0x1);
	cycle_div_inc_one = (std::stoi(argv[7]) & 0x1);

	if(!fileout_create())
	{
		std::cout << "Error: could not create DSP file\n";
		return 0;
	}

	if(!filein_open())
	{
		file_close();
		std::cout << "Error: could not open input file\n";
		return 0;
	}

	filein_pos = data_begin;
	fileout_pos = 0ul;

	buffer_malloc();
	std::cout << "Running DSP...\n";

	while(runtime_loop());
	std::cout << "Done\n";

	file_close();
	buffer_free();

	return 0;
}

bool fileout_create(void)
{
	std::string cmd = "";

	fileout.open(TEMPFILE_DIR, (std::ios_base::in | std::ios_base::out));
	if(fileout.is_open())
	{
		fileout.close();
		cmd = "rm ";
		cmd += TEMPFILE_DIR;
		system(cmd.c_str());
	}

	cmd = "touch ";
	cmd += TEMPFILE_DIR;
	system(cmd.c_str());

	fileout.open(TEMPFILE_DIR, (std::ios_base::in | std::ios_base::out));
	return fileout.is_open();
}

bool filein_open(void)
{
	filein.open(filein_dir, std::ios_base::in);
	return filein.is_open();
}

void file_close(void)
{
	if(filein.is_open()) filein.close();
	if(fileout.is_open()) fileout.close();

	return;
}

void buffer_malloc(void)
{
	buffer_input_0 = (short*) malloc(BUFFER_SIZE_BYTES);
	buffer_input_1 = (short*) malloc(BUFFER_SIZE_BYTES);
	buffer_output = (short*) malloc(BUFFER_SIZE_BYTES);
	buffer_dsp = (short*) malloc(BUFFER_SIZE_BYTES);

	memset(buffer_input_0, 0, BUFFER_SIZE_BYTES);
	memset(buffer_input_1, 0, BUFFER_SIZE_BYTES);
	memset(buffer_output, 0, BUFFER_SIZE_BYTES);
	memset(buffer_dsp, 0, BUFFER_SIZE_BYTES);

	return;
}

void buffer_free(void)
{
	free(buffer_input_0);
	free(buffer_input_1);
	free(buffer_output);
	free(buffer_dsp);

	return;
}

bool runtime_loop(void)
{
	if(!read_proc()) return false;
	run_dsp();
	write_proc();
	curr_buf_cycle = !curr_buf_cycle;
	return true;
}

bool read_proc(void)
{
	if(filein_pos >= data_end) return false;

	if(curr_buf_cycle)
	{
		curr_in = buffer_input_1;
		prev_in = buffer_input_0;
	}
	else
	{
		curr_in = buffer_input_0;
		prev_in = buffer_input_1;
	}

	filein.seekg(filein_pos);
	filein.read((char*) curr_in, BUFFER_SIZE_BYTES);
	filein_pos += BUFFER_SIZE_BYTES;

	return true;
}

void write_proc(void)
{
	fileout.seekg(fileout_pos);
	fileout.write((char*) buffer_output, BUFFER_SIZE_BYTES);
	fileout_pos += BUFFER_SIZE_BYTES;

	return;
}

void run_dsp(void)
{
	n_frame = 0;
	while(n_frame < BUFFER_SIZE_FRAMES)
	{
		load_delay();
		buffer_output[2*n_frame] = (curr_in[2*n_frame] + buffer_dsp[2*n_frame])/2;
		buffer_output[2*n_frame + 1] = (curr_in[2*n_frame + 1] + buffer_dsp[2*n_frame + 1])/2;

		n_frame++;
	}

	return;
}

void load_delay(void)
{
	static int n_delay;
	static int n_cycle;
	static int cycle_div;
	static int pol;
	static int l_sample;
	static int r_sample;

	n_delay = 0;
	n_cycle = 1;
	cycle_div = 1;
	pol = 1;
	l_sample = 0;
	r_sample = 0;

	while(n_cycle <= dsp_n_cycles)
	{
		if(feedback_pol_alt)
		{
			if(n_cycle & 0x1) pol = -1;
			else pol = 1;
		}

		if(cycle_div_inc_one) cycle_div++;
		else cycle_div = (1 << n_cycle);

		n_delay = n_cycle*dsp_n_delay;
		if(n_frame < n_delay)
		{
			l_sample += pol*prev_in[BUFFER_SIZE_SAMPLES - 2*(n_delay - n_frame)]/cycle_div;
			r_sample += pol*prev_in[BUFFER_SIZE_SAMPLES - 2*(n_delay - n_frame) + 1]/cycle_div;
		}
		else
		{
			l_sample += pol*curr_in[2*(n_frame - n_delay)]/cycle_div;
			r_sample += pol*curr_in[2*(n_frame - n_delay) + 1]/cycle_div;
		}

		n_cycle++;
	}

	if(l_sample > SAMPLE_MAX_VALUE) buffer_dsp[2*n_frame] = SAMPLE_MAX_VALUE;
	else if(l_sample < SAMPLE_MIN_VALUE) buffer_dsp[2*n_frame] = SAMPLE_MIN_VALUE;
	else buffer_dsp[2*n_frame] = l_sample;

	if(r_sample > SAMPLE_MAX_VALUE) buffer_dsp[2*n_frame + 1] = SAMPLE_MAX_VALUE;
	else if(r_sample < SAMPLE_MIN_VALUE) buffer_dsp[2*n_frame + 1] = SAMPLE_MIN_VALUE;
	else buffer_dsp[2*n_frame + 1] = r_sample;

	return;
}

