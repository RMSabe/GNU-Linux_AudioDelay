/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 2.5
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#include "AudioRTDSP_i16.hpp"

#include <string.h>
#include <iostream>

AudioRTDSP_i16::AudioRTDSP_i16(const audio_rtdsp_params_t *p_params) : AudioBaseClass(p_params)
{
}

AudioRTDSP_i16::~AudioRTDSP_i16(void)
{
	this->stop_all_threads();

	this->filein_close();
	this->audio_hw_deinit();
	this->buffer_free();

	this->status = this->STATUS_UNINITIALIZED;
}

bool AudioRTDSP_i16::audio_hw_init(void)
{
	snd_pcm_hw_params_t *p_hwparams = NULL;
	snd_pcm_uframes_t n_frames = 0u;
	int n_ret = -1;
	uint32_t rate = 0u;

	n_ret = snd_pcm_open(&this->p_audiodev, this->audio_dev_desc.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
	if(n_ret < 0)
	{
		this->error_msg = "AUDIO HW INIT ERROR: Could not open audio device.";
		return false;
	}

	snd_pcm_hw_params_malloc(&p_hwparams);
	snd_pcm_hw_params_any(this->p_audiodev, p_hwparams);

	n_ret = snd_pcm_hw_params_set_access(this->p_audiodev, p_hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(n_ret < 0)
	{
		this->error_msg = "AUDIO HW INIT ERROR: Could not set device access to read/write interleaved.";
		snd_pcm_hw_params_free(p_hwparams);
		return false;
	}

	n_ret = snd_pcm_hw_params_set_format(this->p_audiodev, p_hwparams, SND_PCM_FORMAT_S16_LE);
	if(n_ret < 0)
	{
		this->error_msg = "AUDIO HW INIT ERROR: Could not set device format to signed 16-bit little-endian.";
		snd_pcm_hw_params_free(p_hwparams);
		return false;
	}

	n_ret = snd_pcm_hw_params_set_channels(this->p_audiodev, p_hwparams, this->n_channels);
	if(n_ret < 0)
	{
		this->error_msg = "AUDIO HW INIT ERROR: Could not set device channels to " + std::to_string(this->n_channels) + ".";
		snd_pcm_hw_params_free(p_hwparams);
		return false;
	}

	rate = this->sample_rate;
	n_ret = snd_pcm_hw_params_set_rate_near(this->p_audiodev, p_hwparams, &rate, 0);
	if((n_ret < 0) || (rate < this->sample_rate))
	{
		this->error_msg = "AUDIO HW INIT ERROR: Could not set device sampling rate to " + std::to_string(this->sample_rate) + " Hz.";
		snd_pcm_hw_params_free(p_hwparams);
		return false;
	}

	n_frames = (snd_pcm_uframes_t) this->AUDIOBUFFER_SIZE_FRAMES_PRESET;
	n_ret = snd_pcm_hw_params_set_period_size_near(this->p_audiodev, p_hwparams, &n_frames, NULL);

	if((n_ret < 0) || (n_frames != ((snd_pcm_uframes_t) this->AUDIOBUFFER_SIZE_FRAMES_PRESET)))
		std::cout << "AUDIO HW: Warning: Buffer size preset failed. Latency might occur.\n";

	n_ret = snd_pcm_hw_params(this->p_audiodev, p_hwparams);
	if(n_ret < 0)
	{
		this->error_msg = "AUDIO HW INIT ERROR: Could not validate device settings.";
		snd_pcm_hw_params_free(p_hwparams);
		return false;
	}

	snd_pcm_hw_params_get_period_size(p_hwparams, &n_frames, 0);

	this->BUFFERIN_SIZE_SAMPLES = this->BUFFERIN_SIZE_FRAMES*((size_t) this->n_channels);
	this->BUFFERIN_SIZE_BYTES = this->BUFFERIN_SIZE_SAMPLES*2u;

	this->DSPFRAME_SIZE_BYTES = this->DSPFRAME_SAMPLE_SIZE_BYTES*((size_t) this->n_channels);

	this->AUDIOBUFFER_SIZE_FRAMES = (size_t) n_frames;
	this->AUDIOBUFFER_SIZE_SAMPLES = this->AUDIOBUFFER_SIZE_FRAMES*((size_t) this->n_channels);
	this->AUDIOBUFFER_SIZE_BYTES = this->AUDIOBUFFER_SIZE_SAMPLES*2u;

	this->BUFFEROUT_SIZE_FRAMES = this->AUDIOBUFFER_SIZE_FRAMES*this->BUFFEROUT_N_SEGMENTS;
	this->BUFFEROUT_SIZE_SAMPLES = this->BUFFEROUT_SIZE_FRAMES*((size_t) this->n_channels);
	this->BUFFEROUT_SIZE_BYTES = this->BUFFEROUT_SIZE_SAMPLES*2u;

	this->BUFFERIN_N_SEGMENTS = this->BUFFERIN_SIZE_FRAMES/this->AUDIOBUFFER_SIZE_FRAMES;

	snd_pcm_hw_params_free(p_hwparams);
	return true;
}

bool AudioRTDSP_i16::buffer_alloc(void)
{
	size_t n_seg = 0u;

	this->buffer_free(); /*Clear any previous allocations*/

	this->p_bufferin = std::malloc(this->BUFFERIN_SIZE_BYTES);
	this->p_bufferout = std::malloc(this->BUFFEROUT_SIZE_BYTES);

	this->pp_bufferin_segments = (void**) std::malloc(this->BUFFERIN_N_SEGMENTS*sizeof(void*));
	this->pp_bufferout_segments = (void**) std::malloc(this->BUFFEROUT_N_SEGMENTS*sizeof(void*));

	this->p_dspframe = (int32_t*) std::malloc(this->DSPFRAME_SIZE_BYTES);

	if(this->p_bufferin == NULL)
	{
		this->buffer_free();
		return false;
	}

	if(this->p_bufferout == NULL)
	{
		this->buffer_free();
		return false;
	}

	if(this->pp_bufferin_segments == NULL)
	{
		this->buffer_free();
		return false;
	}

	if(this->pp_bufferout_segments == NULL)
	{
		this->buffer_free();
		return false;
	}

	if(this->p_dspframe == NULL)
	{
		this->buffer_free();
		return false;
	}

	memset(this->p_bufferin, 0, this->BUFFERIN_SIZE_BYTES);
	memset(this->p_bufferout, 0, this->BUFFEROUT_SIZE_BYTES);
	memset(this->p_dspframe, 0, this->DSPFRAME_SIZE_BYTES);

	for(n_seg = 0u; n_seg < this->BUFFERIN_N_SEGMENTS; n_seg++) this->pp_bufferin_segments[n_seg] = &((char*) this->p_bufferin)[n_seg*this->AUDIOBUFFER_SIZE_BYTES];
	for(n_seg = 0u; n_seg < this->BUFFEROUT_N_SEGMENTS; n_seg++) this->pp_bufferout_segments[n_seg] = &((char*) this->p_bufferout)[n_seg*this->AUDIOBUFFER_SIZE_BYTES];

	return true;
}

void AudioRTDSP_i16::buffer_free(void)
{
	if(this->p_bufferin != NULL)
	{
		std::free(this->p_bufferin);
		this->p_bufferin = NULL;
	}

	if(this->p_bufferout != NULL)
	{
		std::free(this->p_bufferout);
		this->p_bufferout = NULL;
	}

	if(this->pp_bufferin_segments != NULL)
	{
		std::free(this->pp_bufferin_segments);
		this->pp_bufferin_segments = NULL;
	}

	if(this->pp_bufferout_segments != NULL)
	{
		std::free(this->pp_bufferout_segments);
		this->pp_bufferout_segments = NULL;
	}

	if(this->p_dspframe != NULL)
	{
		std::free(this->p_dspframe);
		this->p_dspframe = NULL;
	}

	return;
}

void AudioRTDSP_i16::buffer_load(void)
{
	if(this->filein_pos >= this->audio_data_end)
	{
		this->stop = true;
		return;
	}

	memset(this->pp_bufferin_segments[this->bufferin_nsegment_curr], 0, this->AUDIOBUFFER_SIZE_BYTES);

	__LSEEK(this->h_filein, this->filein_pos, SEEK_SET);
	read(this->h_filein, this->pp_bufferin_segments[this->bufferin_nsegment_curr], this->AUDIOBUFFER_SIZE_BYTES);
	this->filein_pos += (__offset) this->AUDIOBUFFER_SIZE_BYTES;

	return;
}

void AudioRTDSP_i16::dsp_proc(void)
{
	int16_t *currin_seg = NULL;
	int16_t *previn_seg = NULL;
	int16_t *loadout_seg = NULL;

	size_t previn_nseg = 0u;

	size_t currin_seg_nframe = 0u;
	size_t previn_seg_nframe = 0u;

	size_t n_currsample = 0u;
	size_t n_prevsample = 0u;
	size_t n_channel = 0u;

	int32_t n_cycles = 0;
	int32_t n_cycle = 0;
	int32_t n_reldelay = 0;
	int32_t n_delay = 0;
	int32_t cycle_div = 0;
	int32_t pol = 0;

	bool feedback_alt_pol = false;
	bool cyclediv_inc_one = false;

	currin_seg = (int16_t*) this->pp_bufferin_segments[this->bufferin_nsegment_curr];
	loadout_seg = (int16_t*) this->pp_bufferout_segments[this->bufferout_nsegment_load];

	n_reldelay = (int32_t) this->rtdsp_var.n_delay;
	n_cycles = (int32_t) (this->rtdsp_var.n_feedback_loops + 1);

	feedback_alt_pol = this->rtdsp_var.feedback_alt_pol;
	cyclediv_inc_one = this->rtdsp_var.cyclediv_inc_one;

	for(currin_seg_nframe = 0u; currin_seg_nframe < this->AUDIOBUFFER_SIZE_FRAMES; currin_seg_nframe++)
	{
		for(n_channel = 0u; n_channel < ((size_t) this->n_channels); n_channel++)
		{
			n_currsample = currin_seg_nframe*((size_t) this->n_channels) + n_channel;
			this->p_dspframe[n_channel] = (int32_t) currin_seg[n_currsample];
		}

		pol = 1;
		n_cycle = 1;

		while(n_cycle <= n_cycles)
		{
			if(feedback_alt_pol) pol ^= 0xfffffffe; /*Toggle between -1 and 1*/

			if(cyclediv_inc_one) cycle_div = n_cycle + 1;
			else cycle_div = (1 << n_cycle);

			if(!cycle_div) break; /*Stop if cycle_div == 0*/

			n_delay = n_cycle*n_reldelay;

			this->retrieve_previn_nframe(this->bufferin_nsegment_curr, currin_seg_nframe, (size_t) n_delay, NULL, &previn_nseg, &previn_seg_nframe);

			previn_seg = (int16_t*) this->pp_bufferin_segments[previn_nseg];

			for(n_channel = 0u; n_channel < ((size_t) this->n_channels); n_channel++)
			{
				n_prevsample = previn_seg_nframe*((size_t) this->n_channels) + n_channel;
				this->p_dspframe[n_channel] += pol*((int32_t) previn_seg[n_prevsample])/cycle_div;
			}

			n_cycle++;
		}

		for(n_channel = 0u; n_channel < ((size_t) this->n_channels); n_channel++)
		{
			n_currsample = currin_seg_nframe*((size_t) this->n_channels) + n_channel;

			this->p_dspframe[n_channel] /= 2;

			if(this->p_dspframe[n_channel] > this->SAMPLE_MAX_VALUE) loadout_seg[n_currsample] = (int16_t) this->SAMPLE_MAX_VALUE;
			else if(this->p_dspframe[n_channel] < this->SAMPLE_MIN_VALUE) loadout_seg[n_currsample] = (int16_t) this->SAMPLE_MIN_VALUE;
			else loadout_seg[n_currsample] = (int16_t) p_dspframe[n_channel];
		}
	}

	return;
}

