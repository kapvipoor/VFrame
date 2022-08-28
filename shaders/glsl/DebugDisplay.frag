#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "Common.h"
#include "DebugCommon.h"

layout (location = 0) out vec4 outFragColor;

void main()
{
	outFragColor = vec4(0.0, 0.5f, 0.5f, 1.0);
}