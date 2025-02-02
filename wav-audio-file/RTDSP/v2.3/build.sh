#!/bin/bash

g++ globldef.c -c -o globldef.o
g++ delay.c -c -o delay.o
g++ cstrdef.c -c -o cstrdef.o
g++ cppdelay.cpp -c -o cppdelay.o
g++ strdef.cpp -c -o strdef.o
g++ main.cpp -c -o main.o
g++ AudioBaseClass.cpp -c -o AudioBaseClass.o
g++ AudioRTDSP_16bit.cpp -c -o AudioRTDSP_16bit.o
g++ AudioRTDSP_24bit.cpp -c -o AudioRTDSP_24bit.o

g++ main.o globldef.o delay.o cstrdef.o cppdelay.o strdef.o AudioBaseClass.o AudioRTDSP_16bit.o AudioRTDSP_24bit.o -lpthread -lasound -o rtdsp.elf

rm globldef.o
rm delay.o
rm cstrdef.o
rm cppdelay.o
rm strdef.o
rm main.o
rm AudioBaseClass.o
rm AudioRTDSP_16bit.o
rm AudioRTDSP_24bit.o

