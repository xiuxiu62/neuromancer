#include "core/logger.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "neural_net.hpp"
#include "renderer.hpp"
#include "state.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void process_input(GLFWwindow *window);

static State state = {
    .network_paused = false,
    .renderer_paused = false,
    .neuron_color = {.active = {1.0, 1.0, 1.0, 1.0}, .inactive = {0.1, 0.1, 0.2, 1.0}},
    .synapse_color = {.active = {0.0, 0.5, 0.0, 0.5}, .inactive = {0.5, 0.0, 0.0, 0.5}},
};

// void try_load_state() {
// }

// void try_save_state() {
// }

int main() {
    if (!glfwInit()) {
        error("Failed to initialize GLFW");
        return -1;
    }

    // Get primary monitor and video mode
    GLFWmonitor *primary = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primary);
    if (!mode) {
        error("Failed to get video mode");
        glfwTerminate();
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Additional compatibility hints
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GL_TRUE);

    // For windowed mode, make it slightly smaller than full screen
    int width = mode->width / 2;   // 50% of screen width
    int height = mode->height / 2; // 50% of screen height

    GLFWwindow *window = glfwCreateWindow(width, height, "example", nullptr, nullptr);
    if (!window) {
        error("Failed to create window");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        error("Failed to initialize GLAD");
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // const char *data = file_read_to_string("example.xiu");

    Network network;
    // const auto temp = network_deserialize(network, (u8 *)data, strlen(data));
    // network_init(network, MAX_NEURONS / 8);
    network_init(network, MAX_NEURONS / 32);

    Renderer renderer;
    renderer_init(renderer);

    // Initialize imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Optional: Enable Keyboard Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    ImGui::StyleColorsDark();
    ImGui::LoadIniSettingsFromDisk("imgui.ini");

    // TODO: track time delta for network
    usize frame = 0;
    while (!glfwWindowShouldClose(window)) {
        // Start imgui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Mother Ship");
        if (ImGui::Button(state.network_paused ? "Unpause Network" : "Pause Network")) {
            state.network_paused = !state.network_paused;
        }

        if (ImGui::Button(state.renderer_paused ? "Unpause Renderer" : "Pause Renderer")) {
            state.renderer_paused = !state.renderer_paused;
        }

        static bool show_colors = false;
        if (ImGui::CollapsingHeader("Color Settings")) {
            ImGui::ColorEdit4("Active Neuron", state.neuron_color.active);
            ImGui::ColorEdit4("Inactive Neuron", state.neuron_color.inactive);
            ImGui::ColorEdit4("Active Synapse", state.synapse_color.active);
            ImGui::ColorEdit4("Inactive Synapse", state.synapse_color.inactive);
        }
        ImGui::End();

        process_input(window);

        if (!state.network_paused && frame++ % 3 == 0) {
            network_update(network);
        }

        glClear(GL_COLOR_BUFFER_BIT);
        if (!state.renderer_paused) {
            renderer_render(renderer, network, state);
        }

        // Render imgui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui::SaveIniSettingsToDisk("imgui.ini");

    // Cleanup imgui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    renderer_deinit(renderer);
    network_deinit(network);
    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void process_input(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}
