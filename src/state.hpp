#pragma once

#include "core/types.h"
// #include "serialize.hpp"

// #include <cstdlib>
// #include <cstring>

struct State {
    struct Color {
        f32 active[4];
        f32 inactive[4];
    };

    bool network_paused, renderer_paused;
    Color neuron_color, synapse_color;
};

// bool save(State &state, const char *path) {
//     return false;
// }

// bool load(State &state, const char *path) {
//     return false;
// }

// const u8 *serialize(State &state) {
//     constexpr usize header_size = sizeof(usize);
//     constexpr usize state_size = sizeof(State::Color) * 2;
//     constexpr usize total_size = header_size + state_size + sizeof(u8) /* null terminator */;

//     constexpr usize paused_size = sizeof(bool);
//     constexpr usize color_state_size = sizeof(State::Color);

//     u8 *data = static_cast<u8 *>(calloc(total_size, sizeof(u8)));

//     usize *header = reinterpret_cast<usize *>(data);
//     bool *paused = reinterpret_cast<bool *>(data + header_size);
//     State::Color *neuron_color = reinterpret_cast<State::Color *>(data + header_size + paused_size);
//     State::Color *synapse_color = reinterpret_cast<State::Color *>(data + header_size + paused_size +
//     color_state_size); u8 *terminator = reinterpret_cast<u8 *>(data + header_size + paused_size + color_state_size *
//     2);

//     *header = BIN_MAGIC;
//     memcpy(neuron_color->active, state.neuron_color.active, sizeof(f32) * 4);
//     memcpy(neuron_color->inactive, state.neuron_color.inactive, sizeof(f32) * 4);
//     memcpy(synapse_color->active, state.synapse_color.active, sizeof(f32) * 4);
//     memcpy(synapse_color->inactive, state.synapse_color.inactive, sizeof(f32) * 4);
//     *terminator = '\0';

//     return data;
// }

// // Assumes data is null terminated
// bool deserialize(State &state, const u8 *data) {
//     constexpr usize header_size = sizeof(usize);
//     constexpr usize state_size = sizeof(State);
//     // TODO: ensure we need to include the terminator in total_size
//     constexpr usize total_size = header_size + state_size + sizeof(u8);

//     const usize *header = reinterpret_cast<const usize *>(data);
//     if (*header != BIN_MAGIC) {
//         return false;
//     }

//     const usize len = strlen((char *)data);
//     if (len < total_size) {
//         return false;
//     }

//     const usize paused_size = sizeof(bool);
//     const usize color_state_size = sizeof(State::Color);

//     const bool *paused = reinterpret_cast<const bool *>(data + header_size);

//     const State::Color *neuron_color = reinterpret_cast<const State::Color *>(data + header_size + paused_size);
//     const State::Color *synapse_color =
//         reinterpret_cast<const State::Color *>(data + header_size + paused_size + color_state_size);

//     state.network_paused = false;
//     state.renderer_paused = false;
//     memcpy(state.neuron_color.active, neuron_color->active, sizeof(f32) * 4);
//     memcpy(state.neuron_color.inactive, neuron_color->inactive, sizeof(f32) * 4);
//     memcpy(state.synapse_color.active, synapse_color->active, sizeof(f32) * 4);
//     memcpy(state.synapse_color.inactive, synapse_color->inactive, sizeof(f32) * 4);

//     return true;
// }
