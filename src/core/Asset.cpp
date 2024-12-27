#include "Asset.h"
#include "SceneGraph.h"
#include "RandGen.h"

#include <algorithm>
#include <thread>

#include "external/imgui/imgui.h"
#include "external/imguizmo/ImGuizmo.h"

std::vector<CUIParticipant*> CUIParticipantManager::m_uiParticipants;

void CDescriptor::AddDescriptor(CVulkanRHI::DescriptorData p_dData, uint32_t p_id)
{
	m_descList[p_id].descDataList.push_back(p_dData);
}

void CDescriptor::BindlessWrite(uint32_t p_swId, uint32_t p_index, const VkDescriptorImageInfo* p_imageInfo, uint32_t p_count, uint32_t p_arrayDestIndex)
{
	if (p_swId >= m_descList.size())
	{
		std::cerr << "CDescriptor::BindlessWrite: Attempting to Bindless Write a descriptor with a bad swap chain index" << std::endl;
		return;
	}
		

	if (p_index >= m_descList[0].descDataList.size())
	{
		std::cerr << "CDescriptor::BindlessWrite: Attempting to Bindless Write a descriptor with a bad binding index" << std::endl;
		return;
	}

	m_descList[p_swId].descDataList[p_index].count = p_count;
	m_descList[p_swId].descDataList[p_index].arrayDestIndex = p_arrayDestIndex;
	m_descList[p_swId].descDataList[p_index].imgDesinfo = p_imageInfo;
}

void CDescriptor::BindlessWrite(uint32_t p_swId, uint32_t p_index, const VkDescriptorBufferInfo* p_bufferInfo, uint32_t p_count)
{
	if (p_swId >= m_descList.size())
	{
		std::cerr << "Attempting to Bindless Write a descriptor with a bad swap chain index" << std::endl;
		return;
	}


	if (p_index >= m_descList[0].descDataList.size())
	{
		std::cerr << "Attempting to Bindless Write a descriptor with a bad binding index" << std::endl;
		return;
	}

	m_descList[p_swId].descDataList[p_index].count = p_count;
	m_descList[p_swId].descDataList[p_index].bufDesInfo = p_bufferInfo;
}

void CDescriptor::BindlessWrite(uint32_t p_swId, uint32_t p_index, const VkAccelerationStructureKHR* p_accStructure, uint32_t p_count)
{
	if (p_swId >= m_descList.size())
	{
		std::cerr << "Attempting to Bindless Write a descriptor with a bad swap chain index" << std::endl;
		return;
	}


	if (p_index >= m_descList[0].descDataList.size())
	{
		std::cerr << "Attempting to Bindless Write a descriptor with a bad binding index" << std::endl;
		return;
	}

	m_descList[p_swId].descDataList[p_index].count = p_count;
	m_descList[p_swId].descDataList[p_index].accelerationStructure = p_accStructure;
}

void CDescriptor::BindlessUpdate(CVulkanRHI* p_rhi, uint32_t p_swId)
{
	if (p_swId >= m_descList.size())
	{
		std::cerr << "Attempting to Bindless update a descriptor set with a bad swap chain index" << std::endl;
		return;
	}

	p_rhi->WriteUpdateDescriptors(&m_descList[p_swId].descSet, m_descList[p_swId].descDataList);
}

CDescriptor::CDescriptor(CVulkanRHI::DescriptorBindFlags p_bindFlags, uint32_t p_descSetCount)
	: m_bindFlags(p_bindFlags)
{
	m_descList.resize(p_descSetCount);
}

bool CDescriptor::CreateDescriptors(CVulkanRHI* p_rhi, uint32_t p_id, std::string p_debugName)
{
	if (!p_rhi->CreateDescriptors(m_descList[p_id].descDataList, m_descPool, m_descList[p_id].descLayout, m_descList[p_id].descSet, m_bindFlags, p_debugName))
		return false;

	return true;
}

void CDescriptor::DestroyDescriptors(CVulkanRHI* p_rhi)
{
	for (auto& desc : m_descList)
	{
		p_rhi->DestroyDescriptorSetLayout(desc.descLayout);
		desc.descDataList.clear();
	}
	p_rhi->DestroyDescriptorPool(m_descPool);
	m_descList.clear();
}

CRenderable::CRenderable(VkMemoryPropertyFlags p_memPropFlags, uint32_t p_BufferCount)
	: m_vertexBuffers(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, p_memPropFlags, p_BufferCount)
	, m_indexBuffers(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, p_memPropFlags, p_BufferCount)
	, m_instanceCount(1)
{
}

CRenderable::CRenderable(VkBufferUsageFlags p_usageFlags, VkMemoryPropertyFlags p_memPropFlags, uint32_t p_BufferCount)
	: m_vertexBuffers(p_usageFlags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, p_memPropFlags, p_BufferCount)
	, m_indexBuffers(p_usageFlags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, p_memPropFlags, p_BufferCount)
	, m_instanceCount(1)
{
}

void CRenderable::Clear(CVulkanRHI* p_rhi, uint32_t p_idx)
{
	m_vertexBuffers.DestroyBuffer(p_rhi, p_idx);
	m_indexBuffers.DestroyBuffer(p_rhi, p_idx);

	//m_vertexBuffers[p_idx].descInfo.offset = 0;
	//m_vertexBuffers[p_idx].descInfo.range = 0;
	//p_rhi->FreeMemoryDestroyBuffer(m_vertexBuffers[p_idx]);
	//
	//m_indexBuffers[p_idx].descInfo.offset = 0;
	//m_indexBuffers[p_idx].descInfo.range = 0;
	//p_rhi->FreeMemoryDestroyBuffer(m_indexBuffers[p_idx]);
}

void CRenderable::DestroyRenderable(CVulkanRHI* p_rhi)
{
	//m_blasBuffer->DestroyBuffers(p_rhi);
	//p_rhi->DestroyAccelerationStrucutre(m_BLAS);

	m_vertexBuffers.DestroyBuffers(p_rhi);
	m_indexBuffers.DestroyBuffers(p_rhi);

	//for (int i = 0; i < m_vertexBuffers.size(); i++)
	//{
	//	m_vertexBuffers[i].descInfo.offset = 0;
	//	m_vertexBuffers[i].descInfo.range = 0;
	//	p_rhi->FreeMemoryDestroyBuffer(m_vertexBuffers[i]);
	//
	//	m_indexBuffers[i].descInfo.offset = 0;
	//	m_indexBuffers[i].descInfo.range = 0;
	//	p_rhi->FreeMemoryDestroyBuffer(m_indexBuffers[i]);
	//}
}

//bool CRenderable::CreateVertexIndexBuffer(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, 
//	const MeshRaw* p_meshRaw, CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugStr, int32_t p_id, bool p_useForRaytracing)
//{
//	std::clog << "Creating Vertex and Index Buffer for " << p_debugStr << std::endl;
//
//	// for creating Acceleration Structure
//	m_primitiveCount = (uint32_t)p_meshRaw->indicesList.size() / 3;
//	m_vertexStrideInBytes = p_meshRaw->vertexList.GetVertexSize() * sizeof(float);
//	m_vertexCount = (uint32_t)p_meshRaw->vertexList.size() / (uint32_t)p_meshRaw->vertexList.GetVertexSize();
//	uint32_t rayTracingUsage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
//
//	{
//		CVulkanRHI::Buffer vertexStg;
//		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeof(float) * p_meshRaw->vertexList.size(), vertexStg, 
//			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_debugStr + "_vert_transfer"));
//
//		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)p_meshRaw->vertexList.data(), vertexStg));
//		p_stgList.push_back(vertexStg);
//
//		CVulkanRHI::Buffer vertexBuffer;
//		VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
//
//		if (p_useForRaytracing)
//			bufferUsageFlags |= rayTracingUsage;
//
//		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(vertexStg.descInfo.range, vertexBuffer, 
//			bufferUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_debugStr + "_vertex"));
//
//		RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(vertexStg, vertexBuffer, p_cmdBfr));
//		
//		// An index (p_id) is passed in situations where we want to track the
//		// specific buffer later in the frame This is useful in cases when you
//		// want to access this buffer during the application's lifetime to
//		// specially draw it with some conditions. Eg: The debug frustum vertex
//		// buffer needs to be destroyed and recreated multiple time during the
//		// application's lifetime.
//		if (p_id >= 0)
//			m_vertexBuffers[p_id] = vertexBuffer;
//		else
//			m_vertexBuffers.push_back(vertexBuffer);
//	}
//
//	{
//		CVulkanRHI::Buffer indxStg;
//		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeof(uint32_t) * p_meshRaw->indicesList.size(), indxStg, 
//			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_debugStr + "_ind_transfer"));
//
//		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)p_meshRaw->indicesList.data(), indxStg));
//		p_stgList.push_back(indxStg);
//
//		CVulkanRHI::Buffer indexBuffer;
//		VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
//
//		if (p_useForRaytracing)
//			bufferUsageFlags |= rayTracingUsage;
//
//		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(indxStg.descInfo.range, indexBuffer, 
//			bufferUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_debugStr + "_index"));
//
//		RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(indxStg, indexBuffer, p_cmdBfr));
//		
//		// An index (p_id) is passed in situations where we want to track the
//		// specific buffer later in the frame This is useful in cases when you
//		// want to access this buffer during the application's lifetime to
//		// specially draw it with some conditions. Eg: The debug frustum index
//		// buffer needs to be destroyed and recreated multiple time during the
//		// application's lifetime.
//		if (p_id >= 0)
//			m_indexBuffers[p_id] = indexBuffer;
//		else
//			m_indexBuffers.push_back(indexBuffer);
//	}
//
//	if(p_useForRaytracing)
//	{
//		RETURN_FALSE_IF_FALSE(CreateBuildBLAS(p_rhi, p_stgList, p_cmdBfr, p_debugStr));
//	}
//
//	return true;
//}

bool CRenderable::CreateVertexIndexBuffer(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stg, const MeshRaw* p_meshRaw, CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugStr, int32_t index)
{
	if (!(m_vertexBuffers.GetMemoryProperties() & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
	{
		std::cerr << "CRenderable::CreateVertexIndexBuffer: Trying to create host memory resource for a buffer that is device local - " << p_debugStr << std::endl;
		return false;
	}

	// for creating Acceleration Structure
	m_primitiveCount = (uint32_t)p_meshRaw->indicesList.size() / 3;
	m_vertexStrideInBytes = p_meshRaw->vertexList.GetVertexSize() * sizeof(float);
	m_vertexCount = (uint32_t)p_meshRaw->vertexList.size() / (uint32_t)p_meshRaw->vertexList.GetVertexSize();

	CVulkanRHI::Buffer vertexStg;
	RETURN_FALSE_IF_FALSE(m_vertexBuffers.CreateBuffer(p_rhi, vertexStg, (void*)p_meshRaw->vertexList.data(), p_meshRaw->vertexList.size(), p_cmdBfr, p_debugStr + "_Vertex"));
	p_stg.push_back(vertexStg);

	CVulkanRHI::Buffer indexStg;
	RETURN_FALSE_IF_FALSE(m_indexBuffers.CreateBuffer(p_rhi, indexStg, (void*)p_meshRaw->indicesList.data(), p_meshRaw->indicesList.size(), p_cmdBfr, p_debugStr + "_Index"));
	p_stg.push_back(indexStg);

	return true;
}

bool CRenderable::CreateVertexIndexBuffer(CVulkanRHI* p_rhi, const InData& p_inData, std::string p_debugStr, int32_t index)
{
	if (m_vertexBuffers.GetMemoryProperties() & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	{
		std::cerr << "CRenderable::CreateVertexIndexBuffer: Trying to create device memory resource for a buffer that is not device local - " << p_debugStr << std::endl;
		return false;
	}

	RETURN_FALSE_IF_FALSE(m_vertexBuffers.CreateBuffer(p_rhi, p_inData.vertexBufferData, p_inData.vertexBufferSize, p_debugStr + "_Vertex"));
	RETURN_FALSE_IF_FALSE(m_indexBuffers.CreateBuffer(p_rhi, p_inData.indexBufferData, p_inData.indexBufferSize, p_debugStr + "_Index"));

	return true;
}

const CVulkanRHI::Buffer CRenderable::GetVertexBuffer(uint32_t p_idx) const
{
	return m_vertexBuffers.GetBuffer(p_idx);
}

const CVulkanRHI::Buffer CRenderable::GetIndexBuffer(uint32_t p_idx) const
{
	return m_indexBuffers.GetBuffer(p_idx);
}

CBuffers::CBuffers(int p_maxSize)
{
	if(p_maxSize > 0)
		m_buffers.resize(p_maxSize);
}

CBuffers::CBuffers(VkBufferUsageFlags p_usageFlags, VkMemoryPropertyFlags p_memPropFlags, int p_maxSize)
	: m_usageFlags(p_usageFlags)
	, m_memPropFlags(p_memPropFlags)
{
	if (p_maxSize > 0)
		m_buffers.resize(p_maxSize);
}

bool CBuffers::CreateBuffer(CVulkanRHI* p_rhi, VkBufferUsageFlags p_usageFlags, VkMemoryPropertyFlags p_memPropFlags, size_t p_size, std::string p_debugName, int32_t p_id)
{
	CVulkanRHI::Buffer buffer;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(p_size, buffer, p_usageFlags, p_memPropFlags, p_debugName));

	if (p_id == -1)
		m_buffers.push_back(buffer);
	else
		m_buffers[p_id] = buffer;

	return true;
}

bool CBuffers::CreateBuffer(CVulkanRHI* p_rhi, void* p_data, size_t p_size, std::string p_debugName, int32_t p_id)
{
	CVulkanRHI::Buffer buffer;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(p_size, buffer, m_usageFlags, m_memPropFlags, p_debugName));

	RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)p_data, buffer));

	if (p_id == -1)
		m_buffers.push_back(buffer);
	else
		m_buffers[p_id] = buffer;

	return true;
}

bool CBuffers::CreateBuffer(CVulkanRHI* p_rhi, size_t p_size, std::string p_debugName, int32_t p_id)
{
	CVulkanRHI::Buffer buffer;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(p_size, buffer, m_usageFlags, m_memPropFlags, p_debugName));

	if (p_id == -1)
		m_buffers.push_back(buffer);
	else
		m_buffers[p_id] = buffer;

	return true;
}

bool CBuffers::CreateBuffer(CVulkanRHI* p_rhi, CVulkanRHI::Buffer& p_stg, void* p_data, size_t p_size, CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugName, int32_t p_id)
{
	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(p_size, p_stg, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_debugName + "_transfer"));

	RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)p_data, p_stg));

	CVulkanRHI::Buffer buffer;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(p_size, buffer, 
		m_usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, m_memPropFlags, p_debugName));

	RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(p_stg, buffer, p_cmdBfr));

	// An index (p_id) is passed in situations where we want to track the
	// specific buffer later in the frame This is useful in cases when you
	// want to access this buffer during the application's lifetime to
	// specially draw it with some conditions. Eg: The debug frustum index
	// buffer needs to be destroyed and recreated multiple time during the
	// application's lifetime.
	if (p_id == -1)
	{
		m_buffers.push_back(buffer);
	}
	else
	{
		m_buffers[p_id] = buffer;
	}

	return true;
}

const CVulkanRHI::Buffer& CBuffers::GetBuffer(uint32_t p_id)
{
	if (p_id >= m_buffers.size())
	{
		std::cerr << "CBuffers::GetBuffer: Index is greater than the size of buffer list" << std::endl;
		assert(p_id >= m_buffers.size());
	}

	return m_buffers[p_id]; 
}

const CVulkanRHI::Buffer CBuffers::GetBuffer(uint32_t p_id) const
{
	if (p_id >= m_buffers.size())
	{
		std::cerr << "CBuffers::GetBuffer: Index is greater than the size of buffer list" << std::endl;
		return CVulkanRHI::Buffer();
	}

	return m_buffers[p_id];
}

void CBuffers::DestroyBuffer(CVulkanRHI* p_rhi, uint32_t p_idx)
{
	if (p_idx >= m_buffers.size())
	{
		std::cerr << "CBuffers::DestroyBuffer: Index is greater than the size of buffer list" << std::endl;
	}

	p_rhi->FreeMemoryDestroyBuffer(m_buffers[p_idx]);
	//m_buffers.erase(m_buffers.begin() + p_idx);
}

