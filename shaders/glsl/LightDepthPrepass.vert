#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.h"
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inTangent;

void main() 
{
	MeshData meshData = g_meshUniform.data[g_pushConstant.mesh_id];
	gl_Position = g_Info.sunLightViewProj * meshData.model * vec4(inPos, 1.0);
}