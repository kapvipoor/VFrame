#extension GL_EXT_ray_query : require

// The set here refers to Scene Descriptor
// Ideally it seems like a good to make all ray tracing resources
// part of another descriptor set. This will keep the Ray Tracing 
// feature modular. Also not force #version 460 to other non-ray tracing
// shader that might* indirectly include RayTracingCommon.h
layout(set = 1, binding = 6) uniform accelerationStructureEXT g_TLAS;

int TraceRay(vec3 origin, vec3 direction, float rayLength)
{
	rayQueryEXT rq;
	rayQueryInitializeEXT(rq, g_TLAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsCullNoOpaqueEXT, 
	0xff, origin, 1e-1, direction, rayLength);
	rayQueryProceedEXT(rq);
	return (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT) ? 1 : 0;
}

// Generates a seed for a random number generator from 2 inputs plus a backoff
uint genSeed(uint val0, uint val1, uint backoff) {
    uint v0 = val0, v1 = val1, s0 = 0;

    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float nextRand(inout uint s) {
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

vec3 getConeSample(inout uint randSeed, vec3 direction, float coneAngle)
{
	// Calculate a vector that is perpendicular to the direction
	vec3 perpL = cross(direction, vec3(0.f, 1.0f, 0.f));
	perpL = normalize(perpL);

	// Handle case where L = up -> perpL should then be (1,0,0)
	if (all(equal(perpL, vec3(0.0f)))) {
		perpL[0] = 1.0;
	}

	// Calculate a vector that is perpendicular to the 2 orthonormal vectors
	// The random point generated will lie on the LQ plane
	vec3 perpQ = cross(direction, perpL);
	
	float RMax = tan(coneAngle);

	// 2 pi is the circular base of the cone
	// we are randomizing it to pick a point
	// somewhere on that circle
	float theta = nextRand(randSeed) * (2.0f * float(PI));
	float u = cos(coneAngle) * nextRand(randSeed);
	float r = RMax * float(sqrt(1.0f - pow(u, 2)));

	vec3 L = perpL * cos(theta);
	vec3 Q = perpQ * sin(theta);

	return direction + r * (L + Q);
}