void CBuffers::DestroyBuffers(CVulkanRHI* p_rhi)
{
	for (auto& buffer : m_buffers)
	{
		p_rhi->FreeMemoryDestroyBuffer(buffer);
	}
	m_buffers.clear();
}

CTextures::CTextures(int p_maxSize)
{
	if(p_maxSize > 0)
		m_textures.resize(p_maxSize);
}

bool CTextures::CreateRenderTarget(CVulkanRHI* p_rhi, uint32_t p_id, VkFormat p_format, uint32_t p_width, uint32_t p_height, 
	uint32_t p_mipLevel, VkImageLayout p_layout, std::string p_debugName, VkImageUsageFlags p_usage)
{
	CVulkanRHI::Image renderTarget;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateRenderTarget(p_format, p_width, p_height, p_mipLevel, p_layout, p_usage, renderTarget, p_debugName));

	m_textures[p_id] = renderTarget;

	return true;
}

bool CTextures::CreateTexture(CVulkanRHI* p_rhi, CVulkanRHI::Buffer& p_stg, const ImageRaw* p_rawImg, VkFormat p_format, 
							  CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugName, int p_id)
{
	std::clog << "Creating GPU Texture Buffer for " << p_debugName << std::endl;

	if (p_rawImg->raw != nullptr)
	{
		CVulkanRHI::Image img;

		size_t texSize = p_rawImg->width * p_rawImg->height * p_rawImg->channels;

		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(texSize, p_stg, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_debugName + "_transfer"));

		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)p_rawImg->raw, p_stg));

		img.format = p_format;//VK_FORMAT_R8G8B8A8_UNORM;
		img.width = p_rawImg->width;
		img.height = p_rawImg->height;
		img.viewType = VK_IMAGE_VIEW_TYPE_2D;
		img.SetLevelCount(p_rawImg->mipLevels);

		VkImageCreateInfo imgCrtInfo = CVulkanCore::ImageCreateInfo();
		imgCrtInfo.extent.width = p_rawImg->width;
		imgCrtInfo.extent.height = p_rawImg->height;
		imgCrtInfo.format = img.format;
		imgCrtInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imgCrtInfo.mipLevels = p_rawImg->mipLevels;

		// If the mip count is more than 1, we will use the texture
		// to read from and write to when generating the mip chain
		// Hence the usage needs to be both Source and Destination
		if (imgCrtInfo.mipLevels > 1)
			imgCrtInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		RETURN_FALSE_IF_FALSE(p_rhi->CreateTexture(p_stg, img, imgCrtInfo, p_cmdBfr, p_debugName))

		// Doing this because; if the id is set to -1, then the intent is to grow the image list at runtime and not a fixed size
		if (p_id == -1)
		{
			m_textures.push_back(img);
		}
		else
		{
			m_textures[p_id] = img;
		}

		return true;
	}

	return false;
}

bool CTextures::CreateCubemap(CVulkanRHI* p_rhi, CVulkanRHI::Buffer& p_stg, ImageRaw& cubeMapRaw, const CVulkanRHI::SamplerList& p_samplers, CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugName, int p_id)
{
	VkFormat cubeMapFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

	size_t cubeMapSize = CVulkanRHI::Image::GetTextureSizePerLayer(cubeMapRaw.width, cubeMapRaw.height, cubeMapFormat,
		cubeMapRaw.mipLevels, nullptr);

	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(cubeMapSize * cubeMapRaw.depthOrArraySize, p_stg,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		p_debugName + "_transfer"));

	if (cubeMapRaw.raw)
	{
		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)cubeMapRaw.raw, p_stg));
	}
	else
	{
		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)cubeMapRaw.raw_hdr, p_stg));
	}

	CVulkanRHI::Image cubemap;
	cubemap.descInfo.imageLayout					= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	cubemap.descInfo.sampler						= p_samplers[SamplerId::s_Linear].descInfo.sampler;
	cubemap.format									= cubeMapFormat;
	cubemap.width									= cubeMapRaw.width;
	cubemap.height									= cubeMapRaw.height;
	cubemap.layerCount								= 6;
	cubemap.viewType								= VK_IMAGE_VIEW_TYPE_CUBE;
	cubemap.SetLevelCount(cubeMapRaw.mipLevels);

	VkImageCreateInfo imgInfo						= CVulkanCore::ImageCreateInfo();
	imgInfo.extent.width							= cubeMapRaw.width;		// assuming all the loaded cube map images have same width
	imgInfo.extent.height							= cubeMapRaw.height;	// assuming all the loaded cube map images have same height
	imgInfo.arrayLayers								= 6;
	imgInfo.flags									= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imgInfo.usage									= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imgInfo.format									= cubeMapFormat;
	imgInfo.mipLevels								= cubeMapRaw.mipLevels;

	RETURN_FALSE_IF_FALSE(p_rhi->CreateTexture(p_stg, cubemap, imgInfo, p_cmdBfr, p_debugName, false /* upload, don't create mips, provided in staging */));
	
	// Doing this because; if the id is set to -1, then the intent 
	// is to grow the texture list at runtime and not a fixed size
	if (p_id == -1)
	{
		m_textures.push_back(cubemap);
	}
	else
	{
		m_textures[p_id] = cubemap;
	}

	return true;
}

void CTextures::IssueLayoutBarrier(CVulkanRHI* p_rhi, CVulkanRHI::ImageLayout p_imageLayout, CVulkanRHI::CommandBuffer& p_cmdBfr, uint32_t p_id, int p_mipLevel)
{
	p_rhi->IssueLayoutBarrier(p_imageLayout, m_textures[p_id], p_cmdBfr, p_mipLevel);
}

void CTextures::PushBackPreLoadedTexture(uint32_t p_texIndex)
{
	if (p_texIndex < 0 || p_texIndex > m_textures.size() - 1)
		return;

	m_textures.push_back(m_textures[p_texIndex]);
}

void CTextures::DestroyTextures(CVulkanRHI* p_rhi)
{
	for (auto& tex : m_textures)
	{
		p_rhi->DestroyImage(tex.image);
		p_rhi->DestroyImageView(tex.descInfo.imageView);
		p_rhi->FreeDeviceMemory(tex.devMem);
	}
	m_textures.clear();
}

uint32_t CRenderableUI::fontID;
CRenderableUI::CRenderableUI()
	: CRenderable(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, FRAME_BUFFER_COUNT)
	, CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_new, "VFrame Interface")
	, CSelectionListener()
	, m_showImguiDemo(false)
	, m_latestFPS(100)
{
	m_participantManager = new CUIParticipantManager();
}

CRenderableUI::~CRenderableUI()
{
	delete m_participantManager;
}

void CRenderableUI::Show(CVulkanRHI* p_rhi)
{
	ImGui::TextUnformatted("VFrame Analysis");
	{
		int rtype = (int)p_rhi->GetRendererType();
		ImGui::RadioButton("Forward ", &rtype, 0); ImGui::SameLine(); ImGui::RadioButton("Deferred ", &rtype, 1);
		p_rhi->SetRenderType((CVulkanRHI::RendererType)rtype);
		
	}
	ImGui::Separator();

	ImGui::TableNextColumn(); ImGui::Checkbox("Show imGui Demo", &m_showImguiDemo);
	if (m_showImguiDemo)
	{
		bool showDemo = true;
		ImGui::ShowDemoWindow(&showDemo);
	}
	ImGui::Separator();
		
	ImGui::TextUnformatted("VFrame Performance");
	{
		std::vector<float> data;
		data.resize(m_latestFPS.Size());
		m_latestFPS.Data(data.data());
		//ImGui::PlotLines("Frame Time", data, (float)m_latestFPS.Size());
		// some math magic, will look at this later
		{
			// Fill an array of contiguous float values to plot
			// Tip: If your float aren't contiguous but part of a structure, you can pass a pointer to your first float
			// and the sizeof() of your structure in the "stride" parameter.
			static float values[90] = {};
			static int values_offset = 0;
			static double refresh_time = 0.0;
			if (refresh_time == 0.0)
				refresh_time = ImGui::GetTime();
			while (refresh_time < ImGui::GetTime()) // Create data at fixed 60 Hz rate for the demo
			{
				static float phase = 0.0f;
				values[values_offset] = cosf(phase);
				values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
				phase += 0.10f * values_offset;
				refresh_time += 1.0f / 60.0f;
			}
			char overlay[32];
			sprintf_s(overlay, "avg %f", m_latestFPS.Average());
			ImGui::PlotLines("cpu (ms)", data.data(), (int)data.size(), values_offset, overlay, 0.0f, 32.0f, ImVec2(0, 60.0f));
		}
	}
	ImGui::Separator();
}

bool CRenderableUI::Create(CVulkanRHI* p_rhi, const CVulkanRHI::CommandPool& p_cmdPool)
{
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	CVulkanRHI::CommandBuffer cmdBfr;
	CVulkanRHI::BufferList stgList;
	
	std::string debugMarker = "UI Loading";
	RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffers(p_cmdPool, &cmdBfr, 1, &debugMarker));

	RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, debugMarker.c_str()));

	if (!LoadFonts(p_rhi, stgList, cmdBfr))
	{
		std::cerr << "CRenderableUI::Create Error: Failed to Load UI Fonts" << std::endl;
		return false;
	}

	RETURN_FALSE_IF_FALSE(p_rhi->EndCommandBuffer(cmdBfr));

	CVulkanRHI::CommandBufferList cbrList{ cmdBfr };
	CVulkanRHI::PipelineStageFlagsList psfList{ VkPipelineStageFlags {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT} };
	bool waitForFinish = true;
	RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandBuffers(&cbrList, &psfList, waitForFinish));

	for (auto& stg : stgList)
		p_rhi->FreeMemoryDestroyBuffer(stg);

	if (!CreateUIDescriptors(p_rhi))
	{
		std::cerr << "CRenderableUI::Create Error: Failed to create UI Descriptors" << std::endl;
		return false;
	}

	// binding is 1 followed by font buffer index;
	CRenderableUI::fontID = (1 << 16) | 0;
	ImGui::GetIO().Fonts->SetTexID(&CRenderableUI::fontID);

	return true;
}

void CRenderableUI::DestroyRenderable(CVulkanRHI* p_rhi)
{
	DestroyTextures(p_rhi);
}

bool CRenderableUI::Update(CVulkanRHI* p_rhi, const LoadedUpdateData& p_loadedUpdate)
{
	m_latestFPS.Add(p_loadedUpdate.timeElapsed);

	ImGuiIO& imguiIO						= ImGui::GetIO();
	imguiIO.DisplaySize						= ImVec2(p_loadedUpdate.screenRes[0], p_loadedUpdate.screenRes[1]);
	imguiIO.DeltaTime						= p_loadedUpdate.timeElapsed;
	imguiIO.MousePos						= ImVec2(p_loadedUpdate.curMousePos[0], p_loadedUpdate.curMousePos[1]);
	imguiIO.MouseDown[0]					= p_loadedUpdate.isLeftMouseDown;
	imguiIO.MouseDown[1]					= p_loadedUpdate.isRightMouseDown;

	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	ShowUI(p_rhi);
	ShowGuizmo(p_rhi, p_loadedUpdate.camView, p_loadedUpdate.camProjection);

	return true;
}

bool CRenderableUI::LoadFonts(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	std::clog << "Loading UI Resources" << std::endl;

	ImGuiIO& imguiIO						= ImGui::GetIO();
	ImageRaw tex;
	imguiIO.Fonts->GetTexDataAsRGBA32(&tex.raw, &tex.width, &tex.height, &tex.channels);
	size_t texSize							= tex.width * tex.height * tex.channels * sizeof(char); 
	
	if (tex.raw != nullptr)
	{
		CVulkanRHI::Buffer stg;
		RETURN_FALSE_IF_FALSE(CreateTexture(p_rhi, stg, &tex, VK_FORMAT_R8G8B8A8_UNORM, p_cmdBfr, "UI_font"));
		p_stgList.push_back(stg);
	}

	return true;
}

bool CRenderableUI::CreateUIDescriptors(CVulkanRHI* p_rhi)
{
	std::vector<VkDescriptorImageInfo> imageInfoList;
	for (const auto& imgInfo : m_textures)
		imageInfoList.push_back(imgInfo.descInfo);

	uint32_t texturesCount = (uint32_t)m_textures.size();

	AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_UI_TexArray, texturesCount, 
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT,	VK_NULL_HANDLE,	imageInfoList.data()});

	RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi));

	return true;
}

bool CRenderableUI::ShowGuizmo(CVulkanRHI* p_rhi, nm::float4x4 p_camView, nm::float4x4 p_camProjection)
{
	// ImGuizmo Type Selection
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
		{
			const float PAD					= 10.0f;
			const ImGuiViewport* viewport	= ImGui::GetMainViewport();
			ImVec2 work_pos					= viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
			ImVec2 work_size				= viewport->WorkSize;
			ImVec2 window_pos, window_pos_pivot;
			window_pos.x					= (work_pos.x + work_size.x - PAD);
			window_pos.y					= (work_pos.y + PAD);
			window_pos_pivot.x				= 1.0f;
			window_pos_pivot.y				= 0.0f;
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			window_flags |= ImGuiWindowFlags_NoMove;
		}
		ImGui::SetNextWindowBgAlpha(0.15f); // Transparent background
		if (ImGui::Begin("Gizmo Type", nullptr, window_flags))
		{
			int guizmoType = (int)m_guizmo.type;
			ImGui::RadioButton("T", &guizmoType, 0); ImGui::SameLine();
			ImGui::RadioButton("R", &guizmoType, 1); ImGui::SameLine();
			ImGui::RadioButton("S", &guizmoType, 2);
			m_guizmo.type = (Guizmo::Type)guizmoType;
		}
		ImGui::End();
	}

	if(m_selectedEntity)
	{
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetRect(0.0f, 0.0f, (float)p_rhi->GetScreenWidth(), (float)p_rhi->GetScreenHeight());

		nm::Transform transform		= m_selectedEntity->GetTransform();

		if (m_guizmo.type == Guizmo::Translation)
		{
			nm::float4x4 translation = transform.GetTranslate();
			if (ImGuizmo::Manipulate(&p_camView.column[0][0], &p_camProjection.column[0][0], ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::LOCAL, &translation.column[0][0]))
			{
				if (!(transform.GetTranslate() == translation))
				{
					transform.SetTranslate(translation);
					m_selectedEntity->SetTransform(transform);
				}
			}
		}
		else if (m_guizmo.type == Guizmo::Rotation)
		{
			nm::float4x4 guizmoTransform = transform.GetTranslate() * transform.GetRotate();
			if (ImGuizmo::Manipulate(&p_camView.column[0][0], &p_camProjection.column[0][0], ImGuizmo::OPERATION::ROTATE, ImGuizmo::LOCAL, &guizmoTransform.column[0][0]))
			{
				nm::float4x4 rotate = nm::float4x4::identity();
				rotate.column[0][0] = guizmoTransform.column[0][0];
				rotate.column[0][1] = guizmoTransform.column[0][1];
				rotate.column[0][2] = guizmoTransform.column[0][2];

				rotate.column[1][0] = guizmoTransform.column[1][0];
				rotate.column[1][1] = guizmoTransform.column[1][1];
				rotate.column[1][2] = guizmoTransform.column[1][2];

				rotate.column[2][0] = guizmoTransform.column[2][0];
				rotate.column[2][1] = guizmoTransform.column[2][1];
				rotate.column[2][2] = guizmoTransform.column[2][2];
			
				transform.SetRotation(rotate);
				m_selectedEntity->SetTransform(transform);
			}
		}
		else if (m_guizmo.type == Guizmo::Scale)
		{
			nm::float4x4 guizmoTransform = transform.GetTranslate() * transform.GetScale();
			if (ImGuizmo::Manipulate(&p_camView.column[0][0], &p_camProjection.column[0][0], ImGuizmo::OPERATION::SCALE, ImGuizmo::LOCAL, &guizmoTransform.column[0][0]))
			{
				nm::float4x4 scaling = nm::float4x4::identity();
				scaling.column[0][0] = guizmoTransform.column[0][0];
				scaling.column[1][1] = guizmoTransform.column[1][1];
				scaling.column[2][2] = guizmoTransform.column[2][2];

				transform.SetScale(scaling);
				m_selectedEntity->SetTransform(transform);
			}
		}
	}

	return true;
}

