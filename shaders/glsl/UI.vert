#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common.h"
#include "UICommon.h"

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inColor;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;

out gl_PerVertex { vec4 gl_Position; };

void main() 
{
	outColor = inColor;
	outUV = inUV;
	gl_Position = vec4(inPos * g_pushConstant.scale + g_pushConstant.translate, 0.0f, 1.0f);
}