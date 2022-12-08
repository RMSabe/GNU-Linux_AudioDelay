#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>
#include <alsa/asoundlib.h>

//Input Audio File Directory
#define AUDIO_FILE_DIR "/media/rafael-user/HDD2/Common/AudioFiles/RAW/Moskau_stereo_44100_16bit.raw"
//Audio Playback Device. Set as "default" to use system default playback device.
#define AUDIO_DEV "plughw:0,0"

//Buffer Size in 16bit samples
#define BUFFER_SIZE_SAMPLES 65536

//Delay time in number of samples. Should not be greater than half buffer size.
#define DSP_N_DELAY 132

//Number of delay feedbacks. Set to 0 for no feedback.
#define DSP_N_FEEDBACK_LOOPS 20

//If defined, enables alternating feedback polarity.
//Else, disables alternating feedback polarity.
#define DSP_FEEDBACK_POL_ALTERNATE

//Select delay cycle divider increase progression
//If defined, cycle divider equals cycle number + 1
//Else, cycle divider increases exponentially
#define DSP_CYCLE_DIV_INC_ONE

//Buffer size in bytes
#define BUFFER_SIZE_BYTES (2*BUFFER_SIZE_SAMPLES)
//Buffer size per channel
#define BUFFER_SIZE_PER_CHANNEL (BUFFER_SIZE_SAMPLES/2)
//Number of delay cycles (First delay + number of delay feedbacks)
#define DSP_N_DELAY_CYCLES (DSP_N_FEEDBACK_LOOPS + 1)

#define SAMPLE_MAX_VALUE (32767)
#define SAMPLE_MIN_VALUE (-32768)

//Threads
std::thread loadthread;
std::thread playthread;

//File variables
std::fstream audio_file;
unsigned int audio_file_size = 0;
unsigned int audio_file_pos = 0;

//Audio Device variables
snd_pcm_t *audio_dev = NULL;
snd_pcm_uframes_t n_frames;
unsigned int audio_buffer_size = 0;
unsigned int buffer_n_div = 1;
short **pp_startpoints = NULL;

//Static buffers
short *buffer_input_0 = NULL;
short *buffer_input_1 = NULL;
short *buffer_output_0 = NULL;
short *buffer_output_1 = NULL;
short *buffer_output_2 = NULL;
short *buffer_output_3 = NULL;
short *buffer_dsp = NULL;

unsigned int curr_buf_cycle = 0;

//Dynamic buffers
short *curr_in = NULL;
short *prev_in = NULL;
short *load_out = NULL;
short *play_out = NULL;

int n_sample = 0;

int n_cycle = 0;
int n_delay = 0;
int cycle_div = 0;
int pol = 0;
int l_sample = 0;
int r_sample = 0;

bool stop = false;

void update_buf_cycle(void)
{
	if(curr_buf_cycle == 3) curr_buf_cycle = 0;
	else curr_buf_cycle++;

	return;
}

void buffer_remap(void)
{
	switch(curr_buf_cycle)
	{
		case 0:
			curr_in = buffer_input_0;
			prev_in = buffer_input_1;
			load_out = buffer_output_0;
			play_out = buffer_output_2;
			break;

		case 1:
			curr_in = buffer_input_1;
			prev_in = buffer_input_0;
			load_out = buffer_output_1;
			play_out = buffer_output_3;
			break;

		case 2:
			curr_in = buffer_input_0;
			prev_in = buffer_input_1;
			load_out = buffer_output_2;
			play_out = buffer_output_0;
			break;

		case 3:
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
	if(audio_file_pos >= audio_file_size)
	{
		stop = true;
		return;
	}

	audio_file.seekg(audio_file_pos);
	audio_file.read((char*) curr_in, BUFFER_SIZE_BYTES);
	audio_file_pos += BUFFER_SIZE_BYTES;

	return;
}

void buffer_play(void)
{
	unsigned int n_div = 0;
	int n_return = 0;

	while(n_div < buffer_n_div)
	{
		n_return = snd_pcm_writei(audio_dev, pp_startpoints[n_div], n_frames);
		if(n_return == -EPIPE) snd_pcm_prepare(audio_dev);

		n_div++;
	}

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
		load_out[2*n_sample] = ((curr_in[2*n_sample]) + (buffer_dsp[2*n_sample]))/2;
		load_out[2*n_sample + 1] = ((curr_in[2*n_sample + 1]) + (buffer_dsp[2*n_sample + 1]))/2;

		n_sample++;
	}

	return;
}

void load_startpoints(void)
{
	pp_startpoints[0] = play_out;
	unsigned int n_div = 1;
	while(n_div < buffer_n_div)
	{
		pp_startpoints[n_div] = &play_out[n_div*audio_buffer_size];
		n_div++;
	}

	return;
}

void loadthread_proc(void)
{
	buffer_load();
	run_dsp();
	update_buf_cycle();
	return;
}

void playthread_proc(void)
{
	load_startpoints();
	buffer_play();
	return;
}

