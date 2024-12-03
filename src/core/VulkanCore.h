#pragma once

#include "../SharedGlobal.h"

// 0 disable vulkan debug
// 1 enable vulkan debug
#define VULKAN_DEBUG 1

#if VULKAN_DEBUG == 1
// 1 show vulkan validations errors
// 2 also show vulkan warnings 
// 3 show all vulkan messages
#define VULKAN_SHOW_MESSAGES 2
#define VULKAN_DEBUG_MARKERS 1
#endif

#define VK_USE_PLATFORM_WIN32_KHR

#include <assert.h>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include <filesystem>

char* BinaryLoader(const std::string pPath, size_t& pDataSize);

class CVulkanCore
{
public:
	struct InitData
	{
		HINSTANCE											winInstance;
		HWND												winHandle;
		VkQueueFlagBits										queueType;
		VkImageUsageFlags									swapChaineImageUsage;
		VkFormat											swapchainImageFormat;
	};

	struct VertexBinding
	{
		VkVertexInputBindingDescription						bindingDescription;
		std::vector<VkVertexInputAttributeDescription>		attributeDescription;
	};

	struct Renderpass
	{
		std::vector<VkAttachmentDescription>				attachemntDescList;
		std::vector<VkAttachmentReference>					colorAttachmentRefList;
		VkAttachmentReference								depthAttachemntRef;
		VkRenderPass										renderpass;
		uint32_t											framebufferWidth;
		uint32_t											framebufferHeight;

		std::vector<VkClearColorValue>						colorClearValue;
		VkClearDepthStencilValue							depthStencilClearValue;

		Renderpass() : depthAttached(false) {}
		
		void AttachColor(VkFormat p_format,	VkAttachmentLoadOp p_loadOp, VkAttachmentStoreOp p_stOp, VkImageLayout p_iniLayout, VkImageLayout p_finLayout, VkImageLayout p_refLayout)
		{
			VkAttachmentDescription attachDesc{};
			attachDesc.format = p_format;
			attachDesc.loadOp = p_loadOp;
			attachDesc.storeOp = p_stOp;
			attachDesc.initialLayout = p_iniLayout;
			attachDesc.finalLayout = p_finLayout;
			attachDesc.samples = VK_SAMPLE_COUNT_1_BIT;

			VkAttachmentReference ref{};
			ref.attachment = (uint32_t)attachemntDescList.size();
			ref.layout = p_refLayout;

			colorAttachmentRefList.push_back(ref);
			attachemntDescList.push_back(attachDesc);

			colorClearValue.push_back(VkClearColorValue{ 1.0f, 1.0f, 1.0f, 1.0f });
		}
		
		void AttachDepth(VkFormat p_format, VkAttachmentLoadOp p_loadOp, VkAttachmentStoreOp p_stOp, VkImageLayout p_iniLayout, VkImageLayout p_finLayout, VkImageLayout p_refLayout)
		{
			if (depthAttached == true)
			{
				// depth already attached
				std::cout << "Warning: Depth already attached" << std::endl;
				assert(!depthAttached);
			}

			VkAttachmentDescription attachDesc{};
			attachDesc.format = p_format;
			attachDesc.loadOp = p_loadOp;
			attachDesc.storeOp = p_stOp;
			attachDesc.initialLayout = p_iniLayout;
			attachDesc.finalLayout = p_finLayout;
			attachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			attachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

			VkAttachmentReference ref{};
			ref.attachment = (uint32_t)attachemntDescList.size();
			ref.layout = p_refLayout;

			depthAttachemntRef.attachment = ref.attachment;
			depthAttachemntRef.layout = ref.layout;
			depthAttached = true;

			depthStencilClearValue = VkClearDepthStencilValue{ 1.0f, 0 };

			attachemntDescList.push_back(attachDesc);
		}

		bool isDepthAttached() { return depthAttached; }

	private:
		bool depthAttached;
	};

	struct ShaderPaths
	{
		std::filesystem::path								shaderpath_vertex;
		std::filesystem::path								shaderpath_fragment;
		std::filesystem::path								shaderpath_compute;
	};

	struct Pipeline
	{
		VkShaderModule										vertexShader;
		VkShaderModule										fragmentShader;
		VkShaderModule										computeShader;
		VkVertexInputBindingDescription						vertexInBinding;
		std::vector<VkVertexInputAttributeDescription>		vertexAttributeDesc;
		bool												enableDepthTest;
		bool												enableDepthWrite;
		VkCullModeFlags										cullMode;
		VkCompareOp											depthCmpOp;
		bool												enableBlending;
		Renderpass											renderpassData;
		bool												isWireframe;
		VkPipelineLayout									pipeLayout;
		VkPipeline											pipeline;

