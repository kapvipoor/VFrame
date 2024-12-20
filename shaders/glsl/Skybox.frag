#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_nonuniform_qualifier : require
#include "Common.h"
#include "MeshCommon.h"

layout (location = 0) in vec3 inUVW;

//layout (location = 0) out vec4 outPosition;
//layout (location = 1) out vec4 outNormal;
layout (location = 0) out vec4 outFragColor;
void main() 
{
	// Forcing the highest resolution mip level since the other
	// mip levels are Pre-filtered Specular IBL data
	outFragColor = textureLod(g_env_specular_Sampler, inUVW, 0);
}