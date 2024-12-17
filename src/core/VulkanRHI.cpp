#include "VulkanRHI.h"

CVulkanRHI::CVulkanRHI(const char* p_applicaitonName, int p_renderWidth, int p_renderHeight)
: CVulkanCore(p_applicaitonName, p_renderWidth, p_renderHeight)
, m_rendererType(RendererType::Forward)
{
}

CVulkanRHI::~CVulkanRHI()
{
}

bool CVulkanRHI::SubmitCommandBuffers(
	CommandBufferList* p_commndBfrList, PipelineStageFlagsList* p_psfList, bool p_waitForFinish, 
	VkFence* p_fence, bool p_waitforFence, SemaphoreList* p_signalList, SemaphoreList* p_waitList, 
	QueueType p_queueType)
{
	if (!p_commndBfrList || !p_psfList)
	{
		std::cerr << "CVulkanRHI::SubmitCommandBuffers Failed - CommandBuffer/PipelineStageFlag is nullptr. " << std::endl;
		return false;
	}

	if (p_commndBfrList->empty() || p_psfList->empty())
	{
		std::cerr << "CVulkanRHI::SubmitCommandBuffers - CommandBuffer/PipelineStageFlag count is 0. " << std::endl;
		return false;
	}

	if (p_commndBfrList->size() < p_psfList->size())
	{
		std::cerr << "CVulkanRHI::SubmitCommandBuffers - CommandBuffer count and PipelineStageFlag count mismatch. " << std::endl;
		return false;
	}

	CVulkanRHI::Queue queue = VK_NULL_HANDLE;
	if (p_queueType == QueueType::qt_Secondary)
	{
		queue = GetSecondaryQueue();
	}
	else // defaulting to executing to primary (graphics) queue if none provided
	{
		queue = GetQueue(0);
	}
		
	//VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = (uint32_t)p_commndBfrList->size();
	submitInfo.pCommandBuffers = p_commndBfrList->data();
	submitInfo.pSignalSemaphores = p_signalList == nullptr ? nullptr : p_signalList->data();
	submitInfo.signalSemaphoreCount = p_signalList == nullptr ? 0 : (uint32_t)p_signalList->size();
	submitInfo.pWaitSemaphores = p_waitList == nullptr ? nullptr : p_waitList->data();
	submitInfo.waitSemaphoreCount = p_waitList == nullptr ? 0 : (uint32_t)p_waitList->size();
	submitInfo.pWaitDstStageMask = p_psfList->data();
	if (!SubmitCommandbuffer(queue, &submitInfo, 1, (p_fence == nullptr ? VK_NULL_HANDLE : *p_fence)))
		return false;

	if (p_waitForFinish)
	{
		// This is a blocking call
		if (!WaitToFinish(queue))
			return false;
	}

	if (p_waitforFence)
	{
		// This is a blocking call
		if (!WaitFence(*p_fence))
			return false;
	}

	return true;
}

bool CVulkanRHI::CreateAllocateBindBuffer(size_t p_size, Buffer& p_buffer, VkBufferUsageFlags p_bfrUsg, 
	VkMemoryPropertyFlags p_propFlag, std::string p_DebugName)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType							= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.usage							= p_bfrUsg;
	bufferCreateInfo.sharingMode					= VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.pQueueFamilyIndices			= &m_QFIndex;
	bufferCreateInfo.queueFamilyIndexCount			= 1;
	bufferCreateInfo.size							= p_size;

	p_buffer.descInfo.range							= p_size;
	p_buffer.descInfo.offset						= 0;
	p_buffer.memPropFlags							= p_propFlag;

	if (!CreateBuffer(bufferCreateInfo, p_buffer.descInfo.buffer))
		return false;
	if (!AllocateBufferMemory(p_buffer.descInfo.buffer, p_propFlag, p_buffer.devMem, p_buffer.reqMemSize))
		return false;
	if (!BindBufferMemory(p_buffer.descInfo.buffer, p_buffer.devMem))
		return false;

	SetDebugName((uint64_t)p_buffer.descInfo.buffer, VK_OBJECT_TYPE_BUFFER, (p_DebugName + "_buffer").c_str());

	return true;
}