		Pipeline() :
			cullMode(VK_CULL_MODE_BACK_BIT)
			, depthCmpOp(VK_COMPARE_OP_LESS_OR_EQUAL)
			, enableBlending(false)
			, isWireframe(false) {}
	};

	struct Buffer
	{
		VkDescriptorBufferInfo								descInfo;
		VkDeviceMemory										devMem;
		VkMemoryAllocateFlags								memPropFlags;
		size_t												reqMemSize;

		Buffer():
				devMem(VK_NULL_HANDLE)
			,	reqMemSize(0)
			,	descInfo(VkDescriptorBufferInfo{VK_NULL_HANDLE, 0, 0})
		{}
	};

	struct Image
	{
		VkImageUsageFlags									usage;
		VkImageLayout										curLayout;

		VkDescriptorImageInfo								descInfo;
		VkImage												image;
		VkDeviceMemory										devMem;

		VkImageViewType										viewType;

		VkFormat											format;
		uint32_t											width;
		uint32_t											height;

		uint32_t											layerCount;
		uint32_t											bufOffset;			// assuming the buffer offset for each layer is going to be same

		Image() :
			layerCount(1)
			, bufOffset(0) {}
	};

	struct Sampler
	{
		VkDescriptorImageInfo								descInfo;
		VkFilter											filter;
		float												maxAnisotropy;
	};

	static VkImageCreateInfo ImageCreateInfo()
	{
		VkImageCreateInfo p_createInfo{};
		p_createInfo.sType									= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		p_createInfo.imageType								= VK_IMAGE_TYPE_2D;
		p_createInfo.extent.depth							= 1;
		p_createInfo.mipLevels								= 1;
		p_createInfo.arrayLayers							= 1;
		p_createInfo.tiling									= VK_IMAGE_TILING_OPTIMAL;
		p_createInfo.initialLayout							= VK_IMAGE_LAYOUT_UNDEFINED;
		p_createInfo.sharingMode							= VK_SHARING_MODE_EXCLUSIVE;
		p_createInfo.samples								= VK_SAMPLE_COUNT_1_BIT;
		p_createInfo.flags									= 0;

		return p_createInfo;
	}

	enum QueueType
	{
			qt_Primary = 0
		,	qt_Secondary
	};

	typedef std::vector<VkFramebuffer>						FrameBuffer;
	typedef	VkQueue											Queue;
	typedef VkCommandPool									CommandPool;
	typedef VkCommandBuffer									CommandBuffer;
	typedef std::vector<VkCommandBuffer>					CommandBufferList;
	typedef std::vector<VkSemaphore>						SemaphoreList;
	typedef std::vector<VkPipelineStageFlags>				PipelineStageFlagsList;
	typedef std::vector<VkFence>							FenceList;
	typedef std::vector<Buffer>								BufferList;
	typedef std::vector<Image>								ImageList;
	typedef std::vector<Sampler>							SamplerList;
	typedef VkImageLayout									ImageLayout;
	typedef VkSwapchainKHR									SwapChain;

	CVulkanCore(const char* p_applicaitonName, int p_renderWidth, int p_renderHeight);
	~CVulkanCore();

	void BeginDebugMarker(VkCommandBuffer p_vkCmdBuff, const char* pMsg);
	void EndDebugMarker(VkCommandBuffer p_vkCmdBuff);
	void InsertMarker(VkCommandBuffer p_vkCmdBuff, const char* pMsg);
	void SetDebugName(uint64_t pObject, VkObjectType pObjectType, const char* pName);

	VkQueue GetQueue(uint32_t p_scIdx)						{ return m_vkQueue[p_scIdx]; }
	VkQueue GetSecondaryQueue()								{ return m_secondaryQueue;}
	uint32_t GetQueueFamiliyIndex() const					{ return m_QFIndex; }
	VkImageView GetSCImageView(uint32_t p_scIdx)			{ return m_swapchainImageViewList[p_scIdx]; }
	uint32_t GetRenderWidth()								{ return m_renderWidth; }
	uint32_t GetRenderHeight()								{ return m_renderHeight; }
	uint32_t GetScreenWidth()								{ return m_screenWidth; }
	uint32_t GetScreenHeight()								{ return m_screenHeight; }

	VkSwapchainKHR GetSwapChain()							{ return m_vkSwapchain; }

protected:
	uint32_t												m_renderWidth;
	uint32_t												m_renderHeight;
	uint32_t												m_screenWidth;
	uint32_t												m_screenHeight;

