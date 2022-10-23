#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "common.h"
#include "MeshCommon.h"
#include "PBRHelper.h"

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;
layout (location = 3) out vec2 outRoughMetal;

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
	Material mat 							= g_materials.data[g_pushConstant.material_id];
	// if roughness is 0, NDF is 0 and so is the entire cook-torrence factor withotu specular or diffuse
	outRoughMetal							= vec2(mat.roughness, mat.metallic);//vec2(max(mat.roughness, 0.1), mat.metallic);	
	outRoughMetal							= GetRoughMetalPBR(mat.roughMetal_id, inUV, outRoughMetal);

	outPosition 							= inPosition;
	outAlbedo 								= GetColor(mat.color_id, inUV); // vec4(roughMetal.x, roughMetal.y, 1.0, 1.0); //
	mat3 TBN 								= mat3(inTangent, inBiTangent, inNormal);	
	outNormal 								= vec4(GetNormal(TBN, mat.normal_id, inUV, inNormal), 1.0);
	//PickMeshID();
}