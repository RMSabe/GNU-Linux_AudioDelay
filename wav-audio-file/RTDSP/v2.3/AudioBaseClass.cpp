/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 2.3
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */


#include "AudioBaseClass.hpp"

#include "delay.h"
#include "cppdelay.hpp"

#include "cstrdef.h"
#include "strdef.hpp"

#include <poll.h>
#include <string.h>

#include <iostream>

AudioBaseClass::AudioBaseClass(const audio_rtdsp_params_t *p_params)
{
	this->setParameters(p_params);
}

bool AudioBaseClass::setParameters(const audio_rtdsp_params_t *p_params)
{
	if(p_params == nullptr) return false;
	if(p_params->audio_dev_desc == nullptr) return false;
	if(p_params->filein_dir == nullptr) return false;

	this->audio_dev_desc = p_params->audio_dev_desc;
	this->filein_dir = p_params->filein_dir;
	this->audio_data_begin = p_params->audio_data_begin;
	this->audio_data_end = p_params->audio_data_end;
	this->sample_rate = p_params->sample_rate;
	this->n_channels = p_params->n_channels;

	return true;
}

bool AudioBaseClass::initialize(void)
{
	if(!this->filein_open())
	{
		this->status = this->STATUS_ERROR_NOFILE;
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
		return false;
	}

	this->status = this->STATUS_INITIALIZED;
	return true;
}

bool AudioBaseClass::runDSP(void)
{
	if(this->status < 1) return false;

	std::cout << "Playback started\n\n";

	this->playback_proc();

	std::cout << "Playback finished\n\n";

	this->wait_all_threads(); //Wait joinable threads
	this->stop_all_threads(); //Stop detached threads

	this->filein_close();
	this->audio_hw_deinit();
	this->buffer_free();

	this->status = this->STATUS_UNINITIALIZED;
	return true;
}

std::string AudioBaseClass::getLastErrorMessage(void)
{
	switch(this->status)
	{
		case this->STATUS_ERROR_MEMALLOC:
			return "Error: memory allocation failed.";

		case this->STATUS_ERROR_NOFILE:
			return "Error: could not open input audio file.";

		case this->STATUS_ERROR_AUDIOHW:
			return "Error: audio hardware initialize failed.\nExtended error message: " + this->error_msg;

		case this->STATUS_ERROR_GENERIC:
			return "Error: something went wrong.\nExtended error message: " + this->error_msg;

		case this->STATUS_UNINITIALIZED:
			return "Error: audio object not initialized.";
	}

	return this->error_msg;
}

bool AudioBaseClass::filein_open(void)
{
	this->h_filein = open(this->filein_dir.c_str(), O_RDONLY);
	if(this->h_filein < 0) return false;

	this->filein_size = __LSEEK(this->h_filein, 0, SEEK_END);
	return true;
}

void AudioBaseClass::filein_close(void)
{
	if(this->h_filein < 0) return;

	close(this->h_filein);
	this->h_filein = -1;
	this->filein_size = 0;
	return;
}

void AudioBaseClass::audio_hw_deinit(void)
{
	if(this->p_audiodev == nullptr) return;

	snd_pcm_drain(this->p_audiodev);
	snd_pcm_close(this->p_audiodev);

	this->p_audiodev = nullptr;
	return;
}

void AudioBaseClass::wait_all_threads(void)
{
	this->wait_thread(this->loadthread, nullptr);
	this->wait_thread(this->playthread, nullptr);
	this->wait_thread(this->userthread, nullptr);

	return;
}

bool AudioBaseClass::wait_thread(pthread_t thread, void **value_ptr)
{
	pthread_attr_t thread_attr;
	int thread_detach_state = 0;

	if(!pthread_getattr_np(thread, &thread_attr)) return false;
	if(!pthread_attr_getdetachstate(&thread_attr, &thread_detach_state)) return false;

	if(thread_detach_state != PTHREAD_CREATE_JOINABLE) return false;

	pthread_join(thread, value_ptr);
	return true;
}

void AudioBaseClass::stop_all_threads(void)
{
	this->stop_thread(this->loadthread);
	this->stop_thread(this->playthread);
	this->stop_thread(this->userthread);

	return;
}

void AudioBaseClass::stop_thread(pthread_t thread)
{
	pthread_attr_t thread_attr;
	int thread_detach_state = 0;

	pthread_getattr_np(thread, &thread_attr);
	pthread_attr_getdetachstate(&thread_attr, &thread_detach_state);

	if(thread_detach_state == PTHREAD_CREATE_JOINABLE) pthread_detach(thread);

	pthread_cancel(thread);
	return;
}

void AudioBaseClass::playback_proc(void)
{
	this->playback_init();
	this->playback_loop();
	return;
}

