#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "Common.h"
#include "MeshCommon.h"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inTangent;

layout (location = 0) out vec3 outUVW;
void main() 
{
	outUVW = inPos;
	//outUVW.xy = -1.0 * outUVW.xy;
	gl_Position = g_Info.camProj * g_Info.skyboxView  * vec4(inPos, 1.0);
}