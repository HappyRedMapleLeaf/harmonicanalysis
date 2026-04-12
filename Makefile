PA_OPTIONS = external/portaudio/lib/.libs/libportaudio.a external/portaudio/bindings/cpp/lib/.libs/libportaudiocpp.a -Iexternal/portaudio/include -Iexternal/portaudio/bindings/cpp/include -lrt -lm -lasound -pthread
INCLUDE_OPTIONS = -Iinc

all: step1

step0: step0.cpp
	g++ step0.cpp -o build/step0 -std=c++23

step1: step1.cpp circ_buf.cpp
	g++ step1.cpp circ_buf.cpp $(PA_OPTIONS) $(INCLUDE_OPTIONS) -o build/step1 -std=c++23
