#ifdef CPU
#pragma once

#include <string>
#include <queue>
#include <filesystem>
#include <array>

#define RETURN_FALSE_IF_FALSE(v)	\
	{ if (!v) {	return false; } }	\

inline std::filesystem::path g_DefaultPath = "D:/Projects/MyPersonalProjects/VFrame/default";
inline std::filesystem::path g_EnginePath = "D:/Projects/MyPersonalProjects/VFrame";
inline std::filesystem::path g_AssetPath = "D:/Projects/MyPersonalProjects/assets";
#endif

#define PI                                      3.14159265359

#define DISPLAY_RESOLUTION_X					1920
#define DISPLAY_RESOLUTION_Y					1080

#define RENDER_RESOLUTION_X						1920
#define RENDER_RESOLUTION_Y						1080

#define THREAD_GROUP_SIZE_X						8
#define THREAD_GROUP_SIZE_Y						8
#define THREAD_GROUP_SIZE_Z						1

#define FRAME_BUFFER_COUNT                      2
#define MAX_SUPPORTED_DEBUG_DRAW_ENTITES        256
#define MAX_SUPPORTED_MESHES                    100
#define MAX_SUPPORTED_MATERIALS                 1000000
#define MAX_SUPPORTED_TEXTURES                  2048

#define TEXTURE_READ_ID_SSAO_NOISE              0
#define DEFAULT_TEXTURE_ID                      0

#define SAMPLE_PINGPONG_DEPTH_0				    0			// This frame's depth target for History/Current use
#define SAMPLE_PINGPONG_DEPTH_1			        1			// This frame's depth target for History/Current use
#define SAMPLE_POSITION						    2			// Position in view space for Current use
#define SAMPLE_PINGPONG_NORMAL_MESH_ID_0	    3			// XYZ [Normal in view space for History/Current use] W[Mesh Id for History/Current use]
#define SAMPLE_PINGPONG_NORMAL_MESH_ID_1	    4			// XYZ [Normal in view space for History/Current use] W[Mesh Id for History/Current use]
#define SAMPLE_ALBEDO						    5			// Albedo
#define SAMPLE_SSAO_AND_BLUR			        6			// SSAO and Blur 
#define SAMPLE_DIRECTIONAL_SHADOW_DEPTH	        7			// Directional shadow depth
#define SAMPLE_PRIMARY_COLOR				    8			// This frame's color target, copies to swap chain by eof
#define SAMPLE_ROUGH_METAL				        9			// Roughness and Metal
#define SAMPLE_MOTION					        10			// Motion vectors
#define SAMPLE_SS_REFLECTION    		        11			// Screen Space Reflection (RGB)
#define SAMPLE_SSR_BLUR						    12			// Blurred Screen Space Reflection (for rough surfaces)
#define SAMPLE_HISTORY_LENGTH		   		    13			// History Length buffer For temporal accumulation 
#define SAMPLE_RT_SHADOW_TEMPORAL_ACC			14			// Temporal Accumulated ray traced shadows (4 count - 1 directional + 3 point)
#define SAMPLE_RT_SHADOW_DENOISE				15			// Denoised ray traces shadows (4 count - 1 directional + 3 point)
#define SAMPLE_MAX_RENDER_TARGETS			    16

#define STORE_POSITION					        0
#define STORE_PINGPONG_NORMAL_MESH_ID_0			1
#define STORE_PINGPONG_NORMAL_MESH_ID_1			2
#define STORE_ALBEDO						    3
#define STORE_SSAO_AND_BLUR				        4
#define STORE_PRIMARY_COLOR				        5
#define STORE_ROUGH_METAL				        6
#define STORE_MOTION					        7
#define STORE_SS_REFLECTION     		        8
#define STORE_SSR_BLUR					        9
#define STORE_HISTORY_LENGTH			        10
#define STORE_RT_SHADOW_TEMPORAL_ACC			11
#define STORE_RT_SHADOW_DENOISE					12
#define STORE_MAX_RENDER_TARGETS			    13

// Shadow flags
#define ENABLE_SHADOW							1
#define ENABLE_RT_SHADOW						2
#define ENABLE_PCF								4
#define SHADOW_BIAS                             0.005

// Light Flags
#define DIRECTIONAL_LIGHT_TYPE                  0
#define POINT_LIGHT_TYPE                        1

#ifdef CPU
typedef std::array<float, 16>	mat4;
typedef std::array<float, 3>	vec3;
typedef std::array<float, 2>	vec2;
typedef uint32_t				uint;
#endif

///////////////////// Structured Data /////////////////////
struct MeshData
{
	mat4 modelMatrix;						// Current frame model matrix of the Mesh
	mat4 preModelMatrix;					// Previous frame model matrix of the Mesh
	mat4 normalMatrix;						// Inverse transpose of (view * model) of the Mesh
};

