#include <iostream>
#include <thread>
#include <functional>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "audio.hpp"

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
    Shared<AudioSettings> settings;
    AudioSettings &s = settings.fresh;
    s = {
        .stop = false,
        .playing = false,
        .frequency = 440.0,
        .amplitude = 0.5,
        .harmonics = 1,
        .ideal_sawtooth = false
    };
    settings.refresh_after_write();
    std::thread audio_thread(audio_thread_func, std::ref(settings));

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

        {
            const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
            
            ImGui::Begin("Controls");

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
    settings.fresh.stop = true;
    settings.refresh_after_write();
    audio_thread.join();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}