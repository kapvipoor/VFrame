#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "Common.h"
#include "UICommon.h"

#define DISPLAY_MOUSE_POINTER

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;
    
vec4 SampleUITexture(uint binding_id, uint in_texture_id, vec2 uv)
{   
    if(binding_id == 0)
    {
        if(in_texture_id < 0 || in_texture_id > SAMPLE_MAX_RENDER_TARGETS )
        {
            return vec4(0.0f, 0.0f, 0.0f, 0.0f);
        }

        return texture(sampler2D(g_RT_SampledImages[in_texture_id], g_LinearSampler), uv).xyzw;
    }
    else
    {
        if (binding_id == 1)
        {
	        return texture(sampler2D(g_uiTexture[in_texture_id], g_LinearSampler), uv).xyzw;
        }
    }
}

void main()
{
    uint binding_id = g_pushConstant.binding_textureId >> 16;
    uint texture_id = g_pushConstant.binding_textureId - (binding_id << 16);
    outFragColor = inColor *  SampleUITexture(binding_id, texture_id, inUV); // SamplePrimaryColor(inUV) *
}