#ifndef GPU

#pragma once

#include <string>
#include <queue>
#include <filesystem>

#define RETURN_FALSE_IF_FALSE(v)	\
{ if (!v) {	return false; } }		\

extern std::filesystem::path g_DefaultPath;
extern std::filesystem::path g_EnginePath;
extern std::filesystem::path g_AssetPath;

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

#define SAMPLE_PRIMARY_DEPTH				    0
#define SAMPLE_POSITION					        1
#define SAMPLE_NORMAL						    2
#define SAMPLE_ALBEDO						    3
#define SAMPLE_SSAO_AND_BLUR			        4
#define SAMPLE_DIRECTIONAL_SHADOW_DEPTH	        5
#define SAMPLE_PRIMARY_COLOR				    6
#define SAMPLE_ROUGH_METAL				        7
#define SAMPLE_MOTION					        8
#define SAMPLE_SS_REFLECTION    		        9
#define SAMPLE_SSR_BLUR						    10
#define SAMPLE_PREV_PRIMARY_COLOR    		    11
#define SAMPLE_MAX_RENDER_TARGETS			    12

#define STORE_POSITION					        0
#define STORE_NORMAL						    1
#define STORE_ALBEDO						    2
#define STORE_SSAO_AND_BLUR				        3
#define STORE_PRIMARY_COLOR				        4
#define STORE_ROUGH_METAL				        5
#define STORE_MOTION					        6
#define STORE_SS_REFLECTION     		        7
#define STORE_SSR_BLUR					        8
#define STORE_PREV_PRIMARY_COLOR   		        9
#define STORE_MAX_RENDER_TARGETS			    10

#define SHADOW_BIAS                             0.005

#define DIRECTIONAL_LIGHT_TYPE                  0
#define POINT_LIGHT_TYPE                        1