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
#define SAMPLE_SSAO						        4
#define SAMPLE_SSAO_BLUR					    5
#define SAMPLE_DIRECTIONAL_SHADOW_DEPTH	        6
#define SAMPLE_PRIMARY_COLOR				    7
#define SAMPLE_DEFERRED_ROUGH_METAL		        8
#define SAMPLE_SS_REFLECTION    		        9
#define SAMPLE_PREV_PRIMARY_COLOR    		    10
#define SAMPLE_MAX_RENDER_TARGETS			    11

#define STORE_POSITION					        0
#define STORE_NORMAL						    1
#define STORE_ALBEDO						    2
#define STORE_SSAO						        3
#define STORE_SSAO_BLUR					        4
#define STORE_PRIMARY_COLOR				        5
#define STORE_DEFERRED_ROUGH_METAL		        6
#define STORE_SS_REFLECTION     		        7
#define STORE_PREV_PRIMARY_COLOR   		        8
#define STORE_MAX_RENDER_TARGETS			    9

#define SHADOW_BIAS                             0.005

#define DIRECTIONAL_LIGHT_TYPE                  0
#define POINT_LIGHT_TYPE                        1