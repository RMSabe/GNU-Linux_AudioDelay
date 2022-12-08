#ifndef RTDSP_PARAMS_H
#define RTDSP_PARAMS_H

//Input file directory
#define WAVE_FILE_DIR "/home/user/Music/audio.wav"
//System ID for audio output device. Set to "default" to use system's default output device.
#define AUDIO_DEV "plughw:0,0"

//Delay time (in number of samples)
#define DSP_N_DELAY 132
//Number of delay feedback loops
#define DSP_N_FEEDBACK_LOOPS 20

//Enable/Disable feedback polarity alternate
//Set to 0 to disable. Set to 1 to enable
#define DSP_FEEDBACK_POL_ALTERNATE 1
//Set Cycle Divider Increment.
//Set to 1 to increment divider by one.
//Set to 0 to increment divider exponentially.
#define DSP_CYCLE_DIV_INC_ONE 1

#endif
