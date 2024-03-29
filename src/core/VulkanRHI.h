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

	struct DescriptorData
	{
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
		SemaphoreList* p_waitList = nullptr);

	bool CreateAllocateBindBuffer(size_t p_size, Buffer& p_buffer, VkBufferUsageFlags p_bfrUsg, VkMemoryPropertyFlags p_propFlag);
	bool CreateTexture(Buffer& p_staging, Image& p_Image, VkImageCreateInfo p_createInfo, VkCommandBuffer& p_cmdBfr);
	bool CreateRenderTarget(VkFormat p_format, uint32_t p_width, uint32_t p_height, VkImageLayout p_iniLayout, VkImageUsageFlags p_usage, Image& p_renderTarget);

	void FreeMemoryDestroyBuffer(Buffer&);

	bool CreateDescriptors(const DescDataList& p_descdataList, VkDescriptorPool& p_descPool,VkDescriptorSetLayout* p_descLayout, uint32_t p_layoutCount, VkDescriptorSet* p_desc, void* p_next = VK_NULL_HANDLE);
	bool CreateDescriptors(uint32_t p_varDescCount, const DescDataList& p_descDataList, VkDescriptorPool& p_descPool,VkDescriptorSetLayout& p_descSetLayout, VkDescriptorSet& p_descSet, uint32_t p_texArrayIndex);

	RendererType GetRendererType() { return m_rendererType; }
	void SetRenderType(RendererType p_type) { m_rendererType = p_type; }

private:
	RendererType m_rendererType;
};