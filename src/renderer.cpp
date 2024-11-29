#include "renderer.hpp"

#include "shader.hpp"

const char *vertex_shader_source = R"(
#version 430

layout(location = 0) in vec2 position;
layout(location = 1) in float activation;
layout(location = 2) in float threshold;

out float v_activation;
void main() {    
    v_activation = activation;    
    gl_Position = vec4(position * 1.5, 0.0, 1.0);  // Scale down positions    
    gl_PointSize = 100.0;
}
)";

const char *fragment_shader_source = R"(
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

void renderer_init(Renderer &renderer) {
    // Create and compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    check_shader_compilation(vertex_shader);

    // Create and compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    check_shader_compilation(fragment_shader);

    // Create shader program
    renderer.program = glCreateProgram();
    glAttachShader(renderer.program, vertex_shader);
    glAttachShader(renderer.program, fragment_shader);
    glLinkProgram(renderer.program);

    // Cleanup shaders
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Create VAO
    glGenVertexArrays(1, &renderer.vao);
    glBindVertexArray(renderer.vao);
}

void renderer_deinit(Renderer &renderer) {
    glDeleteProgram(renderer.program);
    glDeleteVertexArrays(1, &renderer.vao);
}

void renderer_render(const Renderer &renderer, const Network &network) {
    glUseProgram(renderer.program);
    glBindVertexArray(renderer.vao);

    // Bind network's neuron buffer and set up attributes
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

    // Enable blending for glow effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw points
    glDrawArrays(GL_POINTS, 0, network.neuron_count);

    // Cleanup state
    glDisable(GL_BLEND);
}
