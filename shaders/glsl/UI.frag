#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "Common.h"
#include "UICommon.h"

#define DISPLAY_MOUSE_POINTER

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

vec4 SampleUITexture(uint id, vec2 uv)
{
	return texture(sampler2D(g_uiTexture[id], g_LinearSampler), uv).xyzw;
}

void main()
{
    outFragColor = inColor *  SampleUITexture(0, inUV); // SamplePrimaryColor(inUV) *
}