#include <iostream>
#include <thread>
#include <functional>
#include <algorithm>
#include <cmath>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "audio.hpp"

constexpr size_t CAPTURE_SIZE = SAMPLE_RATE_HZ * 0.001;

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main(int, char**) {
    // glfw init
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // create glfw window
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow(
        (int)(1280 * main_scale), (int)(800 * main_scale),
        "playing with waves", nullptr, nullptr
    );
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // imgui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;
    io.LogFilename = NULL;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    style.FontScaleMain = 1.5;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // set up audio thread
    AudioGuiInterface shared_data(CAPTURE_SIZE);
    Shared<AudioSettings> &settings = shared_data.settings;
    settings.in_ref() = {
        .stop = false,
        .playing = false,
        .frequency = 440.0,
        .amplitude = 0.5,
        .harmonics = 1,
        .ideal_sawtooth = false
    };
    settings.refresh_after_write();
    
    CircBufInOnly &capture = shared_data.capture;
    std::thread audio_thread(audio_thread_func, std::ref(shared_data));

    std::vector<float> capture_data(CAPTURE_SIZE);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        {
            ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(500, 250), ImGuiCond_FirstUseEver);
            ImGui::Begin("Controls");

            AudioSettings &s = settings.in_ref();

            bool settings_changed = false;
            settings_changed |= ImGui::SliderFloat("amplitude", &s.amplitude,
                0.0f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
            settings_changed |= ImGui::SliderFloat("frequency", &s.frequency,
                25.0f, 10000.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
            settings_changed |= ImGui::SliderInt("harmonics", &s.harmonics,
                1, 1000, "%d", ImGuiSliderFlags_Logarithmic);

            if (ImGui::BeginTable("checks", 2)) {
                ImGui::TableNextColumn();
                settings_changed |= ImGui::Checkbox("playing", &s.playing);
                ImGui::TableNextColumn();
                settings_changed |= ImGui::Checkbox("ideal", &s.ideal_sawtooth);
                ImGui::EndTable();
            }

            if (settings_changed) {
                settings.refresh_after_write();
            }

            ImGui::Text("Highest harmonic before aliasing: %d",
                static_cast<int>(std::floor(SAMPLE_RATE_HZ / 2.0 / s.frequency))
            );

            ImGui::End();
        }

        // todo: dynamically change capture width (sounds hard asf)
        if (capture.freeze) {
            capture.copy(capture_data);
            capture.freeze = false;
        }

        // draw captured waveform
        {
            ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y + 250), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(1280, 500), ImGuiCond_FirstUseEver);
            ImGui::Begin("Scope");

            // get current window info
            const ImVec2 contentSize = ImGui::GetContentRegionAvail();
            const ImVec2 windowPos = ImGui::GetCursorScreenPos();

            float mag = 1.0;
            if (!capture_data.empty()) {
                auto minmax = std::minmax_element(capture_data.begin(), capture_data.end());
                mag = std::max(std::abs(*(minmax.first)), std::abs(*(minmax.second)));
                mag = std::max(mag, 0.01f);
            }

            float time_per_pixel = (float) CAPTURE_SIZE / contentSize.x;
            float pixels_per_value = contentSize.y / (mag * 2.0);

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->PathClear();
            for (int i = 0; i < contentSize.x; i++) {
                size_t index = std::clamp(static_cast<size_t>(i * time_per_pixel), (size_t) 0, CAPTURE_SIZE - 1);
                float raw_value = capture_data[index];
                draw_list->PathLineTo(ImVec2(
                    (float) i + windowPos.x,
                    -raw_value * pixels_per_value + (contentSize.y / 2.0) + windowPos.y
                ));
            }
            draw_list->PathStroke(ImColor(100, 100, 255), 0, 1.0);

            ImGui::End();
        }

        {
            ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 500, main_viewport->WorkPos.y), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(500, 250), ImGuiCond_FirstUseEver);
            ImGui::Begin("Stats");
            
            shared_data.stats.refresh_before_read();
            ImGui::Text(
                "GUI %.3f ms/frame\nAudio %ld ms/loop",
                1000.0f / io.Framerate,
                shared_data.stats.out_ref().ns_per_frame / 1000000
            );

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    settings.in_ref().stop = true;
    settings.refresh_after_write();
    capture.freeze.store(false);
    audio_thread.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}