/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 2.4
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */


#ifndef AUDIOBASECLASS_HPP
#define AUDIOBASECLASS_HPP

#include "globldef.h"
#include "filedef.h"
#include "strdef.hpp"
#include "cppthread.hpp"

#include <alsa/asoundlib.h>

struct _audio_rtdsp_params {
	const char *audio_dev_desc;
	const char *filein_dir;
	__offset audio_data_begin;
	__offset audio_data_end;
	std::uint32_t sample_rate;
	std::uint16_t n_channels;
};

typedef struct _audio_rtdsp_params audio_rtdsp_params_t;

class AudioBaseClass {

	/*
	 * Global variable naming
	 *
	 * Variables that refer to an external context such as pointers or file descriptors will have a "p_" or "h_" prefix.
	 *
	 * (Pointer) "p_" prefix means the invalid value for this variable is 0 (nullptr). Example: void *ptr = nullptr;
	 * (Handle or descriptor) "h_" prefix means the invalid value for this variable is -1. Example int file_descriptor = -1;
	 */

	public:
		AudioBaseClass(const audio_rtdsp_params_t *p_params);

		bool setParameters(const audio_rtdsp_params_t *p_params);
		bool initialize(void);
		bool runDSP(void);

		std::string getLastErrorMessage(void);

	protected:

		enum Status {
			STATUS_ERROR_MEMALLOC = -4,
			STATUS_ERROR_NOFILE = -3,
			STATUS_ERROR_AUDIOHW = -2,
			STATUS_ERROR_GENERIC = -1,
			STATUS_UNINITIALIZED = 0,
			STATUS_INITIALIZED = 1
		};

		enum UpdateVarDesc {
			UPDATEVAR_NDELAY = 1,
			UPDATEVAR_NFEEDBACKLOOPS = 2,
			UPDATEVAR_FEEDBACKALTPOL = 3,
			UPDATEVAR_CYCLEDIVINCONE = 4
		};

		struct _rtdsp_var {
			int n_delay;
			int n_feedback_loops;
			bool feedback_alt_pol;
			bool cyclediv_inc_one;
		};

		int status = this->STATUS_UNINITIALIZED;

		std::uint32_t sample_rate = 0u;
		std::uint16_t n_channels = 0u;

		int h_filein = -1;
		__offset filein_size = 0;
		__offset filein_pos = 0;

		__offset audio_data_begin = 0;
		__offset audio_data_end = 0;

		std::string filein_dir = "";
		std::string audio_dev_desc = "";

		std::string error_msg = "";
		std::string user_cmd = "";

		snd_pcm_t *p_audiodev = nullptr;

		/*
		 * Buffers:
		 *
		 * there are 2 main buffers: input and output
		 * input is where file content is loaded
		 * output is where DSP is saved and ready to play.
		 *
		 * both buffers will be split into segments.
		 * each segment is sized based on the playback buffer size.
		 *
		 * input should be as big as possible, to store as many previous samples as possible.
		 * output is just 2 segments: currently loading and currently playing.
		 */

		const size_t BUFFERIN_SIZE_FRAMES = 65536u;
		const size_t N_BUFFEROUT_SEGMENTS = 2u;
		const size_t AUDIOBUFFER_SIZE_FRAMES_PRESET = 1024u;

		//These variables will be initialized at audio_hw_init() through initialize();
		//Treat them as constants.
		size_t BUFFERIN_SIZE_SAMPLES = 0u;
		size_t BUFFERIN_SIZE_BYTES = 0u;
		size_t BUFFEROUT_SIZE_FRAMES = 0u;
		size_t BUFFEROUT_SIZE_SAMPLES = 0u;
		size_t BUFFEROUT_SIZE_BYTES = 0u;
		size_t AUDIOBUFFER_SIZE_FRAMES = 0u;
		size_t AUDIOBUFFER_SIZE_SAMPLES = 0u;
		size_t AUDIOBUFFER_SIZE_BYTES = 0u;
		size_t N_BUFFERIN_SEGMENTS = 0u;

		//Main buffer will be devided into small segments. These variables keep track of which segment is loading output, playing output and current input.
		size_t bufferout_nsegment_load = 0u;
		size_t bufferout_nsegment_play = 0u;
		size_t bufferin_nsegment_curr = 0u;

		//Main input/output buffer
		void *p_bufferin = nullptr;
		void *p_bufferout = nullptr;

		//Main buffer segments array. Each pointer points to the beginning of that segment.
		void **pp_bufferin_segments = nullptr;
		void **pp_bufferout_segments = nullptr;

		std::thread loadthread;
		std::thread playthread;
		std::thread userthread;

		struct _rtdsp_var rtdsp_var;

		bool stop = false;

		bool filein_open(void);
		void filein_close(void);

		virtual bool audio_hw_init(void) = 0;
		void audio_hw_deinit(void);

		void wait_all_threads(void);
		void stop_all_threads(void);

		virtual bool buffer_alloc(void) = 0;
		virtual void buffer_free(void) = 0;

		void playback_proc(void);
		void playback_init(void);
		void playback_loop(void);

		void buffer_segment_remap(void);
		void buffer_play(void);

		virtual void buffer_load(void) = 0;
		virtual void dsp_proc(void) = 0;

		//These methods are used to calculate the previous (delayed) frame index from the current frame index and a given delay
		//..._buf_nframe means frame index within the whole input buffer
		//..._seg_nframe means frame index within a specific segment
		//..._nseg means the index of the given segment
		bool retrieve_previn_nframe(size_t currin_buf_nframe, size_t n_delay, size_t *p_previn_buf_nframe, size_t *p_previn_nseg, size_t *p_previn_seg_nframe);
		bool retrieve_previn_nframe(size_t currin_nseg, size_t currin_seg_nframe, size_t n_delay, size_t *p_previn_buf_nframe, size_t *p_previn_nseg, size_t *p_previn_seg_nframe);

		void loadthread_proc(void);
		void playthread_proc(void);
		void userthread_proc(void);

		void cmd_decode(void);

		//This function behaves similarly to cstr_compare(), but it can be specified compare only up to some characters rather than the whole text.
		bool cmd_compare(const char *auth, const char *input, size_t stop_index);

		void print_help_text(void);
		void print_current_params(void);
		bool attempt_updatevar(const char *numtext, int updatevar_desc);
};

#endif //AUDIOBASECLASS_HPP

