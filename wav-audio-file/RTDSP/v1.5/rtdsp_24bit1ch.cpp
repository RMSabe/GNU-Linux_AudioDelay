/*
 * Real Time Audio Delay for GNU-Linux systems.
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <pthread.h>
#include <alsa/asoundlib.h>

#define BUFFER_SIZE_FRAMES 32768U

#define BUFFER_SIZE_SAMPLES BUFFER_SIZE_FRAMES
#define BUFFER_SIZE_BYTES (4U*BUFFER_SIZE_SAMPLES)

#define BUFFEROUTPUT_SIZE_SAMPLES (2U*BUFFER_SIZE_SAMPLES)
#define BUFFEROUTPUT_SIZE_BYTES (4U*BUFFEROUTPUT_SIZE_SAMPLES)

#define BYTEBUF_SIZE_BYTES (3U*BUFFER_SIZE_SAMPLES)

#define SAMPLE_MAX_VALUE (0x7fffff)
#define SAMPLE_MIN_VALUE (-0x800000)

char *audio_file_dir = NULL;
char *audio_dev_desc = NULL;
unsigned long audio_data_begin = 0ul;
unsigned long audio_data_end = 0ul;
unsigned int sample_rate = 0u;

int dsp_n_delay_update = 0;
int dsp_n_delay_cycles_update = 0;
bool dsp_feedback_pol_alternate_update = false;
bool dsp_cycle_div_inc_one_update = false;

pthread_t loadthread;
pthread_t playthread;
pthread_t userthread;

std::string usr_cmd = "";

std::fstream audio_file;
unsigned long audio_file_pos = 0ul;

snd_pcm_t *audio_dev = NULL;
snd_pcm_uframes_t audio_buffer_size_frames;
unsigned int audio_buffer_size_samples = 0u;
unsigned int buffer_n_div = 1u;
int **pp_startpoints = NULL;

int *buffer_input_0 = NULL;
int *buffer_input_1 = NULL;
int *buffer_output_0 = NULL;
int *buffer_output_1 = NULL;
int *buffer_output_2 = NULL;
int *buffer_output_3 = NULL;
int *buffer_dsp = NULL;

unsigned char *bytebuf = NULL;

unsigned int curr_buf_cycle = 0u;

int *curr_in = NULL;
int *prev_in = NULL;
int *load_out = NULL;
int *play_out = NULL;

int n_frame = 0;
int n_sample = 0;
int n_byte = 0;

bool stop = false;

bool audio_hw_init(void);
void buffer_malloc(void);
void buffer_free(void);

void playback(void);
void buffer_preload(void);
void *p_loadthread_proc(void *args);
void *p_playthread_proc(void *args);
void update_buf_cycle(void);
void buffer_remap(void);
void buffer_load(void);
void buffer_play(void);
void run_dsp(void);
void load_delay(void);
void load_startpoints(void);

void user_init(void);
void *p_userthread_proc(void *args);
void decode_cmd(void);
bool compare_str(const char *auth, const char *text, unsigned int offset);
void print_help_text(void);
void print_current_params(void);
void update_n_delay(const char *text);
void update_n_delay_cycles(const char *text);
void update_feedback_pol_alternate(const char *text);
void update_cycle_div_inc_one(const char *text);
void delay_us(unsigned long time_us);

//===================================================================================================
//STARTUP

int main(int argc, char **argv)
{
	if(argc < 10)
	{
		std::cout << "Error: invalid runtime parameters\n";
		return 0;
	}

	audio_file_dir = argv[1];
	audio_dev_desc = argv[2];
	audio_data_begin = std::stol(argv[3]);
	audio_data_end = std::stol(argv[4]);
	sample_rate = std::stoi(argv[5]);
	dsp_n_delay_update = std::stoi(argv[6]);
	dsp_n_delay_cycles_update = (std::stoi(argv[7]) + 1);
	dsp_feedback_pol_alternate_update = (std::stoi(argv[8]) & 0x00000001);
	dsp_cycle_div_inc_one_update = (std::stoi(argv[9]) & 0x00000001);

	audio_file.open(audio_file_dir, std::ios_base::in);
	if(!audio_file.is_open())
	{
		std::cout << "Error opening audio file\nError code: " << errno << std::endl;
		return 0;
	}

	if(!audio_hw_init())
	{
		std::cout << "Error code: " << errno << std::endl;
		return 0;
	}

	audio_file_pos = audio_data_begin;
	buffer_malloc();

	std::cout << "Playback started\n";
	user_init();
	pthread_create(&userthread, NULL, p_userthread_proc, NULL);
	playback();
	std::cout << "Playback finished\n";
	pthread_cancel(userthread);

	audio_file.close();
	snd_pcm_drain(audio_dev);
	snd_pcm_close(audio_dev);
	buffer_free();

	return 0;
}

bool audio_hw_init(void)
{
	int n_return = 0;
	snd_pcm_hw_params_t *hw_params = NULL;

	n_return = snd_pcm_open(&audio_dev, audio_dev_desc, SND_PCM_STREAM_PLAYBACK, 0);
	if(n_return < 0)
	{
		std::cout << "Audio Hardware: error opening audio device\n";
		return false;
	}

	snd_pcm_hw_params_malloc(&hw_params);
	snd_pcm_hw_params_any(audio_dev, hw_params);

	n_return = snd_pcm_hw_params_set_access(audio_dev, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(n_return < 0)
	{
		std::cout << "Audio Hardware: error setting access to read/write interleaved\n";
		return false;
	}

	n_return = snd_pcm_hw_params_set_format(audio_dev, hw_params, SND_PCM_FORMAT_S24_LE);
	if(n_return < 0)
	{
		std::cout << "Audio Hardware: error setting format to signed 24bit little-endian\n";
		return false;
	}

	n_return = snd_pcm_hw_params_set_channels(audio_dev, hw_params, 2);
	if(n_return < 0)
	{
		std::cout << "Audio Hardware: error setting channels to stereo\n";
		return false;
	}

	unsigned int rate = sample_rate;
	n_return = snd_pcm_hw_params_set_rate_near(audio_dev, hw_params, &rate, 0);
	if(n_return < 0 || rate < sample_rate)
	{
		std::cout << "Audio Hardware: error setting sample rate\n";
		return false;
	}

	n_return = snd_pcm_hw_params(audio_dev, hw_params);
	if(n_return < 0)
	{
		std::cout << "Audio Hardware: error setting hardware parameters\n";
		return false;
	}

	snd_pcm_hw_params_get_period_size(hw_params, &audio_buffer_size_frames, 0);
	snd_pcm_hw_params_free(hw_params);
	return true;
}

void buffer_malloc(void)
{
	audio_buffer_size_samples = 2*audio_buffer_size_frames;
	if(audio_buffer_size_samples < BUFFEROUTPUT_SIZE_SAMPLES) buffer_n_div = BUFFEROUTPUT_SIZE_SAMPLES/audio_buffer_size_samples;
	else buffer_n_div = 1u;
	
	pp_startpoints = (int**) malloc(buffer_n_div*sizeof(int*));

	buffer_input_0 = (int*) malloc(BUFFER_SIZE_BYTES);
	buffer_input_1 = (int*) malloc(BUFFER_SIZE_BYTES);
	buffer_output_0 = (int*) malloc(BUFFEROUTPUT_SIZE_BYTES);
	buffer_output_1 = (int*) malloc(BUFFEROUTPUT_SIZE_BYTES);
	buffer_output_2 = (int*) malloc(BUFFEROUTPUT_SIZE_BYTES);
	buffer_output_3 = (int*) malloc(BUFFEROUTPUT_SIZE_BYTES);
	buffer_dsp = (int*) malloc(BUFFER_SIZE_BYTES);

	bytebuf = (unsigned char*) malloc(BYTEBUF_SIZE_BYTES);
	
	memset(buffer_input_0, 0, BUFFER_SIZE_BYTES);
	memset(buffer_input_1, 0, BUFFER_SIZE_BYTES);
	memset(buffer_output_0, 0, BUFFEROUTPUT_SIZE_BYTES);
	memset(buffer_output_1, 0, BUFFEROUTPUT_SIZE_BYTES);
	memset(buffer_output_2, 0, BUFFEROUTPUT_SIZE_BYTES);
	memset(buffer_output_3, 0, BUFFEROUTPUT_SIZE_BYTES);
	memset(buffer_dsp, 0, BUFFER_SIZE_BYTES);

	memset(bytebuf, 0, BYTEBUF_SIZE_BYTES);

	return;
}

void buffer_free(void)
{
	free(pp_startpoints);

	free(buffer_input_0);
	free(buffer_input_1);
	free(buffer_output_0);
	free(buffer_output_1);
	free(buffer_output_2);
	free(buffer_output_3);
	free(buffer_dsp);

	free(bytebuf);

	return;
}

//STARTUP
//==================================================================================================
//PLAYBACK

void playback(void)
{
	buffer_preload();
	while(!stop)
	{
		pthread_create(&playthread, NULL, p_playthread_proc, NULL);
		pthread_create(&loadthread, NULL, p_loadthread_proc, NULL);
		pthread_join(loadthread, NULL);
		pthread_join(playthread, NULL);
		buffer_remap();
	}

	return;
}

void buffer_preload(void)
{
	curr_buf_cycle = 0u;
	buffer_remap();

	buffer_load();
	run_dsp();
	update_buf_cycle();
	buffer_remap();

	buffer_load();
	run_dsp();
	update_buf_cycle();
	buffer_remap();

	return;
}

void *p_loadthread_proc(void *args)
{
	buffer_load();
	run_dsp();
	update_buf_cycle();
	return NULL;
}

void *p_playthread_proc(void *args)
{
	load_startpoints();
	buffer_play();
	return NULL;
}

void update_buf_cycle(void)
{
	if(curr_buf_cycle == 3u) curr_buf_cycle = 0u;
	else curr_buf_cycle++;

	return;
}

void buffer_remap(void)
{
	switch(curr_buf_cycle)
	{
		case 0u:
			curr_in = buffer_input_0;
			prev_in = buffer_input_1;
			load_out = buffer_output_0;
			play_out = buffer_output_2;
			break;

		case 1u:
			curr_in = buffer_input_1;
			prev_in = buffer_input_0;
			load_out = buffer_output_1;
			play_out = buffer_output_3;
			break;

		case 2u:
			curr_in = buffer_input_0;
			prev_in = buffer_input_1;
			load_out = buffer_output_2;
			play_out = buffer_output_0;
			break;

		case 3u:
			curr_in = buffer_input_1;
			prev_in = buffer_input_0;
			load_out = buffer_output_3;
			play_out = buffer_output_1;
			break;
	}

	return;
}

void buffer_load(void)
{
	if(audio_file_pos >= audio_data_end)
	{
		stop = true;
		return;
	}

	audio_file.seekg(audio_file_pos);
	audio_file.read((char*) bytebuf, BYTEBUF_SIZE_BYTES);
	audio_file_pos += BYTEBUF_SIZE_BYTES;

	n_byte = 0;
	n_sample = 0;
	while(n_sample < BUFFER_SIZE_SAMPLES)
	{
		curr_in[n_sample] = ((bytebuf[n_byte + 2] << 16) | (bytebuf[n_byte + 1] << 8) | bytebuf[n_byte]);

		if(curr_in[n_sample] & 0x800000) curr_in[n_sample] |= 0xff000000;
		else curr_in[n_sample] &= 0x00ffffff; //Not really necessary, but just to be safe.

		n_byte += 3;
		n_sample++;
	}

	return;
}

void buffer_play(void)
{
	unsigned int n_div = 0u;
	int n_return = 0;

	while(n_div < buffer_n_div)
	{
		n_return = snd_pcm_writei(audio_dev, pp_startpoints[n_div], audio_buffer_size_frames);
		if(n_return == -EPIPE) snd_pcm_prepare(audio_dev);

		n_div++;
	}

	return;
}

void run_dsp(void)
{
	n_frame = 0;
	while(n_frame < BUFFER_SIZE_FRAMES)
	{
		load_delay();
		load_out[2*n_frame] = (curr_in[n_frame] + buffer_dsp[n_frame])/2;
		load_out[2*n_frame + 1] = load_out[2*n_frame];

		n_frame++;
	}

	return;
}

void load_delay(void)
{
	static int dsp_n_delay;
	static int dsp_n_delay_cycles;
	static bool dsp_feedback_pol_alternate;
	static bool dsp_cycle_div_inc_one;

	static int n_cycle;
	static int n_delay;
	static int cycle_div;
	static int pol;
	static int sample;

	dsp_n_delay = dsp_n_delay_update;
	dsp_n_delay_cycles = dsp_n_delay_cycles_update;
	dsp_feedback_pol_alternate = dsp_feedback_pol_alternate_update;
	dsp_cycle_div_inc_one = dsp_cycle_div_inc_one_update;

	n_cycle = 1;
	pol = 1;
	sample = 0;

	while(n_cycle <= dsp_n_delay_cycles)
	{
		if(dsp_feedback_pol_alternate)
		{
			if(n_cycle & 0x1) pol = -1;
			else pol = 1;
		}

		if(dsp_cycle_div_inc_one) cycle_div = n_cycle + 1;
		else cycle_div = (1 << n_cycle);

		n_delay = n_cycle*dsp_n_delay;

		if(n_frame < n_delay) sample += pol*prev_in[BUFFER_SIZE_SAMPLES - (n_delay - n_frame)]/cycle_div;
		else sample += pol*curr_in[(n_frame - n_delay)]/cycle_div;

		n_cycle++;
	}

	if(sample > SAMPLE_MAX_VALUE) buffer_dsp[n_frame] = SAMPLE_MAX_VALUE;
	else if(sample < SAMPLE_MIN_VALUE) buffer_dsp[n_frame] = SAMPLE_MIN_VALUE;
	else buffer_dsp[n_frame] = sample;

	return;
}

void load_startpoints(void)
{
	pp_startpoints[0] = play_out;
	unsigned int n_div = 1u;
	while(n_div < buffer_n_div)
	{
		pp_startpoints[n_div] = &play_out[n_div*audio_buffer_size_samples];
		n_div++;
	}

	return;
}

//PLAYBACK
//==================================================================================================
//USER CMD

void user_init(void)
{
	print_help_text();
	print_current_params();
	return;
}

void *p_userthread_proc(void *args)
{
	while(true)
	{
		usr_cmd = std::cin.peek();
		if(usr_cmd.length() > 0) 
		{
			std::cin >> usr_cmd;
			decode_cmd();
		}
		else delay_us(1000ul);
	}

	return NULL;
}

void decode_cmd(void)
{
	const char *cmd = usr_cmd.c_str();
	const char *numtext = NULL;

	if(compare_str("stop", cmd, 0u)) stop = true;
	else if(compare_str("params", cmd, 0u)) print_current_params();
	else if(compare_str("--help", cmd, 0u)) print_help_text();
	else if(compare_str("setnd:", cmd, 0u))
	{
		numtext = &cmd[6];
		update_n_delay(numtext);
	}
	else if(compare_str("setnf:", cmd, 0u))
	{
		numtext = &cmd[6];
		update_n_delay_cycles(numtext);
	}
	else if(compare_str("setfpa:", cmd, 0u))
	{
		numtext = &cmd[7];
		update_feedback_pol_alternate(numtext);
	}
	else if(compare_str("setcdi:", cmd, 0u))
	{
		numtext = &cmd[7];
		update_cycle_div_inc_one(numtext);
	}
	else std::cout << "Invalid Command Line Entered\n\n";

	return;
}

bool compare_str(const char *auth, const char *text, unsigned int offset)
{
	if(auth == NULL) return false;
	if(text == NULL) return false;

	unsigned int auth_len = 0u;
	while(auth[auth_len] != '\0') auth_len++;

	unsigned int text_len = 0u;
	while(text[text_len] != '\0') text_len++;

	if(auth_len == 0u) return false;
	if(text_len == 0u) return false;
	if(text_len < (offset + auth_len)) return false;

	unsigned int nchar = 0u;
	while(nchar < auth_len)
	{
		if(auth[nchar] != text[nchar + offset]) return false;
		nchar++;
	}

	return true;
}

void print_help_text(void)
{
	std::cout << "User Command List:\n";
	std::cout << "\"--help\" : print this list\n";
	std::cout << "\"params\" : print current parameters\n";
	std::cout << "\"setnd:<number>\" : set n delay\n";
	std::cout << "\"setnf:<number>\" : set n feedback loops\n";
	std::cout << "\"setfpa:<number>\" : (enable 1/disable 0) feedback pol alternate\n";
	std::cout << "\"setcdi:<number>\" : (enable 1/disable 0) cycle div inc one\n";
	std::cout << "\"stop\" : stop and quit playback\n\n";
	return;
}

void print_current_params(void)
{
	std::cout << "Current Parameters:\n";
	std::cout << "N DELAY: " << dsp_n_delay_update << std::endl;
	std::cout << "N FEEDBACK LOOPS: " << (dsp_n_delay_cycles_update - 1) << std::endl;
	std::cout << "FEEDBACK POL ALTERNATE: " << dsp_feedback_pol_alternate_update << std::endl;
	std::cout << "CYCLE DIV INC ONE: " << dsp_cycle_div_inc_one_update << std::endl << std::endl;
	return;
}

void update_n_delay(const char *text)
{
	int i32;

	try
	{
		i32 = std::stoi(text);
	}
	catch(...)
	{
		std::cout << "Invalid value entered\n\n";
		return;
	}

	if(i32*dsp_n_delay_cycles_update > BUFFER_SIZE_FRAMES)
	{
		std::cout << "Error: Delay value is too big\n\n";
		return;
	}

	dsp_n_delay_update = i32;
	std::cout << "N DELAY set to: " << dsp_n_delay_update << std::endl << std::endl;
	return;
}

void update_n_delay_cycles(const char *text)
{
	int i32;

	try
	{
		i32 = std::stoi(text);
	}
	catch(...)
	{
		std::cout << "Invalid value entered\n\n";
		return;
	}

	if(i32*dsp_n_delay_update > BUFFER_SIZE_FRAMES)
	{
		std::cout << "Error: Delay Feedback Loops value is too big\n\n";
		return;
	}

	dsp_n_delay_cycles_update = i32;
	std::cout << "N FEEDBACK LOOPS set to: " << dsp_n_delay_cycles_update << std::endl << std::endl;
	return;
}

void update_feedback_pol_alternate(const char *text)
{
	int i32;

	try
	{
		i32 = std::stoi(text);
	}
	catch(...)
	{
		std::cout << "Invalid value entered\n\n";
		return;
	}

	if((i32 != 0) && (i32 != 1))
	{
		std::cout << "Invalid value entered\nAccepted values are \"0\" and/or \"1\"\n\n";
		return;
	}

	dsp_feedback_pol_alternate_update = (i32 & 0x00000001);
	std::cout << "FEEDBACK POL ALTERNATE set to: " << dsp_feedback_pol_alternate_update << std::endl << std::endl;
	return;
}

void update_cycle_div_inc_one(const char *text)
{
	int i32;

	try
	{
		i32 = std::stoi(text);
	}
	catch(...)
	{
		std::cout << "Invalid value entered\n\n";
		return;
	}

	if((i32 != 0) && (i32 != 1))
	{
		std::cout << "Invalid value entered\nAccepted values are \"0\" and/or \"1\"\n\n";
		return;
	}

	dsp_cycle_div_inc_one_update = (i32 & 0x00000001);
	std::cout << "CYCLE DIV INC ONE set to: " << dsp_cycle_div_inc_one_update << std::endl << std::endl;
	return;
}

void delay_us(unsigned long time_us)
{
	std::this_thread::sleep_for(std::chrono::nanoseconds(1024ul*time_us));
	return;
}


//USER CMD
//====================================================================================================

