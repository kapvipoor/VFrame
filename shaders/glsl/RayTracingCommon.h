#extension GL_EXT_ray_query : require

// The set here refers to Scene Descriptor
// Ideally it seems like a good to make all ray tracing resources
// part of another descriptor set. This will keep the Ray Tracing 
// feature modular. Also not force #version 460 to other non-ray tracing
// shader that might* indirectly include RayTracingCommon.h
layout(set = 2, binding = 0) uniform accelerationStructureEXT g_TLAS;

float TraceRay(vec3 origin, vec3 direction)
{
	rayQueryEXT rq;
	rayQueryInitializeEXT(rq, g_TLAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsCullNoOpaqueEXT, 
	0xff, origin, 1e-1, direction, 1e3);
	rayQueryProceedEXT(rq);
	return (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT) ? 0.0 : 1.0;
}