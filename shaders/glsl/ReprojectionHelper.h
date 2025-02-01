#ifndef REPROJECTION_HELPER_H
#define REPROJECTION_HELPER_H

#include "common.h"

// returns Position in View Space
vec4 GetPositionfromDepth(vec2 sampleUV, float sampleDepth)
{	
    vec4 samplePosInCS = vec4((sampleUV * 2.0f) - 1.0f, sampleDepth, 1.0f);  
    samplePosInCS.y *= -1;  // flipping y

    vec4 samplePositionInVS = g_Info.invCamProj * samplePosInCS;
    samplePositionInVS /= samplePositionInVS.w;

    return samplePositionInVS;
}

bool OutOfFrameDisocclusionCheck(ivec2 xy, ivec2 resolution)
{
	if(any(lessThan(xy, ivec2(0.0))) || any(greaterThan(xy, resolution - ivec2(1,1))))
		return true;
	else
		return false;
}

bool NormalDisocclusionCheck(vec3 curNormal, vec3 prevNormal)
{
    // nomal distance threshold = 0.1f
    if(pow(abs(dot(curNormal, prevNormal)), 2) > 0.1)
        return false;
    else
        return true;
}

bool PlaneDistanceOcclusionCheck(vec3 curPosition, vec3 prevPosition, vec3 curNormal)
{
    // plane to distance = 5.0f
    vec3 toCur = curPosition - prevPosition;
    float disToPlane = abs(dot(toCur, curNormal));
    return disToPlane > 5.0f;
}

bool CheckReprojectionValidity(
    ivec2 xy, ivec2 resolution, 
    vec3 curNormal, vec3 prevNormal,
    vec3 curPosition, vec3 prevPosition)
{
	
    if(OutOfFrameDisocclusionCheck(xy, resolution)) 
        return false;
    
    if(NormalDisocclusionCheck(curNormal, prevNormal))
        return false;
    
    if(PlaneDistanceOcclusionCheck(curPosition, prevPosition, curNormal))
        return false;
	
    // sample previous and current mesh IDs for mesh id disocclusion check	
	
    return true;
}

