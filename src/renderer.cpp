#include "renderer.hpp"

#include "neural_net.hpp"
#include "shader.hpp"

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
    float inverse_aspect = viewport.y / viewport.x;
    vec2 scale = vec2(1.0 / aspect, 1.0);
    
    gl_Position = vec4((position * scale), 0.0, 1.0);  // Scale down positions    
    // gl_PointSize = 7.5 * viewport.y / viewport.x;
    gl_PointSize = 7.5 * inverse_aspect;
}
)";

const char *neuron_fragment_shader_source = R"(
#version 430

in float v_activation;
out vec4 fragColor;

uniform vec4 active_color;
uniform vec4 inactive_color;

void main() {    
    vec2 coord = gl_PointCoord * 2.0 - 1.0;    
    float r = dot(coord, coord);    
    if (r > 1.0) discard;        
    
    // Base color changes with activation
    vec4 baseColor = mix(        
        inactive_color,
        active_color,
        v_activation    
    );
    
    float glow = exp(-r * 1.5);
    vec3 finalColor = baseColor.rgb + vec3(0.1) * glow;
    float alpha = min(baseColor.a, glow + 0.2);
    
    fragColor = vec4(finalColor, alpha);
}
)";

const char *synapse_vertex_shader_source = R"(
#version 430

layout(location = 0) in uint vertex_id;

layout(std430, binding = 0) buffer NeuronData {
    vec4 neurons[];
};

layout(std430, binding = 1) buffer SynapseData {
    int synapses[];
};

uniform vec2 viewport;
uniform int max_synapses;

out float v_activation;

void main() {
    uint synapse_id = vertex_id / 2;
    uint is_end = vertex_id % 2;

    uint source_neuron = synapse_id / max_synapses;
    uint synapse_offset = synapse_id % max_synapses;

    int target_neuron = synapses[source_neuron * max_synapses + synapse_offset];
    
    // Get the appropriate neuron data based on whether this is start or end of line
    vec4 neuron = neurons[is_end == 1 ? target_neuron : source_neuron];
    
    v_activation = neuron.z;  // activation is stored in z component
    
    float aspect = viewport.x / viewport.y;    
    vec2 scale = vec2(1.0 / aspect, 1.0);    
    gl_Position = vec4(neuron.xy * scale, 0.0, 1.0);
}
)";

const char *synapse_fragment_shader_source = R"(
#version 430

in float v_activation;
out vec4 fragColor;

uniform vec4 active_color;
uniform vec4 inactive_color;

void main() {
    vec4 color = mix(
        inactive_color,
        active_color,
        v_activation
    );

    // float alpha = color.a * v_activation;
  
    // fragColor = vec4(color.rgb, alpha);  // Semi-transparent lines
    fragColor = color;
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

uniform sampler2D texture;
uniform vec2 direction;
uniform vec2 resolution;

void main() {
    vec2 texel = 1.0 / resolution;
    vec3 color = vec3(0.0);
    
    // 9-tap Gaussian blur
    color += texture(texture, v_texcoord + texel * direction * -4.0).rgb * 0.0162;
    color += texture(texture, v_texcoord + texel * direction * -3.0).rgb * 0.0540;
    color += texture(texture, v_texcoord + texel * direction * -2.0).rgb * 0.1216;
    color += texture(texture, v_texcoord + texel * direction * -1.0).rgb * 0.1945;
    color += texture(texture, v_texcoord).rgb * 0.2270;
    color += texture(texture, v_texcoord + texel * direction * 1.0).rgb * 0.1945;
    color += texture(texture, v_texcoord + texel * direction * 2.0).rgb * 0.1216;
    color += texture(texture, v_texcoord + texel * direction * 3.0).rgb * 0.0540;
    color += texture(texture, v_texcoord + texel * direction * 4.0).rgb * 0.0162;

    fragColor = vec4(color, 1.0);
}
)";

const char *bloom_fragment_shader = R"(
#version 430

in vec2 v_texcoord;
out vec4 frag_color;

uniform sampler2D origin;
uniform sampler2D bloom;

void main() {
    vec3 original = texture(original, v_texcoord).rgb;
    vec3 bloom = texture(bloom, v_texcoord).rgb;

    vec3 final = original + bloom * 2.0;

    fragColor = vec4(final, 1.0);
}
)";

