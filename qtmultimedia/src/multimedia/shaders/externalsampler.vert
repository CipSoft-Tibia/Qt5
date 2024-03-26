#version 440
#extension GL_GOOGLE_include_directive : enable

#include "uniformbuffer.glsl"

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;

layout(location = 0) out vec2 texCoord;

out gl_PerVertex { vec4 gl_Position; };

void main() {
    texCoord = (ubuf.colorMatrix * vec4(vertexTexCoord, 0.0, 1.0)).xy;
    gl_Position = ubuf.matrix * vertexPosition;
}