void AudioBaseClass::playback_init(void)
{
	this->bufferout_nsegment_load = 1u;
	this->bufferout_nsegment_play = 0u;

	this->bufferin_nsegment_curr = 0u;

	this->rtdsp_var.n_delay = 240;
	this->rtdsp_var.n_feedback_loops = 20;
	this->rtdsp_var.feedback_alt_pol = true;
	this->rtdsp_var.cyclediv_inc_one = true;

	this->stop = false;
	this->filein_pos = this->audio_data_begin;

	pthread_create(&this->userthread, NULL, ((void* (*)(void*)) &AudioBaseClass::userthread_proc), this);
	return;
}

void AudioBaseClass::playback_loop(void)
{
	while(!this->stop)
	{
		pthread_create(&this->playthread, NULL, ((void* (*)(void*)) &AudioBaseClass::playthread_proc), this);
		pthread_create(&this->loadthread, NULL, ((void* (*)(void*)) &AudioBaseClass::loadthread_proc), this);
		pthread_join(this->loadthread, NULL);
		pthread_join(this->playthread, NULL);

		this->buffer_segment_remap();
	}

	return;
}

void AudioBaseClass::buffer_segment_remap(void)
{
	this->bufferin_nsegment_curr++;
	if(this->bufferin_nsegment_curr >= this->N_BUFFERIN_SEGMENTS) this->bufferin_nsegment_curr = 0u;

	this->bufferout_nsegment_load++;
	if(this->bufferout_nsegment_load >= this->N_BUFFEROUT_SEGMENTS) this->bufferout_nsegment_load = 0u;

	this->bufferout_nsegment_play++;
	if(this->bufferout_nsegment_play >= this->N_BUFFEROUT_SEGMENTS) this->bufferout_nsegment_play = 0u;

	return;
}

void AudioBaseClass::buffer_play(void)
{
	int n_ret = 0;

	n_ret = snd_pcm_writei(this->p_audiodev, this->pp_bufferout_segments[this->bufferout_nsegment_play], this->AUDIOBUFFER_SIZE_FRAMES);
	if(n_ret == -EPIPE) snd_pcm_prepare(this->p_audiodev);

	return;
}

bool AudioBaseClass::retrieve_previn_nframe(size_t currin_buf_nframe, size_t n_delay, size_t *p_previn_buf_nframe, size_t *p_previn_nseg, size_t *p_previn_seg_nframe)
{
	size_t previn_buf_nframe = 0u;
	size_t previn_nseg = 0u;
	size_t previn_seg_nframe = 0u;

	if(currin_buf_nframe >= this->BUFFERIN_SIZE_FRAMES) return false;

	if(n_delay > currin_buf_nframe) previn_buf_nframe = this->BUFFERIN_SIZE_FRAMES - (n_delay - currin_buf_nframe);
	else previn_buf_nframe = currin_buf_nframe - n_delay;

	previn_nseg = previn_buf_nframe/this->AUDIOBUFFER_SIZE_FRAMES;
	previn_seg_nframe = previn_buf_nframe%this->AUDIOBUFFER_SIZE_FRAMES;

	if(p_previn_buf_nframe != nullptr) *p_previn_buf_nframe = previn_buf_nframe;
	if(p_previn_nseg != nullptr) *p_previn_nseg = previn_nseg;
	if(p_previn_seg_nframe != nullptr) *p_previn_seg_nframe = previn_seg_nframe;

	return true;
}

bool AudioBaseClass::retrieve_previn_nframe(size_t currin_nseg, size_t currin_seg_nframe, size_t n_delay, size_t *p_previn_buf_nframe, size_t *p_previn_nseg, size_t *p_previn_seg_nframe)
{
	size_t currin_buf_nframe = 0u;

	if(currin_nseg >= this->N_BUFFERIN_SEGMENTS) return false;
	if(currin_seg_nframe >= this->AUDIOBUFFER_SIZE_FRAMES) return false;

	currin_buf_nframe = currin_nseg*this->AUDIOBUFFER_SIZE_FRAMES + currin_seg_nframe;

	return this->retrieve_previn_nframe(currin_buf_nframe, n_delay, p_previn_buf_nframe, p_previn_nseg, p_previn_seg_nframe);
}

void *AudioBaseClass::loadthread_proc(void *args)
{
	this->buffer_load();

	if(this->stop) return nullptr;

	this->dsp_proc();

	return nullptr;
}

void *AudioBaseClass::playthread_proc(void *args)
{
	this->buffer_play();
	return nullptr;
}

void *AudioBaseClass::userthread_proc(void *args)
{
	int n_ret = 0;
	struct pollfd poll_userinput;

	poll_userinput.fd = STDIN_FILENO;
	poll_userinput.events = POLLIN;

	this->print_help_text();
	this->print_current_params();

	while(!this->stop)
	{
		n_ret = poll(&poll_userinput, 1, 1);

		if(n_ret > 0)
		{
			this->user_cmd = "";
			std::cin >> this->user_cmd;
			this->cmd_decode();
		}
	}

	return nullptr;
}

