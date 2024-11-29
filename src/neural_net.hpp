#pragma once

#include "core/logger.h"
#include "core/types.h"
#include "shader.hpp"

#include <cmath>
#include <cstdlib>

#define MAX_NEURONS 1024
#define MAX_CONNECTIONS 16

struct Network {
    GLuint program;
    GLuint neuron_buffer;
    GLuint connection_buffer;
    GLuint weight_buffer;

    f32 *neuron_data;
    i32 *connection_data;
    f32 *weight_data;
    usize neuron_count;
};

void network_init(Network &net, usize neuron_count);

void network_deinit(Network &net);

void network_update(Network &net);
