#pragma once

#include <unordered_map>
#include "core/WinCore.h"
#include "core/VulkanCore.h"
#include "core/Camera.h"
#include "core/Scene.h"

class CRasterRender : public CWinCore, public CVulkanCore
{
public:
	CRasterRender(const char* name, int screen_width_, int screen_height_, int window_scale);
	~CRasterRender();

	void on_destroy() override;
	bool on_create(HINSTANCE pInstance) override;
	bool on_update(float delta) override;
	void on_present() override;
		
private:
	enum BindingSet
	{
		  bs_Primary	= 0
		, bs_Scene		= 1 
	};

	enum BindingDest
	{  // Set 0 - Primary				   // Set 1 - Scene 
		  bd_Gloabl_Uniform			= 0		, bd_Scene_MeshInfo_Uniform		= 0
		, bd_Linear_Sampler			= 1		, bd_CubeMap_Texture			= 1
		, bd_ObjPicker_Storage		= 2		, bd_Material_Storage			= 2
		, bd_Depth_Image			= 3		, bd_SceneRead_TexArray			= 3
		, bd_PosGBuf_Image			= 4		, bd_Scene_max					= 4
		, bd_NormGBuf_Image			= 5
		, bd_AlbedoGBuf_Image		= 6
		, bd_SSAO_Image				= 7
		, bd_SSAOBlur_Image			= 8
		, bd_SSAOKernel_Storage		= 9
		, bd_LightDepth_Image		= 10
		, bd_DeferredLighting_Image = 11
		, bd_PrimaryRead_TexArray	= 12
		, bd_Primary_max			= 13
	};
	
	enum FixedBufferId
	{
		  fb_PrimaryUniform_0	= 0
		, fb_PrimaryUniform_1		
		, fb_ObjectPickerRead
		, fb_ObjectPickerWrite
		, fb_max
	};

	enum RenderTargetId
	{
		  rt_PrimaryDepth		= 0
		, rt_Position			= 1
		, rt_Normal				= 2
		, rt_Albedo				= 3
		, rt_SSAO				= 4
		, rt_SSAOBlur			= 5
		, rt_LightDepth			= 6
		, rt_DeferredLighting	= 7
		, rt_max
	};

	enum CommandBufferId
	{
		  cb_ShadowMap			= 0
		, cb_Forward			= 1
		, cb_Deferred_GBuf		= 2
		, cb_SSAO				= 3
		, cb_Deferred_Lighting	= 4
		, cb_PickerCopy2CPU		= 5
		, cb_max
	};

	enum BufferReadOnlyId
	{
		  br_SSAOKernel = 0
		, br_max
	};

	enum TextureReadOnlyId
	{
		  tr_SSAONoise = 0
		, tr_max
	};

	enum SamplerId
	{
		  s_Linear = 0
		, s_max
	};

	enum MeshType
	{
		  mt_Skybox = 0
		, mt_Scene
	};

	enum FragTexType
	{
		  ft_diffuse = 0
		, ft_normal
		, ft_max
	};

	struct PushConst
	{
		uint32_t mesh_id;
		uint32_t material_id;
	};

	struct DescriptorData
	{
		unsigned int bindingDest;
		uint32_t count;
		VkDescriptorType type;
		VkShaderStageFlags shaderStage;
		const VkDescriptorBufferInfo* bufDesInfo;
		const VkDescriptorImageInfo* imgDesinfo;
	};
	typedef std::vector<DescriptorData> DescDataList;

	struct RenderMesh
	{
		uint32_t mesh_id;
		nm::float4x4 modelMatrix;
		nm::float4x4 viewNormalTransform;

		std::vector<SubMesh> submeshes;

		Buffer vertexBuffer;
		Buffer indexBuffer;
	};

	struct RenderScene
	{
		Buffer meshInfo_uniform;		// stores all meshes uniform data
		Buffer material_storage;

		Image skybox_cubemap;
		Image default_tex;				// default texture when no valid texture is assigned to a primitive		

		std::vector<Image> textures;	// list of all textures required by the scene
		std::vector<RenderMesh> meshes;	// list of all meshes required by the scene

		VkDescriptorPool descPool;
		VkDescriptorSetLayout descLayout;
		VkDescriptorSet descSets;
	};
	
	VkDescriptorPool m_primaryDescPool;
	VkDescriptorSetLayout m_primaryDescLayout[FRAME_BUFFER_COUNT];
	VkDescriptorSet m_primaryDescSet[FRAME_BUFFER_COUNT];

	uint32_t m_swapchainIndex;

	std::vector<Image> m_readonlyTextureList;
	std::vector<Buffer> m_readonlyBufferList;

	std::vector<Buffer> m_fixedBufferList;
	std::vector<Image> m_renderTargets;
	std::vector<Sampler> m_samplers;

