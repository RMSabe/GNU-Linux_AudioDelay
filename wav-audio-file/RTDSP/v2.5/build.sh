#!/bin/bash

g++ globldef.c -c -o globldef.o
g++ delay.c -c -o delay.o
g++ cstrdef.c -c -o cstrdef.o
g++ strdef.cpp -c -o strdef.o
g++ cppthread.cpp -c -o cppthread.o
g++ main.cpp -c -o main.o
g++ AudioBaseClass.cpp -c -o AudioBaseClass.o
g++ AudioRTDSP_i16.cpp -c -o AudioRTDSP_i16.o
g++ AudioRTDSP_i24.cpp -c -o AudioRTDSP_i24.o

g++ main.o globldef.o delay.o cstrdef.o strdef.o cppthread.o AudioBaseClass.o AudioRTDSP_i16.o AudioRTDSP_i24.o -lpthread -lasound -o rtdsp.elf

rm *.o

