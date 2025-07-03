/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 2.5
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#ifndef AUDIORTDSP_I16_HPP
#define AUDIORTDSP_I16_HPP

#include "AudioBaseClass.hpp"

class AudioRTDSP_i16 : public AudioBaseClass {
	public:
		AudioRTDSP_i16(const audio_rtdsp_params_t *p_params);
		~AudioRTDSP_i16(void);

	protected:
		const int32_t SAMPLE_MAX_VALUE = 0x7fff;
		const int32_t SAMPLE_MIN_VALUE = -0x8000;

		//DSPFRAME is a buffer meant to store a single frame of audio
		//with a sample size usually bigger than the normal sample size.
		//This is where most math operations happens, and it's done to prevent
		//Integer overflow on regular sized samples.

		const size_t DSPFRAME_SAMPLE_SIZE_BYTES = 4u;
		size_t DSPFRAME_SIZE_BYTES = 0u;

		int32_t *p_dspframe = NULL;

		bool audio_hw_init(void) override;

		bool buffer_alloc(void) override;
		void buffer_free(void) override;

		void buffer_load(void) override;
		void dsp_proc(void) override;
};

#endif //AUDIORTDSP_I16_HPP

