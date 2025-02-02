An audio delay real time digital signal processing application for GNU-Linux systems.

This code plays audio files in wave (.wav) format. 

These codes requires the ALSA build resources to compile. 
On Debian-based distros, these resources can be installed with "sudo apt install libasound2-dev"

When compiling, two resources must be explicitly linked: -lasound and -lpthread

Note (v1.1 Update and forward): 
If your audio playback device buffer is too big, you might notice a delay when changing settings. 
(The code works fine, but it takes too long to lock and load the new settings).

Author: Rafael Sabe 
Email: rafaelmsabe@gmail.com
