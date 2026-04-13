PA_OPTIONS = external/portaudio/lib/.libs/libportaudio.a external/portaudio/bindings/cpp/lib/.libs/libportaudiocpp.a -Iexternal/portaudio/include -Iexternal/portaudio/bindings/cpp/include -lrt -lm -lasound -pthread
INCLUDE_OPTIONS = -Iinc

all: main

main: main.cpp circ_buf.cpp
	g++ main.cpp circ_buf.cpp $(PA_OPTIONS) $(INCLUDE_OPTIONS) -o build/app -std=c++23
