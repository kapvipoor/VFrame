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

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outTangent;
layout (location = 4) out vec3 outBiTangent;
layout (location = 5) out vec4 outPosinClipSpace;
layout (location = 6) out vec4 outPrevPosinClipSpace;

void main() 
{
	MeshData meshData 		= g_meshUniform.data[g_pushConstant.mesh_id];

	outUV 					= inUV;
	
	outNormal 				= normalize((meshData.normalMatrix * vec4(inNormal.x, inNormal.y, inNormal.z, 0.0f)).xyz); 
	outTangent 				= normalize((meshData.normalMatrix * vec4(inTangent.x, inTangent.y, inTangent.z, 0.0f)).xyz); 
	outBiTangent 			= cross(outNormal, outTangent) * inTangent.w;

	vec4 PosinWorldSpace 	= (meshData.modelMatrix * vec4(inPos, 1.0));
	gl_Position 			= g_Info.camJitteredViewProj * PosinWorldSpace;

	outPosition 			= g_Info.camView * PosinWorldSpace;

	// Used to calculate the motion vectors
	outPosinClipSpace		= g_Info.camViewProj * PosinWorldSpace;
	outPrevPosinClipSpace	= g_Info.camPreViewProj * PosinWorldSpace;
}