/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 3.0
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#ifndef AUDIORTDSP_I24_HPP
#define AUDIORTDSP_I24_HPP

#include "AudioRTDSP.hpp"

class AudioRTDSP_i24 : public AudioRTDSP {
	public:
		AudioRTDSP_i24(const audiortdsp_pb_params_t *p_pbparams);
		~AudioRTDSP_i24(void);

	private:
		static constexpr int32_t SAMPLE_MAX_VALUE = 0x7fffff;
		static constexpr int32_t SAMPLE_MIN_VALUE = -0x800000;

		size_t BYTEBUF_SIZE = 0u;

		uint8_t *p_bytebuf = NULL;

		bool audio_hw_init(void) override;
		bool buffer_alloc(void) override;
		void buffer_free(void) override;
		void buffer_load(void) override;
		void dsp_proc(void) override;
};

#endif /*AUDIORTDSP_I24_HPP*/

