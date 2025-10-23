/*
 * Real Time Audio Delay for GNU-Linux systems.
 * Version 3.0
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#ifndef AUDIORTDSP_HPP
#define AUDIORTDSP_HPP

#include "globldef.h"
#include "filedef.h"
#include "strdef.hpp"
#include "cppthread.hpp"

#include "shared.hpp"

#include <alsa/asoundlib.h>

/*
 * This is how this application works:
 *
 * There are 4 parameters: delay (int), feedback loops (int), alternate feedback polarity (bool), cycle divider increment one (bool).
 *
 * delay: specifies the delay time in number of samples.
 *
 * feedback loops: specifies how many times the same delay will be serialized and added to the output.
 *
 * Observation: "feedback" is not the right word for it. The way how feedback loop works is, it will calculate the new delay time based on the
 * old delay time and the loop iteration. There is no actual feedback, it's just multiple parallel feedforward delays, with each delay time being a multiple of the initial delay time.
 * I kept the term "feedback" for retro-compatibility purposes. Might change that in a future version.
 *
 * alternate feedback polarity: if true, it will toggle each loop between positive and negative amplitude polarity.
 *
 * cycle divider increment one: in every loop iteration, the amplitude of the delayed sample is divided by a divider factor, which is calculated by the loop iteration.
 * If cycle divider increment is set to true, then the cycle divider will increment by one, following the iteration value.
 * If cycle divider increment is set to false, then the cycle divider will increment exponentially.
 */

struct _audiortdsp_pb_params {
	const char *audio_dev_desc;
	const char *filein_dir;
	__offset audio_data_begin;
	__offset audio_data_end;
	uint32_t sample_rate;
	uint16_t n_channels;
};

struct _audiortdsp_fx_params {
	int32_t n_delay;
	int32_t n_feedback;
	bool feedback_altpol;
	bool cyclediv_incone;
};

typedef struct _audiortdsp_pb_params audiortdsp_pb_params_t;
typedef struct _audiortdsp_fx_params audiortdsp_fx_params_t;

class AudioRTDSP {
	public:
		AudioRTDSP(const audiortdsp_pb_params_t *p_pbparams);

		bool setPlaybackParameters(const audiortdsp_pb_params_t *p_pbparams);
		bool initialize(void);
		bool runPlayback(void);

		std::string getLastErrorMessage(void);

		enum Status {
			STATUS_ERROR_MEMALLOC = -4,
			STATUS_ERROR_AUDIOHW = -3,
			STATUS_ERROR_NOFILE = -2,
			STATUS_ERROR_GENERIC = -1,
			STATUS_UNINITIALIZED = 0,
			STATUS_READY = 1,
			STATUS_PLAYING = 2
		};

	protected:
		enum UpdateVarDesc {
			UPDATEVAR_NDELAY = 1,
			UPDATEVAR_NFEEDBACK = 2,
			UPDATEVAR_FEEDBACKALTPOL = 3,
			UPDATEVAR_CYCLEDIVINCONE = 4
		};

		/*
		 * The way this application works is:
		 * There are 2 buffers: input and output.
		 * Both buffers are split into multiple segments.
		 *
		 * Segment size is defined by the audio device buffer segment size (period size).
		 *
		 * Output buffer is only two segments: currently loading and currently playing.
		 * Input buffer will have multiple segments.
		 */

		static constexpr size_t BUFFERIN_SIZE_FRAMES = 65536u;
		static constexpr size_t BUFFEROUT_N_SEGMENTS = 2u;

		size_t BUFFERIN_SIZE_SAMPLES = 0u;
		size_t BUFFERIN_SIZE_BYTES = 0u;
		size_t BUFFERIN_N_SEGMENTS = 0u;

		size_t BUFFEROUT_SIZE_FRAMES = 0u;
		size_t BUFFEROUT_SIZE_SAMPLES = 0u;
		size_t BUFFEROUT_SIZE_BYTES = 0u;

		size_t AUDIOBUFFER_SIZE_FRAMES = 0u;
		size_t AUDIOBUFFER_SIZE_SAMPLES = 0u;
		size_t AUDIOBUFFER_SIZE_BYTES = 0u;

		size_t AUDIOBUFFER_SEGMENT_SIZE_FRAMES = 0u;
		size_t AUDIOBUFFER_SEGMENT_SIZE_SAMPLES = 0u;
		size_t AUDIOBUFFER_SEGMENT_SIZE_BYTES = 0u;

