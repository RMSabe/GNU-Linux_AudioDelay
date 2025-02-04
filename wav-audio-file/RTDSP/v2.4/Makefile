globldef.o: globldef.c
	g++ globldef.c -c -o globldef.o

delay.o: delay.c
	g++ delay.c -c -o delay.o

cstrdef.o: cstrdef.c
	g++ cstrdef.c -c -o cstrdef.o

cthread.o: cthread.c
	g++ cthread.c -c -o cthread.o

cppdelay.o: cppdelay.cpp
	g++ cppdelay.cpp -c -o cppdelay.o

strdef.o: strdef.cpp
	g++ strdef.cpp -c -o strdef.o

cppthread.o: cppthread.cpp
	g++ cppthread.cpp -c -o cppthread.o

lib_res: globldef.o delay.o cstrdef.o cthread.o cppdelay.o strdef.o cppthread.o

AudioBaseClass.o: AudioBaseClass.cpp
	g++ AudioBaseClass.cpp -c -o AudioBaseClass.o

AudioRTDSP_16bit.o: AudioRTDSP_16bit.cpp
	g++ AudioRTDSP_16bit.cpp -c -o AudioRTDSP_16bit.o

AudioRTDSP_24bit.o: AudioRTDSP_24bit.cpp
	g++ AudioRTDSP_24bit.cpp -c -o AudioRTDSP_24bit.o

audio_rtdsp: AudioBaseClass.o AudioRTDSP_16bit.o AudioRTDSP_24bit.o

main.o: main.cpp
	g++ main.cpp -c -o main.o

rtdsp.elf: main.o lib_res audio_rtdsp
	g++ main.o globldef.o delay.o cstrdef.o cthread.o cppdelay.o strdef.o cppthread.o AudioBaseClass.o AudioRTDSP_16bit.o AudioRTDSP_24bit.o -lpthread -lasound -o rtdsp.elf

clear:
	rm globldef.o
	rm delay.o
	rm cstrdef.o
	rm cthread.o
	rm cppdelay.o
	rm strdef.o
	rm cppthread.o
	rm main.o
	rm AudioBaseClass.o
	rm AudioRTDSP_16bit.o
	rm AudioRTDSP_24bit.o

all: rtdsp.elf clear


