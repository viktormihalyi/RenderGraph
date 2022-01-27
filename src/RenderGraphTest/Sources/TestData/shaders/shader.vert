#version 450


layout (binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout (binding = 1) uniform Time {
    float time;
} time;

layout (binding = 2) uniform sampler2D testSampler;

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec3 inColor;

layout (location = 0) out vec3 fragColor;

void main ()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4 (inPosition + vec2 (time.time, 0.f), 0.0, 1.0);
    fragColor = texture (testSampler, vec2 (0.5, 0.5)).rgb;
}