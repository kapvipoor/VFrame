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

layout (location = 0) out vec3 outNormalinViewSpace;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outTangentinVieSpace;
layout (location = 3) out vec3 outBiTangentinViewSpace;
layout (location = 4) out vec4 outPosinLightSpace;
layout (location = 5) out vec4 outPosinViewSpace;
layout (location = 6) out vec4 outPosinClipSpace;
layout (location = 7) out vec4 outPrevPosinClipSpace;

// Using Gram-Schmidth process to orthogonalize Tanget w.r.t Normal
vec3 MakeTangentOrthogonal(in vec3 N, in vec3 T)
{
	return normalize(T - dot(T, N) * N);
}

void main() 
{
	MeshData meshData 					= g_meshUniform.data[g_pushConstant.mesh_id];
	outUV 								= inUV;	
	outNormalinViewSpace 				= normalize((meshData.normalMatrix * vec4(inNormal.x, inNormal.y, inNormal.z, 0.0f))).xyz; 
	outTangentinVieSpace	 			= normalize((meshData.normalMatrix * vec4(inTangent.x, inTangent.y, inTangent.z, 0.0f))).xyz; 
	outBiTangentinViewSpace 			= cross(outNormalinViewSpace, outTangentinVieSpace) * inTangent.w;
	vec4 PosinWorldSpace 				= (meshData.modelMatrix * vec4(inPos, 1.0));
	outPosinViewSpace 					= g_Info.camView * PosinWorldSpace;
	gl_Position 						= g_Info.camJitteredViewProj * PosinWorldSpace;

	// Used to calculate the motion vectors
	outPosinClipSpace					= g_Info.camViewProj * PosinWorldSpace;
	outPrevPosinClipSpace				= PosinWorldSpace;
	outPrevPosinClipSpace				= g_Info.camPreViewProj * PosinWorldSpace;

	for(int i = 0; i < g_lights.count; i++)
	{
		Light light = g_lights.lights[i];

		// Unpacking light type
		uint lightType = light.type_castShadow >> 16;

		if(lightType == DIRECTIONAL_LIGHT_TYPE)
		{
			// Light view projection matrix
			vec4 v0 = vec4(light.viewProj[0], light.viewProj[1], light.viewProj[2], light.viewProj[3]);
			vec4 v1 = vec4(light.viewProj[4], light.viewProj[5], light.viewProj[6], light.viewProj[7]);
			vec4 v2 = vec4(light.viewProj[8], light.viewProj[9], light.viewProj[10], light.viewProj[11]);
			vec4 v3 = vec4(light.viewProj[12], light.viewProj[13], light.viewProj[14], light.viewProj[15]);
			outPosinLightSpace 					= mat4(v0, v1, v2, v3) * PosinWorldSpace;
		}
	}
}