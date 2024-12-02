#pragma once

#include "core/types.h"

struct State {
    struct Color {
        f32 active[4];
        f32 inactive[4];
    };

    bool paused;
    Color neuron_color, synapse_color;
};