bool CVulkanRHI::CreateTexture(Buffer& p_staging, Image& p_Image, 
	VkImageCreateInfo p_createInfo, VkCommandBuffer& p_cmdBfr, std::string p_DebugName)
{
	p_Image.descInfo.imageLayout					= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // final layout
	p_Image.usage									= p_createInfo.usage;

	if (!CreateImage(p_createInfo, p_Image.image))
		return false;
	if (!AllocateImageMemory(p_Image.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_Image.devMem))
		return false;
	if (!BindImageMemory(p_Image.image, p_Image.devMem))
		return false;

	IssueImageLayoutBarrier(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_Image.layerCount, p_Image.GetLevelCount(), p_Image.image, p_Image.usage, p_cmdBfr);
	UploadFromHostToDevice(p_staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_Image, p_cmdBfr);

	if (p_Image.GetLevelCount() > 1)
		CreateMipmaps(p_Image, p_cmdBfr);
	else
		IssueImageLayoutBarrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_Image.descInfo.imageLayout, p_Image.layerCount, p_Image.GetLevelCount(), p_Image.image, p_Image.usage, p_cmdBfr);
	
	SetDebugName((uint64_t)p_Image.image, VK_OBJECT_TYPE_IMAGE, (p_DebugName + "_image").c_str());

	if (!CreateImagView(p_createInfo.usage, p_Image.image, p_Image.format, p_Image.viewType, p_Image.GetLevelCount(), p_Image.descInfo.imageView))
		return false;
	
	SetDebugName((uint64_t)p_Image.descInfo.imageView, VK_OBJECT_TYPE_IMAGE_VIEW, (p_DebugName + "_image_view").c_str());

	return true;
}

void CVulkanRHI::CreateMipmaps(Image& p_image, VkCommandBuffer& p_cmdBfr)
{
	int32_t mipWidth = p_image.width;
	int32_t mipHeight = p_image.height;
	for (uint32_t i = 1; i < p_image.GetLevelCount(); i++)
	{
		uint32_t baseMipLevel = i - 1;

		IssueImageLayoutBarrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, 1, p_image.image, p_image.usage, p_cmdBfr, baseMipLevel);

		VkImageBlit imgBlt{};
		imgBlt.srcOffsets[0] = { 0, 0, 0 };
		imgBlt.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		imgBlt.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBlt.srcSubresource.baseArrayLayer = 0;
		imgBlt.srcSubresource.mipLevel = baseMipLevel;
		imgBlt.srcSubresource.layerCount = 1;
		imgBlt.dstOffsets[0] = { 0, 0, 0 };
		imgBlt.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		imgBlt.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBlt.dstSubresource.mipLevel = i;
		imgBlt.dstSubresource.baseArrayLayer = 0;
		imgBlt.dstSubresource.layerCount = 1;
		BlitImage(p_cmdBfr, imgBlt, p_image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_image.format);

		IssueImageLayoutBarrier(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_image.descInfo.imageLayout, 1, 1, p_image.image, p_image.usage, p_cmdBfr, baseMipLevel);

		mipWidth = (mipWidth > 1) ? mipWidth / 2 : 1;
		mipHeight = (mipHeight > 1) ? mipHeight / 2 : 1;
	}
}

