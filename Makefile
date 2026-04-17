PA_DIR = external/portaudio
PA_SOURCES = $(PA_DIR)/lib/.libs/libportaudio.a $(PA_DIR)/bindings/cpp/lib/.libs/libportaudiocpp.a
PA_INCLUDES = -I$(PA_DIR)/include -I$(PA_DIR)/bindings/cpp/include
PA_OPTIONS = -lrt -lm -lasound -pthread

IMGUI_DIR = external/imgui
IMGUI_SOURCES = $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp \
                $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp \
                $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
IMGUI_INCLUDES = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
IMGUI_OPTIONS = -lGL `pkg-config --static --libs glfw3`

INCLUDES = -Iinc
OPTIONS = -g -Wall -Wformat

BUILD_DIR = build
OUT_DIR = $(BUILD_DIR)/out
IMGUI_BUILD_DIR = $(BUILD_DIR)/imgui

IMGUI_OBJS = $(patsubst %.cpp, $(IMGUI_BUILD_DIR)/%.o, $(notdir $(IMGUI_SOURCES)))
MAIN_OBJS  = $(BUILD_DIR)/main.o $(BUILD_DIR)/circ_buf.o

.PHONY: all clean

all: $(OUT_DIR)/test $(OUT_DIR)/app

clean:
	rm -rf $(BUILD_DIR)

# link
$(OUT_DIR):
	mkdir -p $(OUT_DIR)
$(OUT_DIR)/app: $(MAIN_OBJS) | $(OUT_DIR)
	g++ $^ $(PA_SOURCES) $(PA_OPTIONS) -o $@ -std=c++23
$(OUT_DIR)/test: $(BUILD_DIR)/imgui_test.o $(IMGUI_OBJS) | $(OUT_DIR)
	g++ $^ $(IMGUI_OPTIONS) -o $@

# build my stuff
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
$(BUILD_DIR)/main.o: main.cpp | $(BUILD_DIR)
	g++ -c $< $(PA_INCLUDES) $(INCLUDES) $(OPTIONS) -MMD -MP -std=c++23 -o $@
$(BUILD_DIR)/circ_buf.o: circ_buf.cpp | $(BUILD_DIR)
	g++ -c $< $(INCLUDES) $(OPTIONS) -MMD -MP -std=c++23 -o $@
$(BUILD_DIR)/imgui_test.o: imgui_test.cpp | $(BUILD_DIR)
	g++ -c $< $(IMGUI_INCLUDES) $(INCLUDES) $(OPTIONS) -MMD -MP -o $@

# build library stuff: imgui
$(IMGUI_BUILD_DIR):
	mkdir -p $(IMGUI_BUILD_DIR)
$(IMGUI_BUILD_DIR)/%.o: $(IMGUI_DIR)/%.cpp | $(IMGUI_BUILD_DIR)
	g++ -c $< $(IMGUI_INCLUDES) $(OPTIONS) -MMD -MP -o $@
$(IMGUI_BUILD_DIR)/%.o: $(IMGUI_DIR)/backends/%.cpp | $(IMGUI_BUILD_DIR)
	g++ -c $< $(IMGUI_INCLUDES) $(OPTIONS) -MMD -MP -o $@

# check for headers
-include $(MAIN_OBJS:.o=.d)
-include $(IMGUI_OBJS:.o=.d)
-include $(BUILD_DIR)/main.d $(BUILD_DIR)/imgui_test.d
