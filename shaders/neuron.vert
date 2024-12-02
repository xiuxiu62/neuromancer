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
    
    gl_Position = vec4((position * scale), 0.0, 1.0);  // Scale down positions    
    // gl_PointSize = 7.5 * viewport.y / viewport.x;
    gl_PointSize = 10.0 * viewport.y / viewport.x;
}
