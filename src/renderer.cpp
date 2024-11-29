#include "renderer.hpp"

#include "neural_net.hpp"
#include "shader.hpp"
#include <urlmon.h>

// TODO: calculate the aspect ratio on framebuffer resize host side and pass that in
const char *neuron_vertex_shader_source = R"(
#version 430

layout(location = 0) in vec2 position;
layout(location = 1) in float activation;
layout(location = 2) in float threshold;

uniform vec2 viewport;

out float v_activation;

void main() {    
    v_activation = activation;    

    float aspect = viewport.x / viewport.y;
    vec2 scale = vec2(1.0 / aspect, 1.0);
    
    gl_Position = vec4(position * scale * 1.5, 0.0, 1.0);  // Scale down positions    
    gl_PointSize = 10.0 * viewport.y / viewport.x;
}
)";

const char *neuron_fragment_shader_source = R"(
#version 430

in float v_activation;
out vec4 fragColor;

void main() {    
    vec2 coord = gl_PointCoord * 2.0 - 1.0;    
    float r = dot(coord, coord);    
    if (r > 1.0) discard;        
    
    // Base color changes with activation
    vec3 baseColor = mix(        
        vec3(0.1, 0.2, 0.4),  // Inactive state (darker blue)
        vec3(0.4, 0.8, 1.0),  // Active state (bright blue)
        v_activation    
    );
    
    float glow = exp(-r * 1.5);
    vec3 finalColor = baseColor + vec3(0.3) * glow;
    float alpha = min(1.0, glow + 0.2);
    
    fragColor = vec4(finalColor, alpha);
}
)";

const char *bloom_vertex_shader_source = R"(
#version 430

layout(location = 0) in vec2 position;
layout(location = 0) in vec2 texcoord;

out vec2 v_texcoord;

void main() {
    v_texcoord = texcoord;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

const char *blur_vertex_shader_source = R"(
#version 430

layout(location = 0) in vec2 position;
layout(location = 0) in vec2 texcoord;

out vec2 v_texcoord;

void main() {
    v_texcoord = texcoord;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

// In renderer.cpp, add line shaders:
const char *synapse_vertex_shader_source = R"(
#version 430

layout(location = 0) in vec2 position;
layout(location = 1) in float activation;

uniform vec2 viewport;

out float v_activation;

void main() {
    v_activation = activation;    
    float aspect = viewport.x / viewport.y;    
    vec2 scale = vec2(1.0 / aspect, 1.0);    
    gl_Position = vec4(position * scale * 2.0, 0.0, 1.0);
    
    // v_activation = activation;
    // gl_Position = vec4(position * 0.01, 0.0, 1.0);  // Use same scale as neurons
}
)";

const char *synapse_fragment_shader_source = R"(
#version 430

in float v_activation;
out vec4 fragColor;

void main() {
    vec3 color = mix(
        vec3(0.05, 0.1, 0.15),  // Dim color for inactive connections
        vec3(0.1, 0.2, 0.4),  // Brighter for active connections
        v_activation
    );

    float alpha = 0.1 * v_activation;
    
    fragColor = vec4(color, alpha);  // Semi-transparent lines
}
)";

void renderer_render_synapses(const Renderer &renderer, const Network &network);
void renderer_render_neurons(const Renderer &renderer, const Network &network);
void renderer_update_synapse_buffer(const Renderer &renderer, const Network &network, f32 *synapse_data,
                                    usize &synapse_count);

