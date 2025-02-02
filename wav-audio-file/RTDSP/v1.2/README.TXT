This code plays audio files in wave (.wav) format. Currently, the only supported format is stereo 16bit.
Sample rate compatibility depends on your audio hardware.

This code has several .cpp files. Each one of them generates an individual executable. "main.cpp" generates the main executable, which should be called by user to start the program. 
The other executables will be called from "main" as necessary.

When compiling, all executables except "main" should be named as their source files. Example: executable from "rtdsp_16bit2ch.cpp" should be named "rtdsp_16bit2ch.elf". 
If executable names (apart from "main") don't match their source file names, user will need to make changes on "main.cpp" in order to make the code run properly.

This code requires the ALSA build resources to compile. On Debian-based distros, these resources can be installed with "sudo apt install libasound2-dev"

When compiling, two resources must be explicitly linked: -lasound and -lpthread

v1.2 Update: Same thing as v1.1, however there's no longer a "rtdsp_params.h" file in the project. 
Audio Output Device and File Directory are passed by user as arguments when starting the application. "--help" argument also available. 
Unlike the previous versions, this doesn't need to be recompiled to set audio device and file directory. 
Recompiling "main.cpp" will be necessary only if user wants to change preset parameters (defined in "main.cpp").

Author: Rafael Sabe 
Email: rafaelmsabe@gmail.com
