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

void main() 
{
	MeshData meshData 			= g_meshUniform.data[g_pushConstant.mesh_id];

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

			gl_Position 				= mat4(v0, v1, v2, v3) * meshData.modelMatrix * vec4(inPos, 1.0);
		}
	}
}