#include "network.hpp"

#include "neural_net.hpp"
#include "shader.hpp"

#include <cmath>
#include <cstdlib>

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

void init_gpu_resources(Network &net, usize neuron_buffer_size, usize connection_buffer_size);
void init_shaders(Network &net);

void network_init(Network &net, usize neuron_count) {
    net.neuron_count = neuron_count;
    net.connection_count = 0;

    usize neuron_data_size = sizeof(Neuron) * neuron_count;
    usize connection_data_size = sizeof(Connection) * MAX_CONNECTIONS * neuron_count;

    net.neurons = static_cast<Neuron *>(malloc(neuron_data_size));
    net.connections = static_cast<Connection *>(malloc(connection_data_size));

    for (usize i = 0; i < neuron_count; i++) {
        f32 angle = i * 0.5f;
        f32 radius = sqrt((f32)i / neuron_count);

        Neuron &neuron = net.neurons[i];
        neuron.position.x = cos(angle) * radius;
        neuron.position.y = sin(angle) * radius;
        neuron.position.z = 0.0f;
        neuron.activation = 0.0f; // activation
        neuron.threshold = 0.5f;  // threshold

        for (usize j = 0; j < MAX_CONNECTIONS; j++) {
            if (j < 3 + rand() % (MAX_CONNECTIONS - 3)) {
                net.connections[net.connection_count] = {
                    .id = static_cast<u32>(rand() % neuron_count),
                    .weight = 0.1f + static_cast<f32>(rand()) / RAND_MAX * 0.4f,
                };
                net.connection_count++;
            }
        }
    }

    connection_data_size = sizeof(Connection) * net.connection_count * neuron_count;
    net.connections =
        static_cast<Connection *>(realloc(reinterpret_cast<void *>(net.connections), connection_data_size));

    init_gpu_resources(net, neuron_data_size, connection_data_size);
    init_shaders(net);
}

void network_deinit(Network &net) {
    free(net.neurons);
    free(net.connections);
}

void init_gpu_resources(Network &net, usize neuron_buffer_size, usize connection_buffer_size) {
    // Create storage buffers
    GLuint buffers[2];
    glGenBuffers(2, buffers);
    net.neuron_buffer = buffers[0];
    net.connection_buffer = buffers[1];

    // Initialize buffers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, net.neuron_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, neuron_buffer_size, net.neurons, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, net.connection_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, connection_buffer_size, net.connections, GL_STATIC_DRAW);
}

void init_shaders(Network &net) {
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &compute_shader_source, NULL);
    glCompileShader(shader);
    check_shader_compilation(shader);

    net.program = glCreateProgram();
    glAttachShader(net.program, shader);
    glLinkProgram(net.program);

    glDeleteShader(shader);
}
