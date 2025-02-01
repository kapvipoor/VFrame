#ifdef CPU
	#pragma once
	
	#include <string>
	#include <queue>
	#include <filesystem>
	
	#define RETURN_FALSE_IF_FALSE(v)	\
	{ if (!v) {	return false; } }		\
	
	inline std::filesystem::path g_DefaultPath	= "D:/Projects/MyPersonalProjects/VFrame/default";
	inline std::filesystem::path g_EnginePath	= "D:/Projects/MyPersonalProjects/VFrame";;
	inline std::filesystem::path g_AssetPath	= "D:/Projects/MyPersonalProjects/assets";
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

#define SAMPLE_PINGPONG_DEPTH_0				    0
#define SAMPLE_PINGPONG_DEPTH_1			        1
#define SAMPLE_POSITION						    2
#define SAMPLE_PINGPONG_NORMAL_0			    3
#define SAMPLE_PINGPONG_NORMAL_1			    4
#define SAMPLE_ALBEDO						    5
#define SAMPLE_SSAO_AND_BLUR			        6
#define SAMPLE_DIRECTIONAL_SHADOW_DEPTH	        7
#define SAMPLE_PRIMARY_COLOR				    8
#define SAMPLE_ROUGH_METAL				        9
#define SAMPLE_MOTION					        10
#define SAMPLE_SS_REFLECTION    		        11
#define SAMPLE_SSR_BLUR						    12
#define SAMPLE_HISTORY_PRIMARY_COLOR   		    13
#define SAMPLE_RT_SHADOW_TEMPORAL_ACC			14
#define SAMPLE_RT_SHADOW_DENOISE				15
#define SAMPLE_MAX_RENDER_TARGETS			    16

#define STORE_POSITION					        0
#define STORE_PINGPONG_NORMAL_0					1
#define STORE_PINGPONG_NORMAL_1					2
#define STORE_ALBEDO						    3
#define STORE_SSAO_AND_BLUR				        4
#define STORE_PRIMARY_COLOR				        5
#define STORE_ROUGH_METAL				        6
#define STORE_MOTION					        7
#define STORE_SS_REFLECTION     		        8
#define STORE_SSR_BLUR					        9
#define STORE_HISTORY_PRIMARY_COLOR		        10
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

///// Normal Current and History Getters /////
#ifdef CPU
	inline
#endif
	int GetNormalStorageID(int pingPongIndex)
	{
		// Depending on the ping pong index that flips between 0 and 1 every frame,
		// we decide what is current Normal buffer and previous normal buffer.
		return pingPongIndex == 0 ? STORE_PINGPONG_NORMAL_0 : STORE_PINGPONG_NORMAL_1;
	}

#ifdef CPU
	inline
#endif
	int GetNormalTextureID(int pingPongIndex)
	{
		// Depending on the ping pong index that flips between 0 and 1 every frame,
		// we decide what is current Normal buffer and previous normal buffer.
		return pingPongIndex == 0 ? SAMPLE_PINGPONG_NORMAL_0 : SAMPLE_PINGPONG_NORMAL_1;
	}

#ifdef CPU
	inline
#endif
	int GetHistoryNormalStorageID(int pingPongIndex)
	{
		// Depending on the ping pong index that flips between 0 and 1 every frame,
		// we decide what is current Normal buffer and previous normal buffer.
		return pingPongIndex == 0 ? STORE_PINGPONG_NORMAL_1 : STORE_PINGPONG_NORMAL_0;
	}

#ifdef CPU
	inline
#endif
	int GetHistoryNormalTextureID(int pingPongIndex)
	{
		// Depending on the ping pong index that flips between 0 and 1 every frame,
		// we decide what is current Normal buffer and previous normal buffer.
		return pingPongIndex == 0 ? SAMPLE_PINGPONG_NORMAL_1 : SAMPLE_PINGPONG_NORMAL_0;
	}