#ifndef RTDSP_PARAMS_H
#define RTDSP_PARAMS_H

//Input file directory
#define WAVE_FILE_DIR "/home/user/Music/audio.wav"
//System ID for audio output device. Set to "default" to use system's default output device.
#define AUDIO_DEV "plughw:0,0"

//Preset: Delay time (in number of samples)
#define DSP_N_DELAY 240
//Preset: Number of delay feedback loops.
#define DSP_N_FEEDBACK_LOOPS 20

//Preset: Enable/Disable feedback alternating polarity
//Set to 1 to enable. Set to 0 to disable
#define DSP_FEEDBACK_POL_ALTERNATE 1
//Preset: Set cycle divider increment.
//Set to 1 to make cycle divider increment by 1
//Set to 0 to make cycle divider increment exponentially
#define DSP_CYCLE_DIV_INC_ONE 1

#endif
