/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 3.0
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#include "AudioRTDSP.hpp"
#include "cstrdef.h"

#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <iostream>

AudioRTDSP::AudioRTDSP(const audiortdsp_pb_params_t *p_pbparams)
{
	this->setPlaybackParameters(p_pbparams);
}

bool AudioRTDSP::setPlaybackParameters(const audiortdsp_pb_params_t *p_pbparams)
{
	if(this->status > 0)
	{
		this->err_msg = "AudioRTDSP::setPlaybackParameters: Error: audio object is already initialized.";
		return false;
	}

	this->status = this->STATUS_UNINITIALIZED;

	if(p_pbparams == NULL)
	{
		this->err_msg = "AudioRTDSP::setPlaybackParameters: Error: given p_pbparams object is NULL.";
		return false;
	}

	if(p_pbparams->audio_dev_desc == NULL)
	{
		this->err_msg = "AudioRTDSP::setPlaybackParameters: Error: given p_pbparams object: audio_dev_desc is invalid.";
		return false;
	}
	
	if(p_pbparams->filein_dir == NULL)
	{
		this->err_msg = "AudioRTDSP::setPlaybackParameters: Error: given p_pbparams object: filein_dir is invalid.";
		return false;
	}

	this->AUDIODEV_DESC = p_pbparams->audio_dev_desc;
	this->FILEIN_DIR = p_pbparams->filein_dir;
	this->AUDIO_DATA_BEGIN = p_pbparams->audio_data_begin;
	this->AUDIO_DATA_END = p_pbparams->audio_data_end;
	this->SAMPLE_RATE = (size_t) p_pbparams->sample_rate;
	this->N_CHANNELS = (size_t) p_pbparams->n_channels;

	return true;
}

bool AudioRTDSP::initialize(void)
{
	if(this->status > 0) return true;

	this->status = this->STATUS_UNINITIALIZED;

	if(!this->filein_open())
	{
		this->status = this->STATUS_ERROR_NOFILE;
		this->err_msg = "AudioRTDSP::initialize: Error: could not open input file.";
		return false;
	}

	if(!this->audio_hw_init())
	{
		this->filein_close();
		this->status = this->STATUS_ERROR_AUDIOHW;
		return false;
	}

	if(!this->buffer_alloc())
	{
		this->filein_close();
		this->audio_hw_deinit();
		this->status = this->STATUS_ERROR_MEMALLOC;
		this->err_msg = "AudioRTDSP::initialize: Error: memory allocate failed.";
		return false;
	}

	this->status = this->STATUS_READY;
	return true;
}

bool AudioRTDSP::runPlayback(void)
{
	if(this->status != this->STATUS_READY)
	{
		this->err_msg = "AudioRTDSP::runPlayback: Error: audio object is either not ready or already running playback.";
		return false;
	}

	std::cout << "Playback started\n";

	this->userthread = std::thread(&AudioRTDSP::userthread_proc, this);

	this->playback_proc();

	this->wait_all_threads();

	std::cout << "Playback finished\n";

	this->filein_close();
	this->audio_hw_deinit();
	this->buffer_free();

	this->status = this->STATUS_UNINITIALIZED;
	return true;
}

std::string AudioRTDSP::getLastErrorMessage(void)
{
	if(this->status == this->STATUS_UNINITIALIZED)
		return "Error: audio object has not been initialized.\nExtended error message: " + this->err_msg;

	return this->err_msg;
}

void AudioRTDSP::wait_all_threads(void)
{
	cppthread_wait(&(this->playthread));
	cppthread_wait(&(this->userthread));
	return;
}

void AudioRTDSP::stop_all_threads(void)
{
	cppthread_stop(&(this->playthread));
	cppthread_stop(&(this->userthread));
	return;
}

bool AudioRTDSP::filein_open(void)
{
	this->filein_close(); /*Clear any previous file handle*/

	this->h_filein = open(this->FILEIN_DIR.c_str(), O_RDONLY);
	if(this->h_filein < 0) return false;

	this->filein_size = __LSEEK(this->h_filein, 0, SEEK_END);
	return true;
}

void AudioRTDSP::filein_close(void)
{
	if(this->h_filein < 0) return;

	close(this->h_filein);
	this->h_filein = -1;
	this->filein_size = 0;

	return;
}

void AudioRTDSP::audio_hw_deinit(void)
{
	if(this->p_audiodev == NULL) return;

	snd_pcm_drop(this->p_audiodev);
	snd_pcm_close(this->p_audiodev);

	this->p_audiodev = NULL;
	return;
}

void AudioRTDSP::playback_proc(void)
{
	this->playback_init();
	this->playback_loop();

	snd_pcm_drain(this->p_audiodev);
	return;
}

void AudioRTDSP::playback_init(void)
{
	this->bufferout_nseg_load = 0u;
	this->bufferout_nseg_play = 1u;

	this->bufferin_nseg_curr = 0u;

	this->fx_params.n_delay = 240;
	this->fx_params.n_feedback = 20;
	this->fx_params.feedback_altpol = true;
	this->fx_params.cyclediv_incone = true;

	this->stop_playback = false;
	this->filein_pos = this->AUDIO_DATA_BEGIN;

	return;
}

