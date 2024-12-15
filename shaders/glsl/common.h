#extension GL_EXT_shader_image_load_formatted : require 

#include "../../src/SharedGlobal.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	float	toneMapperSelect;
	float	cameraLookFromX;
	float	cameraLookFromY;
	float	cameraLookFromZ;
	mat4	camViewProj;				// without jitter
	mat4	camJitteredViewProj;		// with jitter
	mat4	camInvViewProj;				// without jitter
	mat4	camPreViewProj;				// without jitter
	mat4	camProj;
	mat4	camView;
	mat4	invCamView;
	mat4	invCamProj;
	mat4	skyboxView;
	vec2	mousePosition;
	vec2	ssaoNoiseScale;
	float	ssaoKernelSize;
	float	ssaoRadius;
	bool	enableShadowPCF;
	float 	pbrAmbientFactor;
	float 	enabelSSAO;
	float 	biasSSAO;
	float	ssrEnabled;
	float 	ssrMaxDistance;
	float 	ssrResolution;
	float 	ssrThickness;
	float 	ssrSteps;
	float 	taaResolveWeight;
	float	taaUseMotionVectors;
	float	taaFlickerCorrectionMode;
	float	taaReprojectionFilter;
	float	toneMappingExposure;
	float	UNASSIGINED_Float0;
	float	UNASSIGINED_Float1;
	float	UNASSIGINED_Float2;
	float	UNASSIGINED_Float3;
} g_Info;

layout(set = 0, binding = 1) uniform sampler g_LinearSampler;
layout(set = 0, binding = 2) uniform sampler g_NearestSampler;

layout(set = 0, binding = 3) buffer ObjPickerStorage
{
	uint id;
} g_objpickerStorage;

layout(set = 0, binding = 4) buffer SSAOKernel
{
	vec4 kernel[1];
} g_SSAOStorage;

layout(set = 0, binding = 5) uniform texture2D g_ReadOnlyTexures[1];
layout(set = 0, binding = 6) uniform image2D g_RT_StorageImages[STORE_MAX_RENDER_TARGETS];
layout(set = 0, binding = 7) uniform texture2D g_RT_SampledImages[SAMPLE_MAX_RENDER_TARGETS];

vec2 XY2UV(in vec2 xy, in vec2 resolution)
{
	return vec2(xy.x / resolution.x, xy.y / resolution.y);
}

vec2 UV2XY(in vec2 uv, in vec2 resolution)
{
	return vec2(uv.x * resolution.x, uv.y * resolution.y);
}

// Trowbridge-Reitz GGX normal distribution function
float DistrubutionGGX(vec3 N, vec3 H, float Roughness)
{
	float roughnessSq	= Roughness * Roughness;
	float a2			= roughnessSq * roughnessSq;
	float NdotH			= max(dot(N, H), 0.0);
	float NdotHSq		= NdotH * NdotH;
	float denom			= (NdotHSq * (a2 - 1.0) + 1.0);
	denom				= PI * denom * denom;
	return a2 / denom;
}

// Schlick GGX to statistically estiamte light ray occlusion
float GeometrySchlickGGX(float NdotV, float K)
{
	float denom = (NdotV * (1.0 - K) + K);
	return NdotV / denom;
}

// Applying Schlick GGX on Light vector to compute for geometry shadowing
// Applying SchlickGGX on View vector to compute for geometry occlusion
float GeometrySmith(vec3 N, vec3 V, vec3 L, float Roughness)
{
	float k = (Roughness + 1.0);
	k = k * k;
	k = k / 8.0;
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	return GeometrySchlickGGX(NdotL, k) * GeometrySchlickGGX(NdotV, k);
}

// Fresnel Schlick Approximation for Fresnel Effect
vec3 FresnelSchlick(float cosThetha, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosThetha, 0.0, 1.0), 5.0);
}

float ComputeLuminance(vec3 color)
{
	return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 log2_safe(vec3 inVal)
{
	//return log2(x + 1.0);
	//return (x > vec3(0.0)) ? log2(x) : -10.0;
	if(all(greaterThan(inVal, vec3(0.0))))
	{
		return vec3(log2(inVal.x), log2(inVal.y), log2(inVal.z));
	}
	else
	{
		return vec3(-10.0);
	}
}