	const char*												m_applicationName;
	VkInstance												m_vkInstance;
	VkDevice												m_vkDevice;
	VkPhysicalDevice										m_vkPhysicalDevice;
	uint32_t												m_QFIndex;
	VkQueue													m_vkQueue[FRAME_BUFFER_COUNT];
	VkQueue													m_secondaryQueue;
	VkSurfaceKHR											m_vkSurface;
	VkSwapchainKHR											m_vkSwapchain;
	VkImage													m_swapchainImageList[FRAME_BUFFER_COUNT];
	VkImageView												m_swapchainImageViewList[FRAME_BUFFER_COUNT];

	VkPhysicalDeviceMemoryProperties m_vkPhysicalDeviceMemProp{};

#if VULKAN_DEBUG == 1
	VkDebugUtilsMessengerEXT                                m_debugUtilsMessenger{ VK_NULL_HANDLE };
	VkDebugReportCallbackEXT                                m_debugReportCallback{ VK_NULL_HANDLE };

	PFN_vkCmdBeginDebugUtilsLabelEXT						m_fpvkCmdBeginDebugUtilsLabel = NULL;
	PFN_vkCmdEndDebugUtilsLabelEXT							m_fpvkCmdEndDebugUtilsLabelEXT = NULL;
	PFN_vkCmdInsertDebugUtilsLabelEXT						m_fpvkCmdInsertDebugUtilsLabelEXT = NULL;
	PFN_vkSetDebugUtilsObjectNameEXT						m_fpVkSetDebugUtilsObjectName = NULL;
#endif

public:
	void cleanUp();
	bool initialize(const InitData& p_initData);
	
	bool CreateInstance(const char* p_applicaitonName);
	bool CreateDevice(VkQueueFlagBits p_queueType);
	bool CreateSurface(HINSTANCE p_hnstns, HWND p_Hwnd);
	bool CreateSwapChain(VkFormat p_format, VkImageUsageFlags p_imageUsage);
	bool CreateSwapChainImages(VkFormat p_format);
	bool CreateFramebuffer(VkRenderPass p_renderPass, VkFramebuffer& p_frameBuffer, VkImageView* p_imageViewList, uint32_t p_fbCount, uint32_t p_width, uint32_t p_height);
	void DestroyFramebuffer(VkFramebuffer p_framebuffer);

	bool AcquireNextSwapChain(VkSemaphore p_semaphore, uint32_t& p_swapChainID);

	bool LoadShader(const char* p_shaderpath, VkShaderModule& p_shader);
		
	bool CreateDescriptorPool(VkDescriptorPoolSize* p_dpSizeList, uint32_t p_dpSizeCount, VkDescriptorPool& p_vkdescriptorPool);
	void DestroyDescriptorPool(VkDescriptorPool p_descPool);

