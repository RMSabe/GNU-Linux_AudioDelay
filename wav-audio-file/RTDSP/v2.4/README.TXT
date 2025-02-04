Real Time Audio Delay for GNU-Linux systems.
Version 2.4

This code supports .wav audio files, 16bit and 24bit.
Number of channels and sampling rate depends on your audio hardware.

Changes:
wait_thread() and stop_thread() were moved from AudioBaseClass.cpp to cthread.c (pthread_t) and cppthread.cpp (std::thread).
I used cppthread.cpp (std::thread) this time.

Bug fix:
v2.3 had a bug on wait_thread().
Some if statements evaluate for false, when they should actually evaluate for true.
That bug was fixed in cthread.c .

Author: Rafael Sabe
Email: rafaelmsabe@gmail.com

