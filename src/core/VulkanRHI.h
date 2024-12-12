#pragma once

#include "VulkanCore.h"

class CVulkanRHI : public CVulkanCore
{
public:
	enum RendererType
	{
		 Forward
		,Deferred
	};

	enum DescriptorBindFlag
	{
			Traditonal		= 0
		,	Variable_Count	= 1
		,	Bindless		= 2
	};
	typedef int DescriptorBindFlags;

	struct DescriptorData
	{
		uint32_t							arrayDestIndex;
		int									bindingDest;
		uint32_t							count;
		VkDescriptorType					type;
		VkShaderStageFlags					shaderStage;
		const VkDescriptorBufferInfo*		bufDesInfo;
		const VkDescriptorImageInfo*		imgDesinfo;
	};
	typedef std::vector<DescriptorData> DescDataList;

	CVulkanRHI(const char* p_applicaitonName, int p_renderWidth, int p_renderHeight);
	~CVulkanRHI();

	bool SubmitCommandBuffers(
		CommandBufferList* p_commndBfrList, PipelineStageFlagsList* p_psfList, 
		bool p_waitForFinish = false, VkFence* p_fence = VK_NULL_HANDLE, 
		bool p_waitforFence = false, SemaphoreList* p_signalList = nullptr, 
		SemaphoreList* p_waitList = nullptr, QueueType p_queueType = QueueType::qt_Primary);

	bool CreateAllocateBindBuffer(size_t p_size, Buffer& p_buffer, VkBufferUsageFlags p_bfrUsg, VkMemoryPropertyFlags p_propFlagm, std::string p_DebugName);
	bool CreateTexture(Buffer& p_staging, Image& p_Image, VkImageCreateInfo p_createInfo, VkCommandBuffer& p_cmdBfr, std::string p_DebugName);
	void CreateMipmaps(Image& p_image, VkCommandBuffer& p_cmdBfr);
	bool CreateRenderTarget(VkFormat p_format, uint32_t p_width, uint32_t p_height, VkImageLayout p_Layout, VkImageUsageFlags p_usage, Image& p_renderTarget, std::string p_DebugName);
	void ClearImage(CommandBuffer p_cmdBfr, CVulkanRHI::Image p_src, VkClearValue p_clearValue);
	void CopyImage(CommandBuffer p_cmdBfr, CVulkanRHI::Image p_src, CVulkanRHI::Image p_dest);

	void FreeMemoryDestroyBuffer(Buffer&);

	bool CreateDescriptors(const DescDataList& p_descdataList, VkDescriptorPool& p_descPool,VkDescriptorSetLayout* p_descLayout, uint32_t p_layoutCount, VkDescriptorSet* p_desc, void* p_next = VK_NULL_HANDLE, bool p_bindless = false, std::string p_DebugName = "");
	bool CreateDescriptors(const DescDataList& p_descDataList, VkDescriptorPool& p_descPool, VkDescriptorSetLayout& p_descSetLayout, VkDescriptorSet& p_descSet, DescriptorBindFlags p_bindType = DescriptorBindFlag::Traditonal, std::string p_DebugName = "");
	void WriteUpdateDescriptors(VkDescriptorSet* p_desc, const DescDataList& p_descDataList);

	RendererType GetRendererType() { return m_rendererType; }
	void SetRenderType(RendererType p_type) { m_rendererType = p_type; }

private:
	RendererType m_rendererType;
};