#version 430

in float v_activation;
out vec4 fragColor;

void main() {
    vec3 color = mix(
        vec3(0.3, 0.0, 0.0),  // Dim color for inactive connections
        vec3(0.0, 0.3, 0.0),  // Brighter for active connections
        v_activation
    );

    // float alpha = 0.3 * v_activation;
    
    fragColor = vec4(color, 0.2);  // Semi-transparent lines
}
