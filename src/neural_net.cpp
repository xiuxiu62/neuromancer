#include "neural_net.hpp"

#include "serialize.hpp"
#include "shader.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>

const char *compute_shader_source = R"(
#version 430

layout(local_size_x = 256) in;

layout(std430, binding = 0) buffer NeuronData {
  vec4 neurons[]; // x,y = position, z = activation, w = threshold
};

layout(std430, binding = 1) buffer SynapseData {
  int synapses[];
};

layout(std430, binding = 2) buffer WeightData {
  float weights[]; 
};

uniform float delta_t;

void main() {
  uint neuronId = gl_GlobalInvocationID.x;
  if (neuronId >= neurons.length()) return;

  // Get current neuron data
  vec4 neuron = neurons[neuronId];
  float activation = neuron.z;
  float threshold = neuron.w;

  // Sum inputs from connected neurons
  float input_sum = 0.0;
  uint synapse_offset = neuronId * 16;  // MAX_SYNAPSES

  // Update activation
  if (input_sum > threshold) {
    activation = 1.0;
  } else {
    // activation *= 0.95;  // Decay
    activation *= 0.9;  // Decay
  }

  // Store updated activation
  neurons[neuronId].z = activation;
}
)";

void network_init_remote_resources(Network &net, usize neuron_data_size, usize synapse_data_size,
                                   usize weight_data_size) {
    // Create OpenGL buffers
    glGenBuffers(1, &net.neuron_buffer);
    glGenBuffers(1, &net.connection_buffer);
    glGenBuffers(1, &net.weight_buffer);

    // Initialize buffers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, net.neuron_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, neuron_data_size, net.neuron_data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, net.connection_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, synapse_data_size, net.synapse_data, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, net.weight_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, weight_data_size, net.weight_data, GL_STATIC_DRAW);
}

void network_init_shaders(Network &net) {
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &compute_shader_source, NULL);
    glCompileShader(shader);
    check_shader_compilation(shader);

    net.program = glCreateProgram();
    glAttachShader(net.program, shader);
    glLinkProgram(net.program);

    glDeleteShader(shader);
}

void network_init(Network &net, usize neuron_count) {
    net.neuron_count = neuron_count;

    // Allocate host memory
    usize neuron_data_size = neuron_count * 4 * sizeof(float); // vec4 per neuron
    usize synapse_data_size = neuron_count * MAX_SYNAPSES * sizeof(int);
    usize weight_data_size = neuron_count * MAX_SYNAPSES * sizeof(float);

    net.neuron_data = (f32 *)malloc(neuron_data_size);
    net.synapse_data = (i32 *)malloc(synapse_data_size);
    net.weight_data = (f32 *)malloc(weight_data_size);

    // Initialize neurons in a spiral pattern
    for (usize i = 0; i < neuron_count; i++) {
        f32 angle = i * 0.5f;
        f32 radius = sqrt((f32)i / neuron_count);

        // Position
        net.neuron_data[i * 4 + 0] = cos(angle) * radius;
        net.neuron_data[i * 4 + 1] = sin(angle) * radius;
        net.neuron_data[i * 4 + 2] = 0.0f; // activation
        net.neuron_data[i * 4 + 3] = 0.5f; // threshold

        // Random connections
        for (int j = 0; j < MAX_SYNAPSES; j++) {
            if (j < 3 + rand() % (MAX_SYNAPSES - 3)) {
                net.synapse_data[i * MAX_SYNAPSES + j] = rand() % neuron_count;
                net.weight_data[i * MAX_SYNAPSES + j] = 0.1f + (f32)rand() / RAND_MAX * 0.4f;
            } else {
                net.synapse_data[i * MAX_SYNAPSES + j] = -1;
                net.weight_data[i * MAX_SYNAPSES + j] = 0.0f;
            }
        }
    }

    network_init_remote_resources(net, neuron_data_size, synapse_data_size, weight_data_size);
    network_init_shaders(net);
}

void network_deinit(Network &net) {
    free(net.neuron_data);
    free(net.synapse_data);
    free(net.weight_data);
}

