#extension GL_EXT_shader_image_load_formatted : require 

#include "../../src/SharedGlobal.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	float	UNASSIGINED_float;
	float	cameraLookFromX;
	float	cameraLookFromY;
	float	cameraLookFromZ;
	mat4	camViewProj;				// with jitter
	mat4	camInvViewProj;				// without jitter
	mat4	camPreViewProj;				// without jitter
	mat4	camProj;
	mat4	camView;
	mat4	invCamView;
	mat4	skyboxView;
	vec2	mousePosition;
	vec2	ssaoNoiseScale;
	float	ssaoKernelSize;
	float	ssaoRadius;
	bool	enableShadowPCF;
	float 	pbrAmbientFactor;
	float 	enabelSSAO;
	float 	biasSSAO;
	float 	ssrMaxDistance;
	float 	ssrResolution;
	float 	ssrThickness;
	float 	ssrSteps;
	float 	taaResolveWeight;
	vec2	taaJitterOffset;
} g_Info;

layout(set = 0, binding = 1) uniform sampler g_LinearSampler;

layout(set = 0, binding = 2) buffer ObjPickerStorage
{
	uint id;
} g_objpickerStorage;

layout(set = 0, binding = 3) buffer SSAOKernel
{
	vec4 kernel[1];
} g_SSAOStorage;

layout(set = 0, binding = 4) uniform texture2D g_ReadOnlyTexures[1];
layout(set = 0, binding = 5) uniform image2D g_RT_StorageImages[STORE_MAX_RENDER_TARGETS];
layout(set = 0, binding = 6) uniform texture2D g_RT_SampledImages[SAMPLE_MAX_RENDER_TARGETS];

vec2 XY2UV(in vec2 xy, in vec2 resolution)
{
	return vec2(xy.x / resolution.x, xy.y / resolution.y);
}

vec2 UV2XY(in vec2 uv, in vec2 resolution)
{
	return vec2(uv.x * resolution.x, uv.y * resolution.y);
}