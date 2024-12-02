#include "core/logger.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "neural_net.hpp"
#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cstring>
#include <glad/glad.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void process_input(GLFWwindow *window);

static struct {
    bool paused = false;
    struct {
        f32 active[4] = {0};
        f32 inactive[4] = {0};
    } neuron_color, synapse_color;
} state;

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

    // Setup imgui style
    ImGui::StyleColorsDark();

    // TODO: track time delta for network
    usize frame = 0;
    while (!glfwWindowShouldClose(window)) {
        // Start imgui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Mother Ship");
        if (ImGui::Button(state.paused ? "Unpause" : "Pause")) {
            state.paused = !state.paused;
        }

        static bool show_colors = false;
        if (ImGui::Button("Color Settings")) {
            show_colors = !show_colors;
        }

        if (show_colors) {
            ImGui::Begin("Colors", &show_colors);
            ImGui::ColorEdit4("Active Neuron", state.neuron_color.active);
            ImGui::ColorEdit4("Inactive Neuron", state.neuron_color.inactive);
            ImGui::ColorEdit4("Active Synapse", state.synapse_color.active);
            ImGui::ColorEdit4("Inactive Synapse", state.synapse_color.inactive);
            ImGui::End();
        }

        ImGui::End();
        if (!state.paused && frame++ % 3 == 0) {
            network_update(network);
        }

        process_input(window);

        glClear(GL_COLOR_BUFFER_BIT);
        renderer_render(renderer, network);

        // Render imgui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

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
