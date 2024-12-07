#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "Common.h"

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main()
{
    
    vec3 color = texture(sampler2D(g_RT_SampledImages[SAMPLE_PRIMARY_COLOR], g_LinearSampler), inUV).xyz;

    if(g_Info.enabelSSAO == 1)
	{
		float ssaoFactor = imageLoad(g_RT_StorageImages[STORE_SSAO_AND_BLUR], ivec2(gl_FragCoord.xy)).y;
        color *=  ssaoFactor;
	}

    if(g_Info.ssrEnabled == 1)
    {
        vec4 reflectedUV = imageLoad(g_RT_StorageImages[STORE_SS_REFLECTION], ivec2(gl_FragCoord.xy));
        vec3 reflectedColor = texture(sampler2D(g_RT_SampledImages[SAMPLE_PRIMARY_COLOR], g_LinearSampler), reflectedUV.xy).xyz;
        color += mix(vec3(0.0), reflectedColor, reflectedUV.a);
    }

    // Reinhard Operator for Tonemapping (moving from HDR to LDR)
	color 							    = color / (color + vec3(1.0));
	
    // Gamma Correction because all light calculation is in sRGB and not linear
	color 						        = pow(color, vec3(1.0/2.2));
	
    outFragColor                        = vec4(color, 1.0);
}