#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "common.h"
#include "MeshCommon.h"

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;
layout (location = 5) in vec4 inPosinClipSpace;
layout (location = 6) in vec4 inPrevPosinClipSpace;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
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
	outRoughMetal.xy						= GetRoughMetalPBR(mat.roughMetal_id, inUV, outRoughMetal.xy).xy;

	outPosition 							= inPosition;
	outAlbedo 								= GetColor(mat.color_id, inUV);
	mat3 TBN 								= mat3(inTangent, inBiTangent, inNormal);	
	outNormal 								= vec4(GetNormal(TBN, mat.normal_id, inUV, inNormal), 1.0);

	// Calculate motion vectors
	{
		// Perspective divide both positions
		vec2 posinClipSpace 					= inPosinClipSpace.xy/inPosinClipSpace.w;
		vec2 prevPosinClipSpace 				= inPrevPosinClipSpace.xy/inPrevPosinClipSpace.w;

		// Compute difference and convert to UVs
		vec2 velocity							= (prevPosinClipSpace - posinClipSpace);
		velocity								= (velocity * vec2(0.5, -0.5));

		// now correct the motion vectors with the jitter that has been applied to
		// the current view-projection matrix only - g_Info.camViewProj
		// No need to do this because we have multiplied with non-jittered
		// viewproj of this and previous frame
		//velocity								-= g_Info.taaJitterOffset;
		outMotion.xy							= velocity;
	}
	//PickMeshID();
}