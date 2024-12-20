#pragma once

#include "core/logger.h"
#include "core/types.h"
#include "shader.hpp"

#include <cmath>
#include <cstdlib>

#define MAX_NEURONS 2048
#define MAX_SYNAPSES 16

struct Network {
    GLuint program;
    GLuint neuron_buffer;
    GLuint synapse_buffer;
    GLuint weight_buffer;

    f32 *neuron_data;
    i32 *synapse_data;
    f32 *weight_data;
    usize neuron_count;
};

struct Neuron {
    enum Kind {
        Input,
        Hidden,
        Output,
    };

    Kind kind;
    f32 activation;
    f32 threshold;
};

struct NeuronTwo {
    enum Kind { Input, Hidden, Output };

    Kind kind;
    f32 activation;
    f32 threshold;
};

struct NetworkThree {
    // const usize MAX_NEURONS = 2048;
    // const usize MAX_CONNECTIONS = 8;

    struct Connection {
        usize connection;
        f32 weight;
    };

    NeuronTwo *neurons;
    Connection *connections;
};

struct NetworkTwo {
    Neuron *neurons;
    usize neuron_count;

    i32 *connections;
    f32 *weights;
    usize max_connections;

    struct { // Remote resources
        GLuint program;
        struct {
            GLuint neurons;
            GLuint connections;
            GLuint weights;
        } buffers;
    } compute;
};

void network_init(Network &net, usize neuron_count);
void network_deinit(Network &net);
bool save(Network &net, const char *path);
bool load(Network &net, const char *path);
const u8 *serialize(Network &net);
bool deserialize(Network &net, const u8 *data, usize len);
usize network_bin_size(Network &net);
void network_update(Network &net);
