#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "Common.h"
#include "MeshCommon.h"

#define DISPLAY_MOUSE_POINTER

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec3 inBiTangent;
layout (location = 4) in vec4 inLightPosition;

layout (location = 0) out vec4 outFragColor;

void PickMeshID()
{
	vec4 fragCoord = gl_FragCoord;

	if(fragCoord.x >= g_Info.mousePosition.x - 2.0
		&&	fragCoord.x <= g_Info.mousePosition.x + 2.0
		&&	fragCoord.y >= g_Info.mousePosition.y - 2.0
		&&	fragCoord.y <= g_Info.mousePosition.y + 2.0)
	{
		g_objpickerStorage.id = (g_pushConstant.mesh_id + 1);	

#ifdef DISPLAY_MOUSE_POINTER
		outFragColor = vec4(1.0, 0.0, 0.0, 0.0);
#endif
	}
}

float InShadow(vec4 lightPositionNDC, bool applyPCF)
{
	vec2 shadowMaxSize = vec2(4096, 4096); //textureSize(g_LightDepthImage);
	// NDC ranges between -1 and 1; forcing values bewteen 0 and 1 for shadow map sampling
	vec2 lightPositionUV = lightPositionNDC.xy * 0.5f + 0.5f;
	if(lightPositionNDC.z > -1.0 && lightPositionNDC.z < 1.0f)
	{
		if(applyPCF == true)
    	{
    	    float shadowFactor = 0.0f;
    	    for(int dx = -1; dx < 2; dx++)
    	    {
    	        for(int dy = -1; dy < 2; dy++)
    	        {
    	            vec2 uv = lightPositionUV.xy + (vec2(dx, dy)/shadowMaxSize);
    	            float sampledDepth = texture(sampler2D(g_LightDepthImage, g_LinearSampler), uv).r;
    	            shadowFactor = shadowFactor + ((lightPositionNDC.z > sampledDepth) ? g_shadowFactor : 1.0f);
    	        }
    	    }

    	    return (shadowFactor / 9.0f);
    	}
    	else
    	{
    	    float sampledDepth = texture(sampler2D(g_LightDepthImage, g_LinearSampler), lightPositionUV.xy).r;
    	    return (lightPositionNDC.z > sampledDepth) ? g_shadowFactor : 1.0f;
    	}
	}
	return 1.0f;
}

void main()
{
	Material mat = g_materials.data[g_pushConstant.material_id];
	vec4 color;
	if(mat.color_id < MAX_SUPPORTED_TEXTURES)
	{
		color = texture(sampler2D(g_textures[mat.color_id], g_LinearSampler), inUV);
	}
	else
	{
		color = texture(sampler2D(g_textures[0], g_LinearSampler), inUV);
	}

	vec3 N = inNormal;

	//Test 1: Following Cauldron's way of calculating normals
	mat3 TBN = mat3(inTangent, inBiTangent, inNormal);	
	if(mat.normal_id < MAX_SUPPORTED_TEXTURES)
	{
		vec2 xy = (texture(sampler2D(g_textures[mat.normal_id], g_LinearSampler), inUV).xy * 2.0) - vec2(1.0);
		float z = sqrt(1.0 - dot(xy, xy));
		N = vec3(xy, z);
		N = normalize(TBN * N);
	}

	// applying ambient
	float lighting = g_ambientCoeff;
	vec3 lightDir = normalize(g_Info.sunLightDirWorld.xyz);
	float NdotL = max(dot(N, lightDir), 0.0);
	
	// ensuring tht shadows are not self applied
	if(NdotL > 0.0f)
	{
		float shadow = InShadow(inLightPosition/inLightPosition.w, true);

		// Simple lambertian diffuse
		lighting =  lighting + (g_diffuseCoeff * NdotL);
		lighting = lighting * shadow;
	}
	
	outFragColor = vec4(lighting * color.xyz, color.a);
	
	PickMeshID();
}