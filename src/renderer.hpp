#pragma once

#include "neural_net.hpp"

struct Renderer {
    // Neuron resources
    GLuint neuron_program;
    GLuint neuron_vao;

    // Synapse resources
    GLuint synapse_program;
    GLuint synapse_vao;
    GLuint synapse_buffer;

    // Post processing
    GLuint bloom_fbo;
    GLuint bloom_texture;
    GLuint bloom_ping_texture;
    GLuint bloom_pong_texture;
};

void renderer_init(Renderer &renderer);
void renderer_deinit(Renderer &renderer);
void renderer_render(const Renderer &renderer, const Network &network);
