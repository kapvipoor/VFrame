#define PI 3.14159265359
#define TWO_PI 6.28318530718
#define ONE_OVER_PI (1.0 / PI)
#define ONE_OVER_2PI (1.0 / TWO_PI)

#define spatialrand vec2
#define MAX_SUPPORTED_TEXTURES 2048

#define TEXTURE_READ_ID_SSAO_NOISE 0
#define DEFAULT_TEXTURE_ID 0


const float PHI = 1.61803398874989484820459; //Golden Ratio

// certain constants that will be moved out from the shader
// and passed as unfiorms/push constants
const float g_RenderDim_x = 2400;
const float g_RenderDim_y = 1200;

const float g_ambientCoeff = 0.25;
const float g_diffuseCoeff = 0.75;
const float g_shadowFactor = 0.2;

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	float	elapsedTime;
	float	cameraLookFromX;
	float	cameraLookFromY;
	float	cameraLookFromZ;
	mat4	camViewProj;
	mat4	camProj;
	mat4	camView;
	mat4	invCamView;
	mat4	skyboxView;
	vec2	renderResolution;
	vec2	mousePosition;
	vec2	ssaoNoiseScale;
	float	ssaoKernelSize;
	float	ssaoRadius;
	bool	enableShadowPCF;
	float 	pbrAmbientFactor;
	float 	enabelSSAO;
	float 	biasSSAO;
	float 	unassigned_1;
} g_Info;

layout(set = 0, binding = 1) uniform sampler g_LinearSampler;

layout(set = 0, binding = 2) buffer ObjPickerStorage
{
	uint id;
} g_objpickerStorage;

layout(set = 0, binding = 3)			uniform 			texture2D 	g_PriamryDepthImage; 
layout(set = 0, binding = 4, rgba32f)	uniform readonly	image2D 	g_PositionImage;
layout(set = 0, binding = 5, rgba32f)	uniform readonly	image2D 	g_NormalImage;
layout(set = 0, binding = 6, rgba32f)	uniform readonly	image2D 	g_AlbedoImage;
layout(set = 0, binding = 7, r32f)		uniform 			image2D 	g_SSAOImage;
layout(set = 0, binding = 8, r32f)		uniform 			image2D 	g_SSAOBlurImage;
layout(set = 0, binding = 9) 								buffer		SSAOKernel
{
	vec4 kernel[1];
} g_SSAOStorage;

layout(set = 0, binding = 10)			uniform				texture2D	g_LightDepthImage; 
layout(set = 0, binding = 11, rgba32f)	uniform 		 	image2D		g_PrimaryColor;
layout(set = 0, binding = 12)			uniform 		 	texture2D	g_PrimaryColorTexture;
layout(set = 0, binding = 13, rg16f)	uniform readonly	image2D		g_DeferredRoughMetal;
layout(set = 0, binding = 14) 			uniform				texture2D	g_ReadOnlyTexures[];

vec4 LoadPrimaryColor(ivec2 xy)
{
	return imageLoad(g_PrimaryColor, xy);
}

vec4 SamplePrimaryColor(vec2 uv)
{
	return texture(sampler2D(g_PrimaryColorTexture, g_LinearSampler), uv);
}


// currently being used by the deferred pipeline
vec2 GetSceenSapceRoughMetal(ivec2 xy)
{
	return imageLoad(g_DeferredRoughMetal, xy).xy;
}

float GetSSAOBlur(ivec2 xy)
{
	return imageLoad(g_SSAOBlurImage, xy).x;
}

struct Ray
{
	vec3 origin;
	vec3 direction;
};

struct Lambertian
{
	vec3 albedo;
};

struct Metal
{
	float roughness;
	vec3 albedo;
};

struct Dielectric
{
	float refractiveIndex;
};

struct Sphere
{
	float radius;
	vec3 center;
	int	mat_type;
	int mat_index;
};

struct HitRecord
{
	float rayLength;
	vec3 hitPoint;
	vec3 normal;
	int	mat_type;
	int mat_index;
};

float gold_noise(in vec2 xy, in float seed)
{
	return fract(tan(distance(xy * PHI, xy) * seed) * xy.x);
}

vec2 XY2UV(in vec2 xy, in vec2 resolution)
{
	return vec2(xy.x / resolution.x, xy.y / resolution.y);
}

vec2 UV2XY(in vec2 uv, in vec2 resolution)
{
	return vec2(uv.x * resolution.x, uv.y * resolution.y);
}

vec2 XYtoUV(in vec2 xy)
{
	return vec2(xy.x / g_RenderDim_x, xy.y / g_RenderDim_y);
}

uvec3 pcg3d(uvec3 v) {
	v = v * 1664525u + 1013904223u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v ^= v >> 16u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	return v;
}

vec3 random3(vec3 f) 
{
	return uintBitsToFloat((pcg3d(floatBitsToUint(f)) & 0x007FFFFFu) | 0x3F800000u) - 1.0;
}

vec3 random_in_unit_sphere(uvec2 xy, float seed)
{
	float n1 = 1.0;
	float n2 = 2.0;
	float n3 = 3.0;

	vec3 result = vec3(1.0, 2.0, 3.0);
	float lengthSq = 1.0;
	while (lengthSq >= 1.0)
	{
		vec3 rand = random3(vec3(xy + ivec2(seed), g_Info.elapsedTime));
		// Randf generates floating point values betweem 0 and 1.0f. So converting them between -1.0f and 1.0f
		result = (2.0 * rand) - vec3(1.0f, 1.0f, 1.0f);
		lengthSq = length(result);
		lengthSq *= lengthSq;
	}
	return result;
}

vec3 NDCtoView(in vec3 value)
{
	return (value + 1.0) / 2.0;
}