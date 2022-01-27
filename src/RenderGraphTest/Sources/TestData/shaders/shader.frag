#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;


layout(binding = 1) uniform Time {
    float time;
} time;

void main ()
{
    outColor = vec4(fragColor, time.time);
}