void AudioBaseClass::cmd_decode(void)
{
	const char *cmd = nullptr;
	const char *numtext = nullptr;

	this->user_cmd = str_tolower(this->user_cmd);

	cmd = this->user_cmd.c_str();

	if(cstr_compare("stop", cmd)) this->stop = true;
	else if(cstr_compare("params", cmd)) this->print_current_params();
	else if(cstr_compare("help", cmd) || cstr_compare("--help", cmd)) this->print_help_text();
	else if(this->cmd_compare("setnd:", cmd, 6u))
	{
		numtext = &cmd[6];
		this->attempt_updatevar(numtext, this->UPDATEVAR_NDELAY);
	}
	else if(this->cmd_compare("setnf:", cmd, 6u))
	{
		numtext = &cmd[6];
		this->attempt_updatevar(numtext, this->UPDATEVAR_NFEEDBACKLOOPS);
	}
	else if(this->cmd_compare("setfpa:", cmd, 7u))
	{
		numtext = &cmd[7];
		this->attempt_updatevar(numtext, this->UPDATEVAR_FEEDBACKALTPOL);
	}
	else if(this->cmd_compare("setcdi:", cmd, 7u))
	{
		numtext = &cmd[7];
		this->attempt_updatevar(numtext, this->UPDATEVAR_CYCLEDIVINCONE);
	}
	else std::cout << "Error: invalid command entered\n\n";

	return;
}

bool AudioBaseClass::cmd_compare(const char *auth, const char *input, size_t stop_index)
{
	if(auth == nullptr) return false;
	if(input == nullptr) return false;
	if(stop_index >= TEXTBUF_SIZE_CHARS) return false;

	snprintf(textbuf, TEXTBUF_SIZE_CHARS, "%s", input);

	textbuf[stop_index] = '\0';

	return cstr_compare(auth, textbuf);
}

void AudioBaseClass::print_help_text(void)
{
	std::cout << "User Command List:\n\n";
	std::cout << "\"help\" or \"--help\" : print this list\n";
	std::cout << "\"params\" : print current parameters\n";
	std::cout << "\"setnd:<number>\" : set delay time (in number of samples)\n";
	std::cout << "\"setnf:<number>\" : set number of feedback loops\n";
	std::cout << "\"setfpa:<number>\" : alternate feedback polarity (0 = disable | 1 = enable)\n";
	std::cout << "\"setcdi:<number>\" : set cycle divider increment (0 = increment exponentially | 1 = increment by one)\n";
	std::cout << "\"stop\" : stop playback and quit application\n\n";

	return;
}

void AudioBaseClass::print_current_params(void)
{
	std::cout << "Current parameters:\n\n";

	std::cout << "Delay time (number of samples): " << std::to_string(this->rtdsp_var.n_delay) << std::endl;
	std::cout << "Number of feedback loops: " << std::to_string(this->rtdsp_var.n_feedback_loops) << std::endl;

	std::cout << "Alternate feedback polarity: ";

	if(this->rtdsp_var.feedback_alt_pol) std::cout << "enabled\n";
	else std::cout << "disabled\n";

	std::cout << "Cycle divider increment: ";

	if(this->rtdsp_var.cyclediv_inc_one) std::cout << "by one\n\n";
	else std::cout << "exponential\n\n";

	return;
}

bool AudioBaseClass::attempt_updatevar(const char *numtext, int updatevar_desc)
{
	int value = 0;

	if(numtext == nullptr) return false;

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
				std::cout << "Error: invalid value entered.\n";
				return false;
			}
			if((value*(this->rtdsp_var.n_feedback_loops + 1)) >= this->BUFFERIN_SIZE_FRAMES)
			{
				std::cout << "Error: delay time value is too big.\n";
				return false;
			}

			this->rtdsp_var.n_delay = value;
			break;

		case this->UPDATEVAR_NFEEDBACKLOOPS:
			if(value < 0)
			{
				std::cout << "Error: invalid value entered.\n";
				return false;
			}
			if(((value + 1)*this->rtdsp_var.n_delay) >= this->BUFFERIN_SIZE_FRAMES)
			{
				std::cout << "Error: number of feedback loops is too big.\n";
				return false;
			}

			this->rtdsp_var.n_feedback_loops = value;
			break;

		case this->UPDATEVAR_FEEDBACKALTPOL:
			if((value < 0) || (value > 1))
			{
				std::cout << "Error: invalid value entered.\nValid values are \"0\" and \"1\"\n";
				return false;
			}

			this->rtdsp_var.feedback_alt_pol = (bool) value;
			break;

		case this->UPDATEVAR_CYCLEDIVINCONE:
			if((value < 0) || (value > 1))
			{
				std::cout << "Error: invalid value entered.\nValid values are \"0\" and \"1\"\n";
				return false;
			}

			this->rtdsp_var.cyclediv_inc_one = (bool) value;
			break;
	}

	this->print_current_params();
	return true;
}