bool CRenderableUI::ShowUI(CVulkanRHI* p_rhi)
{
	m_participantManager->Show(p_rhi);
	return true;
}

bool CRenderableUI::PreDraw(CVulkanRHI* p_rhi, uint32_t p_scIdx)
{
	ImGui::Render();
	ImDrawData* drawData					= ImGui::GetDrawData();
	int fbWidth								= (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	int fbHeight							= (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	const bool isMinimized					= (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);

	if (isMinimized || fbWidth <= 0 || fbHeight <= 0)
	{
		return false;
	}

	{
		if (drawData->TotalVtxCount > 0)
		{
			// Create or resize the vertex / index buffers
			CRenderable::InData data{};

			data.vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
			data.indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

			// if the vertex buffer if already created from previous frame, destroy them and free the associated memory for this frame's use
			if (m_vertexBuffers.GetBuffer(p_scIdx).descInfo.buffer != VK_NULL_HANDLE)
			{
				Clear(p_rhi, p_scIdx);
			}

			// Creating vertex and index buffers and upload all data to single continuous GPU buffers respectively
			{
				ImDrawVert* vertexMemIdx = nullptr;
				ImDrawIdx* indexMemIdx = nullptr;
				for (int n = 0; n < drawData->CmdListsCount; n++)
				{
					const ImDrawList* cmdList = drawData->CmdLists[n];
					memcpy(vertexMemIdx, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
					memcpy(indexMemIdx, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

					vertexMemIdx += cmdList->VtxBuffer.Size;
					indexMemIdx += cmdList->IdxBuffer.Size;
				}

				data.vertexBufferData = vertexMemIdx;
				data.indexBufferData = indexMemIdx;
				RETURN_FALSE_IF_FALSE(CreateVertexIndexBuffer(p_rhi, data, "imgui"));

				/*p_rhi->CreateAllocateBindBuffer(verteSize, m_vertexBuffers[p_scIdx], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "imgui_vert");
				p_rhi->CreateAllocateBindBuffer(indexSize, m_indexBuffers[p_scIdx], VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "imgui_index");
				
				ImDrawVert* vertexMemIdx	= nullptr;
				ImDrawIdx* indexMemIdx		= nullptr;
				bool flushMemRanges			= true;
				std::vector<VkMappedMemoryRange> memRanges;

				if (!p_rhi->MapMemory(m_vertexBuffers[p_scIdx], flushMemRanges, (void**)&vertexMemIdx, &memRanges))
					return false;

				if (!p_rhi->MapMemory(m_indexBuffers[p_scIdx], flushMemRanges, (void**)&indexMemIdx, &memRanges))
					return false;

				for (int n = 0; n < drawData->CmdListsCount; n++)
				{
					const ImDrawList* cmdList = drawData->CmdLists[n];
					memcpy(vertexMemIdx, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
					memcpy(indexMemIdx, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

					vertexMemIdx			+= cmdList->VtxBuffer.Size;
					indexMemIdx				+= cmdList->IdxBuffer.Size;
				}

				if (!p_rhi->FlushMemoryRanges(&memRanges))
					return false;

				p_rhi->UnMapMemory(m_vertexBuffers[p_scIdx]);
				p_rhi->UnMapMemory(m_indexBuffers[p_scIdx]);*/
			}
		}
	}

	return true;
}

CRenderableMesh::CRenderableMesh(std::string p_name, uint32_t p_meshId, nm::Transform p_modelMat, VkAccelerationStructureInstanceKHR* p_accStructInstance)
	: CRayTracingRenderable(p_accStructInstance)
	, CEntity(p_name)
	, m_mesh_id(p_meshId)
	, m_selectedSubMeshId(-1)
{
	CEntity::m_transform = p_modelMat;
	p_accStructInstance->transform = p_modelMat.GetTransformAffine();
}

CRenderableMesh::~CRenderableMesh()
{
	if (m_boundingVolume)
		delete m_boundingVolume;
}

void CRenderableMesh::Show(CVulkanRHI* p_rhi)
{
	CEntity::Show(p_rhi);

	ImGui::Indent();

	bool drawSubBBox = IsSubmeshDebugDrawEnabled();
	ImGui::Checkbox(" ", &drawSubBBox);
	SetSubmeshDebugDrawEnable(drawSubBBox);
	ImGui::SameLine(75);

	std::string treeNodeName = "Sub-meshes (" + std::to_string(GetSubmeshCount()) + ")";
	if (ImGui::TreeNode(treeNodeName.c_str()))
	{
		for (uint32_t i = 0; i < GetSubmeshCount(); i++)
		{
			if (ImGui::Selectable(GetSubmesh(i)->name.c_str(), m_selectedSubMeshId == i, ImGuiSelectableFlags_AllowDoubleClick))
			{
				m_selectedSubMeshId = i;
			}

			//if (m_selectedSubMeshIndex == i)
			//{
			//	ImGui::Indent();

			//	int matId = GetSubmesh(i)->materialId;
			//	Material mat = m_materialsList[matId];
			//	ImGui::Text("Albedo Id %d", mat.color_id);
			//	ImGui::Text("Normal Id %d", mat.normal_id);
			//	ImGui::Text("Metal/Rough Id %d", mat.roughMetal_id);
			//	ImGui::InputFloat("Metallic ", &mat.metallic);
			//	ImGui::InputFloat("Roughness ", &mat.roughness);
			//	ImGui::InputFloat3("Color ", &mat.color[0]);
			//	ImGui::InputFloat3("PBR Color ", &mat.pbr_color[0]);

			//	ImGui::Unindent();
			//}
		}
		ImGui::TreePop();
	}

	ImGui::Unindent();
}

void CRenderableMesh::SetTransform(nm::Transform p_transform, bool p_bRecomputeSceneBBox)
{
	m_dirty						= true;
	m_transform					= p_transform;

	if(p_bRecomputeSceneBBox)
		CSceneGraph::RequestSceneBBoxUpdate();
}

CScene::CScene(CSceneGraph* p_sceneGraph)
	: CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_new, "Scene")
	, CDescriptor(CVulkanRHI::DescriptorBindFlag::Variable_Count | CVulkanRHI::DescriptorBindFlag::Bindless, FRAME_BUFFER_COUNT)
	, m_sceneGraph(p_sceneGraph)
{
	m_sceneTextures = new CTextures();
	m_sceneLights = new CLights();
	m_materialsList.reserve(MAX_SUPPORTED_MATERIALS);
	m_accStructInstances.resize(MAX_SUPPORTED_MESHES, VkAccelerationStructureInstanceKHR{});
}

CScene::~CScene()
{
	delete m_sceneLights;
	delete m_sceneTextures;
}

bool CScene::Create(CVulkanRHI* p_rhi, const CVulkanRHI::SamplerList* p_samplerList, const CVulkanRHI::CommandPool& p_cmdPool)
{
	m_cmdPool = p_cmdPool;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandPool(p_rhi->GetQueueFamiliyIndex(), m_assetLoaderCommandPool));

	CVulkanRHI::CommandBuffer cmdBfr;
	CVulkanRHI::BufferList stgList;

	std::string debugMarker = "Default Resources/Scene Loading";
	{
		RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffer(p_cmdPool, &cmdBfr, debugMarker));
		if (!LoadDefaultTextures(p_rhi, p_samplerList, stgList, cmdBfr))
		{
			std::cerr << "CScene::Create Error: Failed to Load Default Textures" << std::endl;
			return false;
		}

		if (!LoadDefaultScene(p_rhi, stgList, cmdBfr))
		{
			std::cerr << "CScene::Create Error: Failed to Load Scene" << std::endl;
			return false;
		}

		if (!LoadLights(p_rhi, stgList, cmdBfr))
		{
			std::cerr << "CScene::Create Error: Failed to Load Lights" << std::endl;
			return false;
		}

		RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandBuffer(cmdBfr, true/*wait for finish*/));
	}
	debugMarker = "TLAS Creation";
	{
		RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffer(p_cmdPool, &cmdBfr, debugMarker));
		RETURN_FALSE_IF_FALSE(LoadTLAS(p_rhi, stgList, cmdBfr));
		RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandBuffer(cmdBfr, true/*wait for finish*/));
	}

	DestroyStaging(p_rhi, stgList);

	RETURN_FALSE_IF_FALSE(CreateMeshUniformBuffer(p_rhi));

	RETURN_FALSE_IF_FALSE(CreateSceneDescriptors(p_rhi));

	return true;
}

void CScene::Destroy(CVulkanRHI* p_rhi)
{
	m_skyBox->DestroyRenderable(p_rhi);
	delete m_skyBox;

	DestroyDescriptors(p_rhi);
	m_sceneTextures->DestroyTextures(p_rhi);

	p_rhi->FreeMemoryDestroyBuffer(m_instanceBuffer);
	p_rhi->FreeMemoryDestroyBuffer(m_TLASscratchBuffer);
	m_tlasBuffers->DestroyBuffers(p_rhi);
	p_rhi->DestroyAccelerationStrucutre(m_TLAS);

	for (auto& mesh : m_meshes)
	{
		mesh->DestroyRenderable(p_rhi);
		delete mesh;
	}
	m_meshes.clear();

	p_rhi->FreeMemoryDestroyBuffer(m_material_storage);

	for(int i =0; i< FRAME_BUFFER_COUNT; i++)
		p_rhi->FreeMemoryDestroyBuffer(m_meshInfo_uniform[i]);

	p_rhi->DestroyCommandPool(m_assetLoaderCommandPool);
}

bool CScene::UpdateTLAS(CVulkanRHI* p_rhi, const CVulkanRHI::CommandPool& p_cmdPool, uint32_t p_scId)
{
	std::string debugMarker = "TLAS Building";
	{
		CVulkanRHI::CommandBuffer cmdBfr;
		RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffer(p_cmdPool, &cmdBfr, debugMarker));
		RETURN_FALSE_IF_FALSE(UpdateTLAS(p_rhi, cmdBfr));
		RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandBuffer(cmdBfr, true/*wait for finish*/));
	}
	return true;
}

void CScene::Show(CVulkanRHI* p_rhi)
{
	bool doubleClickedEntity = false;
	int32_t sceneIndex = 0;
	if (Header("Entity Settings"))
	{
		if (ImGui::Button("Load Asset"))
		{
			m_fileDialog.Open();
		}

		m_fileDialog.Display();

		if (m_fileDialog.HasSelected())
		{	
			if (!m_fileDialog.GetSelected().string().empty())
			{
				ImGui::OpenPopup("Create Entity");
			}
		}

		AddEntity(p_rhi, m_fileDialog.GetSelected().string());

		bool drawSGBBox = m_sceneGraph->IsDebugDrawEnabled();
		ImGui::Checkbox(" ", &drawSGBBox);
		m_sceneGraph->SetDebugDrawEnable(drawSGBBox);
		ImGui::SameLine(30);
		if (ImGui::TreeNode("Scene Graph"))
		{
			CSceneGraph::EntityList* entities = CSceneGraph::GetEntities();
			for (int i = 0; i < entities->size(); i++)
			{
				CEntity* entity = (*entities)[i];
				CLight* light = dynamic_cast<CLight*>(entity);
				CRenderableMesh* mesh = dynamic_cast<CRenderableMesh*>(entity);
				{
					if (mesh || light)
					{
						bool drawBBox = entity->IsDebugDrawEnabled();
						std::string str = std::to_string(i) + ". ";
						ImGui::Checkbox(str.c_str(), &drawBBox);
						entity->SetDebugDrawEnable(drawBBox);
						ImGui::SameLine(65);
					}
				}

				if (ImGui::Selectable(entity->GetName(), m_selectedEntity == entity, ImGuiSelectableFlags_AllowDoubleClick))
				{
					if (ImGui::IsMouseDoubleClicked(0))
					{
						doubleClickedEntity = true;
					}
					m_sceneGraph->SetCurSelectEntityId(i);
				}

				if (m_selectedEntity == entity)
				{
					entity->Show(p_rhi);
					if (mesh)
					{
						m_sceneGraph->SetCurSelectedSubMeshId(mesh->GetSelectedSubMeshId());
					}
				}
			}
			ImGui::TreePop();

			//// Assuming primary camera is object 0
			//if (doubleClickedEntity)
			//{
			//	nm::Transform primaryCameraTransform = (*entities)[0]->GetTransform();
			//	primaryCameraTransform.SetTranslate(m_selectedEntity->GetTransform().GetTranslate());
			//	(*entities)[0]->SetTransform(primaryCameraTransform);
			//}
		}
	}
}

bool CScene::Update(CVulkanRHI* p_rhi, const LoadedUpdateData& p_loadedUpdate)
{
	m_sceneLights->Update(p_loadedUpdate.cameraData, m_sceneGraph);
	if (m_sceneLights->IsDirty())
	{
		CVulkanRHI::CommandBuffer cmdBfr;
		CVulkanRHI::BufferList stgList;

		std::string debugMarker = "Lights Loading";
		RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffers(p_loadedUpdate.commandPool, &cmdBfr, 1, &debugMarker));
		RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, debugMarker.c_str()));

		// Every time any light is dirty(has change in transform, color or intensity), 
		// the raw data is updated and the entire GPU resource is reloaded
		if (!LoadLights(p_rhi, stgList, cmdBfr))
		{
			std::cerr << "CScene::Update Error: Failed to Update Lights" << std::endl;
			return false;
		}		

		if (!p_rhi->EndCommandBuffer(cmdBfr))
			return false;

		CVulkanRHI::CommandBufferList cbrList{ cmdBfr };
		CVulkanRHI::PipelineStageFlagsList psfList{ VkPipelineStageFlags {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT} };
		bool waitForFinish = true;
		RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandBuffers(&cbrList, &psfList, waitForFinish));

		for (auto& stg : stgList)
			p_rhi->FreeMemoryDestroyBuffer(stg);

		m_sceneLights->SetDirty(false);
	}

	std::vector<float> perMeshUniformData;
	perMeshUniformData.reserve(m_meshInfo_uniform[p_loadedUpdate.swapchainIndex].reqMemSize);
	for (auto& mesh : m_meshes)
	{
		mesh->SetDirty(false);
		mesh->m_viewNormalTransform					= (p_loadedUpdate.camView * mesh->GetTransform().GetTransform());	// nm::inverse(nm::transpose(p_loadedUpdate.viewMatrix * mesh->GetTransform().GetTransform()));

		const float* modelMat						= &(mesh->GetTransform().GetTransform()).column[0][0];

		const float* trn_inv_model					= &mesh->m_viewNormalTransform.column[0][0];							// this needs to be inverse transpose so as to negate the scaling in the matrix before multiplying with normal. But this isn't working and I do not know why !

		std::copy(&modelMat[0], &modelMat[16], std::back_inserter(perMeshUniformData));										// model matrix for this mesh
		std::copy(&trn_inv_model[0], &trn_inv_model[16], std::back_inserter(perMeshUniformData));							// Transpose(inverse(view * model)) for transforming normal to view space
	}

	uint8_t* data = (uint8_t*)(perMeshUniformData.data());
	RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(data, m_meshInfo_uniform[p_loadedUpdate.swapchainIndex], false));

	return true;
}

