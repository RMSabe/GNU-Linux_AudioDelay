/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 2.5
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#ifndef AUDIORTDSP_I24_HPP
#define AUDIORTDSP_I24_HPP

#include "AudioBaseClass.hpp"

class AudioRTDSP_i24 : public AudioBaseClass {
	public:
		AudioRTDSP_i24(const audio_rtdsp_params_t *p_params);
		~AudioRTDSP_i24(void);

	protected:
		const int32_t SAMPLE_MAX_VALUE = 0x7fffff;
		const int32_t SAMPLE_MIN_VALUE = -0x800000;

		size_t BYTEBUF_SIZE = 0u;
		uint8_t *p_bytebuf = NULL;

		bool audio_hw_init(void) override;

		bool buffer_alloc(void) override;
		void buffer_free(void) override;

		void buffer_load(void) override;
		void dsp_proc(void) override;
};

#endif //AUDIORTDSP_I24_HPP

