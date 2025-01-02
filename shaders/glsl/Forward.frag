#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "common.h"
#include "MeshCommon.h"

#define DISPLAY_MOUSE_POINTER

layout (location = 0) in vec3 inNormalinViewSpace;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inTangentinViewSpace;
layout (location = 3) in vec3 inBiTangentinViewSpace;
layout (location = 4) in vec4 inPosinLightSpace;
layout (location = 5) in vec4 inPosinViewSpace;
layout (location = 6) in vec4 inPosinClipSpace;
layout (location = 7) in vec4 inPrevPosinClipSpace;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outFragColor;
//layout (location = 3) out vec2 outRoughMetal;
layout (location = 3) out vec2 outMotion;

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

void main()
{
	vec4 finalColor = vec4(0.0);
	
	Material mat = g_materials.data[g_pushConstant.material_id];
	
	vec4 albedo = GetColor(mat.color_id, inUV); 
	vec2 roughMetal = vec2(mat.roughness, mat.metallic);
	roughMetal = GetRoughMetalPBR(mat.roughMetal_id, inUV, roughMetal);

	vec3 camPosVS = vec3(0.0f);
	vec3 V = normalize(camPosVS - inPosinViewSpace.xyz);
	mat3 TBN = mat3(inTangentinViewSpace, inBiTangentinViewSpace, inNormalinViewSpace);	
	vec3 N = GetNormal(TBN, mat.normal_id, inUV, inNormalinViewSpace);
	
	// For non-metallic surfaces, use a constant
	// For metals, use albedo as metals have color to their
	// specular
	vec3 F0 = mix(vec3(0.04), albedo.xyz, roughMetal.y);
				
	// Irradiance at the current position
    vec3 Lo = vec3(0.0);

	// Higher the value, lower the contribution
	float shadow = 1.0f;
	
	// for each light source
	for(int i = 0; i < g_lights.count; i++)
	{
		vec3 radiance = vec3(0.0f);
		
		Light light = g_lights.lights[i];
		uint lightType = light.type_castShadow >> 16;

		// As light color is over 1, the result will be over 1, hence will need tone mapping
		vec3 lightColor = vec3(light.color[0], light.color[1], light.color[2]);
		vec3 L = vec3(0.0f);

		if(lightType == DIRECTIONAL_LIGHT_TYPE)
		{
			// this needs to be inverse transpose so as to negate the scaling in the matrix before multipling with Light vector. But this isn't working and I do not know why!
			vec4 lightDir = vec4(light.vector3[0], light.vector3[1], light.vector3[2], 0.0f);
			L = normalize(g_Info.camView * lightDir).xyz;

			// Compute Shadow if enabled
			int enableShadowRTPCF = int(g_Info.enable_Shadow_RT_PCF);
			if((enableShadowRTPCF & ENABLE_SHADOW) == ENABLE_SHADOW)
			{
				if((enableShadowRTPCF & ENABLE_RT_SHADOW) != ENABLE_RT_SHADOW)
				{
					bool enablePCF = ((enableShadowRTPCF & ENABLE_PCF) == ENABLE_PCF);
					shadow = CalculateDirectonalShadow(inPosinLightSpace, N, enablePCF);
				}
			}

			// Compute directional light if IBL is disabled
			// Otherwise the ambient light is picked from
			// Diffuse Irradiance and Specular IBL Maps
			if(g_Info.enableIBL == 0)
			{
		 		// There is no attenuation for directional light
				float attenuation = 1.0;
				radiance = lightColor * light.intensity * attenuation * shadow;
			}
		}
		else if (lightType == POINT_LIGHT_TYPE)
		{
			// light.vector3 is a position of the point light here.
			// the light vector is going to be inPosinViewSpace - (view matrix * light position)
			// L in View Space
			L = ((g_Info.camView * vec4(light.vector3[0], light.vector3[1], light.vector3[2], 1.0f)) - inPosinViewSpace).xyz;
			float distance = length(L);
			L = normalize(L);
			float attenuation = max(0.0f, light.intensity - distance);
			radiance = lightColor * light.intensity * attenuation;
		}
		
		if(any(greaterThan(radiance, vec3(0.0f))))
		{
			Lo += CalculatePBRReflectance(radiance, F0, L, N, V, albedo.xyz, roughMetal.x, roughMetal.y);
		}
	}

	if(g_Info.enableIBL == 1)
	{
		vec3 ambient = vec3(1.0) /* * ambient occlusion */;	
		if(shadow == 0)
		{
			ambient *= g_Info.pbrAmbientFactor * albedo.xyz;
		}
		else
		{
			ambient *=  CalculateIBLAmbience(F0, N, V, albedo.xyz, roughMetal.x, roughMetal.y) * shadow;
		}
		finalColor = vec4(ambient + Lo, 1.0);	
	}
	else
	{
		vec3 ambient = vec3(g_Info.pbrAmbientFactor) * albedo.xyz;
		finalColor = vec4(ambient + Lo, 1.0);
	}
	
	// Calculate motion vectors
	{
		// Perspective divide both positions
		vec2 posinClipSpace = (inPosinClipSpace.xy/inPosinClipSpace.w);
		vec2 prevPosinClipSpace	= (inPrevPosinClipSpace.xy/inPrevPosinClipSpace.w);

		// Compute difference and convert to UVs
		vec2 velocity = (prevPosinClipSpace - posinClipSpace);
		velocity = velocity * vec2(0.5, -0.5);

		// now correct the motion vectors with the jitter that has been passed to
		// the current view-projection matrix only - g_Info.camViewProj
		// No need to do this because we have multiplied with non-jittered
		// viewproj of this and previous frame
		//velocity								-= g_Info.taaJitterOffset;
		outMotion.xy = velocity;
	}

	outPosition	= inPosinViewSpace;				// for ssao
	outNormal = vec4(N, 0.0f);					// for ssao
	outFragColor = vec4(finalColor.xyz, 1.0f);
	
	//PickMeshID();
}