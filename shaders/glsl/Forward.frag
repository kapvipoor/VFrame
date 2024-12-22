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

float CalculateDirectonalShadow(vec4 L, vec3 N, bool applyPCF)
{
	vec3 lightPositionNDC 				= L.xyz/L.w;
	vec2 shadowMapSize 					= textureSize(sampler2D(g_RT_SampledImages[SAMPLE_DIRECTIONAL_SHADOW_DEPTH], g_LinearSampler), 0);
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
    	            float sampledDepth 	= texture(sampler2D(g_RT_SampledImages[SAMPLE_DIRECTIONAL_SHADOW_DEPTH], g_LinearSampler), uv).r;			
    	            shadowFactor 		= shadowFactor + (((lightPositionNDC.z) > sampledDepth) ? 1.0f : 0.0f);
    	        }
    	    }
    	    return (shadowFactor / 9.0f);
    	}
    	else
    	{
    	    float sampledDepth 			= texture(sampler2D(g_RT_SampledImages[SAMPLE_DIRECTIONAL_SHADOW_DEPTH], g_LinearSampler), lightPositionUV.xy).r;
    	    return ((lightPositionNDC.z) > sampledDepth) ? 1.0 : 0.0f;
    	}
	}
	return 0.0f;
}

void main()
{
	Material mat 							= g_materials.data[g_pushConstant.material_id];
	
	vec2 roughMetal							= vec2(mat.roughness, mat.metallic);																			//vec2(max(mat.roughness, 0.1), mat.metallic);	
	roughMetal								= GetRoughMetalPBR(mat.roughMetal_id, inUV, roughMetal);

	vec4 albedo 								= GetColor(mat.color_id, inUV); 																				// vec4(roughMetal.x, roughMetal.y, 1.0, 1.0);
	vec3 camPosInViewSpace					= (g_Info.camView * vec4(g_Info.cameraLookFromX, g_Info.cameraLookFromY, g_Info.cameraLookFromZ, 0.0f)).xyz;
	vec3 V 									= normalize(camPosInViewSpace - inPosinViewSpace.xyz);															// V in View Space
	mat3 TBN 								= mat3(inTangentinViewSpace, inBiTangentinViewSpace, inNormalinViewSpace);	
	vec3 N 									= GetNormal(TBN, mat.normal_id, inUV, inNormalinViewSpace);
	
	// For non-metallic surfaces, use a constant
	// For metals, use albedo as metals have color to their
	// specular
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo.xyz, roughMetal.y);
				
    vec3 Lo = vec3(0.0);
	float shadow = 0.0f;
	vec4 finalColor = vec4(0.0);

	// for each light source
	for(int i = 0; i < g_lights.count; i++)
	{
		vec3 radiance = vec3(0.0f);
		float attenuation = 1.0;

		Light light = g_lights.lights[i];
		uint lightType = light.type_castShadow >> 16;

		// As light color is over 1, the result will be over 1, hence will need tone mapping
		vec3 lightColor 					= vec3(light.color[0], light.color[1], light.color[2]) * light.intensity; //vec3(1.0, 1.0, 0.9) * g_Info.sunLightIntensity;
		vec3 L 								= vec3(0.0f, 0.0f, 0.0f);

		if(lightType == DIRECTIONAL_LIGHT_TYPE)
		{
			//vec3 lightDir 				= g_Info.sunLightDirWorld.xyz - inWorldPosition.xyz;
			// this needs to be inverse transpose so as to negate the scaling in the matrix before multipling with Light vector. But this isn't working and I do not know why!
			L 								= normalize(g_Info.camView * vec4(light.vector3[0], light.vector3[1], light.vector3[2], 0.0f)).xyz; // L in View Space

			// Light color is directly impacted by the intensity
			lightColor						= lightColor * light.intensity;

			// Compute Shadow if enabled
			if(g_Info.enableShadow == 1.0)
			{
				shadow 								= CalculateDirectonalShadow(inPosinLightSpace, N, g_Info.enableShadowPCF);
			}

			// Compute directional light if IBL is disabled
			// Otherwise the ambient light is picked from
			// Diffuse Irradiance and Specular IBL Maps
			if(g_Info.enableIBL == 0)
			{
		 		// There is no attenuation for directional light
				radiance = lightColor * light.intensity * attenuation * (1.0f - shadow);
			}
		}
		else if (lightType == POINT_LIGHT_TYPE)
		{
			// light.vector3 is a position of the point light here.
			// the light vector is going to be inPosinViewSpace - (view matrix * light position)
			// L in View Space
			L = ((g_Info.camView * vec4(light.vector3[0], light.vector3[1], light.vector3[2], 1.0f)) - inPosinViewSpace).xyz;
			
			float distance = length(L);
			attenuation = max(0.0f, light.intensity - distance);

			L = normalize(L);

			// There is no attenuation for directional light
			radiance = lightColor * light.intensity * attenuation;
		}
		
		vec3 H 								= normalize(V+L);

		if(any(greaterThan(lightColor, vec3(0.0f))))
		{
			// calculating fresnel value to get the reflection to refraction ratio
			vec3 F	= FresnelSchlick(max(dot(H, V), 0.0) , F0);

			// calculating Geometry factor and Normal Distribution Function
			float NDF = DistrubutionGGX(N, H, roughMetal.x);
			float G = GeometrySmith(N, V, L, roughMetal.x);

			vec3 numen = NDF * G * F;
			// adding 0.0001 to avoid division by 0
			float denom	= 4.0 * max(dot(N, V), 0.0) * max(dot(L, N), 0.0) + 0.0001;
			vec3 specular = numen/denom;

			// Fresnel value directly corresponds to the reflection factor
			vec3 Ks	= F;

			// Energy conversation: Diffuse and Specular light cannot be over 1 unless the surface
			// emits light. Until emissive surfaces are supported, we are strictly setting Kd + Ks = 1
			vec3 Kd = 1.0 - Ks;
			// As we know metals do not participate in diffuse
			Kd = Kd * (1.0 - roughMetal.y);

			// calculating Lambertian diffuse
			float NdotL	= max(dot(N, L), 0.0);
			
			// Add all outgoing radiance
			Lo = (Lo + ((Kd * albedo.xyz / PI) + specular) * radiance * NdotL);
		}
	}

	if(g_Info.enableIBL == 1)
	{
		// Ambient lighting to use IBL
		vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughMetal.x);
		vec3 Ks = F;
		vec3 Kd = 1.0 - Ks;
		Kd = Kd * (1.0 - roughMetal.y);
		// We are sampling the irradiance at this given pixel position along the specified normal.
		// This is computed by sampling all* possible directions of incoming light and averaging
		// it. It is tonemapped because the flux at this point on the surface cannot be over 1 
		// unless point is emitting light of its own
		vec3 irradiance = texture(g_env_diffuse_Sampler, N).rgb;
		vec3 diffuse = irradiance * albedo.xyz;

		const float MAX_REFLECTION_LOD = 9.0;
		vec3 R = reflect(-V, N);
		vec3 preFilteredColor = textureLod(g_env_specular_Sampler, R, MAX_REFLECTION_LOD * roughMetal.x).xyz;
		vec2 envBRDF = texture(sampler2D(g_brdf_lut, g_LinearSampler), vec2(max(dot(N, V), 0.0), roughMetal.x)).bg;
		vec3 specular = preFilteredColor * (F * envBRDF.x + envBRDF.y);
		
		vec3 ambient = (Kd * diffuse + specular) * (1.0 - shadow) /* * ambient occlusion */;
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
		vec2 posinClipSpace 					= (inPosinClipSpace.xy/inPosinClipSpace.w);
		vec2 prevPosinClipSpace 				= (inPrevPosinClipSpace.xy/inPrevPosinClipSpace.w);

		// Compute difference and convert to UVs
		vec2 velocity							= (prevPosinClipSpace - posinClipSpace);
		velocity								= velocity * vec2(0.5, -0.5);

		// now correct the motion vectors with the jitter that has been passed to
		// the current view-projection matrix only - g_Info.camViewProj
		// No need to do this because we have multiplied with non-jittered
		// viewproj of this and previous frame
		//velocity								-= g_Info.taaJitterOffset;
		outMotion.xy							= velocity;
	}

	outPosition								= inPosinViewSpace;				// for ssao
	outNormal								= vec4(N, 0.0f);				// for ssao
	outFragColor 							= vec4(finalColor.xyz, 1.0f);
	
	PickMeshID();
}