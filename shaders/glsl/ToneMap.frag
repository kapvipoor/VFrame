#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "Common.h"

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

//--------------------------------------------------------------------------------------
// Timothy Lottes tone mapper
//--------------------------------------------------------------------------------------
// General tonemapping operator, build 'b' term.
float ColToneB(float hdrMax, float contrast, float shoulder, float midIn, float midOut) 
{
    return
        -((-pow(midIn, contrast) + (midOut*(pow(hdrMax, contrast*shoulder)*pow(midIn, contrast) -
            pow(hdrMax, contrast)*pow(midIn, contrast*shoulder)*midOut)) /
            (pow(hdrMax, contrast*shoulder)*midOut - pow(midIn, contrast*shoulder)*midOut)) /
            (pow(midIn, contrast*shoulder)*midOut));
}

// General tonemapping operator, build 'c' term.
float ColToneC(float hdrMax, float contrast, float shoulder, float midIn, float midOut) 
{
    return (pow(hdrMax, contrast*shoulder)*pow(midIn, contrast) - pow(hdrMax, contrast)*pow(midIn, contrast*shoulder)*midOut) /
           (pow(hdrMax, contrast*shoulder)*midOut - pow(midIn, contrast*shoulder)*midOut);
}

// General tonemapping operator, p := {contrast,shoulder,b,c}.
float ColTone(float x, vec4 p) 
{ 
    float z = pow(x, p.r); 
    return z / (pow(z, p.g)*p.b + p.a); 
}

vec3 TimothyTonemapper(vec3 color)
{
    float hdrMax = 16.0; // How much HDR range before clipping. HDR modes likely need this pushed up to say 25.0.
    float contrast = 2.0; // Use as a baseline to tune the amount of contrast the tonemapper has.
    float shoulder = 1.0; // Likely don�t need to mess with this factor, unless matching existing tonemapper is not working well..
    float midIn = 0.18; // most games will have a {0.0 to 1.0} range for LDR so midIn should be 0.18.
    float midOut = 0.18; // Use for LDR. For HDR10 10:10:10:2 use maybe 0.18/25.0 to start. For scRGB, I forget what a good starting point is, need to re-calculate.

    float b = ColToneB(hdrMax, contrast, shoulder, midIn, midOut);
    float c = ColToneC(hdrMax, contrast, shoulder, midIn, midOut);

    #define EPS 1e-6f
    float peak = max(color.r, max(color.g, color.b));
    peak = max(EPS, peak);

    vec3 ratio = color / peak;
    peak = ColTone(peak, vec4(contrast, shoulder, b, c) );
    // then process ratio

    // probably want send these pre-computed (so send over saturation/crossSaturation as a constant)
    float crosstalk = 4.0; // controls amount of channel crosstalk
    float saturation = contrast; // full tonal range saturation control
    float crossSaturation = contrast*16.0; // crosstalk saturation

    float white = 1.0;

    // wrap crosstalk in transform
    ratio = pow(abs(ratio), vec3(saturation / crossSaturation));
    ratio = mix(ratio, vec3(white), vec3(pow(peak, crosstalk)));
    ratio = pow(abs(ratio), vec3(crossSaturation));

    // then apply ratio to peak
    color = peak * ratio;
    return color;
}

vec3 Reinhard(vec3 color)
{
    return color / (color + vec3(1.0));
}

void main()
{    
    vec3 color = texture(sampler2D(g_RT_SampledImages[SAMPLE_PRIMARY_COLOR], g_LinearSampler), inUV).xyz;

    if(g_Info.data.enableSSAO == 1)
	{
		float ssaoFactor = imageLoad(g_RT_StorageImages[STORE_SSAO_AND_BLUR], ivec2(gl_FragCoord.xy)).y;
        color *=  ssaoFactor;
	}

    if(g_Info.data.ssrEnabled == 1)
    {
        vec4 reflectedColor = texture(sampler2D(g_RT_SampledImages[SAMPLE_SS_REFLECTION], g_LinearSampler), inUV).xyzw;
        //if(reflectedColor.w < 0.8)
        //{
        //    reflectedColor.xyz = texture(sampler2D(g_RT_SampledImages[SAMPLE_SSR_BLUR], g_LinearSampler), inUV).xyz;
        //}
        
        color += mix(vec3(0.0), reflectedColor.xyz, reflectedColor.w);
    }

    // Applying Exposure
    color *= g_Info.data.toneMappingExposure;

    // Tonemapping (moving from HDR to LDR)
	if(g_Info.data.toneMapperSelect == 1)
        color = Reinhard(color);
    else if(g_Info.data.toneMapperSelect == 2)
        color = TimothyTonemapper(color);
    	
    // Gamma Correction because all light calculation is in sRGB and not linear
	color = pow(color, vec3(1.0/2.2));
    outFragColor = vec4(color, 1.0);
}