#if defined(REPROJECT_SINGLE_CHANNEL)
bool Reprojection(vec2 uv, ivec2 xy, int historyImageId, out float historyColor)
#else
bool Reprojection(vec2 uv, ivec2 xy, int historyImageId, out vec4 historyColor)
#endif
{
	ivec2 renderRes         = ivec2(RENDER_RESOLUTION_X, RENDER_RESOLUTION_Y);
	vec2 motion             = imageLoad(g_RT_StorageImages[STORE_MOTION], ivec2(xy)).xy;    
	vec2 history_xy         = xy + (motion * renderRes) + vec2(0.5);
	vec2 history_xy_floor   = xy + vec2(motion * renderRes);
	
    
    vec3 curNormal          = SampleNearest(GetNormalTextureID(g_Info.pingPongIndex), uv).xyz;

    float curDepth          = SampleNearest(GetDepthTextureID(g_Info.pingPongIndex), uv).x;
    vec3 curPosition        = GetPositionfromDepth(uv.xy, curDepth).xyz;
    
    bool valid              = false;
    bool v[4];
	// Check reprojection for previous uv (use either 4 tap bilinear filter
    // Using the following kernel: 	x 0
	//	                            0 0
	const ivec2 offset[4] = { ivec2(0, 0), ivec2(1, 0), ivec2(0, 1), ivec2(1, 1) }; 
	for(int i = 0; i < 4; i++)
	{
		bool result         = false;
		ivec2 sample_xy     = ivec2(history_xy_floor) + offset[i];
        vec2 sample_uv      = sample_xy / renderRes;

        vec3 prevNormal     = SampleNearest(GetHistoryNormalTextureID(g_Info.pingPongIndex), sample_uv).xyz;
        
        float prevDepth     = SampleNearest(GetHistoryDepthTextureID(g_Info.pingPongIndex), sample_uv).x;  
        vec3 prevPosition   = GetPositionfromDepth(sample_uv.xy, prevDepth).xyz;

        v[i]                = CheckReprojectionValidity(sample_xy, renderRes, curNormal, prevNormal, curPosition, prevPosition);	
		valid               = valid || v[i];
	}

#if defined(REPROJECT_SINGLE_CHANNEL)
    historyColor = 0.0f;
#else
    historyColor = vec4(0.0f);
#endif    
    
    // if reprojection is valid, sample using bilinear filtering and 
    // use bilinear weights - { (1 - x) * (1 - y), x * (1 - y), (1 - x) * y, x * y };
    if(valid)
    {
        float sumw = 0;
        float x = fract(history_xy_floor.x);
        float y = fract(history_xy_floor.y);

        // bilinear weights
        float w[4] = {(1.0 - x)*(1.0 - y),
                       x * (1.0 - y),
                       (1.0 - x) * y,
                       (x * y)};

        for(int i = 0; i < 4; i++)
        {
            ivec2 sample_xy = ivec2(history_xy_floor) + offset[i];

            if(v[i] == true)
            {
#if defined(REPROJECT_SINGLE_CHANNEL)
                historyColor += w[i] * SampleNearest(historyImageId, sample_xy).x;  
#else
                historyColor += w[i] * SampleNearest(historyImageId, sample_xy).xyzw;    
#endif
                sumw += w[i];
            }
        }

        valid = (sumw >= 0.01);
#if defined(REPROJECT_SINGLE_CHANNEL)
        historyColor = (valid) ? historyColor / sumw : 0.0f; 
#else
        historyColor = (valid) ? historyColor / sumw : vec4(0.0f);    
#endif  
    }
    // If nothing is valid, sample all neighbors and check for reprojection
	// 0 0 0
	// 0 x 0
	// 0 0 0
    float denom = 0;
    if(!valid)
    {
        const int radius = 1;
        for(int xx = -radius; xx <= radius; xx++)
        {
            for(int yy = -radius; yy <= radius; yy++)
            {
                ivec2 sample_xy     = ivec2(history_xy) + ivec2(xx, yy);
                vec2 sample_uv      = sample_xy / renderRes;

                vec3 prevNormal     = SampleNearest(GetHistoryNormalTextureID(g_Info.pingPongIndex), sample_uv).xyz;
                
                float prevDepth     = SampleNearest(GetHistoryDepthTextureID(g_Info.pingPongIndex), sample_uv).x;  
                vec3 prevPosition   = GetPositionfromDepth(sample_uv.xy, prevDepth).xyz;

               if(CheckReprojectionValidity(sample_xy, renderRes , curNormal, prevNormal, curPosition, prevPosition))
               {
#if defined(REPROJECT_SINGLE_CHANNEL)
                    historyColor += SampleNearest(historyImageId, sample_xy).x;    
#else
                    historyColor += SampleNearest(historyImageId, sample_xy).xyzw;    
#endif   
                    denom += 1.0;
               }			      
            }
        }
    }

    if(denom > 0)
    {
        valid = true;
        historyColor /= denom;
    }

    if(!valid)
    {
#if defined(REPROJECT_SINGLE_CHANNEL)
        historyColor = 0.0f;    
#else
        historyColor = vec4(0.0f);
#endif   
    }

	return valid;
}

#if defined(REPROJECT_SINGLE_CHANNEL)
bool ReprojectionFast(ivec2 xy, int historyImageId, out float historyColor)
#else
bool ReprojectionFast(vec2 uv, ivec2 xy, int historyImageId, out vec4 historyColor)
#endif
{
    vec2 renderRes     = vec2(RENDER_RESOLUTION_X, RENDER_RESOLUTION_Y);
	vec2 motion        = imageLoad(g_RT_StorageImages[STORE_MOTION], xy).xy;    
    vec2 history_xy    = vec2(xy) + (motion * renderRes) + vec2(0.5, 0.5);
    vec2 history_uv    = history_xy / renderRes;
	
    vec3 curNormal      = SampleNearest(GetNormalTextureID(g_Info.pingPongIndex), uv).xyz;
    vec3 prevNormal     = SampleNearest(GetHistoryNormalTextureID(g_Info.pingPongIndex), history_uv).xyz;
    
    float curDepth      = SampleNearest(GetDepthTextureID(g_Info.pingPongIndex), uv).x;
    vec3 curPosition    = GetPositionfromDepth(uv.xy, curDepth).xyz;

    float prevDepth     = SampleNearest(GetHistoryDepthTextureID(g_Info.pingPongIndex), history_uv).x;  
    vec3 prevPosition   = GetPositionfromDepth(history_uv.xy, curDepth).xyz;

    bool valid          = CheckReprojectionValidity(ivec2(history_xy), ivec2(renderRes), curNormal, prevNormal, curPosition, prevPosition);
    if(valid)
    {
#if defined(REPROJECT_SINGLE_CHANNEL)
        historyColor = SampleNearest(historyImageId, history_uv).x;  
#else
        historyColor = SampleNearest(historyImageId, history_uv);  
#endif     
    }
    else
    {
#if defined(REPROJECT_SINGLE_CHANNEL)
        historyColor = 0.0f;    
#else
        historyColor = vec4(0.0f);    
#endif     
    }
    return valid;
}

#endif