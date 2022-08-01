#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "Common.h"
#include "MeshCommon.h"

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

#define DISPLAY_MOUSE_POINTER

void PickMeshID()
{
	vec4 fragCoord = gl_FragCoord;

	if(		fragCoord.x >= g_Info.mousePosition.x - 2.0
		&&	fragCoord.x <= g_Info.mousePosition.x + 2.0
		&&	fragCoord.y >= g_Info.mousePosition.y - 2.0
		&&	fragCoord.y <= g_Info.mousePosition.y + 2.0)
	{
		g_objpickerStorage.id = (g_pushConstant.mesh_id + 1);	

#ifdef DISPLAY_MOUSE_POINTER
		outAlbedo = vec4(1.0, 0.0, 0.0, 0.0);
#endif
	}
}

void main()
{
	Material mat = g_materials.data[g_pushConstant.material_id];
	
	outAlbedo = texture(sampler2D(g_textures[mat.color_id], g_LinearSampler), inUV);
	outPosition = inPosition;
	outNormal	= vec4(inNormal, 0.0f);
	
	mat3 TBN = mat3(inTangent, inBiTangent, inNormal);	
	if(mat.normal_id < MAX_SUPPORTED_TEXTURES)
	{
		vec2 xy = (texture(sampler2D(g_textures[mat.normal_id], g_LinearSampler), inUV).xy * 2.0) - vec2(1.0);
		float z = sqrt(1.0 - dot(xy, xy));
		outNormal.xyz = vec3(xy, z);
		outNormal.xyz = normalize(TBN * outNormal.xyz);
	}

	//PickMeshID();
}