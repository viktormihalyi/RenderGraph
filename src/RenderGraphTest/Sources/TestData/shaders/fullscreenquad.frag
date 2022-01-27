#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 textureCoords;
layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2D sampl;

void main () {
    outColor = vec4 ((texture (sampl, textureCoords).rgb + vec3 (textureCoords, 0.f)) * 0.5f, 1.f);
}