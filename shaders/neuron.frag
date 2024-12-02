#version 430

in float v_activation;
out vec4 fragColor;

void main() {    
    vec2 coord = gl_PointCoord * 2.0 - 1.0;    
    float r = dot(coord, coord);    
    if (r > 1.0) discard;        
    
    // Base color changes with activation
    vec3 baseColor = mix(        
        vec3(0.1, 0.1, 0.2),  // Inactive state (darker blue)
        // vec3(0.4, 0.8, 1.0),  // Active state (bright blue)
        vec3(1.0, 1.0, 1.0),  // Active state (bright blue)
        v_activation    
    );
    
    float glow = exp(-r * 1.5);
    vec3 finalColor = baseColor + vec3(0.1) * glow;
    float alpha = min(1.0, glow + 0.2);
    
    fragColor = vec4(finalColor, alpha);
}