bool CScene::LoadDefaultTextures(CVulkanRHI* p_rhi, const CVulkanRHI::SamplerList* p_samplerList, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	// load default texture to compensate for bad textures
	{
		ImageRaw tex;
		
		std::filesystem::path resourcePath = g_DefaultPath / "Textures/Placeholder/tex_not_found.png";
		std::clog << "Loading Placeholder for tex_not_found: " << resourcePath.string().c_str() << std::endl;

		RETURN_FALSE_IF_FALSE(LoadRawImage(resourcePath.string().c_str(), tex));

		CVulkanRHI::Buffer stg;
		RETURN_FALSE_IF_FALSE(m_sceneTextures->CreateTexture(p_rhi, stg, &tex, VK_FORMAT_B8G8R8A8_SRGB,p_cmdBfr, "tex_not_found"));
		
		FreeRawImage(tex);
		p_stgList.push_back(stg);
	}

	// Load specular environment map
	{
		ImageRaw cubeMapRaw;

		std::filesystem::path cubemap_path = g_DefaultPath / "Textures/IBL/georgentor_Specular.dds";
		std::clog << "Loading Specular IBL Mips: " << cubemap_path.string().c_str() << std::endl;
		
		RETURN_FALSE_IF_FALSE(LoadRawImage(cubemap_path.string().c_str(), cubeMapRaw));

		CVulkanRHI::Buffer stg;
		RETURN_FALSE_IF_FALSE(m_sceneTextures->CreateCubemap(p_rhi, stg, cubeMapRaw, *p_samplerList, p_cmdBfr, "environment_specular"));

		p_stgList.push_back(stg);

		FreeRawImage(cubeMapRaw);
	}

	// Load diffuse environment map
	{
		ImageRaw cubeMapRaw;
		std::filesystem::path cubemap_path = g_DefaultPath / "Textures/IBL/georgentor_Diffuse.dds";
		std::clog << "Loading Diffuse Irradiance Mips: " << cubemap_path.string().c_str() << std::endl;

		RETURN_FALSE_IF_FALSE(LoadRawImage(cubemap_path.string().c_str(), cubeMapRaw));

		CVulkanRHI::Buffer stg;
		RETURN_FALSE_IF_FALSE(m_sceneTextures->CreateCubemap(p_rhi, stg, cubeMapRaw, *p_samplerList, p_cmdBfr, "environment_diffuse"));

		p_stgList.push_back(stg);

		FreeRawImage(cubeMapRaw);
	}

	// Load BRDFLut
	{
		ImageRaw tex;

		std::filesystem::path resourcePath = g_DefaultPath / "Textures/BRDF/BrdfLut.dds";
		std::clog << "Loading BRDF Lut: " << resourcePath.string().c_str() << std::endl;

		RETURN_FALSE_IF_FALSE(LoadRawImage(resourcePath.string().c_str(), tex));

		CVulkanRHI::Buffer stg;
		RETURN_FALSE_IF_FALSE(m_sceneTextures->CreateTexture(p_rhi, stg, &tex, VK_FORMAT_B8G8R8A8_SRGB, p_cmdBfr, "brdf_lut"));

		FreeRawImage(tex);
		p_stgList.push_back(stg);
	}

	// Load skybox geometry
	{
		SceneRaw sceneraw;
		ObjLoadData loadData{};
		loadData.flipUV = false;
		loadData.loadMeshOnly = true;

		RETURN_FALSE_IF_FALSE(LoadObj((g_DefaultPath / "3D/cube.obj").string().c_str(), sceneraw, loadData));

		MeshRaw meshraw = sceneraw.meshList[0];
		m_skyBox = new CRenderable(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
		RETURN_FALSE_IF_FALSE(m_skyBox->CreateVertexIndexBuffer(p_rhi, p_stgList, &meshraw, p_cmdBfr, "skybox"));
	}

	return true;
}

bool CScene::LoadDefaultScene(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr, bool p_dumpBinaryToDisk)
{
	std::vector<std::filesystem::path>		defaultScenePaths;
	//m_scenePaths.push_back(g_AssetPath/"glTFSampleModels / 2.0 / DragonAttenuation / glTF / DragonAttenuation.gltf");				//0
	//m_scenePaths.push_back(g_AssetPath/"shadow_test_3.gltf");																		//1
	//defaultScenePaths.push_back(g_AssetPath/"glTFSampleModels/2.0/TransmissionTest/glTF/TransmissionTest.gltf");					//2
	//m_scenePaths.push_back(g_AssetPath/"glTFSampleModels/2.0/NormalTangentMirrorTest/glTF/NormalTangentMirrorTest.gltf");			//3
	//defaultScenePaths.push_back(g_AssetPath / "glTFSampleModels/2.0/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf");
	defaultScenePaths.push_back(g_AssetPath / "Sponza/glTF/Sponza.gltf");
	//defaultScenePaths.push_back(g_AssetPath / "glTFSampleModels/2.0/Sponza/glTF/Sponza.gltf");
	defaultScenePaths.push_back(g_AssetPath/"glTFSampleModels/2.0/Suzanne/glTF/Suzanne.gltf");									//4
	defaultScenePaths.push_back(g_AssetPath/"glTFSampleModels/2.0/SciFiHelmet/glTF/SciFiHelmet.gltf");
	//defaultScenePaths.push_back(g_AssetPath/"glTFSampleModels/2.0/DamagedHelmet/glTF/DamagedHelmet_withTangents.gltf");				//6
	//defaultScenePaths.push_back("D:/Projects/MyPersonalProjects/assets/cube/cube.obj");											//7
	//defaultScenePaths.push_back("D:/Projects/MyPersonalProjects/assets/icosphere.gltf");											//8
	//m_scenePaths.push_back(g_AssetPath/"dragon/dragon.obj");															f			//9
	//m_scenePaths.push_back(g_AssetPath/"stanford_dragon_pbr/scene.gltf");															//10
	//m_scenePaths.push_back(g_AssetPath/"mitsuba/mitsuba.obj");																	//11
	//m_scenePaths.push_back(g_AssetPath/"wall_and_floor/wall_and_floor.gltf");														//12
	//m_scenePaths.push_back(g_AssetPath / "main_sponza/Main.1_Sponza/NewSponza_Main_glTF_002.gltf");								//13
	//m_scenePaths.push_back(g_AssetPath/"main_sponza/PKG_A_Curtains/NewSponza_Curtains_glTF.gltf");								//14
	//m_scenePaths.push_back(g_AssetPath/"an_afternoon_in_a_persian_garden/scene.obj");												//15
	//defaultScenePaths.push_back("D:/Projects/MyPersonalProjects/assets/glTFSampleModels/2.0/SpecGlossVsMetalRough/glTF/SpecGlossVsMetalRough.gltf");

	std::vector<bool> flipYList{ false, false, false };

	SceneRaw sceneraw;
	sceneraw.materialOffset = 0;
	sceneraw.textureOffset = 0;
	
	// Load the assets to system memory
	for (unsigned int i = 0; i < defaultScenePaths.size(); i++)
	{
		std::clog << "Loading Scene Resources - " << defaultScenePaths[i] << std::endl;

		ObjLoadData loadData{};
		loadData.flipUV = flipYList[i];
		loadData.loadMeshOnly = false;

		if (defaultScenePaths[i].extension() == ".gltf" || defaultScenePaths[i].extension() == ".glb")
		{
			RETURN_FALSE_IF_FALSE(LoadGltf(defaultScenePaths[i].string().c_str(), sceneraw, loadData));
		}
		else if (defaultScenePaths[i].extension() == ".obj")
		{
			RETURN_FALSE_IF_FALSE(LoadObj(defaultScenePaths[i].string().c_str(), sceneraw, loadData));
		}
		else
		{
			std::cerr << "Invalid file extension - " << defaultScenePaths[i].extension() << std::endl;
			return false;
		}

		if (p_dumpBinaryToDisk)
		{
			std::string outName = "D:/" + defaultScenePaths[i].stem().string();
			WriteToDisk(std::filesystem::path(outName + "_vertex_float_p3_n3_uv2_t4.charp"), sizeof(Vertex) * sceneraw.meshList[i].vertexList.size(), (char*)sceneraw.meshList[i].vertexList.data());
			WriteToDisk(std::filesystem::path(outName + "_index_uint32.charp"), sizeof(uint32_t) * sceneraw.meshList[i].indicesList.size(), (char*)sceneraw.meshList[i].indicesList.data());
		}

		m_textureOffset = sceneraw.textureOffset;
		m_materialOffset = sceneraw.materialOffset;
	}
	
	// Load to staging and set loading of mesh to device memory
	for (auto& meshraw : sceneraw.meshList)
	{
		std::clog << "CScene::LoadDefaultScene: Loading Asset to GPU - " << meshraw.name << std::endl;
		CRenderableMesh* mesh = new CRenderableMesh(meshraw.name, (uint32_t)m_meshes.size(), meshraw.transform, &m_accStructInstances[m_meshes.size()]);
		mesh->m_submeshes = meshraw.submeshes;

		BVolume* bVol = new BBox(meshraw.bbox);
		mesh->SetBoundingVolume(bVol);
		
		for (auto& bbox : meshraw.submeshesBbox)
		{
			mesh->SetSubBoundingBox(bbox);
		}

		RETURN_FALSE_IF_FALSE(mesh->CreateVertexIndexBuffer(p_rhi, p_stgList, &meshraw, p_cmdBfr, meshraw.name));

		RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandBuffer(p_cmdBfr, true/*wait for finish*/));

		RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffer(m_cmdPool, &p_cmdBfr, ""));

		// TODO: Insert a memory barrier here

		RETURN_FALSE_IF_FALSE(mesh->CreateBuildBLAS(p_rhi, p_stgList, p_cmdBfr, meshraw.name + " BLAS"));

		RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandBuffer(p_cmdBfr, true/*wait for finish*/));

		RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffer(m_cmdPool, &p_cmdBfr, ""));

		m_meshes.push_back(mesh);
	}
	
	// Load to staging and set loading of textures to device memory
	for (const auto& tex : sceneraw.textureList)
	{
		CVulkanRHI::Image img;
		{
			if (tex.raw != nullptr)
			{
				CVulkanRHI::Buffer stg;
				RETURN_FALSE_IF_FALSE(m_sceneTextures->CreateTexture(p_rhi, stg, &tex, VK_FORMAT_R8G8B8A8_UNORM, p_cmdBfr, tex.name));
				p_stgList.push_back(stg);
			}
			else
			{
				m_sceneTextures->PushBackPreLoadedTexture(TextureType::tt_default);
			}
		}
	}

	// Load to staging and set loading of materials to device memory
	std::copy(sceneraw.materialsList.begin(), sceneraw.materialsList.end(), std::back_inserter(m_materialsList));
	if(!m_materialsList.empty())
	{
		CVulkanRHI::Buffer matStg;
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeof(Material) * MAX_SUPPORTED_MATERIALS, matStg,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "scene_materials_transfer"));

		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)m_materialsList.data(), matStg));

		p_stgList.push_back(matStg);

		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(matStg.descInfo.range, m_material_storage, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "scene_materials"));

		RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(matStg, m_material_storage, p_cmdBfr));
	}

	// Clear all loaded resources from system memory
	// can binary dump in future to optimized format for faster binary loading
	{		
		sceneraw.meshList.clear();

		for (auto& tex : sceneraw.textureList)
			FreeRawImage(tex);

		sceneraw.textureList.clear();
	}

	return true;
}

bool CScene::LoadLights(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer& p_cmdBfr, bool p_dumpBinaryToDisk)
{
	std::clog << "Loading Light resources" << std::endl;

	// storage buffer for Lights
	{
		std::vector<CLights::LightGPUData> rawLightList = m_sceneLights->GetLightsGPUData();
		uint32_t rawLightCount = (uint32_t)rawLightList.size();

		size_t bufferSize = sizeof(uint32_t); // number of lights in use
		bufferSize += sizeof(CLights::LightGPUData) * rawLightList.size(); // raw data of all the lights

		char* rawData = new char[bufferSize];
		memcpy(rawData, &rawLightCount, sizeof(uint32_t)); // copying light count into the buffer
		memcpy(rawData + sizeof(uint32_t), rawLightList.data(), rawLightList.size() * sizeof(CLights::LightGPUData)); // copying light raw data into buffer

		CVulkanRHI::Buffer lightStg;
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(bufferSize, lightStg, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "lights_transfer"));

		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(rawData, lightStg));

		p_stgbufferList.push_back(lightStg);

		if (m_light_storage.descInfo.buffer == VK_NULL_HANDLE) 
		{
			RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(lightStg.descInfo.range, m_light_storage, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "lights"));
		}

		RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(lightStg, m_light_storage, p_cmdBfr));

		delete[] rawData;
	}

	return true;
}

bool CScene::LoadTLAS(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	// Needs one TLAS, that will be updated every frame if we are moving
	// objects
	size_t meshCount = m_meshes.size();

	// TODO: As of now, I am only packing transform data
	// of the meshes I am rendering. Once instancing is 
	// implemented, it ll get valuable to implement TLASs
	// that reflect the feature
	{
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeof(VkAccelerationStructureInstanceKHR) * meshCount, m_instanceBuffer,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "TLAS Instance Resource"));
		

		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)m_accStructInstances.data(), m_instanceBuffer));
	}
		
	VkAccelerationStructureGeometryKHR geometry{};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry.geometry.instances.data.deviceAddress = p_rhi->GetBufferDeviceAddress(m_instanceBuffer.descInfo.buffer);
	geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;	// might want to use the update flag when add geometry at runtime?
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &geometry;

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
	p_rhi->GetAccelerationStructureBuildSize(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, (uint32_t)meshCount, &sizeInfo);

	std::clog << "LoadTLAS: Total Acceleration Structure Size: " << sizeInfo.accelerationStructureSize / 1048576.0f << " Mb." << std::endl;
	std::clog << "LoadTLAS: Total Scratch Size: " << sizeInfo.buildScratchSize / 1048576.0f << " Mb." << std::endl;

	// Create Scratch Buffer
	VkDeviceAddress scratchAddress;
	{
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeInfo.buildScratchSize, m_TLASscratchBuffer,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "TLAS Scratch"));
		
		scratchAddress = p_rhi->GetBufferDeviceAddress(m_TLASscratchBuffer.descInfo.buffer);
	}

	// Create Acceleration Buffer
	{
		m_tlasBuffers = new CBuffers(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		(m_tlasBuffers->CreateBuffer(p_rhi, sizeInfo.accelerationStructureSize, "Primary TLAS"));
	}

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = m_tlasBuffers->GetBuffer(0).descInfo.buffer;
	accelerationStructureCreateInfo.size = sizeInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateAccelerationStructure(&accelerationStructureCreateInfo, m_TLAS));
	std::clog << "LoadTLAS: Creating Top Acceleration Structure for the Scene" << std::endl;

	VkAccelerationStructureBuildRangeInfoKHR buildRanges{};
	buildRanges.primitiveCount = (uint32_t)meshCount;
	const VkAccelerationStructureBuildRangeInfoKHR* buildRangePtrs = &buildRanges;

	buildInfo.srcAccelerationStructure = m_TLAS;
	buildInfo.dstAccelerationStructure = m_TLAS;
	buildInfo.scratchData.deviceAddress = scratchAddress;

	p_rhi->BuildAccelerationStructure(p_cmdBfr, 1, &buildInfo, &buildRangePtrs);
	std::clog << "LoadTLAS: Building Acceleration Structure for the Scene " << std::endl;

	return true;
}

bool CScene::UpdateTLAS(CVulkanRHI* p_rhi, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)m_accStructInstances.data(), m_instanceBuffer));

	VkAccelerationStructureGeometryKHR geometry = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry.geometry.instances.data.deviceAddress = p_rhi->GetBufferDeviceAddress(m_instanceBuffer.descInfo.buffer);

	VkAccelerationStructureBuildGeometryInfoKHR buildInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &geometry;

	buildInfo.srcAccelerationStructure = m_TLAS;
	buildInfo.dstAccelerationStructure = m_TLAS;
	buildInfo.scratchData.deviceAddress = p_rhi->GetBufferDeviceAddress(m_TLASscratchBuffer.descInfo.buffer);

	VkAccelerationStructureBuildRangeInfoKHR buildRange = {};
	buildRange.primitiveCount = 1;
	const VkAccelerationStructureBuildRangeInfoKHR* buildRangePtr = &buildRange;

	p_rhi->BuildAccelerationStructure(p_cmdBfr, 1, &buildInfo, &buildRangePtr);	

	return true;
}

bool CScene::CreateMeshUniformBuffer(CVulkanRHI* p_rhi)
{
	size_t uniBufize = MAX_SUPPORTED_MESHES * (
		(sizeof(float) * 16)	// model matrix
	+	(sizeof(float) * 16)	// transpose(inverse(model)) for transforming normal to world space
		);

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(uniBufize, m_meshInfo_uniform[i], 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "mesh_uniform"));
	}

	return true;
}

