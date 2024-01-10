#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common.h"
#include "DebugCommon.h"

layout (location = 0) in vec3 inPos;

void main() 
{
	gl_Position = g_Info.camViewProj *  (g_Uniform.model[gl_InstanceIndex] * vec4(inPos, 1.0));
}