#include "VulkanRHI.h"

CVulkanRHI::CVulkanRHI(const char* p_applicaitonName, int p_renderWidth, int p_renderHeight)
: CVulkanCore(p_applicaitonName, p_renderWidth, p_renderHeight)
{
}

CVulkanRHI::~CVulkanRHI()
{
}

bool CVulkanRHI::CreateAllocateBindBuffer(size_t p_size, Buffer& p_buffer, VkBufferUsageFlags p_bfrUsg, VkMemoryPropertyFlags p_propFlag)
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
	if (!AllocateBufferMemory(p_buffer.descInfo.buffer, p_propFlag, p_buffer.devMem))
		return false;
	if (!BindBufferMemory(p_buffer.descInfo.buffer, p_buffer.devMem))
		return false;

	return true;
}

bool CVulkanRHI::CreateTexture(Buffer& p_staging, Image& p_Image, VkImageCreateInfo p_createInfo, VkCommandBuffer& p_cmdBfr)
{
	p_Image.descInfo.imageLayout					= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // final layout

	if (!CreateImage(p_createInfo, p_Image.image))
		return false;
	if (!AllocateImageMemory(p_Image.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_Image.devMem))
		return false;
	if (!BindImageMemory(p_Image.image, p_Image.devMem))
		return false;

	IssueImageLayoutBarrier(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_Image.layerCount, p_Image.image, p_cmdBfr);
	UploadFromHostToDevice(p_staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_Image, p_cmdBfr);
	IssueImageLayoutBarrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_Image.descInfo.imageLayout, p_Image.layerCount, p_Image.image, p_cmdBfr);

	if (!CreateImagView(p_createInfo.usage, p_Image.image, p_Image.format, p_Image.viewType, p_Image.descInfo.imageView))
		return false;

	return true;
}

bool CVulkanRHI::CreateRenderTarget(VkFormat p_format, uint32_t p_width, uint32_t p_height, VkImageLayout p_iniLayout, VkImageUsageFlags p_usage, Image& p_renderTarget)
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
		return false;

	p_renderTarget.format							= p_format;
	p_renderTarget.descInfo.imageLayout				= p_iniLayout;
	p_renderTarget.height							= p_height;
	p_renderTarget.width							= p_width;

	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType							= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.extent							= VkExtent3D{ p_width, p_height, 1 };
	imageCreateInfo.format							= p_renderTarget.format;
	imageCreateInfo.imageType						= VK_IMAGE_TYPE_2D;
	imageCreateInfo.tiling							= VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.mipLevels						= 1;
	imageCreateInfo.arrayLayers						= 1;
	imageCreateInfo.initialLayout					= p_renderTarget.descInfo.imageLayout;
	imageCreateInfo.usage							= p_usage;
	imageCreateInfo.sharingMode						= VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples							= VK_SAMPLE_COUNT_1_BIT;

	if (!CreateImage(imageCreateInfo, p_renderTarget.image))
		return false;
	if (!AllocateImageMemory(p_renderTarget.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_renderTarget.devMem))
		return false;
	if (!BindImageMemory(p_renderTarget.image, p_renderTarget.devMem))
		return false;
	if (!CreateImagView(p_usage, p_renderTarget.image, p_renderTarget.format, VK_IMAGE_VIEW_TYPE_2D, p_renderTarget.descInfo.imageView))
		return false;
	
	return true;
}

void CVulkanRHI::FreeMemoryDestroyBuffer(Buffer& p_buffer)
{
	FreeDeviceMemory(p_buffer.devMem);
	DestroyBuffer(p_buffer.descInfo.buffer);
}

bool CVulkanRHI::CreateDescriptors(const DescDataList& p_descdataList, VkDescriptorPool& p_descPool, VkDescriptorSetLayout* p_descLayout, uint32_t p_layoutCount, VkDescriptorSet* p_desc, void* p_next)
{
	if (!AllocateDescriptorSets(p_descPool, p_descLayout, p_layoutCount, p_desc, p_next))
		return false;

	std::vector<VkWriteDescriptorSet> writeDescList;
	for (const auto& desc : p_descdataList)
	{
		//for (int i = 0; i < desc.count; i++)
		{
			if (desc.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
				|| desc.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			{
				VkWriteDescriptorSet wdSet{};
				wdSet.sType						= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wdSet.pNext						= nullptr;
				wdSet.dstSet					= *p_desc;
				wdSet.descriptorCount			= desc.count;
				wdSet.descriptorType			= desc.type;
				wdSet.dstBinding				= desc.bindingDest;
				wdSet.pBufferInfo				= desc.bufDesInfo; // since we need 1 uniform descripotr per frame buffer; setting accordinigly

				writeDescList.push_back(wdSet);
			}

			if (desc.type == VK_DESCRIPTOR_TYPE_SAMPLER
				|| desc.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
				|| desc.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
				|| desc.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				VkWriteDescriptorSet wdSet{};
				wdSet.sType						= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wdSet.pNext						= nullptr;
				wdSet.dstSet					= *p_desc;
				wdSet.descriptorCount			= desc.count;
				wdSet.descriptorType			= desc.type;
				wdSet.dstBinding				= desc.bindingDest;
				wdSet.pImageInfo				= desc.imgDesinfo;

				writeDescList.push_back(wdSet);
			}
		}
	}

	vkUpdateDescriptorSets(m_vkDevice, (uint32_t)writeDescList.size(), writeDescList.data(), 0, nullptr);

	return true;
}

bool CVulkanRHI::CreateDescriptors(uint32_t p_varDescCount, const DescDataList& p_descDataList, VkDescriptorPool& p_descPool, VkDescriptorSetLayout& p_descSetLayout, VkDescriptorSet& p_descSet, uint32_t p_texArrayIndex)
{
	std::vector<VkDescriptorPoolSize> dPSize;
	std::vector<VkDescriptorSetLayoutBinding> dsLayoutBinding;
	for (const auto& desc : p_descDataList)
	{
		dPSize.push_back(VkDescriptorPoolSize{ desc.type, desc.count * FRAME_BUFFER_COUNT });
		dsLayoutBinding.push_back(VkDescriptorSetLayoutBinding{ (uint32_t)desc.bindingDest, desc.type, desc.count, desc.shaderStage });
	}

	if (!CreateDescriptorPool(dPSize.data(), (uint32_t)dPSize.size(), p_descPool))
		return false;

	std::vector<VkDescriptorBindingFlagsEXT> descBindFlags(p_descDataList.size(), 0);
	descBindFlags[p_texArrayIndex]				= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT layoutBindFlags{};
	layoutBindFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	layoutBindFlags.bindingCount				= (uint32_t)descBindFlags.size();
	layoutBindFlags.pBindingFlags				= descBindFlags.data();

	if (!CreateDescriptorSetLayout(dsLayoutBinding.data(), (uint32_t)dsLayoutBinding.size(), p_descSetLayout, &layoutBindFlags))
		return false;

	uint32_t varDescCount[] = { p_varDescCount };
	VkDescriptorSetVariableDescriptorCountAllocateInfoEXT varDescCountAllocInfo = {};
	varDescCountAllocInfo.sType					= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	varDescCountAllocInfo.descriptorSetCount	= 1;
	varDescCountAllocInfo.pDescriptorCounts		= varDescCount;

	if (!CreateDescriptors(p_descDataList, p_descPool, &p_descSetLayout, 1, &p_descSet, &varDescCountAllocInfo))
		return false;

	return true;
}


