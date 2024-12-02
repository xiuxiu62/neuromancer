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
