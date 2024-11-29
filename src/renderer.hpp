#pragma once

#include "neural_net.hpp"

struct Renderer {
    GLuint program;
    GLuint vao;
};

void renderer_init(Renderer &renderer);
void renderer_deinit(Renderer &renderer);
void renderer_render(const Renderer &renderer, const Network &network);
