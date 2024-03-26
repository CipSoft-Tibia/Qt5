#version 440

layout(location = 0) in vec4 position;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 color;
};

void main()
{
    gl_Position = mvp  * position;
}