void buffer_preload(void)
{
	curr_buf_cycle = 0;
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

void playback(void)
{
	buffer_preload();
	while(!stop)
	{
		playthread = std::thread(playthread_proc);
		loadthread = std::thread(loadthread_proc);
		loadthread.join();
		playthread.join();
		buffer_remap();
	}

	return;
}

void buffer_malloc(void)
{
	audio_buffer_size = 2*n_frames;
	if(audio_buffer_size < BUFFER_SIZE_SAMPLES) buffer_n_div = BUFFER_SIZE_SAMPLES/audio_buffer_size;
	else buffer_n_div = 1;
	
	pp_startpoints = (short**) malloc(buffer_n_div*sizeof(short*));

	buffer_input_0 = (short*) malloc(BUFFER_SIZE_BYTES);
	buffer_input_1 = (short*) malloc(BUFFER_SIZE_BYTES);
	buffer_output_0 = (short*) malloc(BUFFER_SIZE_BYTES);
	buffer_output_1 = (short*) malloc(BUFFER_SIZE_BYTES);
	buffer_output_2 = (short*) malloc(BUFFER_SIZE_BYTES);
	buffer_output_3 = (short*) malloc(BUFFER_SIZE_BYTES);
	buffer_dsp = (short*) malloc(BUFFER_SIZE_BYTES);
	
	memset(buffer_input_0, 0, BUFFER_SIZE_BYTES);
	memset(buffer_input_1, 0, BUFFER_SIZE_BYTES);
	memset(buffer_output_0, 0, BUFFER_SIZE_BYTES);
	memset(buffer_output_1, 0, BUFFER_SIZE_BYTES);
	memset(buffer_output_2, 0, BUFFER_SIZE_BYTES);
	memset(buffer_output_3, 0, BUFFER_SIZE_BYTES);
	memset(buffer_dsp, 0, BUFFER_SIZE_BYTES);

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

	return;
}

bool open_audio_file(void)
{
	audio_file.open(AUDIO_FILE_DIR, std::ios_base::in);
	if(audio_file.is_open())
	{
		audio_file.seekg(0, audio_file.end);
		audio_file_size = audio_file.tellg();
		audio_file_pos = 0;
		audio_file.seekg(audio_file_pos);
		return true;
	}

	return false;
}

bool audio_hw_init(void)
{
	int n_return = 0;
	snd_pcm_hw_params_t *hw_params = NULL;

	n_return = snd_pcm_open(&audio_dev, AUDIO_DEV, SND_PCM_STREAM_PLAYBACK, 0);
	if(n_return < 0)
	{
		std::cout << "Error opening audio device\n";
		return false;
	}

	snd_pcm_hw_params_malloc(&hw_params);
	snd_pcm_hw_params_any(audio_dev, hw_params);

	n_return = snd_pcm_hw_params_set_access(audio_dev, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(n_return < 0)
	{
		std::cout << "Error setting access to read/write interleaved\n";
		return false;
	}

	n_return = snd_pcm_hw_params_set_format(audio_dev, hw_params, SND_PCM_FORMAT_S16_LE);
	if(n_return < 0)
	{
		std::cout << "Error setting format to signed 16bit little-endian\n";
		return false;
	}

	n_return = snd_pcm_hw_params_set_channels(audio_dev, hw_params, 2);
	if(n_return < 0)
	{
		std::cout << "Error setting channels to stereo\n";
		return false;
	}

	unsigned int sample_rate = 44100;
	n_return = snd_pcm_hw_params_set_rate_near(audio_dev, hw_params, &sample_rate, 0);
	if(n_return < 0 || sample_rate < 44100)
	{
		std::cout << "Error setting sample rate to 44100 Hz\nAttempting to set sample rate to 48000 Hz\n";
		sample_rate = 48000;
		n_return = snd_pcm_hw_params_set_rate_near(audio_dev, hw_params, &sample_rate, 0);
		if(n_return < 0 || sample_rate < 48000)
		{
			std::cout << "Error setting sample rate\n";
			return false;
		}
	}

	n_return = snd_pcm_hw_params(audio_dev, hw_params);
	if(n_return < 0)
	{
		std::cout << "Error setting hardware parameters\n";
		return false;
	}

	snd_pcm_hw_params_get_period_size(hw_params, &n_frames, 0);
	snd_pcm_hw_params_free(hw_params);
	return true;
}

int main(int argc, char **argv)
{
	if(!audio_hw_init())
	{
		std::cout << "Error code: " << errno << "\nTerminated\n";
		return 0;
	}
	std::cout << "Audio hardware initialized\n";

	if(!open_audio_file())
	{
		std::cout << "Error opening audio file\nError code: " << errno << "\nTerminated\n";
		return 0;
	}
	std::cout << "Audio file is open\n";

	buffer_malloc();

	std::cout << "Playback started\n";
	playback();
	std::cout << "Playback finished\n";

	audio_file.close();
	snd_pcm_drain(audio_dev);
	snd_pcm_close(audio_dev);
	buffer_free();

	std::cout << "Terminated\n";
	return 0;
}
