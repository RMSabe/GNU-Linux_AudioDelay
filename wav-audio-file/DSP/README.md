An Audio Delay File Generator for Unix based systems

This code performs "offline" DSP (digital signal processing). It runs the delay DSP on an input audio file signal, and loads it into an output audio file, rather than doing real time DSP.
WARNING: output audio file will be overwritten if it already exists
For real time DSP, check the RTDSP directory.

This code accepts input audio files in WAVE (.wav) format. Currently, the only supported audio formats are mono and stereo, 16bit and 24bit.
Output audio file will have the same audio format as input audio file.

This code has several .cpp files, each one of them generates an individual executable. 
"main.cpp" generates the main executable, which should be called by the user to start the program. The other executables will be called from "main" as necessary.

When compiling, all executables except "main" should be named as their source files.
Example: executable from "dsp_16bit2ch.cpp" should be named "dsp_16bit2ch.elf".
If executable names (apart from "main") don't match their source file names, user will need to make changes on "main.cpp" in order to make the code run properly.

v1.0:
The main executable must be called from within its directory, and must receive 6 arguments:
Input audio file directory
Output audio file directory
Delay time (in number of samples)
Number of feedback loops
Alternate feedback polarity (1 == active / 0 == not active)
Select cycle divider (1 == cycle divider increments by 1 / 0 == cycle divider increments exponentially)

Remember: I'm not a professional developer, I made these just for fun. Don't expect professional performance from them.

Author: Rafael Sabe
Email: rafaelmsabe@gmail.com

