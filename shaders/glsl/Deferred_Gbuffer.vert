#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.h"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inTangent;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outTangent;
layout (location = 4) out vec3 outBiTangent;

void main() 
{
	//MeshData meshData = g_meshUniform.data[g_pushConstant.mesh_id];

	// since gbuffers are required for screen space computtion, multiplying with view vector
	//outPosition 	= meshData.model * vec4(inPos, 1.0);
	//outUV 			= inUV;

	// T, B, N in view space
	//outNormal 		= normalize(meshData.trans_inv_model * vec4(inNormal.x, inNormal.y, inNormal.z, 0.0)).xyz;
	//outTangent 		= normalize(meshData.trans_inv_model * vec4(inTangent.x, inTangent.y, inTangent.z, 0.0)).xyz;
	//outBiTangent 	= cross(outNormal, outTangent) * inTangent.w;

	//outNormal 		= (g_Info.camView * vec4(outNormal, 0.0)).xyz;
	//outTangent 		= (g_Info.camView * vec4(outTangent, 0.0)).xyz;
	//outBiTangent 	= (g_Info.camView * vec4(outBiTangent, 0.0)).xyz;
	
	//gl_Position 	= g_Info.camViewProj * meshData.model * vec4(inPos, 1.0);



	MeshData meshData 	= g_meshUniform.data[g_pushConstant.mesh_id];

	outUV 				= inUV;
	
	outNormal 			= normalize((meshData.trans_inv_model * vec4(inNormal.x, inNormal.y, inNormal.z, 0.0f)).xyz); 
	outTangent 			= normalize((meshData.trans_inv_model * vec4(inTangent.x, inTangent.y, inTangent.z, 0.0f)).xyz); 
	outBiTangent 		= cross(outNormal, outTangent) * inTangent.w;

	outPosition 		= (meshData.model * vec4(inPos, 1.0));
	gl_Position 		= g_Info.camViewProj * outPosition;

	outPosition 		= g_Info.camView * outPosition;
	outNormal 			= (g_Info.camView * vec4(outNormal, 0.0)).xyz;
	outTangent 			= (g_Info.camView * vec4(outTangent, 0.0)).xyz;
	outBiTangent 		= (g_Info.camView * vec4(outBiTangent, 0.0)).xyz;
}