#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#define INPUT_FILE_DIR "/home/user/Music/input_audio.raw"
#define OUTPUT_FILE_DIR "/home/user/Music/output_audio.raw"

#define BUFFER_SIZE_SAMPLES 65536

//Delay time in number of samples. Should not be greater than half buffer size.
#define DSP_N_DELAY 240

//Number of delay feedbacks. Set to 0 for no feedback.
#define DSP_N_FEEDBACK_LOOPS 20

//Enable/Disable alternating feedback polarity.
#define DSP_FEEDBACK_POL_ALTERNATE

//Set delay cycle divider increase. 
//If defined, cycle divider will be equal to cycle number + 1. 
//Else, cycle divider will increase exponentially.
#define DSP_CYCLE_DIV_INC_ONE

#define BUFFER_SIZE_BYTES (2*BUFFER_SIZE_SAMPLES)
#define BUFFER_SIZE_PER_CHANNEL (BUFFER_SIZE_SAMPLES/2)
#define DSP_N_DELAY_CYCLES (DSP_N_FEEDBACK_LOOPS + 1)

#define SAMPLE_MAX_VALUE (32767)
#define SAMPLE_MIN_VALUE (-32768)

std::fstream input_file;
unsigned int input_file_size = 0;
unsigned int input_file_pos = 0;

std::fstream output_file;
unsigned int output_file_pos = 0;

short *buffer_input_0 = NULL;
short *buffer_input_1 = NULL;
short *buffer_output = NULL;
short *buffer_dsp = NULL;
bool curr_buf = false;

short *curr_in = NULL;
short *prev_in = NULL;

int n_sample = 0;

int n_cycle = 0;
int n_delay = 0;
int cycle_div = 0;
int pol = 0;
int l_sample = 0;
int r_sample = 0;

bool read_proc(void)
{
	if(input_file_pos >= input_file_size) return false;

	if(curr_buf)
	{
		curr_in = buffer_input_1;
		prev_in = buffer_input_0;
	}
	else
	{
		curr_in = buffer_input_0;
		prev_in = buffer_input_1;
	}

	input_file.seekg(input_file_pos);
	input_file.read((char*) curr_in, BUFFER_SIZE_BYTES);
	input_file_pos += BUFFER_SIZE_BYTES;

	return true;
}

void write_proc(void)
{
	output_file.seekg(output_file_pos);
	output_file.write((char*) buffer_output, BUFFER_SIZE_BYTES);
	output_file_pos += BUFFER_SIZE_BYTES;

	curr_buf = !curr_buf;
	return;
}

void load_delay(void)
{
	l_sample = 0;
	r_sample = 0;

	n_cycle = 1;
	n_delay = 0;
	cycle_div = 1;
	pol = 1;

	while(n_cycle <= DSP_N_DELAY_CYCLES)
	{
#ifdef DSP_FEEDBACK_POL_ALTERNATE
		if(n_cycle%2) pol = -1;
		else pol = 1;
#endif

#ifdef DSP_CYCLE_DIV_INC_ONE
		cycle_div = n_cycle + 1;
#else
		cycle_div = (1 << n_cycle);
#endif

		n_delay = n_cycle*DSP_N_DELAY;
		if(n_sample < n_delay)
		{
			l_sample += pol*prev_in[BUFFER_SIZE_SAMPLES - 2*(n_delay - n_sample)]/cycle_div;
			r_sample += pol*prev_in[BUFFER_SIZE_SAMPLES - 2*(n_delay - n_sample) + 1]/cycle_div;
		}
		else
		{
			l_sample += pol*curr_in[2*(n_sample - n_delay)]/cycle_div;
			r_sample += pol*curr_in[2*(n_sample - n_delay) + 1]/cycle_div;
		}

		n_cycle++;
	}

	if((l_sample < SAMPLE_MAX_VALUE) && (l_sample > SAMPLE_MIN_VALUE)) buffer_dsp[2*n_sample] = l_sample;
	else if(l_sample >= SAMPLE_MAX_VALUE) buffer_dsp[2*n_sample] = SAMPLE_MAX_VALUE;
	else if(l_sample <= SAMPLE_MIN_VALUE) buffer_dsp[2*n_sample] = SAMPLE_MIN_VALUE;

	if((r_sample < SAMPLE_MAX_VALUE) && (r_sample > SAMPLE_MIN_VALUE)) buffer_dsp[2*n_sample + 1] = r_sample;
	else if(r_sample >= SAMPLE_MAX_VALUE) buffer_dsp[2*n_sample + 1] = SAMPLE_MAX_VALUE;
	else if(r_sample <= SAMPLE_MIN_VALUE) buffer_dsp[2*n_sample + 1] = SAMPLE_MIN_VALUE;

	return;
}

void run_dsp(void)
{
	n_sample = 0;
	while(n_sample < BUFFER_SIZE_PER_CHANNEL)
	{
		load_delay();
		buffer_output[2*n_sample] = ((curr_in[2*n_sample]) + (buffer_dsp[2*n_sample]))/2;
		buffer_output[2*n_sample + 1] = ((curr_in[2*n_sample + 1]) + (buffer_dsp[2*n_sample + 1]))/2;

		n_sample++;
	}

	return;
}

bool runtime_proc(void)
{
	if(!read_proc()) return false;
	run_dsp();
	write_proc();
	return true;
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

bool open_input_file(void)
{
	input_file.open(INPUT_FILE_DIR, std::ios_base::in);
	if(input_file.is_open())
	{
		input_file.seekg(0, input_file.end);
		input_file_size = input_file.tellg();
		input_file_pos = 0;
		input_file.seekg(input_file_pos);
		return true;
	}

	return false;
}

bool open_output_file(void)
{
	output_file.open(OUTPUT_FILE_DIR, (std::ios_base::in | std::ios_base::out));
	return output_file.is_open();
}

void create_output_file(void)
{
	std::string cmd_line = "";

	if(open_output_file())
	{
		output_file.close();
		cmd_line = "rm ";
		cmd_line += OUTPUT_FILE_DIR;
		system(cmd_line.c_str());
	}

	cmd_line = "touch ";
	cmd_line += OUTPUT_FILE_DIR;
	system(cmd_line.c_str());

	return;
}

int main(int argc, char **argv)
{
	if(!open_input_file())
	{
		std::cout << "Error opening input file\nError code: " << errno << "\nTerminated\n";
		return 0;
	}

	create_output_file();
	if(!open_output_file())
	{
		std::cout << "Error opening output file\nError code: " << errno << "\nTerminated\n";
		return 0;
	}

	output_file_pos = 0;
	std::cout << "Files are open\n";
	
	buffer_malloc();

	std::cout << "Running DSP...\n";
	while(runtime_proc());
	std::cout << "Done\n";

	input_file.close();
	output_file.close();
	buffer_free();

	std::cout << "Terminated\n";
	return 0;
}