void AudioRTDSP::playback_loop(void)
{
	while(!this->stop_playback)
	{
		this->playthread = std::thread(&AudioRTDSP::playthread_proc, this);
		loadthread_proc();

		cppthread_wait(&(this->playthread));

		this->buffer_segment_update();
	}

	return;
}

void AudioRTDSP::buffer_segment_update(void)
{
	this->bufferin_nseg_curr++;
	this->bufferin_nseg_curr %= this->BUFFERIN_N_SEGMENTS;

	this->bufferout_nseg_load++;
	this->bufferout_nseg_load %= this->BUFFEROUT_N_SEGMENTS;

	this->bufferout_nseg_play++;
	this->bufferout_nseg_play %= this->BUFFEROUT_N_SEGMENTS;

	return;
}

void AudioRTDSP::buffer_play(void)
{
	ssize_t n_ret = 0;

	n_ret = (ssize_t) snd_pcm_writei(this->p_audiodev, this->pp_bufferoutput_segments[this->bufferout_nseg_play], (snd_pcm_uframes_t) this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES);
	if(n_ret < 0)
	{
		if(n_ret == -EPIPE)
		{
			n_ret = (ssize_t) snd_pcm_prepare(this->p_audiodev);
			if(n_ret < 0) app_exit(1, "AudioRTDSP::buffer_play: Error: snd_pcm_prepare failed.");
		}
		else app_exit(1, "AudioRTDSP::buffer_play: Error: snd_pcm_writei failed.");
	}

	return;
}

bool AudioRTDSP::retrieve_previn_nframe(size_t curr_buf_nframe, size_t n_delay, size_t *p_prev_buf_nframe, size_t *p_prev_nseg, size_t *p_prev_seg_nframe)
{
	size_t prev_buf_nframe = 0u;
	size_t prev_nseg = 0u;
	size_t prev_seg_nframe = 0u;

	if(curr_buf_nframe >= this->BUFFERIN_SIZE_FRAMES) return false;
	if(n_delay >= this->BUFFERIN_SIZE_FRAMES) return false;

	if(n_delay > curr_buf_nframe) prev_buf_nframe = this->BUFFERIN_SIZE_FRAMES - (n_delay - curr_buf_nframe);
	else prev_buf_nframe = curr_buf_nframe - n_delay;

	prev_nseg = prev_buf_nframe/(this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES);
	prev_seg_nframe = prev_buf_nframe%(this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES);

	if(p_prev_buf_nframe != NULL) *p_prev_buf_nframe = prev_buf_nframe;
	if(p_prev_nseg != NULL) *p_prev_nseg = prev_nseg;
	if(p_prev_seg_nframe != NULL) *p_prev_seg_nframe = prev_seg_nframe;

	return true;
}

bool AudioRTDSP::retrieve_previn_nframe(size_t curr_nseg, size_t curr_seg_nframe, size_t n_delay, size_t *p_prev_buf_nframe, size_t *p_prev_nseg, size_t *p_prev_seg_nframe)
{
	size_t curr_buf_nframe = 0u;

	if(curr_nseg >= this->BUFFERIN_N_SEGMENTS) return false;
	if(curr_seg_nframe >= this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES) return false;

	curr_buf_nframe = curr_nseg*(this->AUDIOBUFFER_SEGMENT_SIZE_FRAMES) + curr_seg_nframe;

	return this->retrieve_previn_nframe(curr_buf_nframe, n_delay, p_prev_buf_nframe, p_prev_nseg, p_prev_seg_nframe);
}

void AudioRTDSP::cmdui_cmd_decode(void)
{
	const char *cmd = NULL;
	const char *numtext = NULL;

	this->usr_cmd = str_tolower(this->usr_cmd);

	cmd = this->usr_cmd.c_str();

	if(cstr_compare("stop", cmd))
	{
		this->stop_playback = true;
		return;
	}

	if(cstr_compare("params", cmd))
	{
		this->cmdui_print_current_params();
		return;
	}

	if(cstr_compare("help", cmd) || cstr_compare("--help", cmd))
	{
		this->cmdui_print_help_text();
		return;
	}

	if(this->cmdui_cmd_compare("setnd:", cmd, 6u))
	{
		numtext = &cmd[6];
		this->cmdui_attempt_updatevar(numtext, this->UPDATEVAR_NDELAY);
		return;
	}

	if(this->cmdui_cmd_compare("setnf:", cmd, 6u))
	{
		numtext = &cmd[6];
		this->cmdui_attempt_updatevar(numtext, this->UPDATEVAR_NFEEDBACK);
		return;
	}

	if(this->cmdui_cmd_compare("setfpa:", cmd, 7u))
	{
		numtext = &cmd[7];
		this->cmdui_attempt_updatevar(numtext, this->UPDATEVAR_FEEDBACKALTPOL);
		return;
	}

	if(this->cmdui_cmd_compare("setcdi:", cmd, 7u))
	{
		numtext = &cmd[7];
		this->cmdui_attempt_updatevar(numtext, this->UPDATEVAR_CYCLEDIVINCONE);
		return;
	}

	std::cout << "Error: invalid command entered\n";
	return;
}

