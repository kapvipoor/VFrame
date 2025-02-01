#ifndef SPATIAL_HELPER_H
#define SPATIAL_HELPER_H

#include "common.h"

#define SIGMA 15.0
#define BSIGMA 10.9
#define MSIZE 25

float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

float normpdf3(in vec3 v, in float sigma)
{
	return 0.39894*exp(-0.5*dot(v,v)/(sigma*sigma))/sigma;
}

float normpdf4(in vec4 v, in float sigma)
{
	return 0.39894*exp(-0.5*dot(v,v)/(sigma*sigma))/sigma;
}

//vec4 BilateralFilter(texture2D inImage, sampler inSampler, ivec2 xy, vec4 c)
vec4 BilateralFilter(in int inImageId, ivec2 xy, vec4 c)
{
	vec2 renderRes = vec2(RENDER_RESOLUTION_X, RENDER_RESOLUTION_Y);
	const int kSize = (MSIZE-1)/2;
	float kernel[MSIZE];
	vec4 final_colour = vec4(0.0);

	//create the 1-D kernel
	float Z = 0.0;
	for (int j = 0; j <= kSize; ++j)
	{
		kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), SIGMA);
	}

	vec4 cc;
	float factor;
	float bZ = 1.0/normpdf(0.0, BSIGMA);
	//read out the texels
	for (int i=-kSize; i <= kSize; ++i)
	{
		for (int j=-kSize; j <= kSize; ++j)
		{
			vec2 neighbor_uv = (xy.xy + vec2(float(i),float(j))) / renderRes ;
			cc = SampleNearest(inImageId, neighbor_uv); //texture(sampler2D(inImage, inSampler), neighbor_uv).rgba;
			factor = normpdf4(cc-c, BSIGMA)*bZ*kernel[kSize+j]*kernel[kSize+i];
			Z += factor;
			final_colour += factor*cc;
		}
	}

    return final_colour/Z;
}

#endif