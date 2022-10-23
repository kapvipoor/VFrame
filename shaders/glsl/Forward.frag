#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "common.h"
#include "MeshCommon.h"
#include "PBRHelper.h"

#define DISPLAY_MOUSE_POINTER

layout (location = 0) in vec3 inNormalinViewSpace;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inTangentinViewSpace;
layout (location = 3) in vec3 inBiTangentinViewSpace;
layout (location = 4) in vec4 inPosinLightSpace;
layout (location = 5) in vec4 inPosinViewSpace;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outFragColor;

#define SHADOW_BIAS 0.005

void PickMeshID()
{
	vec4 fragCoord = gl_FragCoord;

	if(fragCoord.x >= g_Info.mousePosition.x - 2.0
		&&	fragCoord.x <= g_Info.mousePosition.x + 2.0
		&&	fragCoord.y >= g_Info.mousePosition.y - 2.0
		&&	fragCoord.y <= g_Info.mousePosition.y + 2.0)
	{
		g_objpickerStorage.id 			= (g_pushConstant.mesh_id + 1);	

#ifdef DISPLAY_MOUSE_POINTER
		outFragColor 					= vec4(1.0, 0.0, 0.0, 0.0);
#endif
	}
}

float CalculateShadow(vec4 L, vec3 N, bool applyPCF)
{
	vec3 lightPositionNDC 				= L.xyz/L.w;
	vec2 shadowMapSize 					= textureSize(sampler2D(g_LightDepthImage, g_LinearSampler), 0);
	// NDC ranges between -1 and 1; forcing values bewteen 0 and 1 for shadow map sampling
	vec2 lightPositionUV 				= lightPositionNDC.xy * 0.5f + 0.5f;
	if(lightPositionNDC.z > -1.0 && lightPositionNDC.z <= 1.0f)
	{
		float bias 						= max(0.05 * (1.0f - dot(N, L.xyz)), SHADOW_BIAS); 

		if(applyPCF == true)
    	{
    	    float shadowFactor = 0.0f;
    	    for(float dx = -1; dx <= 1; ++dx)
    	    {
    	        for(float dy = -1; dy <= 1; ++dy)
    	        {
    	            vec2 uv 			= lightPositionUV.xy + (vec2(dx, dy)/shadowMapSize);
    	            float sampledDepth 	= texture(sampler2D(g_LightDepthImage, g_LinearSampler), uv).r;			
    	            shadowFactor 		= shadowFactor + (((lightPositionNDC.z) > sampledDepth) ? 1.0f : 0.0f);
    	        }
    	    }

    	    return (shadowFactor / 9.0f);
    	}
    	else
    	{
    	    float sampledDepth 			= texture(sampler2D(g_LightDepthImage, g_LinearSampler), lightPositionUV.xy).r;
    	    return ((lightPositionNDC.z) > sampledDepth) ? 1.0 : 0.0f;
    	}
	}
	return 0.0f;
}

void main()
{
	Material mat 							= g_materials.data[g_pushConstant.material_id];
	// if roughness is 0, NDF is 0 and so is the entire cook-torrence factor withotu specular or diffuse
	vec2 roughMetal							= vec2(mat.roughness, mat.metallic);																			//vec2(max(mat.roughness, 0.1), mat.metallic);	
	roughMetal								= GetRoughMetalPBR(mat.roughMetal_id, inUV, roughMetal);

	vec4 color 								= GetColor(mat.color_id, inUV); 																				// vec4(roughMetal.x, roughMetal.y, 1.0, 1.0);
	vec3 camPosInViewSpace					= (g_Info.camView * vec4(g_Info.cameraLookFromX, g_Info.cameraLookFromY, g_Info.cameraLookFromZ, 0.0f)).xyz;
	vec3 V 									= normalize(camPosInViewSpace - inPosinViewSpace.xyz);															// V in View Space
	mat3 TBN 								= mat3(inTangentinViewSpace, inBiTangentinViewSpace, inNormalinViewSpace);	
	vec3 N 									= GetNormal(TBN, mat.normal_id, inUV, inNormalinViewSpace);
	vec3 Lo 								= vec3(0.0);

	// for each light source
	{
		// As light color is over 1, the result will be over 1, hence will need tonemapping
		vec3 lightColor 					= vec3(1.0, 1.0, 0.9) * g_Info.sunLightIntensity;
		//vec3 lightDir 					= g_Info.sunLightDirWorld.xyz - inWorldPosition.xyz;
		// this needs to be inverse transpose so as to negate the scaling in the matrix before multipling with Light vector. But this isn't working and I do not know why !
		vec3 L 								= normalize(g_Info.camView * vec4(g_Info.sunLightDirWorld, 0.0f)).xyz;											// L in View Space
		vec3 H 								= normalize(V+L);

		// before we calculate anything related to light, we want to make sure
		// the position is not in shadow
		float shadow 						= CalculateShadow(inPosinLightSpace, N, g_Info.enableShadowPCF);
		// if not in shadow
		if(shadow != 1.0f)
		{
			// as we do not want to calculate light attenuation on directional light
			// commenting this part of code
			//float distance = length(lightDir);
			//float attenuation = 1.0/(distance * distance);
			vec3 radiance 					= lightColor;																									// * attenuation;

			// calculating fresnel value to get the reflection to refraction ratio
			vec3 F0 						= vec3(0.04);
			F0 								= mix(F0, color.xyz, roughMetal.y); 																			// revisit this if necessary as the color here needs to be the metal's color for fresnel effect
			vec3 F 							= FresnelSchlick(max(dot(H, V), 0.0) , F0);

			// calculating Geometry factor and Normal Distribution Function
			float NDF 						= DistrubutionGGX(N, H, roughMetal.x);
			float G 						= GeometrySmith(N, V, L, roughMetal.x);

			vec3 numen 						= NDF * G * F;
			 // adding 0.0001 to avoid division by 0
			float denom 					= 4.0 * max(dot(N, V), 0.0) * max(dot(L, N), 0.0) + 0.0001;
			vec3 specular					= numen/denom;

			// Fresnel value directly corresponds to the reflection factor
			vec3 Ks 						= F;
			vec3 Kd 						= 1.0 - Ks;
			// As we know metals do not participate in diffuse
			Kd 								= Kd * (1.0 - roughMetal.y);

			// calculating Lambertian diffuse
			float NdotL 					= max(dot(N, L), 0.0);
			Lo 								= (Lo + ((Kd * color.xyz / PI) + specular) * radiance * NdotL) * (1.0f - shadow);
		}
	}

	float ssaoFactor 						= 1.0f;
	vec3 ambient 							= vec3(g_Info.pbrAmbientFactor) * color.xyz * ssaoFactor;
	vec3 finalColor							= ambient + Lo;

	// Reinhard Operator for Tonemapping (moving from HDR to LDR)
	finalColor 							    = finalColor / (finalColor + vec3(1.0));

	outPosition								= inPosinViewSpace;				// for ssao
	outNormal								= vec4(N, 0.0f);				// for ssao
	outFragColor 							= vec4(finalColor.xyz, 1.0f);
	
	PickMeshID();
}