	VkSemaphore m_vkswapchainAcquireSemaphore;
	VkSemaphore m_vksubmitCompleteSemaphore;
	VkFence m_vkFenceCmdBfrFree[FRAME_BUFFER_COUNT];

	VkCommandPool m_vkCmdPool;
	VkCommandBuffer m_vkCmdBfr[FRAME_BUFFER_COUNT][CommandBufferId::cb_max];

	CPerspectiveCamera m_primaryCamera;
	COrthoCamera m_sunLightCamera;
	//CPerspectiveCamera m_sunLightCamera;

	RenderScene m_scene;
	bool m_pickObject;

	uint32_t m_ssaokernelSize = 64;
	uint32_t m_ssaoNoiseDimSquared = 16;
	float m_ssaoRadius = 0.5f;

	Renderpass m_forwardRenderpass;
	VkFramebuffer m_vkFrameBuffer[FRAME_BUFFER_COUNT];

	Renderpass m_deferredRenderpass;
	VkFramebuffer m_gFrameBuffer;

	Renderpass m_lightDepthRenderpass;
	VkFramebuffer m_lightDepthFrameBuffer;

	Pipeline m_skyboxPipeline;
	Pipeline m_lightDepthPipeline;
	Pipeline m_forwardPipeline;
	Pipeline m_deferredPipeline;
	Pipeline m_ssaoComputePipeline;
	Pipeline m_ssaoBlurComputePipeline;
	Pipeline m_deferredLightingPipeline;

private:
	
	/*
		Creates Render Tragets
		Creates Buffers for storage and uniforms
		Creates Samplers
	*/
	bool CreateFixedResources();																																	// Primary

	/*
		Creates and populates textures for shader read only (non-material textures)	
		Creates and populates buffers for shader read only
	*/
	bool CreateReadOnlyResources();

	bool CreateForwardRenderpass();																																	// Primary
	bool CreateDeferredRenderpass();																																// Primary
	bool CreateLightDepthRenderpass();																																// Primary
	bool CreatePipelines();																																			// Primary

	bool CreateSyncPremitives();																																	// Primary

	bool CreateMeshUniformBuffer();																																	// Scene

	bool CreateRenderTarget(VkFormat p_format, uint32_t p_width, uint32_t p_height, VkImageLayout p_iniLayout, VkImageUsageFlags p_usage, Image& p_renderTarget);	// Primary
	bool CreateResourceBuffer(void* p_data, size_t p_size, Buffer& p_host, VkBufferUsageFlags p_bfrUsg, VkMemoryPropertyFlags p_propFlag);							// Primary
	bool CreateLoadedBuffer(Buffer& p_staging, Buffer& p_dest, VkBufferUsageFlags p_bfrUsg, VkCommandBuffer& p_cmdBfr);												// Primary
	bool CreateTexture(Buffer& p_staging, Image& p_Image, VkImageCreateInfo p_createInfo, VkCommandBuffer& p_cmdBfr);												// Primary

	/*
		This is a temporary hack until I can figure out a cleaner way to manage this
	*/
	void SetLayoutForDescriptorCreaton();																															// Primary
	void UnsetLayoutForDescriptorCreaton();																															// Primary
	bool CreateDescriptors(const DescDataList& p_descdataList, VkDescriptorPool& p_descPool, 
		VkDescriptorSetLayout* p_descLayout, uint32_t p_layoutCount, VkDescriptorSet* p_desc, void* p_next = VK_NULL_HANDLE);										// Primary

	bool CreatePrimaryDescriptors();																																// Primary
	bool CreateSceneDescriptors();

	bool CreateScene(std::string* p_paths, bool* p_flipVList, unsigned int p_count);																				// Scene

	bool DoSkybox(uint32_t p_swapchainIndex);																														// Primary
	bool DoLightDepthPrepass(uint32_t p_swapchainIndex);																											// Primary
	bool DoGBuffer_Forward(uint32_t p_swapchainIndex);																												// Primary
	bool DoGBuffer_Deferred(uint32_t p_swapchainIndex);																												// Primary
	bool DoSSAO(uint32_t p_swapchainIndex);																															// Primary
	bool DoDeferredLighting(uint32_t p_swapchainIndex);																												// Primary
	bool DoReadBackObjPickerBuffer(uint32_t p_swapchainIndex);																										// Primary

	bool CreateSSAOResources(Buffer& p_kernelstg, Buffer& p_noisestg, VkCommandBuffer p_cmdBfr);

	bool UpdateScene(uint32_t p_swapchainIndex, float p_elapsedDelta);																								// Primary
	bool UpdateUniform(uint32_t p_swapchainIndex, float p_elapsedDelta);																							// Primary										
	void UpdateCamera(float p_delta);																																// Primary

	bool CreateSkybox(std::vector<Buffer>& p_stgbfrList, VkCommandBuffer& p_cmdbfr);

	void ClearFixedResource();																																		// Primary
};

