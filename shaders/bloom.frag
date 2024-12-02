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
