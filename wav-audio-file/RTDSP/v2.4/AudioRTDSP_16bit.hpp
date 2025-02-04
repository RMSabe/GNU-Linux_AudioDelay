/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 2.4
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */


#ifndef AUDIORTDSP_16BIT_HPP
#define AUDIORTDSP_16BIT_HPP

#include "AudioBaseClass.hpp"

class AudioRTDSP_16bit : public AudioBaseClass {
	public:
		AudioRTDSP_16bit(const audio_rtdsp_params_t *p_params);
		~AudioRTDSP_16bit(void);

	protected:
		const std::int32_t SAMPLE_MAX_VALUE = 0x7fff;
		const std::int32_t SAMPLE_MIN_VALUE = -0x8000;

		//DSPFRAME is a buffer meant to store a single frame of audio
		//with a sample size usually bigger than the normal sample size.
		//This is where most math operations happens, and it's done to prevent
		//Integer overflow on regular sized samples.

		const size_t DSPFRAME_SAMPLE_SIZE_BYTES = 4u;
		size_t DSPFRAME_SIZE_BYTES = 0u;

		std::int32_t *p_dspframe = nullptr;

		bool audio_hw_init(void) override;

		bool buffer_alloc(void) override;
		void buffer_free(void) override;

		void buffer_load(void) override;
		void dsp_proc(void) override;
};

#endif //AUDIORTDSP_16BIT_HPP

