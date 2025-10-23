/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 3.0
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#include "AudioRTDSP_i16.hpp"

#include <stdlib.h>
#include <string.h>

AudioRTDSP_i16::AudioRTDSP_i16(const audiortdsp_pb_params_t *p_pbparams) : AudioRTDSP(p_pbparams)
{
}

AudioRTDSP_i16::~AudioRTDSP_i16(void)
{
	this->stop_playback = true;
	this->stop_all_threads();

	this->status = this->STATUS_UNINITIALIZED;

	this->filein_close();
	this->audio_hw_deinit();
	this->buffer_free();
}

bool AudioRTDSP_i16::audio_hw_init(void)
{
	snd_pcm_hw_params_t *p_hwparams = NULL;
	snd_pcm_uframes_t n_frames = 0u;
	int n_ret = 0;

	this->audio_hw_deinit(); /*Clear any previous instances of the audio device*/

	/*OPEN AUDIO DEVICE*/

	n_ret = snd_pcm_open(&(this->p_audiodev), this->AUDIODEV_DESC.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if(n_ret < 0)
	{
		this->p_audiodev = NULL;
		this->err_msg = "AudioRTDSP_i16::audio_hw_init: Error: snd_pcm_open failed.";
		return false;
	}

	/*ALLOCATE DEVICE PARAMS OBJ*/

	n_ret = snd_pcm_hw_params_malloc(&p_hwparams);
	if((n_ret < 0) || (p_hwparams == NULL))
	{
		this->audio_hw_deinit();
		this->err_msg = "AudioRTDSP_i16::audio_hw_init: Error: snd_pcm_hw_params_malloc failed.";
		return false;
	}

	/*PRELOAD DEVICE PARAMS*/

	n_ret = snd_pcm_hw_params_any(this->p_audiodev, p_hwparams);
	if(n_ret < 0)
	{
		snd_pcm_hw_params_free(p_hwparams);
		this->audio_hw_deinit();
		this->err_msg = "AudioRTDSP_i16::audio_hw_init: Error: snd_pcm_hw_params_any failed.";
		return false;
	}

	/*ENABLE/DISABLE RESAMPLING*/
	/*
	 * Disabled (0) = better performance
	 * Enabled (1) = better compatibility
	 */

	n_ret = snd_pcm_hw_params_set_rate_resample(this->p_audiodev, p_hwparams, 1u);
	if(n_ret < 0)
	{
		snd_pcm_hw_params_free(p_hwparams);
		this->audio_hw_deinit();
		this->err_msg = "AudioRTDSP_i16::audio_hw_init: Error: snd_pcm_hw_params_set_rate_resample failed.";
		return false;
	}

	/*SET DEVICE ACCESS*/

	n_ret = snd_pcm_hw_params_set_access(this->p_audiodev, p_hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(n_ret < 0)
	{
		snd_pcm_hw_params_free(p_hwparams);
		this->audio_hw_deinit();
		this->err_msg = "AudioRTDSP_i16::audio_hw_init: Error: snd_pcm_hw_params_set_access failed.";
		return false;
	}

	/*SET DEVICE FORMAT*/

	n_ret = snd_pcm_hw_params_set_format(this->p_audiodev, p_hwparams, SND_PCM_FORMAT_S16_LE);
	if(n_ret < 0)
	{
		snd_pcm_hw_params_free(p_hwparams);
		this->audio_hw_deinit();
		this->err_msg = "AudioRTDSP_i16::audio_hw_init: Error: snd_pcm_hw_params_set_format failed.";
		return false;
	}

	/*SET DEVICE CHANNELS*/

	n_ret = snd_pcm_hw_params_set_channels(this->p_audiodev, p_hwparams, (unsigned int) this->N_CHANNELS);
	if(n_ret < 0)
	{
		snd_pcm_hw_params_free(p_hwparams);
		this->audio_hw_deinit();
		this->err_msg = "AudioRTDSP_i16::audio_hw_init: Error: snd_pcm_hw_params_set_channels failed.";
		return false;
	}

	/*SET DEVICE SAMPLING RATE*/

	n_ret = snd_pcm_hw_params_set_rate(this->p_audiodev, p_hwparams, (unsigned int) this->SAMPLE_RATE, 0);
	if(n_ret < 0)
	{
		snd_pcm_hw_params_free(p_hwparams);
		this->audio_hw_deinit();
		this->err_msg = "AudioRTDSP_i16::audio_hw_init: Error: snd_pcm_hw_params_set_rate failed.";
		return false;
	}

	/*GET/SET DEVICE BUFFER SIZE*/

	n_ret = snd_pcm_hw_params_get_buffer_size(p_hwparams, &n_frames);
	if(n_ret < 0)
	{
		n_frames = (snd_pcm_uframes_t) _get_closest_power2_ceil(this->SAMPLE_RATE);
		n_ret = snd_pcm_hw_params_set_buffer_size_near(this->p_audiodev, p_hwparams, &n_frames);

		if(n_ret < 0)
		{
			snd_pcm_hw_params_free(p_hwparams);
			this->audio_hw_deinit();
			this->err_msg = "AudioRTDSP_i16::audio_hw_init: Error: snd_pcm_hw_params_set_buffer_size_near failed.";
			return false;
		}
	}

	this->AUDIOBUFFER_SIZE_FRAMES = (size_t) n_frames;
	this->AUDIOBUFFER_SIZE_SAMPLES = (this->AUDIOBUFFER_SIZE_FRAMES)*(this->N_CHANNELS);
	this->AUDIOBUFFER_SIZE_BYTES = this->AUDIOBUFFER_SIZE_SAMPLES*2u;

	/*SET DEVICE BUFFER SEGMENT SIZE (PERIOD SIZE)*/

	this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES = _get_closest_power2_ceil(this->AUDIOBUFFER_SIZE_FRAMES/4u);

	n_ret = snd_pcm_hw_params_set_period_size(this->p_audiodev, p_hwparams, (snd_pcm_uframes_t) this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES, 0);
	if(n_ret < 0)
	{
		snd_pcm_hw_params_free(p_hwparams);
		this->audio_hw_deinit();
		this->err_msg = "AudioRTDSP_i16::audio_hw_init: Error: snd_pcm_hw_params_set_period_size failed.";
		return false;
	}

	/*APPLY SETTINGS TO DEVICE*/

	n_ret = snd_pcm_hw_params(this->p_audiodev, p_hwparams);
	if(n_ret < 0)
	{
		snd_pcm_hw_params_free(p_hwparams);
		this->audio_hw_deinit();
		this->err_msg = "AudioRTDSP_i16::audio_hw_init: Error: snd_pcm_hw_params failed.";
		return false;
	}

	n_ret = snd_pcm_hw_params_get_period_size(p_hwparams, &n_frames, NULL);

	if(n_ret >= 0) this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES = (size_t) n_frames; /*Not really necessary, but just to be safe.*/

	this->AUDIOBUFFER_SEGMENT_SIZE_SAMPLES = (this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES)*(this->N_CHANNELS);
	this->AUDIOBUFFER_SEGMENT_SIZE_BYTES = this->AUDIOBUFFER_SEGMENT_SIZE_SAMPLES*2u;

	this->BUFFEROUT_SIZE_FRAMES = (this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES)*(this->BUFFEROUT_N_SEGMENTS);
	this->BUFFEROUT_SIZE_SAMPLES = (this->BUFFEROUT_SIZE_FRAMES)*(this->N_CHANNELS);
	this->BUFFEROUT_SIZE_BYTES = this->BUFFEROUT_SIZE_SAMPLES*2u;

	this->BUFFERIN_SIZE_SAMPLES = (this->BUFFERIN_SIZE_FRAMES)*(this->N_CHANNELS);
	this->BUFFERIN_SIZE_BYTES = this->BUFFERIN_SIZE_SAMPLES*2u;
	this->BUFFERIN_N_SEGMENTS = (this->BUFFERIN_SIZE_FRAMES)/(this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES);

	this->DSPFRAME_SIZE_BYTES = (this->DSPFRAME_SAMPLE_SIZE_BYTES)*(this->N_CHANNELS);

	snd_pcm_hw_params_free(p_hwparams);
	return true;
}

bool AudioRTDSP_i16::buffer_alloc(void)
{
	size_t n_seg = 0u;

	this->buffer_free(); /*Clear any previous allocations*/

	this->p_bufferinput = malloc(this->BUFFERIN_SIZE_BYTES);
	this->p_bufferoutput = malloc(this->BUFFEROUT_SIZE_BYTES);

	this->pp_bufferinput_segments = (void**) malloc(this->BUFFERIN_N_SEGMENTS*sizeof(void*));
	this->pp_bufferoutput_segments = (void**) malloc(this->BUFFEROUT_N_SEGMENTS*sizeof(void*));

	this->p_dspframe = (int32_t*) malloc(this->DSPFRAME_SIZE_BYTES);

	if(this->p_bufferinput == NULL)
	{
		this->buffer_free();
		return false;
	}

	if(this->p_bufferoutput == NULL)
	{
		this->buffer_free();
		return false;
	}

	if(this->pp_bufferinput_segments == NULL)
	{
		this->buffer_free();
		return false;
	}

	if(this->pp_bufferoutput_segments == NULL)
	{
		this->buffer_free();
		return false;
	}

	if(this->p_dspframe == NULL)
	{
		this->buffer_free();
		return false;
	}

	memset(this->p_bufferinput, 0, this->BUFFERIN_SIZE_BYTES);
	memset(this->p_bufferoutput, 0, this->BUFFEROUT_SIZE_BYTES);
	memset(this->p_dspframe, 0, this->DSPFRAME_SIZE_BYTES);

	for(n_seg = 0u; n_seg < this->BUFFERIN_N_SEGMENTS; n_seg++) this->pp_bufferinput_segments[n_seg] = (void*) (((size_t) this->p_bufferinput) + n_seg*(this->AUDIOBUFFER_SEGMENT_SIZE_BYTES));
	for(n_seg = 0u; n_seg < this->BUFFEROUT_N_SEGMENTS; n_seg++) this->pp_bufferoutput_segments[n_seg] = (void*) (((size_t) this->p_bufferoutput) + n_seg*(this->AUDIOBUFFER_SEGMENT_SIZE_BYTES));

	return true;
}

void AudioRTDSP_i16::buffer_free(void)
{
	if(this->p_bufferinput != NULL)
	{
		free(this->p_bufferinput);
		this->p_bufferinput = NULL;
	}

	if(this->p_bufferoutput != NULL)
	{
		free(this->p_bufferoutput);
		this->p_bufferoutput = NULL;
	}

	if(this->pp_bufferinput_segments != NULL)
	{
		free(this->pp_bufferinput_segments);
		this->pp_bufferinput_segments = NULL;
	}

	if(this->pp_bufferoutput_segments != NULL)
	{
		free(this->pp_bufferoutput_segments);
		this->pp_bufferoutput_segments = NULL;
	}

	if(this->p_dspframe != NULL)
	{
		free(this->p_dspframe);
		this->p_dspframe = NULL;
	}

	return;
}

void AudioRTDSP_i16::buffer_load(void)
{
	if(this->filein_pos >= this->AUDIO_DATA_END)
	{
		this->stop_playback = true;
		return;
	}

	memset(this->pp_bufferinput_segments[this->bufferin_nseg_curr], 0, this->AUDIOBUFFER_SEGMENT_SIZE_BYTES);

	__LSEEK(this->h_filein, this->filein_pos, SEEK_SET);
	read(this->h_filein, this->pp_bufferinput_segments[this->bufferin_nseg_curr], this->AUDIOBUFFER_SEGMENT_SIZE_BYTES);
	this->filein_pos += (__offset) this->AUDIOBUFFER_SEGMENT_SIZE_BYTES;

	return;
}

void AudioRTDSP_i16::dsp_proc(void)
{
	int16_t *p_currin_seg = NULL;
	int16_t *p_loadout_seg = NULL;
	int16_t *p_bufferin = NULL;

	size_t curr_seg_nframe = 0u;
	size_t prev_buf_nframe = 0u;

	size_t n_currsample = 0u;
	size_t n_prevsample = 0u;
	size_t n_channel = 0u;

	int32_t n_cycles = 0;
	int32_t n_cycle = 0;
	int32_t n_reldelay = 0;
	int32_t n_delay = 0;
	int32_t cycle_div = 0;
	int32_t pol = 0;

	bool feedback_altpol = false;
	bool cyclediv_incone = false;

	p_currin_seg = (int16_t*) (this->pp_bufferinput_segments[this->bufferin_nseg_curr]);
	p_loadout_seg = (int16_t*) (this->pp_bufferoutput_segments[this->bufferout_nseg_load]);
	p_bufferin = (int16_t*) (this->p_bufferinput);

	n_reldelay = this->fx_params.n_delay;
	n_cycles = this->fx_params.n_feedback + 1;

	feedback_altpol = this->fx_params.feedback_altpol;
	cyclediv_incone = this->fx_params.cyclediv_incone;

	for(curr_seg_nframe = 0u; curr_seg_nframe < this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES; curr_seg_nframe++)
	{
		n_currsample = curr_seg_nframe*(this->N_CHANNELS);

		for(n_channel = 0u; n_channel < this->N_CHANNELS; n_channel++)
		{
			this->p_dspframe[n_channel] = (int32_t) p_currin_seg[n_currsample];
			n_currsample++;
		}

		pol = 1;
		n_cycle = 1;

		while(n_cycle <= n_cycles)
		{
			if(feedback_altpol) pol ^= 0xfffffffe; /*Toggle between -1 and 1*/

			if(cyclediv_incone) cycle_div = n_cycle + 1;
			else cycle_div = (1 << n_cycle);

			if(!cycle_div) break; /*Stop if cycle_div == 0*/

			n_delay = n_cycle*n_reldelay;

			this->retrieve_previn_nframe(this->bufferin_nseg_curr, curr_seg_nframe, (size_t) n_delay, &prev_buf_nframe, NULL, NULL);

			n_prevsample = prev_buf_nframe*(this->N_CHANNELS);

			for(n_channel = 0u; n_channel < this->N_CHANNELS; n_channel++)
			{
				this->p_dspframe[n_channel] += pol*((int32_t) p_bufferin[n_prevsample])/cycle_div;
				n_prevsample++;
			}

			n_cycle++;
		}

		n_currsample = curr_seg_nframe*(this->N_CHANNELS);
		for(n_channel = 0u; n_channel < this->N_CHANNELS; n_channel++)
		{
			this->p_dspframe[n_channel] /= 2;

			if(this->p_dspframe[n_channel] > this->SAMPLE_MAX_VALUE) p_loadout_seg[n_currsample] = (int16_t) this->SAMPLE_MAX_VALUE;
			else if(this->p_dspframe[n_channel] < this->SAMPLE_MIN_VALUE) p_loadout_seg[n_currsample] = (int16_t) this->SAMPLE_MIN_VALUE;
			else p_loadout_seg[n_currsample] = (int16_t) this->p_dspframe[n_channel];

			n_currsample++;
		}
	}

	return;
}

