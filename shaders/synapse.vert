#version 430

layout(location = 0) in vec2 position;
layout(location = 1) in float activation;

uniform vec2 viewport;

out float v_activation;

void main() {
    v_activation = activation;    
    float aspect = viewport.x / viewport.y;    
    vec2 scale = vec2(1.0 / aspect, 1.0);    
    gl_Position = vec4(position * scale, 0.0, 1.0);
    
    // v_activation = activation;
    // gl_Position = vec4(position * 0.01, 0.0, 1.0);  // Use same scale as neurons
}
