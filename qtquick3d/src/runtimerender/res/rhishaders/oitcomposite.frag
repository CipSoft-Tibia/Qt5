// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440
#extension GL_GOOGLE_include_directive : enable

#include "../effectlib/tonemapping.glsllib"
#include "../effectlib/oitcommon.glsllib"

layout(location = 0) out vec4 fragOutput;

layout(location = 0) in vec2 uv_coord;
#if QSHADER_VIEW_COUNT >= 2
layout(location = 1) flat in uint v_viewIndex;
#endif

#if QSSG_OIT_METHOD == QSSG_OIT_WEIGHTED_BLENDED

#if QSHADER_VIEW_COUNT >= 2
#   ifdef QSSG_MULTISAMPLE
#       define samplerType sampler2DMSArray
#   else
#       define samplerType sampler2DArray
#   endif
#else
#   ifdef QSSG_MULTISAMPLE
#      define samplerType sampler2DMS
#   else
#      define samplerType sampler2D
#   endif
#endif

layout(binding = 1) uniform samplerType accumTexture;
layout(binding = 2) uniform samplerType revealageTexture;

void main()
{
#if QSHADER_HLSL || QSHADER_MSL
    vec2 uv = vec2(uv_coord.x, 1.0 - uv_coord.y);

#else
    vec2 uv = uv_coord;
#endif
#if QSHADER_VIEW_COUNT >= 2
#ifdef QSSG_MULTISAMPLE
    ivec2 iuv = ivec2(uv * textureSize(accumTexture).xy);
    vec4 accum = texelFetch(accumTexture, ivec3(iuv, v_viewIndex), gl_SampleID);
    float a = 1.0 - texelFetch(revealageTexture, ivec3(iuv, v_viewIndex), gl_SampleID).r;
#else
    vec4 accum = texture(accumTexture, vec3(uv, v_viewIndex));
    float a = 1.0 - texture(revealageTexture, vec3(uv, v_viewIndex)).r;
#endif
#else
#ifdef QSSG_MULTISAMPLE
    ivec2 iuv = ivec2(uv * textureSize(accumTexture));
    vec4 accum = texelFetch(accumTexture, iuv, gl_SampleID);
    float a = 1.0 - texelFetch(revealageTexture, iuv, gl_SampleID).r;
#else
    vec4 accum = texture(accumTexture, uv);
    float a = 1.0 - texture(revealageTexture, uv).r;
#endif
#endif
    vec4 color = vec4(accum.rgb / clamp(accum.a, 1e-4, 5e6), a);
    fragOutput = vec4(color);
}

#endif // QSSG_OIT_WEIGHTED_BLENDED
