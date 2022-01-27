#version 450

struct A {
    vec3 abc;
    double hello;
};

struct B {
    A bs[3];
};

struct C {
    B cs[3];
    mat3x4 WTF;
    float hehe[3];
};

layout (std140, binding = 2) uniform Quadrics {
    float dddddddddddd;
    vec3 dddddddddddd33[3];
    C quadrics[2];
    A cx[56];
};

layout (location = 0) out vec4 presented;

void main ()
{
    presented = vec4 (vec3 (1), dddddddddddd);
}
