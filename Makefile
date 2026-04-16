
PA_DIR = external/portaudio
PA_SOURCES = $(PA_DIR)/lib/.libs/libportaudio.a $(PA_DIR)/bindings/cpp/lib/.libs/libportaudiocpp.a
PA_INCLUDES = -I$(PA_DIR)/include -I$(PA_DIR)/bindings/cpp/include
PA_OPTIONS = -lrt -lm -lasound -pthread

IMGUI_DIR = external/imgui
IMGUI_SOURCES = $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
IMGUI_INCLUDES = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
IMGUI_OPTIONS = -lGL `pkg-config --static --libs glfw3`

INCLUDES = -Iinc
OPTIONS = -g -Wall -Wformat

# todo: separate compiling and linking so that i dont have to recompile everything

all: test

main: main.cpp circ_buf.cpp
	g++ main.cpp circ_buf.cpp $(PA_SOURCES) $(PA_OPTIONS) $(INCLUDE_OPTIONS) -o build/app -std=c++23

test: imgui_test.cpp
	g++ imgui_test.cpp $(IMGUI_SOURCES) $(IMGUI_INCLUDES) $(IMGUI_OPTIONS) $(INCLUDES) $(OPTIONS) -o build/test