// void create_framebuffers(Renderer &renderer, usize width, usize height) {
// }

void renderer_render_synapses(const Renderer &renderer, const Network &network, const State &state);
void renderer_render_neurons(const Renderer &renderer, const Network &network, const State &state);
void renderer_update_synapse_buffer(const Renderer &renderer, const Network &network, f32 *synapse_data,
                                    usize &synapse_count);

void renderer_init(Renderer &renderer) {
    glEnable(GL_PROGRAM_POINT_SIZE);

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

void renderer_render(const Renderer &renderer, const Network &network, const State &state) {
    // Render synapses first (they should be behind neurons)
    renderer_render_synapses(renderer, network, state);

    // Then render neurons on top
    renderer_render_neurons(renderer, network, state);
}

void renderer_render_neurons(const Renderer &renderer, const Network &network, const State &state) {
    // Get viewport dimensions
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float width = static_cast<float>(viewport[2]);
    float height = static_cast<float>(viewport[3]);

    glUseProgram(renderer.neuron_program);

    GLint viewport_loc = glGetUniformLocation(renderer.neuron_program, "viewport");
    GLint active_loc = glGetUniformLocation(renderer.neuron_program, "active_color");
    GLint inactive_loc = glGetUniformLocation(renderer.neuron_program, "inactive_color");

    glUniform2f(viewport_loc, width, height);
    glUniform4fv(active_loc, 1, state.neuron_color.active);
    glUniform4fv(inactive_loc, 1, state.neuron_color.inactive);

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

void renderer_render_synapses(const Renderer &renderer, const Network &network, const State &state) {
    // Get viewport dimensions
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float width = static_cast<float>(viewport[2]);
    float height = static_cast<float>(viewport[3]);

    glUseProgram(renderer.synapse_program);

    GLint viewport_loc = glGetUniformLocation(renderer.synapse_program, "viewport");
    GLint active_loc = glGetUniformLocation(renderer.synapse_program, "active_color");
    GLint inactive_loc = glGetUniformLocation(renderer.synapse_program, "inactive_color");
    GLint max_synapses_loc = glGetUniformLocation(renderer.synapse_program, "max_synapses");

    glUniform2f(viewport_loc, width, height);
    glUniform4fv(active_loc, 1, state.synapse_color.active);
    glUniform4fv(inactive_loc, 1, state.synapse_color.inactive);
    glUniform1i(max_synapses_loc, MAX_SYNAPSES);

    glBindVertexArray(renderer.synapse_vao);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, network.neuron_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, network.connection_buffer);

    // Enable blending for transparent lines
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw all possible synapse vertices
    usize total_vertices = network.neuron_count * MAX_SYNAPSES * 2;
    glDrawArrays(GL_LINES, 0, total_vertices);

    // const usize SYNAPSE_CACHE_STRIDE = MAX_NEURONS * MAX_SYNAPSES * 6;
    // f32 synapse_data[SYNAPSE_CACHE_STRIDE];
    // usize synapse_count = 0;

    // renderer_update_synapse_buffer(renderer, network, synapse_data, synapse_count);

    // Draw the synapses as lines
    // glDrawArrays(GL_LINES, 0, synapse_count / 3); // 3 floats per vertex

    glDisable(GL_BLEND);
}

// void renderer_update_synapse_buffer(const Renderer &renderer, const Network &network, f32 *synapse_data,
//                                     usize &synapse_count) {
//     synapse_count = 0;

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

//                 // First vertex of the line
//                 synapse_data[synapse_count++] = x1;
//                 synapse_data[synapse_count++] = y1;
//                 synapse_data[synapse_count++] = activation1;

//                 // Second vertex of the line
//                 synapse_data[synapse_count++] = x2;
//                 synapse_data[synapse_count++] = y2;
//                 synapse_data[synapse_count++] = activation2;
//             }
//         }
//     }

//     // Upload synapse data
//     glBindBuffer(GL_ARRAY_BUFFER, renderer.synapse_buffer);
//     glBufferData(GL_ARRAY_BUFFER, synapse_count * sizeof(f32), synapse_data, GL_STREAM_DRAW);

//     // Setup synapse attributes
//     glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, (void *)0);                 // position
//     glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, (void *)(sizeof(f32) * 2)); // activation
//     glEnableVertexAttribArray(0);
//     glEnableVertexAttribArray(1);
// }