const u8 *network_serialize(Network &net) {
    usize neuron_data_size = net.neuron_count * 4 * sizeof(f32); // vec4 per neuron
    usize synapse_data_size = net.neuron_count * MAX_SYNAPSES * sizeof(i32);
    usize weight_data_size = net.neuron_count * MAX_SYNAPSES * sizeof(f32);
    usize total_size = sizeof(usize) * 2 + neuron_data_size + synapse_data_size + weight_data_size;

    u8 *data = static_cast<u8 *>(calloc(total_size, sizeof(u8)));

    usize *header = reinterpret_cast<usize *>(data);
    usize *neuron_count = reinterpret_cast<usize *>(data + sizeof(usize));
    f32 *neuron_data = reinterpret_cast<f32 *>(data + sizeof(usize) * 2);
    i32 *synapse_data = reinterpret_cast<i32 *>(data + +sizeof(usize) * 2 + neuron_data_size);
    f32 *weight_data = reinterpret_cast<f32 *>(data + sizeof(usize) * 2 + neuron_data_size + synapse_data_size);

    *header = BIN_MAGIC;
    *neuron_count = net.neuron_count;
    memcpy(neuron_data, net.neuron_data, neuron_data_size);
    memcpy(synapse_data, net.synapse_data, synapse_data_size);
    memcpy(weight_data, net.weight_data, weight_data_size);

    return data;
}

bool network_deserialize(Network &net, const u8 *data, usize len) {
    if (len < 2 * sizeof(usize)) {
        return false;
    }

    const usize *header = reinterpret_cast<const usize *>(data);
    if (*header != BIN_MAGIC) {
        return false;
    }

    const usize *neuron_count = reinterpret_cast<const usize *>(data + sizeof(usize));
    usize neuron_data_size = *neuron_count * sizeof(Neuron);
    usize synapse_data_size = *neuron_count * MAX_SYNAPSES * sizeof(i32);
    usize weight_data_size = *neuron_count * MAX_SYNAPSES * sizeof(f32);
    usize expected_size = sizeof(usize) * 2 + neuron_data_size + synapse_data_size + weight_data_size;

    if (len != expected_size) {
        return false;
    }

    net.neuron_count = *neuron_count;
    memcpy(net.neuron_data, data + sizeof(usize) * 2, neuron_data_size);
    memcpy(net.synapse_data, data + neuron_data_size + sizeof(usize) * 2, synapse_data_size);
    memcpy(net.weight_data, data + neuron_data_size + synapse_data_size + sizeof(usize) * 2, weight_data_size);

    network_init_remote_resources(net, neuron_data_size, synapse_data_size, weight_data_size);
    network_init_shaders(net);

    return true;
}

usize network_bin_size(Network &net) {
    usize neuron_data_size = net.neuron_count * 4 * sizeof(f32); // vec4 per neuron
    usize synapse_data_size = net.neuron_count * MAX_SYNAPSES * sizeof(i32);
    usize weight_data_size = net.neuron_count * MAX_SYNAPSES * sizeof(f32);
    return sizeof(usize) * 2 + neuron_data_size + synapse_data_size + weight_data_size;
}

void network_update(Network &net, f32 delta_t) {
    // static int frame = 0;
    // if (frame++ % 120 == 0) { // Every 120 frames
    //     // Stimulate neuron 0
    //     net.neuron_data[2] = 1.0f; // Set activation to max

    //     // Upload the changed data
    //     glBindBuffer(GL_SHADER_STORAGE_BUFFER, net.neuron_buffer);
    //     glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, net.neuron_count * 4 * sizeof(float), net.neuron_data);
    // }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, net.neuron_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, net.connection_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, net.weight_buffer);

    GLint delta_t_location = glGetUniformLocation(net.program, "delta_t");
    glUseProgram(net.program);
    glUniform1f(delta_t_location, 0.016f); // ~60fps

    glUseProgram(net.program);
    glDispatchCompute((net.neuron_count + 255) / 256, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // // Read back the updated data from GPU
    // glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, net.neuron_count * 4 * sizeof(f32), net.neuron_data);

    // // Read back activation values
    // for (usize i = 0; i < net.neuron_count; i++) {
    //     glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, (i * 4 + 2) * sizeof(f32), sizeof(f32),
    //                        &net.neuron_data[i * 4 + 2]);
    // }
}
