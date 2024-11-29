#include "neural_net.hpp"

const char *compute_shader_source = R"(
#version 430

layout(local_size_x = 256) in;

layout(std430, binding = 0) buffer NeuronData {
  vec4 neurons[]; // x,y = position, z = activation, w = threshold
};

layout(std430, binding = 1) buffer ConnectionData {
  int connections[];
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
  uint connection_offset = neuronId * 16;  // MAX_CONNECTIONS

  // Update activation
  if (input_sum > threshold) {
    activation = 1.0;
  } else {
    activation *= 0.95;  // Decay
  }

  // Store updated activation
  neurons[neuronId].z = activation;
}
)";

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
    usize connection_data_size = neuron_count * MAX_CONNECTIONS * sizeof(int);
    usize weight_data_size = neuron_count * MAX_CONNECTIONS * sizeof(float);

    net.neuron_data = (f32 *)malloc(neuron_data_size);
    net.connection_data = (i32 *)malloc(connection_data_size);
    net.weight_data = (f32 *)malloc(weight_data_size);

    // Initialize neurons in a spiral pattern
    for (usize i = 0; i < neuron_count; i++) {
        f32 angle = i * 0.5f;
        f32 radius = sqrt(i) * 20.0f;

        // Position
        net.neuron_data[i * 4 + 0] = (cos(angle) * radius) / (sqrt(neuron_count) * 20.0f);
        net.neuron_data[i * 4 + 1] = (sin(angle) * radius) / (sqrt(neuron_count) * 20.0f);
        // net.neuron_data[i * 4 + 0] = cos(angle) * radius;
        // net.neuron_data[i * 4 + 1] = sin(angle) * radius;
        net.neuron_data[i * 4 + 2] = 0.0f; // activation
        net.neuron_data[i * 4 + 3] = 0.5f; // threshold

        // Random connections
        for (int j = 0; j < MAX_CONNECTIONS; j++) {
            if (j < 3 + rand() % (MAX_CONNECTIONS - 3)) {
                net.connection_data[i * MAX_CONNECTIONS + j] = rand() % neuron_count;
                net.weight_data[i * MAX_CONNECTIONS + j] = 0.1f + (f32)rand() / RAND_MAX * 0.4f;
            } else {
                net.connection_data[i * MAX_CONNECTIONS + j] = -1;
                net.weight_data[i * MAX_CONNECTIONS + j] = 0.0f;
            }
        }
    }

    // Create OpenGL buffers
    glGenBuffers(1, &net.neuron_buffer);
    glGenBuffers(1, &net.connection_buffer);
    glGenBuffers(1, &net.weight_buffer);

    // Initialize buffers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, net.neuron_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, neuron_data_size, net.neuron_data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, net.connection_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, connection_data_size, net.connection_data, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, net.weight_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, weight_data_size, net.weight_data, GL_STATIC_DRAW);

    // Create VAO for rendering
    // glGenVertexArrays(1, &net.vao);
    // glBindVertexArray(net.vao);

    // glBindBuffer(GL_ARRAY_BUFFER, net.neuron_buffer);
    // glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)0);                   // position
    // glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 2)); // activation
    // glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 3)); // threshold

    // glEnableVertexAttribArray(0);
    // glEnableVertexAttribArray(1);
    // glEnableVertexAttribArray(2);

    network_init_shaders(net);
}

void network_deinit(Network &net) {
    free(net.neuron_data);
    free(net.connection_data);
    free(net.weight_data);
}

void network_update(Network &net) {
    static int frame = 0;
    if (frame++ % 120 == 0) { // Every 120 frames
        // Stimulate neuron 0
        net.neuron_data[2] = 1.0f; // Set activation to max

        // Upload the changed data
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, net.neuron_buffer);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, net.neuron_count * 4 * sizeof(float), net.neuron_data);
    }

    glad_glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, net.neuron_buffer);
    glad_glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, net.connection_buffer);
    glad_glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, net.weight_buffer);

    GLint delta_t_location = glGetUniformLocation(net.program, "delta_t");
    glUseProgram(net.program);
    glUniform1f(delta_t_location, 0.016f); // ~60fps

    glUseProgram(net.program);
    glDispatchCompute((net.neuron_count + 255) / 256, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back the updated data from GPU
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, net.neuron_count * 4 * sizeof(f32), net.neuron_data);
}