bool CVulkanRHI::CreateRenderTarget(VkFormat p_format, uint32_t p_width, uint32_t p_height, uint32_t p_levelCount,
	VkImageLayout p_layout, VkImageUsageFlags p_usage, Image& p_renderTarget, std::string p_DebugName)
{
	VkFormatFeatureFlags feature;
	if (p_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		feature |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (p_usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		feature |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
	if (p_usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT || p_usage & VK_IMAGE_USAGE_SAMPLED_BIT)
		feature |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
	if (p_usage & VK_IMAGE_USAGE_STORAGE_BIT)
		feature |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

	if (!IsFormatSupported(p_format, feature))
	{	
		std::cerr << "Failed to Create Render Target (CVulkanRHI::CreateRenderTarget): " << p_DebugName << std::endl;
		return false;
	}

	p_renderTarget.usage							= p_usage;
	p_renderTarget.format							= p_format;
	p_renderTarget.descInfo.imageLayout				= p_layout;
	p_renderTarget.height							= p_height;
	p_renderTarget.width							= p_width;	
	p_renderTarget.SetLevelCount(p_levelCount);
	for(uint32_t  i = 0; i < p_levelCount; i++)
		p_renderTarget.curLayout[i] = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType							= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.extent							= VkExtent3D{ p_width, p_height, 1 };
	imageCreateInfo.format							= p_renderTarget.format;
	imageCreateInfo.imageType						= VK_IMAGE_TYPE_2D;
	imageCreateInfo.tiling							= VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.mipLevels						= p_levelCount;
	imageCreateInfo.arrayLayers						= 1;
	imageCreateInfo.initialLayout					= VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage							= p_usage;
	imageCreateInfo.sharingMode						= VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples							= VK_SAMPLE_COUNT_1_BIT;

	if (!CreateImage(imageCreateInfo, p_renderTarget.image))
		return false;
	if (!AllocateImageMemory(p_renderTarget.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_renderTarget.devMem))
		return false;
	if (!BindImageMemory(p_renderTarget.image, p_renderTarget.devMem))
		return false;
	if (!CreateImagView(p_usage, p_renderTarget.image, p_renderTarget.format, VK_IMAGE_VIEW_TYPE_2D, p_levelCount, p_renderTarget.descInfo.imageView))
		return false;
	
	SetDebugName((uint64_t)p_renderTarget.image, VK_OBJECT_TYPE_IMAGE, (p_DebugName + "_rt").c_str());

	return true;
}

void CVulkanRHI::ClearImage(CommandBuffer p_cmdBfr, CVulkanRHI::Image p_src, VkClearValue p_clearValue)
{
	CVulkanCore::ClearImage(p_cmdBfr, p_src.image, p_src.descInfo.imageLayout, p_clearValue);
}

void CVulkanRHI::CopyImage(CommandBuffer p_cmdBfr, CVulkanRHI::Image p_src, CVulkanRHI::Image p_dest)
{
	CVulkanCore::CopyImage(p_cmdBfr, p_src.image, p_src.descInfo.imageLayout, p_dest.image, p_dest.descInfo.imageLayout, p_src.width, p_src.height);
}

void CVulkanRHI::FreeMemoryDestroyBuffer(Buffer& p_buffer)
{
	FreeDeviceMemory(p_buffer.devMem);
	DestroyBuffer(p_buffer.descInfo.buffer);
	p_buffer.descInfo.buffer = VK_NULL_HANDLE;
}

bool CVulkanRHI::CreateDescriptors(	const DescDataList& p_descdataList, VkDescriptorPool& p_descPool, 
									VkDescriptorSetLayout* p_descLayout, uint32_t p_layoutCount,
									VkDescriptorSet* p_desc, void* p_next, bool p_bindless, std::string p_DebugName)
{
	if (!AllocateDescriptorSets(p_descPool, p_descLayout, p_layoutCount, p_desc, p_next))
		return false;

	SetDebugName((uint64_t)*p_desc, VK_OBJECT_TYPE_DESCRIPTOR_SET, (p_DebugName).c_str());

	// If not bindless; we choose to write and update these descriptors
	if (!p_bindless)
	{
		WriteUpdateDescriptors(p_desc, p_descdataList);
	}

	return true;
}

void CVulkanRHI::WriteUpdateDescriptors(VkDescriptorSet* p_desc, const DescDataList& p_descDataList)
{
	std::vector<VkWriteDescriptorSet> writeDescList;
	for (const auto& desc : p_descDataList)
	{
		//for (int i = 0; i < desc.count; i++)
		{
			if (desc.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
				|| desc.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			{
				if (desc.bufDesInfo == VK_NULL_HANDLE)
					continue;


				VkWriteDescriptorSet wdSet{};
				wdSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wdSet.pNext = nullptr;
				wdSet.dstSet = *p_desc;
				wdSet.descriptorCount = desc.count;
				wdSet.descriptorType = desc.type;
				wdSet.dstBinding = desc.bindingDest;
				wdSet.pBufferInfo = desc.bufDesInfo; // since we need 1 uniform descriptor per frame buffer; setting accordingly
				wdSet.dstArrayElement = desc.arrayDestIndex;

				writeDescList.push_back(wdSet);
			}

			if (desc.type == VK_DESCRIPTOR_TYPE_SAMPLER
				|| desc.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
				|| desc.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
				|| desc.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				if (desc.imgDesinfo == VK_NULL_HANDLE)
					continue;

				VkWriteDescriptorSet wdSet{};
				wdSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wdSet.pNext = nullptr;
				wdSet.dstSet = *p_desc;
				wdSet.descriptorCount = desc.count;
				wdSet.descriptorType = desc.type;
				wdSet.dstBinding = desc.bindingDest;
				wdSet.pImageInfo = desc.imgDesinfo;
				wdSet.dstArrayElement = desc.arrayDestIndex;

				writeDescList.push_back(wdSet);
			}
		}
	}

	vkUpdateDescriptorSets(m_vkDevice, (uint32_t)writeDescList.size(), writeDescList.data(), 0, nullptr);
}

bool CVulkanRHI::CreateDescriptors(	const DescDataList& p_descDataList, VkDescriptorPool& p_descPool, 
									VkDescriptorSetLayout& p_descSetLayout, VkDescriptorSet& p_descSet, 
									DescriptorBindFlags p_bindFlags, std::string p_DebugName)
{
	std::vector<VkDescriptorPoolSize> dPSize;
	std::vector<VkDescriptorSetLayoutBinding> dsLayoutBinding;
	std::vector<VkDescriptorBindingFlags> descBindFlags(p_descDataList.size(), 0);
	bool hasSpecialDescriptorBindingFlags = false;
	uint32_t maxDescriptorCount = 0;
	
	for (uint32_t i = 0; i < (uint32_t)p_descDataList.size(); i++)
	{
		DescriptorData desc = p_descDataList[i];

		dPSize.push_back(VkDescriptorPoolSize{ desc.type, desc.count * FRAME_BUFFER_COUNT });
		dsLayoutBinding.push_back(VkDescriptorSetLayoutBinding{ (uint32_t)desc.bindingDest, desc.type, desc.count, desc.shaderStage });

		if (p_bindFlags & DescriptorBindFlag::Variable_Count
			&& desc.count > 1)
		{
			hasSpecialDescriptorBindingFlags = true;
			descBindFlags[i] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

			maxDescriptorCount = max(maxDescriptorCount, desc.count);
		}

		if (p_bindFlags & DescriptorBindFlag::Bindless)
		{
			hasSpecialDescriptorBindingFlags = true;
			descBindFlags[i] |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
				VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
				VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
		}
	}

	// We are creating a descriptor pool with UPDATE_AFTER_BIND_BIT bit set but that shouldn't stop us
	// from using the pool in a traditional way either.
	if (!CreateDescriptorPool(dPSize.data(), (uint32_t)dPSize.size(), p_descPool))
		return false;

	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT layoutBindFlags{};
	layoutBindFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	layoutBindFlags.bindingCount				= (uint32_t)descBindFlags.size();
	layoutBindFlags.pBindingFlags				= descBindFlags.data();

	void* layoutFlags = (hasSpecialDescriptorBindingFlags) ? &layoutBindFlags : nullptr;
	if (!CreateDescriptorSetLayout(dsLayoutBinding.data(), (uint32_t)dsLayoutBinding.size(), 
									p_descSetLayout, layoutFlags, (p_bindFlags & DescriptorBindFlag::Bindless)))
		return false;

	uint32_t varDescCount[] = { maxDescriptorCount };
	VkDescriptorSetVariableDescriptorCountAllocateInfoEXT varDescCountAllocInfo = {};
	varDescCountAllocInfo.sType					= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	varDescCountAllocInfo.descriptorSetCount	= 1;
	varDescCountAllocInfo.pDescriptorCounts		= varDescCount;

	void* allocInfo = (maxDescriptorCount == 0) ? nullptr : &varDescCountAllocInfo;
	if (!CreateDescriptors(p_descDataList, p_descPool, &p_descSetLayout, 1, &p_descSet, allocInfo, (p_bindFlags & DescriptorBindFlag::Bindless), p_DebugName))
			return false;
	
	return true;
}
