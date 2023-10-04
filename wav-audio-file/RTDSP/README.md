An audio delay real time digital signal processing application for GNU-Linux systems.

This code plays audio files in wave (.wav) format. 
Currently, the only supported format is stereo, 16bit. Sample rate compatibility depends on your audio hardware.

This code has several .cpp files. Each one of them generates an individual executable. 
"main.cpp" generates the main executable, which should be called by user to start the program. 
The other executables will be called from "main" as necessary.

When compiling, all executables except "main" should be named as their source files. 
Example: executable from "rtdsp_16bit2ch.cpp" should be named "rtdsp_16bit2ch.elf". 
If executable names (apart from "main") don't match their source file names, user will need to make changes on "main.cpp" in order to make the code run properly.

This code requires the ALSA build resources to compile. 
On Debian-based distros, these resources can be installed with "sudo apt install libasound2-dev"

When compiling, two resources must be explicitly linked: -lasound and -lpthread

v1.0:
"rtdsp_params.h" file is the user settings file where user sets input file directory, output audio device and dsp parameters. 
All ".cpp" files except "main.cpp" only need to be compiled once. "main.cpp" must be recompiled every time user edits the "rtdsp_params.h" file.

v1.1 Update:
Now there's a command line interface. User may change the dsp parameters during runtime.
The dsp parameters in rtdsp_params.h are still valid as a preset.

v1.2 Update:
Same thing as v1.1, however there's no longer a "rtdsp_params.h" file in the project. 
Audio Output Device and File Directory are passed by user as arguments when starting the application.
"--help" argument also available.
Unlike the previous versions, this doesn't need to be recompiled to set audio device and file directory.
Recompiling "main.cpp" will be necessary only if user wants to change preset parameters (defined in "main.cpp").

v1.3 Update:
Same thing as v1.2, but code is tiddier and better optimized.
"--help" argument is no longer available, instead, "--help" list will automatically appear if user don't add the necessary arguments.

Note (v1.1 Update and forward): 
When using the default system audio output, I noticed there's a big response delay after changing the parameters. 
(The code works fine, but it takes too long to lock and load the new settings).
This is probably being caused because the ALSA buffers are to big. 
It is possible to change that by changing the ALSA buffer size, but I suggest user don't use the system default audio output for this project.

Remember: I'm not a professional developer. I made these just for fun. Don't expect professional performance from them.

Author: Rafael Sabe 
Email: rafaelmsabe@gmail.com