bool CScene::CreateSceneDescriptors(CVulkanRHI* p_rhi)
{
	// We are allocating these descriptors from a bindless pool.
	// This does not need demand we create valid buffers/textures or write-update these descriptors 
	// at the time of their creation. All we need to ensure is that we dont access these "unbound resource"
	// descriptors at run-time. The strategy is to pre-allocate a large chunk of bindless-array of texture
	// descriptors. Load few textures and provide their IDs to the material_storage buffer and access the 
	// de-ref bindless array of textures to access the texture. When a new asset is added, its associated
	// textures are upload to device visible heap to new indices of the bindless array of textures. Their
	// Ids are updated to the material-storage buffer. And then write-update is called on this descriptor.

	std::vector<VkDescriptorImageInfo> imageInfoList;
	for (int i = TextureType::tt_scene; i < m_sceneTextures->GetTextures().size(); i++)
	{
		imageInfoList.push_back(m_sceneTextures->GetTextures()[i].descInfo);
	}

	// We are creating 2 descriptors because we have 2 frames in flights. And during 
	// that process we need to update 1 uniform buffer while the 
	// other is getting updated. Not doing this leads to undefined
	// behavior.

	// Creating descriptor set for swap chain utility 0
	VkShaderStageFlags vertex_frag		= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	VkShaderStageFlags frag				= VK_SHADER_STAGE_FRAGMENT_BIT;
	VkShaderStageFlags vert_frag_comp	= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	VkShaderStageFlags frag_comp		= VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	{
		// Creating Descriptors and descriptor set based on following type and count
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Scene_MeshInfo_Uniform,	1,						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,				vertex_frag},	0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Env_Specular,				1,						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		frag_comp },	0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Env_Diffuse,				1,						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		frag_comp },	0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Brdf_Lut,					1,						VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,				frag_comp },	0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Material_Storage,			1,						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,				frag },			0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Scene_Lights,				1,						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,				vert_frag_comp},0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Scene_TLAS,				1,						VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,	frag_comp},		0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_SceneRead_TexArray,		MAX_SUPPORTED_TEXTURES,	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,				frag },	        0);
		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, 0, "SceneDescriptorSet_0"));
	}

	// Creating descriptor set for swap chain utility 1
	{
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Scene_MeshInfo_Uniform,	1,						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,				vertex_frag},	1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Env_Specular,				1,						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		frag_comp },	1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Env_Diffuse,				1,						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		frag_comp },	1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Brdf_Lut,					1,						VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,				frag_comp },	1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Material_Storage,			1,						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,				frag },			1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Scene_Lights,				1,						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,				vert_frag_comp},1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Scene_TLAS,				1,						VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,	frag_comp },	1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_SceneRead_TexArray,		MAX_SUPPORTED_TEXTURES,	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,				frag },			1);
		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, 1, "SceneDescriptorSet_1"));
	}

	// Calling for Descriptor Write and Update since we are using Bindless for this descriptor
	for (uint32_t i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		BindlessWrite(i, BindingDest::bd_Scene_MeshInfo_Uniform, &m_meshInfo_uniform[0].descInfo, 1);
		BindlessWrite(i, BindingDest::bd_Env_Specular, &m_sceneTextures->GetTexture(TextureType::tt_env_specular).descInfo, 1);
		BindlessWrite(i, BindingDest::bd_Env_Diffuse, &m_sceneTextures->GetTexture(TextureType::tt_env_diffuse).descInfo, 1);
		BindlessWrite(i, BindingDest::bd_Brdf_Lut, &m_sceneTextures->GetTexture(TextureType::tt_brdfLut).descInfo, 1);
		BindlessWrite(i, BindingDest::bd_Material_Storage, &m_material_storage.descInfo, 1);
		BindlessWrite(i, BindingDest::bd_Scene_Lights, &m_light_storage.descInfo, 1);
		BindlessWrite(i, BindingDest::bd_Scene_TLAS, &m_TLAS, 1);
		BindlessWrite(i, BindingDest::bd_SceneRead_TexArray, imageInfoList.data(), (uint32_t)imageInfoList.size());
		BindlessUpdate(p_rhi, i);
	}

	return true;
}

bool showLoadButtons = true;
bool showLoadCompleteButtons = false;

bool CScene::AddEntity(CVulkanRHI* p_rhi, std::string p_path)
{
	std::string fileExtn;
	std::string assetName;
	if (ImGui::BeginPopupModal("Create Entity", NULL, ImGuiWindowFlags_MenuBar))
	{
		// Basic path formatting
		std::string strPath = std::string(p_path);
		std::size_t found = strPath.find_last_of("\\");
		if (found == std::string::npos)
		{
			std::cerr << "Invalid gltf path - " << p_path << std::endl;
			return false;
		}
		std::string folderPath = strPath.substr(0, found);

		if (!GetFileExtention(p_path, fileExtn))
		{
			std::cerr << "CScene::AddEntity Error: Failed to Get Extn - " << p_path << std::endl;
			return false;
		}
				
		if (!GetFileName(p_path, assetName, "\\"))
		{
			std::cerr << "CScene::AddEntity Error: Failed to Get Filename - " << p_path << std::endl;
			return false;
		}

		ImGui::Text("Asset Path: %s", p_path.c_str());
		ImGui::Text("Asset Type: %s", fileExtn.c_str());
		ImGui::Text("Asset Name: %s", assetName.c_str());
		
		// Show buttons to load and cancel until Load is clicked
		// once Load is clicked, show progress bar only
		if (showLoadButtons)
		{
			bool loadButton = ImGui::Button("Load");
			ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
			bool cancelButton = ImGui::Button("Cancel");

			if (loadButton)
			{
				m_assetLoadingTracker.state = AssetLoadingState::als_ReadyToLoad;
				showLoadButtons = false;
			}
			else if (cancelButton)
			{
				m_assetLoadingTracker.state = AssetLoadingState::als_WaitForRequest;
				ImGui::CloseCurrentPopup();
				m_fileDialog.ClearSelected();
			}
		}

		// Lets start loading the asset
		if (m_assetLoadingTracker.state >= AssetLoadingState::als_ReadyToLoad)
		{
			ImGui::ProgressBar(m_assetLoadingTracker.progress, ImVec2(0.0f, 0.0f));
			ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::Text("Loading..");

			if (m_assetLoadingTracker.state == AssetLoadingState::als_ReadyToLoad)
			{
				m_assetLoadingTracker.state = AssetLoadingState::als_Loading;

				// Spawning a new CPU thread to load the asset to RAM, prepare buffers and textures and 
				// upload to GPU. Wait for Command buffer execution to complete.
				std::thread assetLoaderThread([=]() {
					CVulkanRHI::CommandBuffer cmdBfr;
					CVulkanRHI::BufferList stgList;

					std::string debugMarker = "Entity Loading";
					RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffer(m_assetLoaderCommandPool, &cmdBfr, debugMarker));

					std::vector<VkDescriptorImageInfo> imageInfoList;
					uint32_t arrayDestIndex = (uint32_t)m_sceneTextures->GetTextures().size() - TextureType::tt_scene;
					{
						SceneRaw sceneraw;
						sceneraw.materialOffset = m_materialOffset;
						sceneraw.textureOffset = m_textureOffset;
												
						ObjLoadData loadData{};
						loadData.flipUV = false;
						loadData.loadMeshOnly = false;

						if (fileExtn == "gltf" || fileExtn == "glb")
						{
							RETURN_FALSE_IF_FALSE(LoadGltf(p_path.c_str(), sceneraw, loadData));
							m_assetLoadingTracker.progress = 0.3f;
						}
						else if (fileExtn == "obj")
						{
							RETURN_FALSE_IF_FALSE(LoadObj(p_path.c_str(), sceneraw, loadData));
							m_assetLoadingTracker.progress = 0.3f;
						}
						else
						{
							std::cerr << "Invalid file extension - " << fileExtn << std::endl;
							return false;
						}

						m_textureOffset += sceneraw.textureOffset;
						m_materialOffset += sceneraw.materialOffset;

						// Load vertex and index buffers
						for (auto& meshraw : sceneraw.meshList)
						{
							CRenderableMesh* mesh = new CRenderableMesh(meshraw.name, (uint32_t)m_meshes.size(), meshraw.transform, &m_accStructInstances[m_meshes.size()]);
							mesh->m_submeshes = meshraw.submeshes;

							std::clog << "Setting Bounding Volume" << std::endl;
							BVolume* bVol = new BBox(meshraw.bbox);
							mesh->SetBoundingVolume(bVol);

							for (auto& bbox : meshraw.submeshesBbox)
							{
								mesh->SetSubBoundingBox(bbox);
							}

							RETURN_FALSE_IF_FALSE(mesh->CreateVertexIndexBuffer(p_rhi, stgList, &meshraw, cmdBfr, meshraw.name));
							m_meshes.push_back(mesh);
						}

						m_assetLoadingTracker.progress = 0.4f;
						// Load textures
						for (const auto& tex : sceneraw.textureList)
						{
							CVulkanRHI::Image img;
							{
								if (tex.raw != nullptr)
								{
									CVulkanRHI::Buffer stg;
									RETURN_FALSE_IF_FALSE(m_sceneTextures->CreateTexture(p_rhi, stg, &tex, VK_FORMAT_R8G8B8A8_UNORM, cmdBfr, tex.name));
									stgList.push_back(stg);

									uint32_t texIndex = (uint32_t)m_sceneTextures->GetTextures().size() - 1;
									imageInfoList.push_back(m_sceneTextures->GetTextures()[texIndex].descInfo);
								}
								else
								{
									m_sceneTextures->PushBackPreLoadedTexture(TextureType::tt_default);
								}
							}
						}

						m_assetLoadingTracker.progress = 0.5;
						// Load Materials
						std::copy(sceneraw.materialsList.begin(), sceneraw.materialsList.end(), std::back_inserter(m_materialsList));
						if (!m_materialsList.empty())
						{
							if (m_materialsList.size() > MAX_SUPPORTED_MATERIALS)
							{
								std::cerr << "Max Supported Material Size has been exceeded. Loading failed." << std::endl;
								return false;
							}

							std::clog << "Creating GPU Material buffer" << std::endl;

							CVulkanRHI::Buffer matStg;
							RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeof(Material) * m_materialsList.size(), matStg,
								VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "scene_materials_transfer"));

							RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)m_materialsList.data(), matStg));

							stgList.push_back(matStg);

							if (m_material_storage.descInfo.buffer == VK_NULL_HANDLE)
							{
								std::cerr << "Material Storage Buffer is VK_NULL. Loading failed." << std::endl;
								return false;
							}

							RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(matStg, m_material_storage, cmdBfr));
						}

						std::clog << "Cleaning Raw Vertex and Index resource from RAM" << std::endl;
						sceneraw.meshList.clear();

						for (auto& tex : sceneraw.textureList)
						{
							std::clog << "Cleaning Raw Texture resource from RAM - " << tex.name << std::endl;
							FreeRawImage(tex);
						}

						sceneraw.textureList.clear();
					}

					std::clog << "Submitting command buffer" << std::endl;
					RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandBuffer(cmdBfr, true /*wait for finish*/, CVulkanRHI::QueueType::qt_Secondary));
					m_assetLoadingTracker.progress = 0.8f;

					// Destroy local staging resources
					std::clog << "Cleaning all Staging buffers" << std::endl;
					DestroyStaging(p_rhi, stgList);

					// Reset the command pool
					std::clog << "Resetting Command Pool" << std::endl;
					p_rhi->ResetCommandPool(m_assetLoaderCommandPool);
					m_assetLoadingTracker.progress = 1.0f;
					
					// Update the bindless descriptors
					std::clog << "Updating Scene's Bindless Texture Descriptors" << std::endl;
					if(!imageInfoList.empty())
					{
						for (uint32_t i = 0; i < FRAME_BUFFER_COUNT; i++)
						{
							// TODO: I have no idea why, but this is causing a crash on debug. Release works fine.
							// m_DescData[][BindingDest::bd_CubeMap_Texture].imgDesinfo is null right after control returns from
							// Creating and loading textures - m_sceneTextures->CreateTexture 
							BindlessWrite(i, BindingDest::bd_Env_Specular, &m_sceneTextures->GetTexture(TextureType::tt_env_specular).descInfo, 1);
							BindlessWrite(i, BindingDest::bd_Env_Diffuse, &m_sceneTextures->GetTexture(TextureType::tt_env_diffuse).descInfo, 1);
							BindlessWrite(i, BindingDest::bd_Brdf_Lut, &m_sceneTextures->GetTexture(TextureType::tt_brdfLut).descInfo, 1);
							BindlessWrite(i, BindingDest::bd_SceneRead_TexArray, imageInfoList.data(), (uint32_t)imageInfoList.size(), arrayDestIndex);
							BindlessUpdate(p_rhi, i);
						}
					}					

					std::clog << "Asset Loading Successful." << std::endl;
					m_assetLoadingTracker.state = AssetLoadingState::als_RequestComplete;

					return true;
				});				
				assetLoaderThread.detach();
			}
		}

		if (m_assetLoadingTracker.state == AssetLoadingState::als_RequestComplete)
		{
			ImGui::Text("Loading Complete.");
			if (ImGui::Button("Ok"))
			{
				m_assetLoadingTracker.log = "";
				m_assetLoadingTracker.progress = 0.0f;
				m_assetLoadingTracker.state = AssetLoadingState::als_WaitForRequest;

				ImGui::CloseCurrentPopup();
				m_fileDialog.ClearSelected();

				showLoadButtons = true;
				showLoadCompleteButtons = false;
			}
		}

		ImGui::EndPopup();
	}
	return true;
}

bool CScene::DeleteEntity()
{
	return false;
}

void CScene::DestroyStaging(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList)
{
	for (auto& stg : p_stgList)
		p_rhi->FreeMemoryDestroyBuffer(stg);

	p_stgList.clear();
}

CReadOnlyTextures::CReadOnlyTextures()
	:CTextures(TextureReadOnlyId::tr_max)
{
}

bool CReadOnlyTextures::Create(CVulkanRHI* p_rhi, CFixedBuffers& p_fixedBuffers, CVulkanRHI::CommandPool p_commandPool)
{
	CVulkanRHI::CommandBuffer cmdBfr;
	CVulkanRHI::BufferList stgList;

	std::string debugMarker = "Read-only Textures Loading";
	RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffers(p_commandPool, &cmdBfr, 1, &debugMarker));
	RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, debugMarker.c_str()));

	CFixedBuffers::PrimaryUniformData* priUnidata		= p_fixedBuffers.GetPrimaryUnifromData();
	RETURN_FALSE_IF_FALSE(CreateSSAOKernelTexture(p_rhi, stgList, priUnidata, cmdBfr));
	
	if (!p_rhi->EndCommandBuffer(cmdBfr))
		return false;

	CVulkanRHI::CommandBufferList cbrList{ cmdBfr };
	CVulkanRHI::PipelineStageFlagsList psfList{ VkPipelineStageFlags {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT} };
	bool waitForFinish = true;
	RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandBuffers(&cbrList, &psfList, waitForFinish));

	for (auto& stg : stgList)
		p_rhi->FreeMemoryDestroyBuffer(stg);

	return true;
}

void CReadOnlyTextures::Destroy(CVulkanRHI* p_rhi)
{
	DestroyTextures(p_rhi);
}