void renderer_init(Renderer &renderer) {
    // Neuron
    //
    // Create and compile shaders
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &neuron_vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    check_shader_compilation(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &neuron_fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    check_shader_compilation(fragment_shader);

    // Create shader program
    renderer.neuron_program = glCreateProgram();
    glAttachShader(renderer.neuron_program, vertex_shader);
    glAttachShader(renderer.neuron_program, fragment_shader);
    glLinkProgram(renderer.neuron_program);

    // Cleanup shaders
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Create VAO
    glGenVertexArrays(1, &renderer.neuron_vao);
    glBindVertexArray(renderer.neuron_vao);

    // Synapse
    //
    // Create and compile shaders
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &synapse_vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    check_shader_compilation(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &synapse_fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    check_shader_compilation(fragment_shader);

    // Create shader program
    renderer.synapse_program = glCreateProgram();
    glAttachShader(renderer.synapse_program, vertex_shader);
    glAttachShader(renderer.synapse_program, fragment_shader);
    glLinkProgram(renderer.synapse_program);

    // Cleanup shaders
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Create VAO and buffer
    glGenVertexArrays(1, &renderer.synapse_vao);
    glGenBuffers(1, &renderer.synapse_buffer);
}

void renderer_deinit(Renderer &renderer) {
    glDeleteProgram(renderer.neuron_program);
    glDeleteProgram(renderer.synapse_program);

    glDeleteVertexArrays(1, &renderer.neuron_vao);
    glDeleteVertexArrays(1, &renderer.synapse_vao);
}

// void renderer_render(const Renderer &renderer, const Network &network) {
//     GLint viewport[4];
//     glGetIntegerv(GL_VIEWPORT, viewport);
//     f32 width = static_cast<f32>(viewport[2]);
//     f32 height = static_cast<f32>(viewport[3]);

//     glUseProgram(renderer.neuron_program);
//     GLint neuron_viewport_loc = glGetUniformLocation(renderer.neuron_program, "viewport");
//     glUniform2f(neuron_viewport_loc, width, height);

//     glBindVertexArray(renderer.neuron_vao);

//     // TODO: This is our update procedure and should be cached performed externally and cached in the Renderer
//     const usize SYNAPSE_CACHE_STRIDE = MAX_NEURONS * MAX_SYNAPSES * 6;
//     f32 synapse_data[SYNAPSE_CACHE_STRIDE]; // 6 floats per synapse (x1, y1, a1, x2, y2, a2);
//     usize synapse_count = 0;

//     for (usize i = 0; i < network.neuron_count; i++) {
//         float x1 = network.neuron_data[i * 4 + 0];
//         float y1 = network.neuron_data[i * 4 + 1];
//         float activation1 = network.neuron_data[i * 4 + 2];

//         for (int j = 0; j < MAX_SYNAPSES; j++) {
//             int target = network.synapse_data[i * MAX_SYNAPSES + j];
//             if (target >= 0) {
//                 float x2 = network.neuron_data[target * 4 + 0];
//                 float y2 = network.neuron_data[target * 4 + 1];
//                 float activation2 = network.neuron_data[target * 4 + 2];

//                 // Ensure we don't overflow the array
//                 if (synapse_count + 6 <= MAX_SYNAPSES) {
//                     synapse_data[synapse_count++] = x1;
//                     synapse_data[synapse_count++] = y1;
//                     synapse_data[synapse_count++] = activation1;
//                     synapse_data[synapse_count++] = x2;
//                     synapse_data[synapse_count++] = y2;
//                     synapse_data[synapse_count++] = activation2;
//                 }
//             }
//         }
//     }

//     // Upload synapse data
//     glBindBuffer(GL_ARRAY_BUFFER, renderer.synapse_buffer);
//     glBufferData(GL_ARRAY_BUFFER, SYNAPSE_CACHE_STRIDE * sizeof(f32), synapse_data, GL_STREAM_DRAW);

//     // Setup synapse attrs
//     glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 3,
//                           (void *)0); // position
//     glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(f32) * 3,
//                           (void *)(sizeof(f32) * 2)); // activation
//     glEnableVertexAttribArray(0);
//     glEnableVertexAttribArray(1);

//     // Bind network's neuron buffer and set up attributes
//     glBindBuffer(GL_ARRAY_BUFFER, network.neuron_buffer);

//     // Position (x,y)
//     glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)0);

//     // Activation
//     glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 2));

//     // Threshold
//     glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 3));
//     glEnableVertexAttribArray(0);
//     glEnableVertexAttribArray(1);
//     glEnableVertexAttribArray(2);

//     // Enable blending for glow effect
//     glEnable(GL_BLEND);
//     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//     // Draw points
//     glDrawArrays(GL_POINTS, 0, network.neuron_count);

//     // Cleanup state
//     glDisable(GL_BLEND);
// }

void renderer_render(const Renderer &renderer, const Network &network) {
    // Then render neurons on top
    renderer_render_neurons(renderer, network);

    // Render synapses first (they should be behind neurons)
    renderer_render_synapses(renderer, network);
}

void renderer_render_neurons(const Renderer &renderer, const Network &network) {
    // Get viewport dimensions
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float width = static_cast<float>(viewport[2]);
    float height = static_cast<float>(viewport[3]);

    glUseProgram(renderer.neuron_program);
    GLint viewport_loc = glGetUniformLocation(renderer.neuron_program, "viewport");
    glUniform2f(viewport_loc, width, height);

    glBindVertexArray(renderer.neuron_vao);
    glBindBuffer(GL_ARRAY_BUFFER, network.neuron_buffer);

    // Position (x,y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)0);
    // Activation
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 2));
    // Threshold
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 3));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_POINTS, 0, network.neuron_count);

    glDisable(GL_BLEND);
}