	bool AllocateDescriptorSets(VkDescriptorPool p_dPool, VkDescriptorSetLayout* p_dslList, uint32_t p_dslCount, VkDescriptorSet* p_vkDescriptorSet, void* p_next = VK_NULL_HANDLE);
	bool CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding* p_dsLayoutList, uint32_t p_dsLayoutCount, VkDescriptorSetLayout& p_vkdsLayout, void* p_next = VK_NULL_HANDLE, bool p_isBindless = false);
	void DestroyDescriptorSetLayout(VkDescriptorSetLayout p_descLayput);

	bool CreatePipelineLayout(VkPushConstantRange* p_pushConstants, uint32_t p_pcCount,
		VkDescriptorSetLayout* p_descLayouts, uint32_t p_dlCount, 
		VkPipelineLayout& p_vkPipelineLayout, std::string p_debugName);
	void DestroyPipelineLayout(VkPipelineLayout p_pipeLayout);
	
	bool CreateRenderpass(Renderpass& p_rpData);
	void SetClearColorValue(Renderpass& p_renderpass, uint32_t p_attachmentIndex, const VkClearColorValue& p_colorClearValue);
	void BeginRenderpass(VkFramebuffer p_frameBfr, const Renderpass& p_renderpass, VkCommandBuffer& p_cmdBfr);
	void EndRenderPass(VkCommandBuffer& p_cmdBfr);
	void DestroyRenderpass(VkRenderPass p_renderpass);
	
	bool CreateGraphicsPipeline(const ShaderPaths& p_shaderPaths, Pipeline& pData, std::string p_debugName);
	bool CreateComputePipeline(const ShaderPaths& p_shaderPaths, Pipeline& pData, std::string p_debugName);
	void DestroyPipeline(Pipeline& p_pipeline);
		
	bool CreateCommandPool(uint32_t p_qfIndex, VkCommandPool& p_cmdPool);
	bool ResetCommandPool(VkCommandPool& p_cmdPool);
	void DestroyCommandPool(VkCommandPool p_cmdPool);
	
	bool CreateCommandBuffers(VkCommandPool p_cmdPool, VkCommandBuffer* p_cmdBuffers, uint32_t p_cbCount, std::string* p_debugNames);
	bool BeginCommandBuffer(VkCommandBuffer& p_cmdBfr, const char* p_debugMarker);
	void SetViewport(VkCommandBuffer p_cmdbfr, float p_minD, float p_maxD, float p_width, float p_height);
	void SetScissors(VkCommandBuffer p_cmdBfr, uint32_t p_offX, uint32_t p_offY, uint32_t p_width, uint32_t p_height);
	bool EndCommandBuffer(VkCommandBuffer& p_cmdBfr);
	bool SubmitCommandbuffer(VkQueue p_queue, VkSubmitInfo* p_subInfoList, uint32_t p_subInfoCount, VkFence p_fence = VK_NULL_HANDLE);
	bool ResetCommandBuffer(VkCommandBuffer p_cmdBfr);
	
	bool WaitToFinish(VkQueue p_queue);
	bool CreateSemaphor(VkSemaphore& p_semaphore, std::string p_dbgName);
	void DestroySemaphore(VkSemaphore p_semaphore);
	bool CreateFence(VkFenceCreateFlags p_flags, VkFence& p_fence, std::string p_dbgName);
	bool WaitFence(VkFence& p_fence);
	bool ResetFence(VkFence& p_fence);
	void DestroyFence(VkFence p_fence);

	void IssueLayoutBarrier(VkImageLayout p_new, Image& p_image, VkCommandBuffer p_cmdBfr);
	void IssueImageLayoutBarrier(VkImageLayout p_old, VkImageLayout p_new, uint32_t layerCount, VkImage& p_image, VkImageUsageFlags p_usage, VkCommandBuffer p_cmdBfr);
	void IssueBufferBarrier(VkAccessFlags p_srcAcc, VkAccessFlags p_dstAcc, VkPipelineStageFlags p_srcStg, VkPipelineStageFlags p_dstStg, VkBuffer& p_buffer, VkCommandBuffer p_cmdBfr);

	bool IsFormatSupported(VkFormat p_format, VkFormatFeatureFlags p_featureflag);
	uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& p_physicalDeviceMemProp,
		const VkMemoryRequirements* p_memoryReq, const VkMemoryPropertyFlags p_requiredMemPropFlags);
	
	bool CreateImage(VkImageCreateInfo p_imageCreateInfo, VkImage& p_image);
	bool AllocateImageMemory(VkImage p_image, VkMemoryPropertyFlags p_memFlags, VkDeviceMemory& p_devMem);
	bool BindImageMemory(VkImage& p_image, VkDeviceMemory& p_devMem);
	bool CreateImagView(VkImageUsageFlags p_usage, VkImage p_image, VkFormat p_format, VkImageViewType p_viewType, VkImageView& p_imgView);
	void DestroyImageView(VkImageView p_imageView);
	void DestroyImage(VkImage p_image);
	void ClearImage(VkCommandBuffer p_cmdBfr, VkImage p_src, VkImageLayout p_srclayout, VkClearValue p_clearValue);
	void CopyImage(VkCommandBuffer p_cmdBfr, VkImage p_src, VkImageLayout p_srclayout, VkImage p_dest, VkImageLayout p_destLayout, uint32_t p_width, uint32_t p_height);

	bool CreateSampler(Sampler& p_sampler);
	void DestroySampler(VkSampler p_sampler);

	bool CreateBuffer(VkBufferCreateInfo p_bufferCreateInfo, VkBuffer& p_buffer);
	bool AllocateBufferMemory(VkBuffer p_buffer, VkMemoryPropertyFlags p_memFlags, VkDeviceMemory& p_devMem, size_t& p_reqSize);
	bool BindBufferMemory(VkBuffer& p_buffer, VkDeviceMemory& p_devMem);
	void FreeDeviceMemory(VkDeviceMemory& p_devMem);
	void DestroyBuffer(VkBuffer& p_buffer);

	bool MapMemory(Buffer p_buffer, bool p_flushMemRanges, void** p_data, std::vector<VkMappedMemoryRange>* p_memRanges);
	bool FlushMemoryRanges(std::vector<VkMappedMemoryRange>* p_memRanges);
	void UnMapMemory(Buffer p_buffer);

	bool ReadFromBuffer(void* p_data, Buffer p_buffer);
	bool WriteToBuffer(void* p_data, Buffer p_buffer, bool p_bFlush = false);
	bool UploadFromHostToDevice(Buffer& p_staging, Buffer& p_dest, VkCommandBuffer & p_cmdBfr);
	void UploadFromHostToDevice(Buffer& p_staging, VkImageLayout p_finLayout, Image& p_dest, VkCommandBuffer& p_cmdBfr);
	
	bool ListAvailableInstanceLayers(std::vector<const char*> reqList);
	bool ListAvailableInstanceExtensions(std::vector<const char*> reqList);
};
