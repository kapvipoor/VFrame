#version 460

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
layout (location = 5) in vec4 inPosinClipSpace;
layout (location = 6) in vec4 inPrevPosinClipSpace;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormalMeshId;
layout (location = 2) out vec4 outAlbedo;
layout (location = 3) out vec2 outRoughMetal;
layout (location = 4) out vec2 outMotion;

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
	// if roughness is 0, NDF is 0 and so is the entire cook-Torrence factor without specular or diffuse
	outRoughMetal.xy						= vec2(mat.roughness, mat.metallic);
	outRoughMetal.xy						*= GetRoughMetalPBR(mat.roughMetal_id, inUV, outRoughMetal.xy).xy;

	outPosition 							= inPosition;
	
	outAlbedo 								= vec4(mat.pbr_color, 1.0) * GetColor(mat.color_id, inUV);
	outAlbedo								+= vec4(GetEmissive(mat, inUV), 0.0);

	mat3 TBN 								= mat3(inTangent, inBiTangent, inNormal);	
	outNormalMeshId.xyz						= GetNormal(TBN, mat.normal_id, inUV, inNormal);
	outNormalMeshId.w 						= g_pushConstant.mesh_id;

	// Calculate motion vectors
	{
		// Perspective divide both positions
		vec2 posInNDC		= inPosinClipSpace.xy/inPosinClipSpace.w;
		vec2 posInUV 		= posInNDC * vec2(0.5, -0.5) + vec2(0.5);
		
		vec2 prevPosInNDC 	= inPrevPosinClipSpace.xy/inPrevPosinClipSpace.w;
		vec2 prevPosInUV	= prevPosInNDC * vec2(0.5, -0.5) + vec2(0.5);

		outMotion.xy		= (prevPosInUV - posInUV);
	}
	//PickMeshID();
}