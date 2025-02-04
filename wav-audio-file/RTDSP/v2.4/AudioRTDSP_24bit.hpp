/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 2.4
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */


#ifndef AUDIORTDSP_24BIT_HPP
#define AUDIORTDSP_24BIT_HPP

#include "AudioBaseClass.hpp"

class AudioRTDSP_24bit : public AudioBaseClass {
	public:
		AudioRTDSP_24bit(const audio_rtdsp_params_t *p_params);
		~AudioRTDSP_24bit(void);

	protected:
		const std::int32_t SAMPLE_MAX_VALUE = 0x7fffff;
		const std::int32_t SAMPLE_MIN_VALUE = -0x800000;

		size_t BYTEBUF_SIZE = 0u;
		std::uint8_t *p_bytebuf = nullptr;

		bool audio_hw_init(void) override;

		bool buffer_alloc(void) override;
		void buffer_free(void) override;

		void buffer_load(void) override;
		void dsp_proc(void) override;
};

#endif //AUDIORTDSP_24BIT_HPP

