#include "common.h"

layout(push_constant) uniform MeshID
{
	uint mesh_id;
	uint material_id;
}g_pushConstant;

layout(set = 1, binding = 0) uniform Mesh
{
	MeshData data[1];
} g_meshUniform;

layout(set = 1, binding = 1) uniform samplerCube g_env_specular_Sampler;
layout(set = 1, binding = 2) uniform samplerCube g_env_diffuse_Sampler;
layout(set = 1, binding = 3) uniform texture2D g_brdf_lut;

struct Material
{
	vec3 emissive;
	float metallic;
	vec3 pbr_color;
	float roughness;
	uint color_id;
	uint normal_id;
	uint roughMetal_id;
	uint emissive_id;
};
layout(set = 1, binding = 4) buffer Material_Storage
{
	Material data[1];			// list of all material
} g_materials;

struct Light
{
	uint  type_castShadow;
	float color[3];
	float intensity;
	float vector3[3];
	float viewProj[16];
	float coneAngle;
};

layout(set = 1, binding = 5) buffer Light_Storage
{
	uint count;
	Light lights[];
} g_lights;

layout(set = 1, binding = 6) uniform texture2D g_textures[];

vec4 GetColor(uint color_id, vec2 uv)
{
	vec4 color;
	if(color_id < MAX_SUPPORTED_TEXTURES)
	{
		color = texture(sampler2D(g_textures[color_id], g_LinearSampler), uv);
	}
	else
	{
		color = texture(sampler2D(g_textures[DEFAULT_TEXTURE_ID], g_LinearSampler), uv);
	}
	return color;
}

vec3 GetNormal(mat3 TBN, uint normal_id, vec2 uv, vec3 inNormal)
{
	vec3 normal;
	if(normal_id < MAX_SUPPORTED_TEXTURES)
	{
		vec2 xy = (texture(sampler2D(g_textures[normal_id], g_LinearSampler), uv).xy * 2.0) - vec2(1.0);
		float z = sqrt(1.0 - dot(xy, xy));
		normal = vec3(xy, z);
		normal = normalize(TBN * normal);
	}
	else
	{
		normal = normalize(inNormal);
	}

	return normal;
}

vec2 GetRoughMetalPBR(uint roughMetal_id, vec2 uv, vec2 inRoughMetal)
{
	vec2 roughMetal;
	if(roughMetal_id < MAX_SUPPORTED_TEXTURES)
	{
		roughMetal = texture(sampler2D(g_textures[roughMetal_id], g_LinearSampler), uv).yz;
	}
	else
	{
		roughMetal = inRoughMetal;
	}

	return roughMetal;
}

vec3 GetEmissive(Material mat, vec2 uv)
{
	return mat.emissive * 3.0 *  texture(sampler2D(g_textures[mat.emissive_id], g_LinearSampler), uv).xyz;
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
    	    return 1.0 - (shadowFactor / 9.0f);
    	}
    	else
    	{
    	    float sampledDepth 			= texture(sampler2D(g_RT_SampledImages[SAMPLE_DIRECTIONAL_SHADOW_DEPTH], g_LinearSampler), lightPositionUV.xy).r;
    	    return ((lightPositionNDC.z) > sampledDepth) ? 0.0 : 1.0f;
    	}
	}
	return 1.0f;
}

vec3 CalculatePBRReflectance(vec3 radiance, vec3 F0, vec3 L, vec3 N, vec3 V, vec3 albedo, float roughness, float metal)
{
	// calculating half vector
	vec3 H	= normalize(V+L);

	// calculating fresnel value to get the reflection to refraction ratio
	vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

	// calculating Geometry factor and Normal Distribution Function
	float NDF = DistrubutionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);

	vec3 numen = NDF * G * F;
	 // adding 0.0001 to avoid division by 0
	float denom	= 4.0 * max(dot(N, V), 0.0) * max(dot(L, N), 0.0) + 0.0001;
	vec3 specular = numen/denom;

	// Fresnel value directly corresponds to the reflection factor
	vec3 Ks = F;
				
	// Energy conversation: Diffuse and Specular light cannot be over 1 unless the surface
	// emits light. Until emissive surfaces are supported, we are strictly setting Kd + Ks = 1
	vec3 Kd = 1.0 - Ks;
	// As we know metals do not participate in diffuse
	Kd = Kd * (1.0 - metal);

	// calculating Lambertian diffuse
	float NdotL	= max(dot(N, L), 0.0);
				
	// return outgoing radiance
	return (Kd * albedo.xyz / PI + specular) * radiance * NdotL;
}

vec3 CalculateIBLAmbience(vec3 F0, vec3 N, vec3 V, vec3 albedo, float roughness, float metal)
{
	vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
	vec3 Ks = F;
	vec3 Kd = 1.0 - Ks;
	Kd = Kd * (1.0 - metal);

	// We are sampling the irradiance at this given pixel position along the specified normal.
	// This is computed by sampling all* possible directions of incoming light and averaging
	// it. It is tonemapped because the flux at this point on the surface cannot be over 1 
	// unless point is emitting light of its own
	vec3 irradiance = texture(g_env_diffuse_Sampler, N).rgb;
	vec3 diffuse = irradiance * albedo.xyz;

	const float MAX_REFLECTION_LOD = 9.0;
	vec3 R = reflect(-V, N);
	vec3 preFilteredColor = textureLod(g_env_specular_Sampler, R, MAX_REFLECTION_LOD * roughness).xyz;
	vec2 envBRDF = texture(sampler2D(g_brdf_lut, g_LinearSampler), vec2(max(dot(N, V), 0.0), roughness)).bg;
	vec3 specular = preFilteredColor * (F * envBRDF.x + envBRDF.y);

	return (Kd * diffuse + specular);
}