struct PrimaryUniformData //FrameData
{
	int		pingPongIndex;					// Ping Pong Index to switch between current and previous buffers
	float	cameraLookFromX;				// Camera Position X
	float	cameraLookFromY;				// Camera Position Y
	float	cameraLookFromZ;				// Camera Position Z
	mat4	camViewProj;					// Camera View Projection Matrix
	mat4	camJitteredViewProj;			// Camera Jittered View Projection Matrix (for TAA; jittering computed on CPU)
	mat4	camInvViewProj;					// Camera Inverse View Projection Matrix
	mat4	camPreViewProj;					// Camera Previous View Projection Matrix
	mat4	camProj;						// Camera Projection Matrix
	mat4	camView;						// Camera View Matrix
	mat4	invCamView;						// Camera Inverse View Matrix
	mat4	invCamProj;						// Inverse Camera Projection Matrix
	mat4	skyboxView;						// Skybox Model View Matrix
	vec2	mousePosition;					// Current Mouse Position
	vec2	ssaoNoiseScale;					// SSAO Noise Scale
	float	ssaoKernelSize;					// SSAO Kernel Size
	float	ssaoRadius;						// SSAO Sampling Radius
	float	enable_Shadow_RT_PCF;			// ORed Shadow flags
	float	shadowMinAccumWeight;			// Minimum Temporal Accumulation Weight for Ray Traced Shadows
	uint	frameCount;						// Frame Count passed as seed
	float 	enableIBL;						// Enable Image Based Lighting
	float 	pbrAmbientFactor;				// Ambient Factor for PBR
	float 	enableSSAO;						// Enable SSAO
	float 	biasSSAO;						// SSAO bias
	float	ssrEnabled;						// Enable SSR
	float 	ssrMaxDistance;					// Max SSR Ray Length
	float 	ssrResolution;					// SSR Sampling Resolution
	float 	ssrThickness;					// SSR Ray Thickness
	float 	ssrSteps;						// SSR Steps for binary search resolve
	float 	taaResolveWeight;				// Temporal Resolve Weight for TAA
	float	taaUseMotionVectors;			// TAA flag to Use Motion Vectors to compute reprojection
	float	taaFlickerCorrectionMode;		// TAA Mode selection for Flicker Correction
	float	taaReprojectionFilter;			// TAA Reprojection Filter selection
	float	toneMappingExposure;			// Exposure weight for Tone Mapping
	float	toneMapperSelect;				// Tone Mapping Algorithm selection (None, Reinhard, AMD)
};

/////////////////////////////////////////////////////////////////////////

///// Position Current and History Getters /////
#ifdef CPU
inline
#endif
int GetDepthTextureID(int pingPongIndex)
{
	// Depending on the ping pong index that flips between 0 and 1 every frame,
	// we decide what is current depth buffer and previous depth buffer.
	return pingPongIndex == 0 ? SAMPLE_PINGPONG_DEPTH_0 : SAMPLE_PINGPONG_DEPTH_1;
}

#ifdef CPU
inline
#endif
int GetHistoryDepthTextureID(int pingPongIndex)
{
	// Depending on the ping pong index that flips between 0 and 1 every frame,
	// we decide what is current depth buffer and previous depth buffer.
	return pingPongIndex == 0 ? SAMPLE_PINGPONG_DEPTH_1 : SAMPLE_PINGPONG_DEPTH_0;
}

///// Normal + Mesh ID Current and History Getters /////
#ifdef CPU
inline
#endif
int GetNormalMeshIDStorageID(int pingPongIndex)
{
	// Depending on the ping pong index that flips between 0 and 1 every frame,
	// we decide what is current Normal + Mesh id buffer and previous normal buffer.
	return pingPongIndex == 0 ? STORE_PINGPONG_NORMAL_MESH_ID_0 : STORE_PINGPONG_NORMAL_MESH_ID_1;
}

#ifdef CPU
inline
#endif
int GetNormalMeshIDTextureID(int pingPongIndex)
{
	// Depending on the ping pong index that flips between 0 and 1 every frame,
	// we decide what is current Normal + Mesh id buffer and previous normal buffer.
	return pingPongIndex == 0 ? SAMPLE_PINGPONG_NORMAL_MESH_ID_0 : SAMPLE_PINGPONG_NORMAL_MESH_ID_1;
}

#ifdef CPU
inline
#endif
int GetHistoryNormalMeshIDStorageID(int pingPongIndex)
{
	// Depending on the ping pong index that flips between 0 and 1 every frame,
	// we decide what is current Normal + Mesh id buffer and previous normal buffer.
	return pingPongIndex == 0 ? STORE_PINGPONG_NORMAL_MESH_ID_1 : STORE_PINGPONG_NORMAL_MESH_ID_0;
}

#ifdef CPU
inline
#endif
int GetHistoryNormalMeshIDTextureID(int pingPongIndex)
{
	// Depending on the ping pong index that flips between 0 and 1 every frame,
	// we decide what is current Normal + Mesh id buffer and previous normal buffer.
	return pingPongIndex == 0 ? SAMPLE_PINGPONG_NORMAL_MESH_ID_1 : SAMPLE_PINGPONG_NORMAL_MESH_ID_0;
}
//////////////////////////////////////////////////////////////////////