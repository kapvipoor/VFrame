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
        ivec2 xy = ivec2(uv.x * 1920, uv.y * 1080);
        if(in_texture_id == 0)
        {
            return texture(sampler2D(g_PriamryDepthImage, g_LinearSampler), uv).xyzw;
        }
        else if(in_texture_id == 1)
        {
            return imageLoad(g_PositionImage, xy);
        }
        else if(in_texture_id == 2)
        {
            return imageLoad(g_NormalImage, xy);
        }
        else if(in_texture_id == 3)
        {
            return imageLoad(g_AlbedoImage, xy);
        }
        else if(in_texture_id == 4)
        {
            return imageLoad(g_SSAOImage, xy);
        }
        else if(in_texture_id == 5)
        {
            return imageLoad(g_SSAOBlurImage, xy);
        }    
        else if(in_texture_id == 6)
        {
            return texture(sampler2D(g_LightDepthImage, g_LinearSampler), uv).xyzw;
        }    
        else if(in_texture_id == 7)
        {
            return texture(sampler2D(g_PrimaryColorTexture, g_LinearSampler), uv).xyzw;
        }    
        else if(in_texture_id == 8)
        {
            return imageLoad(g_DeferredRoughMetal, xy);
        }
        else
        {
            return texture(sampler2D(g_PriamryDepthImage, g_LinearSampler), uv).xyzw;
        }    
    }
    else if (binding_id == 1)
    {
	    return texture(sampler2D(g_uiTexture[in_texture_id], g_LinearSampler), uv).xyzw;
    }
}

void main()
{
    uint binding_id = g_pushConstant.binding_textureId >> 16;
    uint texture_id = g_pushConstant.binding_textureId - (binding_id << 16);
    outFragColor = inColor *  SampleUITexture(binding_id, texture_id, inUV); // SamplePrimaryColor(inUV) *
}