globldef.o: globldef.c
	g++ globldef.c -c -o globldef.o

delay.o: delay.c
	g++ delay.c -c -o delay.o

cstrdef.o: cstrdef.c
	g++ cstrdef.c -c -o cstrdef.o

strdef.o: strdef.cpp
	g++ strdef.cpp -c -o strdef.o

cppthread.o: cppthread.cpp
	g++ cppthread.cpp -c -o cppthread.o

lib_res: globldef.o delay.o cstrdef.o strdef.o cppthread.o

AudioBaseClass.o: AudioBaseClass.cpp
	g++ AudioBaseClass.cpp -c -o AudioBaseClass.o

AudioRTDSP_i16.o: AudioRTDSP_i16.cpp
	g++ AudioRTDSP_i16.cpp -c -o AudioRTDSP_i16.o

AudioRTDSP_i24.o: AudioRTDSP_i24.cpp
	g++ AudioRTDSP_i24.cpp -c -o AudioRTDSP_i24.o

audio_rtdsp: AudioBaseClass.o AudioRTDSP_i16.o AudioRTDSP_i24.o

main.o: main.cpp
	g++ main.cpp -c -o main.o

rtdsp.elf: main.o lib_res audio_rtdsp
	g++ main.o globldef.o delay.o cstrdef.o strdef.o cppthread.o AudioBaseClass.o AudioRTDSP_i16.o AudioRTDSP_i24.o -lpthread -lasound -o rtdsp.elf

clear:
	rm *.o

all: rtdsp.elf clear


