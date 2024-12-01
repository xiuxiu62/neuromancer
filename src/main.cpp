#include "core/logger.h"
#include "neural_net.hpp"
#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void process_input(GLFWwindow *window);

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

    Network network;
    network_init(network, MAX_NEURONS / 8);

    Renderer renderer;
    renderer_init(renderer);

    usize frame = 0;
    while (!glfwWindowShouldClose(window)) {
        // if (frame++ % 5 == 0) {
        network_update(network);
        // }
        process_input(window);

        glClear(GL_COLOR_BUFFER_BIT);
        renderer_render(renderer, network);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

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