bool CReadOnlyTextures::CreateSSAOKernelTexture(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CFixedBuffers::PrimaryUniformData* p_primaryUniformData, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	uint32_t ssaoNoiseDim = p_rhi->GetRenderWidth() / (uint32_t)p_primaryUniformData->ssaoNoiseScale[0];
	
	std::vector<unsigned char> ssaoNoise;
	// introducing good random rotation can reduce number of samples required
	for (uint32_t i = 0; i < (ssaoNoiseDim * ssaoNoiseDim); i++)
	{
		ssaoNoise.push_back((unsigned char)(randf() * 255.0f));
		ssaoNoise.push_back((unsigned char)(randf() * 255.0f));
		ssaoNoise.push_back((unsigned char)0);
		ssaoNoise.push_back((unsigned char)0);// making sure the rotation happens along the z axis only
	}
	
	CVulkanRHI::Buffer staging;
	ImageRaw raw{};
	raw.name = "ssao_noise";
	raw.raw = ssaoNoise.data();
	raw.width = (int)ssaoNoiseDim;
	raw.height = (int)ssaoNoiseDim;
	raw.channels = 4;

	RETURN_FALSE_IF_FALSE(CreateTexture(p_rhi, staging, &raw, VK_FORMAT_R8G8B8A8_UNORM, p_cmdBfr, raw.name, CReadOnlyTextures::tr_SSAONoise));
	
	p_stgList.push_back(staging);

	return true;
}

CReadOnlyBuffers::CReadOnlyBuffers()
	:CBuffers(BufferReadOnlyId::br_max)
{
}

bool CReadOnlyBuffers::Create(CVulkanRHI* p_rhi, CFixedBuffers& p_fixedBuffers, CVulkanRHI::CommandPool p_commandPool)
{
	CVulkanRHI::CommandBuffer cmdBfr;
	CVulkanRHI::BufferList stgList;

	std::string debugMarker = "Read-only Buffers Loading";
	RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffers(p_commandPool, &cmdBfr, 1, &debugMarker));
	RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, debugMarker.c_str()));

	CFixedBuffers::PrimaryUniformData* priUnidata		= p_fixedBuffers.GetPrimaryUnifromData();
	RETURN_FALSE_IF_FALSE(CreateSSAONoiseBuffer(p_rhi, stgList, priUnidata, cmdBfr));

	if (!p_rhi->EndCommandBuffer(cmdBfr))
		return false;

	CVulkanRHI::CommandBufferList cbrList{ cmdBfr };
	CVulkanRHI::PipelineStageFlagsList psfList{ VkPipelineStageFlags {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT} };
	bool waitForFinish = true;
	RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandBuffers(&cbrList, &psfList, waitForFinish));

	for (auto& stg : stgList)
		p_rhi->FreeMemoryDestroyBuffer(stg);

	return true;
}

void CReadOnlyBuffers::Destroy(CVulkanRHI* p_rhi)
{
	DestroyBuffers(p_rhi);
}

bool CReadOnlyBuffers::CreateSSAONoiseBuffer(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CFixedBuffers::PrimaryUniformData* p_primaryUniformData, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	p_primaryUniformData->ssaoKernelSize			= 64.f;
	p_primaryUniformData->ssaoRadius				= 0.5f;

	std::vector<nm::float4> ssaoKernel;
	for (uint32_t i = 0; i < p_primaryUniformData->ssaoKernelSize; i++)
	{
		// creating random values along a semi-sphere oriented along surface normal (Z axis) in tangent space
		nm::float4 sample = {
				randf() * 2.0f - 1.0f
			,	randf() * 2.0f - 1.0f
			,	randf()
			,	1.0f };

		sample		= nm::normalize(sample);
		sample		= randf() * sample;

		// we also want to make sure the random 3d positions are placed closer to the actual fragment
		float scale = (float)i / 64.0f;
		scale		= 0.1f + (scale * scale) * (0.9f); // lerp(a,b, f) = a + f * (b - a);
		sample		= scale * sample;

		ssaoKernel.push_back(sample);
	}

	CVulkanRHI::Buffer staging;
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, staging, ssaoKernel.data(), sizeof(float) * 4 * ssaoKernel.size(), p_cmdBfr, "ssao_kernel", CReadOnlyBuffers::br_SSAOKernel));
	p_stgList.push_back(staging);

	return true;
}

CLoadableAssets::CLoadableAssets(CSceneGraph* p_sceneGraph)
	:m_scene(p_sceneGraph)
{	
}

CLoadableAssets::~CLoadableAssets()
{
}

bool CLoadableAssets::Create(CVulkanRHI* p_rhi, const CFixedAssets& p_fixedAssets, const CVulkanRHI::CommandPool& p_cmdPool)
{
	const CVulkanRHI::SamplerList* samplers = p_fixedAssets.GetSamplers();

	RETURN_FALSE_IF_FALSE(m_scene.Create(p_rhi, samplers, p_cmdPool));
	RETURN_FALSE_IF_FALSE(m_ui.Create(p_rhi, p_cmdPool));
	
	CFixedBuffers& fixeBuffer = *const_cast<CFixedBuffers*>(p_fixedAssets.GetFixedBuffers());
	RETURN_FALSE_IF_FALSE(m_readOnlyBuffers.Create(p_rhi, fixeBuffer, p_cmdPool));
	RETURN_FALSE_IF_FALSE(m_readOnlyTextures.Create(p_rhi, fixeBuffer, p_cmdPool));

	return true;
}

void CLoadableAssets::Destroy(CVulkanRHI* p_rhi)
{
	m_scene.Destroy(p_rhi);
	m_ui.DestroyRenderable(p_rhi);
	m_ui.DestroyTextures(p_rhi);
	m_readOnlyBuffers.Destroy(p_rhi);
	m_readOnlyTextures.Destroy(p_rhi);
}

bool CLoadableAssets::Update(CVulkanRHI* p_rhi, const LoadedUpdateData& p_updateData)
{
	RETURN_FALSE_IF_FALSE(m_scene.Update(p_rhi, p_updateData));
	RETURN_FALSE_IF_FALSE(m_ui.Update(p_rhi, p_updateData));
	return true;
}

CRenderTargets::CRenderTargets()
	: CTextures(CRenderTargets::RenderTargetId::rt_max)
	, CUIParticipant(CUIParticipant::ParticipationType::pt_onSelect, CUIParticipant::UIDPanelType::uipt_same)
{
	m_rtID.resize(CRenderTargets::RenderTargetId::rt_max);
}

CRenderTargets::~CRenderTargets()
{
}

bool CRenderTargets::Create(CVulkanRHI* p_rhi)
{
	uint32_t fullResWidth = p_rhi->GetRenderWidth();
	uint32_t fullResHeight = p_rhi->GetRenderHeight();

	VkImageLayout general = VK_IMAGE_LAYOUT_GENERAL;
	VkImageLayout shaderRead = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkImageUsageFlags sample_depth = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	VkImageUsageFlags sample_storage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	VkImageUsageFlags sample_storage_color = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	VkImageUsageFlags sample_storage_color_src = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	VkImageUsageFlags sample_storage_color_dest = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VkImageUsageFlags sample_storage_color_src_dest = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	//uint32_t maxMip = static_cast<uint32_t>(std::floor(std::log2(max(fullResWidth, fullResHeight)) + 1));

	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_PrimaryDepth,			VK_FORMAT_D32_SFLOAT,			fullResWidth, fullResHeight, 1,			shaderRead,	"primary_depth",		sample_depth));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_Position,				VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight, 1,			general,	"position",				sample_storage_color));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_Normal,					VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight, 1,			general,	"normal",				sample_storage_color));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_Albedo,					VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight, 1,			general,	"albedo",				sample_storage_color));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_SSAO_Blur,				VK_FORMAT_R16G16_SFLOAT,		fullResWidth, fullResHeight, 1,			general,	"ssao_and_blur",		sample_storage_color));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_DirectionalShadowDepth,  VK_FORMAT_D32_SFLOAT,			4096, 4096,					 1,			shaderRead,	"directional_shadow",	sample_depth));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_PrimaryColor,			VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight, 1,			general,	"primary_color",		sample_storage_color_src));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_RoughMetal,				VK_FORMAT_R16G16_SFLOAT,		fullResWidth, fullResHeight, 1,			general,	"Rough_Metal",			sample_storage_color));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_Motion,					VK_FORMAT_R16G16_SFLOAT,		fullResWidth, fullResHeight, 1,			general,	"Motion",				sample_storage_color));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_SSReflection,			VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight, 1,			general,	"ss_reflection",		sample_storage_color_dest));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_SSRBlur,					VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight, 1,			general,	"ssr_blur",				sample_storage_color_src_dest));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_Prev_PrimaryColor,		VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight, 1,			general,	"prev_primary_color",	sample_storage_color_dest));

	return true;
}

void CRenderTargets::Destroy(CVulkanRHI* p_rhi)
{
	DestroyTextures(p_rhi);
}

void CRenderTargets::Show(CVulkanRHI* p_rhi)
{
	CVulkanRHI::ImageList rendTargetList = GetTextures();
	ImGui::Indent();
	for (int i = 0; i < CRenderTargets::RenderTargetId::rt_max; i++)
	{
		std::string rtName = GetRenderTargetIDinString((CRenderTargets::RenderTargetId)i);
		CVulkanCore::Image renderTarget = rendTargetList[i];
		if (ImGui::TreeNode(rtName.c_str()))
		{
			// binding is 0 followed by render target id;
			m_rtID[i] = (0 << 16) | i;
			ImTextureID my_tex_id = &m_rtID[i];
			float my_tex_w = (float)480; // renderTarget.width;
			float my_tex_h = (float)270; // renderTarget.height;
			{
				ImGui::Text("%.0fx%.0f", (float)renderTarget.width, (float)renderTarget.height);
				ImVec2 pos = ImGui::GetCursorScreenPos();
				ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
				ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
				ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
				ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
				ImGui::Image(my_tex_id, ImVec2(my_tex_w, my_tex_h), uv_min, uv_max, tint_col, border_col);
			}
			ImGui::TreePop();
		}
	}
	ImGui::Unindent();
}

void CRenderTargets::SetLayout(RenderTargetId p_id, VkImageLayout p_layout)
{
	m_textures[p_id].descInfo.imageLayout = p_layout;
}

std::string CRenderTargets::GetRenderTargetIDinString(RenderTargetId p_id)
{
	if (p_id == CRenderTargets::RenderTargetId::rt_PrimaryDepth)
		return "PrimaryDepth";
	else if (p_id == CRenderTargets::RenderTargetId::rt_Position)
		return "Position";
	else if (p_id == CRenderTargets::RenderTargetId::rt_Normal)
		return "Normal";
	else if (p_id == CRenderTargets::RenderTargetId::rt_Albedo)
		return "Albedo";
	else if (p_id == CRenderTargets::RenderTargetId::rt_SSAO_Blur)
		return "SSAO_Motion";
	else if (p_id == CRenderTargets::RenderTargetId::rt_DirectionalShadowDepth)
		return "DirectionalShadowDepth";
	else if (p_id == CRenderTargets::RenderTargetId::rt_PrimaryColor)
		return "PrimaryColor";
	else if (p_id == CRenderTargets::RenderTargetId::rt_RoughMetal)
		return "Rough Metal";
	else if (p_id == CRenderTargets::RenderTargetId::rt_Motion)
		return "Motion";
	else if (p_id == CRenderTargets::RenderTargetId::rt_SSReflection)
		return "Screen Space Reflection (UVs)";
	else if (p_id == CRenderTargets::RenderTargetId::rt_Prev_PrimaryColor)
		return "Previous Color (Temporal)";
	else if (p_id == CRenderTargets::RenderTargetId::rt_SSRBlur)
		return "SSR Blur";
	else
		return "Error Render Target";
}

CFixedBuffers::CFixedBuffers()
	: CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
	, CBuffers(CFixedBuffers::FixedBufferId::fb_max)
{
	m_primaryUniformData.ssaoKernelSize			= 64.0f;
	m_primaryUniformData.ssaoNoiseScale         = nm::float2((float)RENDER_RESOLUTION_X / 4.0f, (float)RENDER_RESOLUTION_Y / 4.0f);
	m_primaryUniformData.ssaoRadius				= 0.5f;
	m_primaryUniformData.enableShadow			= 0;
	m_primaryUniformData.enableShadowPCF		= 0;
	m_primaryUniformData.enableIBL				= 0;
	m_primaryUniformData.pbrAmbientFactor		= 0.05f;
	m_primaryUniformData.enableSSAO				= 1;
	m_primaryUniformData.biasSSAO				= 0.015f;
	m_primaryUniformData.ssrEnable				= false;
	m_primaryUniformData.ssrMaxDistance			= 5.0f;
	m_primaryUniformData.ssrResolution			= 1.0f;
	m_primaryUniformData.ssrThickness			= 0.11f;
	m_primaryUniformData.ssrSteps				= 2;
	m_primaryUniformData.taaResolveWeight		= 0.9f;
	m_primaryUniformData.taaUseMotionVectors	= false;
	m_primaryUniformData.taaFlickerCorectionMode = 0;		// None
	m_primaryUniformData.taaReprojectionFilter	= 0;		// Standard
	m_primaryUniformData.toneMappingSelection	= 0.0f;
	m_primaryUniformData.toneMappingExposure	= 1.0f;
	m_primaryUniformData.UNASSIGNED_float1		= 0.0f;
	m_primaryUniformData.UNASSIGNED_float2		= 0.0f;
}

CFixedBuffers::~CFixedBuffers()
{
}

bool CFixedBuffers::Create(CVulkanRHI* p_rhi)
{
	size_t primaryUniformBufferSize =
		(sizeof(float) * 1)				// Tone Mapper Active
		+ (sizeof(float) * 3)				// camera look-from
		+ (sizeof(float) * 16)				// camera view projection
		+ (sizeof(float) * 16)				// camera view projection with Jitter
		+ (sizeof(float) * 16)				// pre camera view projection
		+ (sizeof(float) * 16)				// camera projection
		+ (sizeof(float) * 16)				// camera view
		+ (sizeof(float) * 16)				// inverse camera view
		+ (sizeof(float) * 16)				// inverse camera projection
		+ (sizeof(float) * 16)				// skybox model view
		+ (sizeof(float) * 2)				// mouse position (x,y)
		+ (sizeof(float) * 2)				// SSAO Noise Scale
		+ (sizeof(float) * 1)				// SSAO Kernel Size
		+ (sizeof(float) * 1)				// SSAO Radius
		+ (sizeof(float) * 16)				// sun light View Projection
		+ (sizeof(float) * 3)				// sun light direction in world space
		+ (sizeof(int) * 1)					// enabled PCF for shadow
		+ (sizeof(float) * 3)				// sun light direction in view space
		+ (sizeof(float) * 1)				// sun intensity
		+ (sizeof(float) * 1)				// Enable IBL
		+ (sizeof(float) * 1)				// PBR ambient Factor
		+ (sizeof(int) * 1)					// enable SSAO
		+ (sizeof(float) * 1)				// biasSSAO
		+ (sizeof(float) * 1)				// SSR Enable
		+ (sizeof(float) * 1)				// SSR Max Distance
		+ (sizeof(float) * 1)				// SSR Resolution
		+ (sizeof(float) * 1)				// SSR Thickness
		+ (sizeof(float) * 1)				// SSR Steps
		+ (sizeof(float) * 1)				// TAA Resolve Weight
		+ (sizeof(float) * 1)				// TAA Use Motion Vectors
		+ (sizeof(float) * 1)				// TAA Flicker Correction Mode
		+ (sizeof(float) * 1)				// TAA Reprojection Filter
		+ (sizeof(float) * 1)				// Tone Mapping Exposure
		+ (sizeof(float) * 1)				// UNASSIGINED_Float_1
		+ (sizeof(float) * 1);				// UNASSIGINED_Float_2
		

	size_t objPickerBufferSize = sizeof(uint32_t) * 1; // selected mesh ID
	size_t debugDrawUniformSize = MAX_SUPPORTED_DEBUG_DRAW_ENTITES * ((sizeof(float) * 16)); // storing transforms

	VkBufferUsageFlags uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	VkBufferUsageFlags dest = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VkBufferUsageFlags src_storage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	VkMemoryPropertyFlags hv_hc = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	VkMemoryPropertyFlags hv = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;


	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, uniform,		hv_hc,	primaryUniformBufferSize, "primary_uniform_0",	fb_PrimaryUniform_0));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, uniform,		hv_hc,	primaryUniformBufferSize, "primary_uniform_1",	fb_PrimaryUniform_1));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, dest,			hv,		objPickerBufferSize		, "pick_read",			fb_ObjectPickerRead));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, src_storage,	hv,		objPickerBufferSize		, "pick_write",			fb_ObjectPickerWrite));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, uniform,		hv_hc,	debugDrawUniformSize	, "debug_uniform_0",	fb_DebugUniform_0));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, uniform,		hv_hc,	debugDrawUniformSize	, "debug_uniform_1",	fb_DebugUniform_1));

	return true;
}

