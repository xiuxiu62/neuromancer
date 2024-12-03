#pragma once

#include "core/types.h"

#define MAX_NEURONS 2048
#define MAX_CONNECTIONS 16

struct Vec3 {
    f32 x, y, z;
};

struct Neuron {
    enum Kind { Input, Hidden, Output };

    Kind kind;
    Vec3 position;
    f32 activation;
    f32 threshold;
};

struct Connection {
    u32 id;
    f32 weight;
};

struct Network {

    Neuron *neurons;
    Connection *connections;
    usize neuron_count;
    usize connection_count;

    // GPU resources
    GLuint program;
    GLuint neuron_buffer;
    GLuint connection_buffer;
};

void network_init(Network &net, usize neuron_count);
void network_deinit(Network &net);
void network_update(Network &net, f32 delta_t = 0.0f);
// bool save(Network &net, const char *path);
// bool load(Network &net, const char *path);
// const u8 *serialize(Network &net);
// bool deserialize(Network &net, const u8 *data, usize len);
// usize network_bin_size(Network &net);