bool AudioRTDSP::cmdui_cmd_compare(const char *auth, const char *input, size_t stop_index)
{
	if(auth == NULL) return false;
	if(input == NULL) return false;
	if(stop_index >= TEXTBUF_SIZE_CHARS) return false;

	snprintf(textbuf, TEXTBUF_SIZE_CHARS, "%s", input);
	textbuf[stop_index] = '\0';

	return cstr_compare(auth, textbuf);
}

void AudioRTDSP::cmdui_print_help_text(void)
{
	std::cout << "User command list:\n\n";
	std::cout << "\"help\" or \"--help\" : print this list\n";
	std::cout << "\"params\" : print current parameters\n";
	std::cout << "\"setnd:<number>\" : set delay time (in number of samples)\n";
	std::cout << "\"setnf:<number>\" : set number of feedback loops\n";
	std::cout << "\"setfpa:<number>\" : alternate feedback polarity (0 = disable | 1 = enable)\n";
	std::cout << "\"setcdi:<number>\" : set cycle divider increment (0 = exponential | 1 = by one)\n";
	std::cout << "\"stop\" : stop playback and quit application\n\n";

	return;
}

void AudioRTDSP::cmdui_print_current_params(void)
{
	std::cout << "Current parameters:\n\n";
	std::cout << "Delay time (number of samples): " << std::to_string(this->fx_params.n_delay) << std::endl;
	std::cout << "Number of feedback loops: " << std::to_string(this->fx_params.n_feedback) << std::endl;

	std::cout << "Alternate feedback polarity: ";

	if(this->fx_params.feedback_altpol) std::cout << "enabled\n";
	else std::cout << "disabled\n";

	std::cout << "Cycle divider increment: ";

	if(this->fx_params.cyclediv_incone) std::cout << "by one\n\n";
	else std::cout << "exponential\n\n";

	return;
}

bool AudioRTDSP::cmdui_attempt_updatevar(const char *numtext, int updatevar_desc)
{
	int value = 0;

	if(numtext == NULL) return false;

	try
	{
		value = std::stoi(numtext);
	}
	catch(...)
	{
		std::cout << "Error: invalid value entered\n";
		return false;
	}

	switch(updatevar_desc)
	{
		case this->UPDATEVAR_NDELAY:
			if(value < 0)
			{
				std::cout << "Error: invalid value entered\n";
				return false;
			}
			if(((size_t) (value*(this->fx_params.n_feedback + 1))) >= this->BUFFERIN_SIZE_FRAMES)
			{
				std::cout << "Error: delay time value is too big\n";
				return false;
			}

			this->fx_params.n_delay = (int32_t) value;
			break;

		case this->UPDATEVAR_NFEEDBACK:
			if(value < 0)
			{
				std::cout << "Error: invalid value entered\n";
				return false;
			}
			if(((size_t) ((value + 1)*(this->fx_params.n_delay))) >= this->BUFFERIN_SIZE_FRAMES)
			{
				std::cout << "Error: number of feedback loops is too big\n";
				return false;
			}

			this->fx_params.n_feedback = (int32_t) value;
			break;

		case this->UPDATEVAR_FEEDBACKALTPOL:
			if((value < 0) || (value > 1))
			{
				std::cout << "Error: invalid value entered\nValid values are \"0\" and \"1\"\n";
				return false;
			}

			this->fx_params.feedback_altpol = (bool) value;
			break;

		case this->UPDATEVAR_CYCLEDIVINCONE:
			if((value < 0) || (value > 1))
			{
				std::cout << "Error: invalid value entered\nValid values are \"0\" and \"1\"\n";
				return false;
			}

			this->fx_params.cyclediv_incone = (bool) value;
			break;
	}

	this->cmdui_print_current_params();
	return true;
}

void AudioRTDSP::loadthread_proc(void)
{
	this->buffer_load();

	if(this->stop_playback) return;

	this->dsp_proc();

	return;
}

void AudioRTDSP::playthread_proc(void)
{
	this->buffer_play();
	snd_pcm_wait(this->p_audiodev, -1);

	return;
}

void AudioRTDSP::userthread_proc(void)
{
	int n_ret = 0;
	struct pollfd poll_userinput;

	memset(&poll_userinput, 0, sizeof(struct pollfd));

	poll_userinput.fd = STDIN_FILENO;
	poll_userinput.events = POLLIN;

	this->cmdui_print_help_text();
	this->cmdui_print_current_params();

	while(!this->stop_playback)
	{
		n_ret = poll(&poll_userinput, 1, 1);

		if(n_ret > 0)
		{
			this->usr_cmd = "";
			std::cin >> this->usr_cmd;
			this->cmdui_cmd_decode();
		}
	}

	return;
}