void CFixedBuffers::Show(CVulkanRHI* p_rhi)
{
	if (Header("Uniforms"))
	{
		ImGui::Indent();
		if (ImGui::TreeNode("General"))
		{
			//ImGui::InputFloat("Time Elapsed", &m_primaryUniformData.elapsedTime);
			//ImGui::InputFloat2("Render Resolution", &m_primaryUniformData.renderRes[0]);
			ImGui::InputFloat2("Mouse Position", &m_primaryUniformData.mousePos[0]);

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Camera"))
		{
			ImGui::TreePop();
		}
		ImGui::Unindent();
	}
}

void CFixedBuffers::Destroy(CVulkanRHI* p_rhi)
{
	DestroyBuffers(p_rhi);
}

bool CFixedBuffers::Update(CVulkanRHI* p_rhi, uint32_t p_scId)
{
	float* cameraViewProj					= const_cast<float*>(&m_primaryUniformData.cameraViewProj.column[0][0]);
	float* cameraJitteredViewProj			= const_cast<float*>(&m_primaryUniformData.cameraJitteredViewProj.column[0][0]);
	float* cameraInvViewProj				= const_cast<float*>(&m_primaryUniformData.cameraInvViewProj.column[0][0]);
	float* cameraPreViewProj				= const_cast<float*>(&m_primaryUniformData.cameraPreViewProj.column[0][0]);
	float* cameraProj						= const_cast<float*>(&m_primaryUniformData.cameraProj.column[0][0]);
	float* cameraView						= const_cast<float*>(&m_primaryUniformData.cameraView.column[0][0]);
	float* cameraInvView					= const_cast<float*>(&m_primaryUniformData.cameraInvView.column[0][0]);
	float* cameraInvProj					= const_cast<float*>(&m_primaryUniformData.cameraInvProj.column[0][0]);
	float* skyboxModelView					= const_cast<float*>(&m_primaryUniformData.skyboxModelView.column[0][0]);
	
	// canceling out translation for skybox rendering
	skyboxModelView[12]						= 0.0;
	skyboxModelView[13]						= 0.0;
	skyboxModelView[14]						= 0.0;
	skyboxModelView[15]						= 1.0;

	std::vector<float> uniformValues;
	uniformValues.push_back((float)m_primaryUniformData.toneMappingSelection);																				// Tone Mapping Selection
	std::copy(&m_primaryUniformData.cameraLookFrom[0], &m_primaryUniformData.cameraLookFrom[3], std::back_inserter(uniformValues));							// camera look from x, y, z
	std::copy(&cameraViewProj[0], &cameraViewProj[16], std::back_inserter(uniformValues));																	// camera view projection matrix
	std::copy(&cameraJitteredViewProj[0], &cameraJitteredViewProj[16], std::back_inserter(uniformValues));													// camera jittered view projection matrix
	std::copy(&cameraInvViewProj[0], &cameraInvViewProj[16], std::back_inserter(uniformValues));															// camera inv view projection matrix
	std::copy(&cameraPreViewProj[0], &cameraPreViewProj[16], std::back_inserter(uniformValues));															// camera pre view projection matrix
	std::copy(&cameraProj[0], &cameraProj[16], std::back_inserter(uniformValues));																			// camera projection matrix
	std::copy(&cameraView[0], &cameraView[16], std::back_inserter(uniformValues));																			// camera view matrix
	std::copy(&cameraInvView[0], &cameraInvView[16], std::back_inserter(uniformValues));																	// inverse camera view matrix
	std::copy(&cameraInvProj[0], &cameraInvProj[16], std::back_inserter(uniformValues));																	// inverse camera projection matrix
	std::copy(&skyboxModelView[0], &skyboxModelView[16], std::back_inserter(uniformValues));																// skybox model view
	uniformValues.push_back((float)m_primaryUniformData.mousePos[0]);	uniformValues.push_back((float)m_primaryUniformData.mousePos[1]);					// mouse pos
	uniformValues.push_back((float)m_primaryUniformData.ssaoNoiseScale[0]); uniformValues.push_back((float)m_primaryUniformData.ssaoNoiseScale[1]);			// SSAO noise scale
	uniformValues.push_back((float)m_primaryUniformData.ssaoKernelSize);																					// SSAO kernel size
	uniformValues.push_back((float)m_primaryUniformData.ssaoRadius);																						// SSAO radius
	uniformValues.push_back((float)m_primaryUniformData.enableShadow);																						// enable Shadows
	uniformValues.push_back((float)m_primaryUniformData.enableShadowPCF);																					// enable PCF for shadows
	uniformValues.push_back((float)m_primaryUniformData.enableIBL);																								// enable IBL
	uniformValues.push_back(m_primaryUniformData.pbrAmbientFactor);																							// PBR Ambient Factor
	uniformValues.push_back((float)m_primaryUniformData.enableSSAO);																						// enable SSAO
	uniformValues.push_back(m_primaryUniformData.biasSSAO);																									// SSAO Bias
	uniformValues.push_back((float)m_primaryUniformData.ssrEnable);																							// SSR Enable
	uniformValues.push_back(m_primaryUniformData.ssrMaxDistance);																							// SSR Max Distance
	uniformValues.push_back(m_primaryUniformData.ssrResolution);																							// SSR Resolution
	uniformValues.push_back(m_primaryUniformData.ssrThickness);																								// SSR Thickness
	uniformValues.push_back((float)m_primaryUniformData.ssrSteps);																							// SSR Steps
	uniformValues.push_back(m_primaryUniformData.taaResolveWeight);																							// TAA Resolve Weight
	uniformValues.push_back((float)m_primaryUniformData.taaUseMotionVectors);																				// TAA Use Motion Vector
	uniformValues.push_back((float)m_primaryUniformData.taaFlickerCorectionMode);																			// TAA Flicker Correction Mode
	uniformValues.push_back((float)m_primaryUniformData.taaReprojectionFilter);																				// TAA Reprojection Filter
	uniformValues.push_back(m_primaryUniformData.toneMappingExposure);																						// Tone Mapping Exposure
	uniformValues.push_back((float)m_primaryUniformData.UNASSIGNED_float1);																					// UNASSIGINED_1
	uniformValues.push_back((float)m_primaryUniformData.UNASSIGNED_float2);																					// UNASSIGINED_2
	
	uint8_t* data							= (uint8_t*)(uniformValues.data());
	RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(data, m_buffers[p_scId], false));
		
	return true;
}

CFixedAssets::CFixedAssets()
	: CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
{
}

bool CFixedAssets::Create(CVulkanRHI* p_rhi, const CVulkanRHI::CommandPool& p_cmdPool)
{
	RETURN_FALSE_IF_FALSE(m_fixedBuffers.Create(p_rhi));
	RETURN_FALSE_IF_FALSE(m_renderTargets.Create(p_rhi));
	RETURN_FALSE_IF_FALSE(CreateSamplers(p_rhi));
	RETURN_FALSE_IF_FALSE(m_renderableDebug.Create(p_rhi, &m_fixedBuffers, p_cmdPool));
	return true;
}

void CFixedAssets::Destroy(CVulkanRHI* p_rhi)
{
	m_fixedBuffers.Destroy(p_rhi);
	m_renderTargets.Destroy(p_rhi);
	m_renderableDebug.Destroy(p_rhi);

	for (auto& sampler: m_samplers)
	{
		p_rhi->DestroySampler(sampler.descInfo.sampler);
	}
	m_samplers.clear();
}

bool CFixedAssets::Update(CVulkanRHI* p_rhi, const FixedUpdateData& p_updateData)
{
	RETURN_FALSE_IF_FALSE(m_fixedBuffers.Update(p_rhi, p_updateData.swapchainIndex));
	return true;
}

void CFixedAssets::Show(CVulkanRHI* p_rhi)
{
	bool doubleClickedEntity = false;
	int32_t sceneIndex = 0;
	if (Header("Fixed Assets"))
	{
		ImGui::Indent();
		if (ImGui::TreeNode("Render Targets"))
		{
			m_renderTargets.Show(p_rhi);
			ImGui::TreePop();
		}
		ImGui::Unindent();
	}
}

bool CFixedAssets::CreateSamplers(CVulkanRHI* p_rhi)
{
	m_samplers.resize(SamplerId::s_max);

	struct SamplerData { uint32_t id; VkFilter filter; VkImageLayout layout; };
	std::vector<SamplerData> samplerDataList
	{
			{s_Linear, VK_FILTER_LINEAR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
		,   {s_Nearest, VK_FILTER_NEAREST, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
	};

	for (const auto& samp : samplerDataList)
	{
		m_samplers[samp.id].filter = samp.filter;
		m_samplers[samp.id].maxAnisotropy = 1.0f;
		m_samplers[samp.id].descInfo.imageLayout = samp.layout;
		m_samplers[samp.id].descInfo.imageView = VK_NULL_HANDLE;
		RETURN_FALSE_IF_FALSE(p_rhi->CreateSampler(m_samplers[samp.id]));
	}

	return true;
}

CPrimaryDescriptors::CPrimaryDescriptors()
	:CDescriptor(CVulkanRHI::DescriptorBindFlag::Bindless, FRAME_BUFFER_COUNT)
{
}

CPrimaryDescriptors::~CPrimaryDescriptors()
{
}

bool CPrimaryDescriptors::Create(CVulkanRHI* p_rhi, CFixedAssets& p_fixedAssets, const CLoadableAssets& p_loadableAssets)
{
	const CReadOnlyTextures* readonlyTex				= p_loadableAssets.GetReadonlyTextures();
	CReadOnlyBuffers* readonlyBuf						= const_cast<CReadOnlyBuffers*>(p_loadableAssets.GetReadonlyBuffers());
	CFixedBuffers* fixedBuf								= p_fixedAssets.GetFixedBuffers();
	CRenderTargets* rendTargets							= p_fixedAssets.GetRenderTargets();
	const CVulkanRHI::SamplerList* samplers				= p_fixedAssets.GetSamplers();
	
	// Sampling Render Targets Descriptor Info
	std::vector<VkDescriptorImageInfo> sampleRenderTargetsDesInfoList;
	for(int i = 0; i < CRenderTargets::RenderTargetId::rt_max; i++)
	{
		sampleRenderTargetsDesInfoList.push_back(rendTargets->GetTexture(i).descInfo);
	}
	
	// Storage Render Targets Descriptor Info
	std::vector<VkDescriptorImageInfo> storeRenderTargetsDesInfoList(STORE_MAX_RENDER_TARGETS, VkDescriptorImageInfo{});
	{
		storeRenderTargetsDesInfoList[STORE_POSITION]				= rendTargets->GetTexture(CRenderTargets::RenderTargetId::rt_Position).descInfo;
		storeRenderTargetsDesInfoList[STORE_NORMAL]					= rendTargets->GetTexture(CRenderTargets::RenderTargetId::rt_Normal).descInfo;
		storeRenderTargetsDesInfoList[STORE_ALBEDO]					= rendTargets->GetTexture(CRenderTargets::RenderTargetId::rt_Albedo).descInfo;
		storeRenderTargetsDesInfoList[STORE_SSAO_AND_BLUR]			= rendTargets->GetTexture(CRenderTargets::RenderTargetId::rt_SSAO_Blur).descInfo;
		storeRenderTargetsDesInfoList[STORE_PRIMARY_COLOR]			= rendTargets->GetTexture(CRenderTargets::RenderTargetId::rt_PrimaryColor).descInfo;
		storeRenderTargetsDesInfoList[STORE_ROUGH_METAL]			= rendTargets->GetTexture(CRenderTargets::RenderTargetId::rt_RoughMetal).descInfo;
		storeRenderTargetsDesInfoList[STORE_MOTION]					= rendTargets->GetTexture(CRenderTargets::RenderTargetId::rt_Motion).descInfo;
		storeRenderTargetsDesInfoList[STORE_SS_REFLECTION]			= rendTargets->GetTexture(CRenderTargets::RenderTargetId::rt_SSReflection).descInfo;
		storeRenderTargetsDesInfoList[STORE_SSR_BLUR]				= rendTargets->GetTexture(CRenderTargets::RenderTargetId::rt_SSRBlur).descInfo;
		storeRenderTargetsDesInfoList[STORE_PREV_PRIMARY_COLOR]		= rendTargets->GetTexture(CRenderTargets::RenderTargetId::rt_Prev_PrimaryColor).descInfo;
	}

	// read only texture desc info
	std::vector<VkDescriptorImageInfo> readTexDesInfoList;
	for (int i = 0; i < CReadOnlyTextures::tr_max; i++)
	{
		readTexDesInfoList.push_back(readonlyTex->GetTexture(i).descInfo);
	}

	VkShaderStageFlags all			= VK_SHADER_STAGE_ALL;
	VkShaderStageFlags fragment		= VK_SHADER_STAGE_FRAGMENT_BIT;
	VkShaderStageFlags compute		= VK_SHADER_STAGE_COMPUTE_BIT;
	VkShaderStageFlags frag_compute = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorType uniform		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	VkDescriptorType sampler		= VK_DESCRIPTOR_TYPE_SAMPLER;
	VkDescriptorType storage_buf	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	VkDescriptorType sampled_img	= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	VkDescriptorType storage_img	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	{
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Gloabl_Uniform,			1,								uniform,		all},			0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Linear_Sampler,			1,								sampler,		all},			0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Nearest_Sampler,			1,								sampler,		all},			0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_ObjPicker_Storage,			1,								storage_buf,	fragment},		0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_SSAOKernel_Storage,		1,								storage_buf,	compute,},		0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_PrimaryRead_TexArray,		CReadOnlyTextures::tr_max,		sampled_img,	compute},		0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_RTs_StorageImages,			STORE_MAX_RENDER_TARGETS,		storage_img,	frag_compute},	0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_RTs_SampledImages,			SAMPLE_MAX_RENDER_TARGETS,		sampled_img,	frag_compute},	0);
		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, 0, "PrimaryDescriptors_0"));
	}

	{
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Gloabl_Uniform,			1,								uniform,		all},			1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Linear_Sampler,			1,								sampler,		all},			1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Nearest_Sampler,			1,								sampler,		all},			1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_ObjPicker_Storage,			1,								storage_buf,	fragment},		1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_SSAOKernel_Storage,		1,								storage_buf,	compute},		1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_PrimaryRead_TexArray,		CReadOnlyTextures::tr_max,		sampled_img,	compute},		1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_RTs_StorageImages,			STORE_MAX_RENDER_TARGETS,		storage_img,	frag_compute},  1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_RTs_SampledImages,			SAMPLE_MAX_RENDER_TARGETS,		sampled_img,	frag_compute},	1);
		
		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, 1, "PrimaryDescriptors_1"));
	}

	for (uint32_t i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		BindlessWrite(i, BindingDest::bd_Gloabl_Uniform, &fixedBuf->GetBuffer(CFixedBuffers::fb_PrimaryUniform_0 + i).descInfo);
		BindlessWrite(i, BindingDest::bd_Linear_Sampler, &(*samplers)[s_Linear].descInfo);
		BindlessWrite(i, BindingDest::bd_Linear_Sampler, &(*samplers)[s_Nearest].descInfo);
		BindlessWrite(i, BindingDest::bd_ObjPicker_Storage, &fixedBuf->GetBuffer(CFixedBuffers::fb_ObjectPickerWrite).descInfo);
		BindlessWrite(i, BindingDest::bd_SSAOKernel_Storage, &readonlyBuf->GetBuffer(CReadOnlyBuffers::br_SSAOKernel).descInfo);
		BindlessWrite(i, BindingDest::bd_PrimaryRead_TexArray, readTexDesInfoList.data(), (uint32_t)readTexDesInfoList.size());
		BindlessWrite(i, BindingDest::bd_RTs_SampledImages, sampleRenderTargetsDesInfoList.data(), (uint32_t)sampleRenderTargetsDesInfoList.size());
		BindlessWrite(i, BindingDest::bd_RTs_StorageImages, storeRenderTargetsDesInfoList.data(), (uint32_t)storeRenderTargetsDesInfoList.size());
		BindlessUpdate(p_rhi, i);
	}

	return true;
}