		/*
		 * These are index variables to keep track of the current buffer segments in context.
		 * bufferin_nseg_curr is the index for the current input buffer segment in context.
		 * bufferout_nseg_load is the index for the output buffer segment to be loaded.
		 * bufferout_nseg_play is the index for the output buffer segment to be played.
		 */

		size_t bufferout_nseg_load = 0u;
		size_t bufferout_nseg_play = 0u;
		size_t bufferin_nseg_curr = 0u;

		/*
		 * p_bufferinput and p_bufferoutput are the input and output buffers.
		 * pp_bufferinput_segments and pp_bufferoutput_segments are pointer arrays, each pointer in the array points to
		 * the beginning of a buffer segment.
		 */

		void *p_bufferinput = NULL;
		void *p_bufferoutput = NULL;

		void **pp_bufferinput_segments = NULL;
		void **pp_bufferoutput_segments = NULL;

		snd_pcm_t *p_audiodev = NULL;

		int h_filein = -1;
		__offset filein_size = 0;
		__offset filein_pos = 0;

		__offset AUDIO_DATA_BEGIN = 0;
		__offset AUDIO_DATA_END = 0;

		std::string AUDIODEV_DESC = "";
		std::string FILEIN_DIR = "";

		size_t N_CHANNELS = 0u;
		size_t SAMPLE_RATE = 0u;

		std::string usr_cmd = "";
		std::string err_msg = "";

		std::thread playthread;
		std::thread userthread;

		int status = this->STATUS_UNINITIALIZED;

		audiortdsp_fx_params_t fx_params = {
			.n_delay = 240,
			.n_feedback = 20,
			.feedback_altpol = true,
			.cyclediv_incone = true
		};

		bool stop_playback = false;

		void wait_all_threads(void);
		void stop_all_threads(void);

		bool filein_open(void);
		void filein_close(void);

		virtual bool audio_hw_init(void) = 0;
		void audio_hw_deinit(void);

		virtual bool buffer_alloc(void) = 0;
		virtual void buffer_free(void) = 0;

		void playback_proc(void);
		void playback_init(void);
		void playback_loop(void);

		void buffer_segment_update(void);
		void buffer_play(void);

		virtual void buffer_load(void) = 0;
		virtual void dsp_proc(void) = 0;

		/*
		 * retrieve_previn_nframe: this method is used to calculate the previous frame index from the current frame index and the delay time in number of frames
		 *
		 * Inputs:
		 * curr_buf_nframe: current frame index (within the whole input buffer)
		 * n_delay: delay time in number of frames
		 *
		 * alternative inputs:
		 * curr_nseg: current index for input buffer segment in context.
		 * curr_seg_nframe: current frame index (within the specified input buffer segment).
		 *
		 * Outputs:
		 * p_prev_buf_nframe: pointer that receives the delayed frame index (within the whole input buffer). Set to NULL if unused.
		 * p_prev_nseg: pointer that receives the delayed input buffer segment index. Set to NULL if unused. Set to NULL if unused.
		 * p_prev_seg_nframe: pointer that receives the delayed frame index (within the delayed input buffer segment). Set to NULL if unused.
		 *
		 * returns true if successful, false otherwise.
		 */

		bool retrieve_previn_nframe(size_t curr_buf_nframe, size_t n_delay, size_t *p_prev_buf_nframe, size_t *p_prev_nseg, size_t *p_prev_seg_nframe);
		bool retrieve_previn_nframe(size_t curr_nseg, size_t curr_seg_nframe, size_t n_delay, size_t *p_prev_buf_nframe, size_t *p_prev_nseg, size_t *p_prev_seg_nframe);

		void cmdui_cmd_decode(void);
		bool cmdui_cmd_compare(const char *auth, const char *input, size_t stop_index);

		void cmdui_print_help_text(void);
		void cmdui_print_current_params(void);
		bool cmdui_attempt_updatevar(const char *numtext, int updatevar_desc);

		void loadthread_proc(void); /*loadthread_proc will be run by main thread*/
		void playthread_proc(void); /*playthread_proc will be run by playthread*/
		void userthread_proc(void); /*userthread_proc will be run by userthread*/
};

#endif /*AUDIORTDSP_HPP*/

