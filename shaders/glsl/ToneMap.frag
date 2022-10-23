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
    float ssaoFactor                    = 1.0f;
    if(g_Info.enabelSSAO == 1)
	{
		ssaoFactor                      = GetSSAOBlur(ivec2(gl_FragCoord.xy));
	}

    vec3 color                          = (SamplePrimaryColor(inUV).xyz) * ssaoFactor; 

    // Reinhard Operator for Tonemapping (moving from HDR to LDR)
	//color 							    = color / (color + vec3(1.0));
	
    // Gamma Correction because all light calculation is in sRGB and not linear
	//color 						        = pow(color, vec3(1.0/2.2));
	
    outFragColor                        = vec4(color, 1.0);
}