void CPrimaryDescriptors::Destroy(CVulkanRHI* p_rhi)
{
	DestroyDescriptors(p_rhi);
}

CRenderableDebug::CRenderableDebug()
	: CDescriptor(false/* is bindless */, FRAME_BUFFER_COUNT)
	, CRenderable(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0)
{
}

bool CRenderableDebug::Create(CVulkanRHI* p_rhi, const CFixedBuffers* p_fixedBuffers, const CVulkanRHI::CommandPool& p_cmdPool)
{
	CVulkanRHI::CommandBuffer cmdBfr;
	CVulkanRHI::BufferList stgList;
	std::string debugMarker = "Debug Resources Loading";
	RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffers(p_cmdPool, &cmdBfr, 1, &debugMarker));
	RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, debugMarker.c_str()));

	RETURN_FALSE_IF_FALSE(CreateBoxSphereBuffers(p_rhi, stgList, cmdBfr))

	RETURN_FALSE_IF_FALSE(p_rhi->EndCommandBuffer(cmdBfr));

	CVulkanRHI::CommandBufferList cbrList{ cmdBfr };
	CVulkanRHI::PipelineStageFlagsList psfList{ VkPipelineStageFlags {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT} };
	bool waitForFinish = true;
	RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandBuffers(&cbrList, &psfList, waitForFinish));

	for (auto& stg : stgList)
		p_rhi->FreeMemoryDestroyBuffer(stg);

	if (!CreateDebugDescriptors(p_rhi, p_fixedBuffers))
	{
		std::cerr << "Failed to Create Debug Descriptors" << std::endl;
	}
	return true;
}

bool CRenderableDebug::Update()
{
	return false;
}

bool CRenderableDebug::PreDrawInstanced(CVulkanRHI* p_rhi, uint32_t p_scIdx, const CFixedBuffers* p_fixedBuffers, 
	const CSceneGraph* p_sceneGraph, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	bool						bufferStale = false;
	CSceneGraph::EntityList*	entityList = p_sceneGraph->GetEntities();
	std::vector<float>			allSelectedTransformData;
	std::vector<float>			selectedSphereTransformData;
	std::vector<float>			selectedFrustumTransformData;
	uint32_t					currentCount = 0;

	//Reserve vector with the buffer's memory range to avoid any memcpy access violation
	allSelectedTransformData.reserve(p_fixedBuffers->GetBuffer(CFixedBuffers::fb_DebugUniform_0).reqMemSize);
	
	m_bBoxDetails.instanceOffset = 0;
	m_bBoxDetails.instanceCount = 0;

	m_bSphereDetails.instanceOffset = 0;
	m_bSphereDetails.instanceCount = 0;

	m_bFrustumDetails.instanceOffset = 0;
	m_bFrustumDetails.instanceCount = 0;

	if (p_sceneGraph->IsDebugDrawEnabled())
	{
		const float* transform = &(p_sceneGraph->GetBoundingBox()->GetUnitBBoxTransform().GetTransform()).column[0][0];
		std::copy(&transform[0], &transform[16], std::back_inserter(allSelectedTransformData));
		++m_bBoxDetails.instanceCount;
	}

	// Loop through the entity list
	for (auto& entity : (*entityList))
	{
		if (entity->IsDebugDrawEnabled() || entity->IsSubmeshDebugDrawEnabled())
		{
			nm::float4x4 entityTransform = entity->GetTransform().GetTransform();
			BVolume* boundingVol = entity->GetBoundingVolume();

			// Append the entity's bounding sphere to raw CPU buffer if selected
			if (boundingVol->GetBoundingType() == BVolume::BType::Sphere)
			{
				if (dynamic_cast<BSphere*>(boundingVol))
				{
					const float* transform = &(entityTransform).column[0][0];
					std::copy(&transform[0], &transform[16], std::back_inserter(selectedSphereTransformData));
					++m_bSphereDetails.instanceCount;
				}
			}
			else if(boundingVol->GetBoundingType() == BVolume::BType::Box)
			{
				BBox* boundingBox = dynamic_cast<BBox*>(boundingVol);
				if (boundingBox)
				{
					// Append the entity's mesh's primary bbox to raw CPU buffer if selected
					if (entity->IsDebugDrawEnabled())
					{
						const float* transform =  &(entityTransform * boundingBox->GetUnitBBoxTransform().GetTransform()).column[0][0];
						std::copy(&transform[0], &transform[16], std::back_inserter(allSelectedTransformData));
						++m_bBoxDetails.instanceCount;
					}

					// Append the sub-mesh's bbox to raw CPU buffer if selected
					if (dynamic_cast<CRenderableMesh*>(entity) && entity->IsSubmeshDebugDrawEnabled())
					{
						CRenderableMesh* mesh = dynamic_cast<CRenderableMesh*>(entity);
						for (uint32_t subBoxID = 0; subBoxID < mesh->GetSubBoundingBoxCount(); subBoxID++)
						{
							BBox* box = mesh->GetSubBoundingBox(subBoxID);
							const float* transform = &(entityTransform * box->GetUnitBBoxTransform().GetTransform()).column[0][0];
							std::copy(&transform[0], &transform[16], std::back_inserter(allSelectedTransformData));
							++m_bBoxDetails.instanceCount;
						}
					}
				}
			}
			else if (boundingVol->GetBoundingType() == BVolume::BType::Frustum)
			{
				BFrustum* frustum = dynamic_cast<BFrustum*>(boundingVol);
				if (frustum)
				{
					// since this is a frustum, we are going to multiply every vertex of a unit-camera-style-box with 
					// its view projection matrix. Hence sending that matrix to get multiplied
					const float* transform = &(frustum->GetViewProjection()).column[0][0];
					std::copy(&transform[0], &transform[16], std::back_inserter(selectedFrustumTransformData));
					++m_bFrustumDetails.instanceCount;
				}
			}
		}
	}

	// Append sphere transform data to the end of all selected bbox transform raw data list
	if (!selectedSphereTransformData.empty())
	{
		std::copy(selectedSphereTransformData.begin(), selectedSphereTransformData.end(), std::back_inserter(allSelectedTransformData));
		m_bSphereDetails.instanceOffset = m_bBoxDetails.instanceCount;
	}

	// Append frustum transform data to the end of all selected bbox + sphere transform raw data list
	if (!selectedFrustumTransformData.empty())
	{
		std::copy(selectedFrustumTransformData.begin(), selectedFrustumTransformData.end(), std::back_inserter(allSelectedTransformData));
		m_bFrustumDetails.instanceOffset = m_bBoxDetails.instanceCount + m_bSphereDetails.instanceCount;
	}

	uint32_t allInstanceCount = 
		m_bBoxDetails.instanceCount + 
		m_bSphereDetails.instanceCount + 
		m_bFrustumDetails.instanceCount;

	// Heuristic to identify if the uniform buffer is stale and needs rebuilding
	//if (allInstanceCount != m_instanceCount ||
	//	p_sceneGraph->GetSceneStatus() != CSceneGraph::SceneStatus::ss_NoChange)
	{
		m_instanceCount = allInstanceCount;
		bufferStale = true;
	}

	// Irrespective of the swap chain buffer id, nuke both uniform buffers and reload them
	if (bufferStale && !allSelectedTransformData.empty())
	{
		uint8_t* data = (uint8_t*)(allSelectedTransformData.data());
		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(data, p_fixedBuffers->GetBuffer(CFixedBuffers::fb_DebugUniform_0), true));
		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(data, p_fixedBuffers->GetBuffer(CFixedBuffers::fb_DebugUniform_1), true));
	}

	return m_instanceCount > 0;
}

void CRenderableDebug::Destroy(CVulkanRHI* p_rhi)
{
}

bool CRenderableDebug::CreateDebugDescriptors(CVulkanRHI* p_rhi, const CFixedBuffers* p_fixedBuffers)
{
	VkShaderStageFlags vertex_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	{
		CVulkanRHI::Buffer uniformBuffer = p_fixedBuffers->GetBuffer(CFixedBuffers::fb_DebugUniform_0);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Debug_Transforms_Uniform,	1,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	vertex_frag,	&uniformBuffer.descInfo,	VK_NULL_HANDLE}, 0);
		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, 0, "UIDescriptorSet_0"));
	}

	{
		CVulkanRHI::Buffer uniformBuffer = p_fixedBuffers->GetBuffer(CFixedBuffers::fb_DebugUniform_1);
		AddDescriptor(CVulkanRHI::DescriptorData{ 0, BindingDest::bd_Debug_Transforms_Uniform,	1,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	vertex_frag,	&uniformBuffer.descInfo,	VK_NULL_HANDLE }, 1);
		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, 1, "UIDescriptorSet_1"));
	}

	return true;
}

bool CRenderableDebug::CreateBoxSphereBuffers(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	std::clog << "Loading Debug Resources" << std::endl;
	MeshRaw	debugMeshes;
	// we are going to instance the bounding boxes
	// so loading the mesh only once as a template
	{
		// Bounding Box Template
		debugMeshes.indicesList = BBox::GetIndexTemplate();
		debugMeshes.vertexList = BBox::GetVertexTempate();

		// Populate bbox details
		m_bBoxDetails.indexCount = (uint32_t)debugMeshes.indicesList.size();
		m_bBoxDetails.indexOffset = 0;
		m_bBoxDetails.vertexOffset = 0;
	}

	// we are going to instance the bounding spheres so loading the mesh only
	// once as a template
	{
		// Bounding Sphere Template
		// create sphere
		RawSphere debugSphere;
		GenerateSphere(25, 25, debugSphere, 1.0f);

		// populate bSphere details
		m_bSphereDetails.indexCount		= (uint32_t)debugSphere.lineIndices.size();
		m_bSphereDetails.indexOffset	= (uint32_t)debugMeshes.indicesList.size();
		m_bSphereDetails.vertexOffset   = (uint32_t)debugMeshes.vertexList.size() / debugMeshes.vertexList.GetVertexSize();
		
		auto rawVertexData = debugSphere.vertices.getRaw();
		std::vector<int> rawIndexData = debugSphere.lineIndices;

		std::copy(rawVertexData.begin(), rawVertexData.end(), std::back_inserter(debugMeshes.vertexList.getRaw()));
		std::copy(rawIndexData.begin(), rawIndexData.end(), std::back_inserter(debugMeshes.indicesList));
	}

	// we are going to instance the bounding frustums so loading the mesh only
	// once as a template
	{
		// Bounding Frustum Template
		auto rawIndexData = BFrustum::GetIndexTemplate();
		auto rawVertexData = BFrustum::GetVertexTempate().getRaw();

		// Populate Frustum details
		m_bFrustumDetails.indexCount = (uint32_t)rawIndexData.size();
		m_bFrustumDetails.indexOffset = 0;	// we are reusing the same indices that bound box is using because they are same for frustum
		m_bFrustumDetails.vertexOffset = (uint32_t)debugMeshes.vertexList.size() / debugMeshes.vertexList.GetVertexSize();

		std::copy(rawVertexData.begin(), rawVertexData.end(), std::back_inserter(debugMeshes.vertexList.getRaw()));
	}
		
	RETURN_FALSE_IF_FALSE(CreateVertexIndexBuffer(p_rhi, p_stgList, &debugMeshes, p_cmdBfr, "renderable_debug"));

	return true;
}

CRayTracingRenderable::CRayTracingRenderable(VkAccelerationStructureInstanceKHR* p_accStructInstance)
	: CRenderable(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0)
	, m_accStructInstance(p_accStructInstance)
	, m_blasBuffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0)
	, m_BLAS(VK_NULL_HANDLE)
{
}

void CRayTracingRenderable::DestroyRenderable(CVulkanRHI* p_rhi)
{
	m_blasBuffer.DestroyBuffers(p_rhi);
	p_rhi->DestroyAccelerationStrucutre(m_BLAS);
}

bool CRayTracingRenderable::CreateVertexIndexBuffer(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, const MeshRaw* p_meshRaw, CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugStr, int32_t index)
{
	RETURN_FALSE_IF_FALSE(CRenderable::CreateVertexIndexBuffer(p_rhi, p_stgList, p_meshRaw, p_cmdBfr, p_debugStr, index));

	return true;
}

bool CRayTracingRenderable::CreateBuildBLAS(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugStr)
{
	VkDeviceAddress vbAddress = p_rhi->GetBufferDeviceAddress(GetVertexBuffer().descInfo.buffer);
	VkDeviceAddress ibAddress = p_rhi->GetBufferDeviceAddress(GetIndexBuffer().descInfo.buffer);

	// Providing the mesh vertex and index data and defining the geometry
	VkAccelerationStructureGeometryKHR geometry{};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT; // refer vertex attribute defined for shadow pass and reused everywhere else
	geometry.geometry.triangles.vertexStride = GetVertexStrideInBytes();
	geometry.geometry.triangles.maxVertex = GetVertexCount();
	geometry.geometry.triangles.vertexData.deviceAddress = vbAddress;
	geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.indexData.deviceAddress = ibAddress;

	// Might want to revisit this when using instancing
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;	// might want to use the update flag when add geometry at runtime?
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &geometry; // as of now, I am using 1 geometry per Mesh (includes all its sub-meshes)

	// Build type is device because we are choosing to create the resources on the device instead
	// of host. The driver spawns a compute shader to build the acceleration structure

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
	p_rhi->GetAccelerationStructureBuildSize(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, GetPrimitiveCount(), &sizeInfo);

	std::clog << "CRenderable::CreateBuildBLAS: Total Acceleration Structure Size: " << sizeInfo.accelerationStructureSize / 1048576.0f << " Mb." << std::endl;
	std::clog << "CRenderable::CreateBuildBLAS: Total Scratch Size: " << sizeInfo.buildScratchSize / 1048576.0f << " Mb." << std::endl;

	// Create Scratch Buffer
	VkDeviceAddress scratchAddress;
	{
		CVulkanRHI::Buffer scratchBuffer;
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeInfo.buildScratchSize, scratchBuffer,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "BLAS Scratch"));

		scratchAddress = p_rhi->GetBufferDeviceAddress(scratchBuffer.descInfo.buffer);

		p_stgbufferList.push_back(scratchBuffer);
	}

	VkAccelerationStructureBuildRangeInfoKHR buildRanges{};
	const VkAccelerationStructureBuildRangeInfoKHR* buildRangePtrs;

	// Create Acceleration Buffer
	RETURN_FALSE_IF_FALSE(m_blasBuffer.CreateBuffer(p_rhi, sizeInfo.accelerationStructureSize, p_debugStr));

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = m_blasBuffer.GetBuffer().descInfo.buffer;
	accelerationStructureCreateInfo.offset = 0;
	accelerationStructureCreateInfo.size = sizeInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateAccelerationStructure(&accelerationStructureCreateInfo, m_BLAS));
	std::clog << "CRenderable::CreateBuildBLAS: Creating Bottom Acceleration Structure for " << p_debugStr << std::endl;

	buildInfo.dstAccelerationStructure = m_BLAS;
	buildInfo.scratchData.deviceAddress = scratchAddress;

	buildRanges.primitiveCount = GetPrimitiveCount();
	buildRangePtrs = &buildRanges;

	p_rhi->BuildAccelerationStructure(p_cmdBfr, 1, &buildInfo, &buildRangePtrs);
	std::clog << "CRenderable::CreateBuildBLAS: Building Acceleration Structures for " << p_debugStr << std::endl;

	return true;
}

void CRayTracingRenderable::UpdateBLASInstance(CVulkanRHI* p_rhi, const nm::Transform& p_transform)
{
	m_accStructInstance->transform = p_transform.GetTransformAffine();
	m_accStructInstance->instanceCustomIndex = 0;
	m_accStructInstance->mask = 0xFF;
	m_accStructInstance->instanceShaderBindingTableRecordOffset = 0;
	m_accStructInstance->flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	m_accStructInstance->accelerationStructureReference = p_rhi->GetAccelerationStructureDeviceAddress(m_BLAS);
}
