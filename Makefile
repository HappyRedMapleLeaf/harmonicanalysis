all: step0

step0: step0.cpp
	g++ step0.cpp -o build/step0 -std=c++23