void renderer_render_synapses(const Renderer &renderer, const Network &network) {
    // Get viewport dimensions
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float width = static_cast<float>(viewport[2]);
    float height = static_cast<float>(viewport[3]);

    glUseProgram(renderer.synapse_program);
    GLint viewport_loc = glGetUniformLocation(renderer.synapse_program, "viewport");
    glUniform2f(viewport_loc, width, height);

    glBindVertexArray(renderer.synapse_vao);

    // Enable blending for transparent lines
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const usize SYNAPSE_CACHE_STRIDE = MAX_NEURONS * MAX_SYNAPSES * 6;
    f32 synapse_data[SYNAPSE_CACHE_STRIDE];
    usize synapse_count = 0;

    renderer_update_synapse_buffer(renderer, network, synapse_data, synapse_count);

    // Draw the synapses as lines
    glDrawArrays(GL_LINES, 0, synapse_count / 3); // 3 floats per vertex

    glDisable(GL_BLEND);
}

void renderer_update_synapse_buffer(const Renderer &renderer, const Network &network, f32 *synapse_data,
                                    usize &synapse_count) {
    synapse_count = 0;

    for (usize i = 0; i < network.neuron_count; i++) {
        float x1 = network.neuron_data[i * 4 + 0];
        float y1 = network.neuron_data[i * 4 + 1];
        float activation1 = network.neuron_data[i * 4 + 2];

        for (int j = 0; j < MAX_SYNAPSES; j++) {
            int target = network.synapse_data[i * MAX_SYNAPSES + j];
            if (target >= 0) {
                float x2 = network.neuron_data[target * 4 + 0];
                float y2 = network.neuron_data[target * 4 + 1];
                float activation2 = network.neuron_data[target * 4 + 2];

                // First vertex of the line
                synapse_data[synapse_count++] = x1;
                synapse_data[synapse_count++] = y1;
                synapse_data[synapse_count++] = activation1;

                // Second vertex of the line
                synapse_data[synapse_count++] = x2;
                synapse_data[synapse_count++] = y2;
                synapse_data[synapse_count++] = activation2;
            }
        }
    }

    // Upload synapse data
    glBindBuffer(GL_ARRAY_BUFFER, renderer.synapse_buffer);
    glBufferData(GL_ARRAY_BUFFER, synapse_count * sizeof(f32), synapse_data, GL_STREAM_DRAW);

    // Setup synapse attributes
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, (void *)0);                 // position
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, (void *)(sizeof(f32) * 2)); // activation
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
}
