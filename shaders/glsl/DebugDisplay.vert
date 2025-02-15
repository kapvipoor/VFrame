#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common.h"
#include "DebugCommon.h"

layout (location = 0) in vec3 inPos;
layout (location = 1) in int  inId;

void main() 
{
	gl_Position = g_Info.data.camViewProj *  (g_Uniform.model[inId] * vec4(inPos, 1.0));
}