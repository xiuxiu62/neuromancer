#pragma once

#include "neural_net.hpp"

struct Renderer {
    GLuint neuron_program;
    GLuint neuron_vao;

    GLuint synapse_program;
    GLuint synapse_vao;
    GLuint synapse_buffer;
};

void renderer_init(Renderer &renderer);
void renderer_deinit(Renderer &renderer);
void renderer_render(const Renderer &renderer, const Network &network);
