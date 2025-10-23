/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 3.0
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#ifndef AUDIORTDSP_I16_HPP
#define AUDIORTDSP_I16_HPP

#include "AudioRTDSP.hpp"

class AudioRTDSP_i16 : public AudioRTDSP {
	public:
		AudioRTDSP_i16(const audiortdsp_pb_params_t *p_pbparams);
		~AudioRTDSP_i16(void);

	private:
		static constexpr int32_t SAMPLE_MAX_VALUE = 0x7fff;
		static constexpr int32_t SAMPLE_MIN_VALUE = -0x8000;

		/*
		 * dspframe is a buffer used to store a single frame of audio.
		 * It uses a sample size bigger than the normal I/O buffer sample size.
		 * This is where all the signal processing happens.
		 * The purpose of this buffer with bigger sample size is to prevent integer overflow/underflow from the signal processing math operations.
		 */

		static constexpr size_t DSPFRAME_SAMPLE_SIZE_BYTES = 4u;

		size_t DSPFRAME_SIZE_BYTES = 0u;

		int32_t *p_dspframe = NULL;

		bool audio_hw_init(void) override;
		bool buffer_alloc(void) override;
		void buffer_free(void) override;
		void buffer_load(void) override;
		void dsp_proc(void) override;
};

#endif /*AUDIORTDSP_I16_HPP*/

