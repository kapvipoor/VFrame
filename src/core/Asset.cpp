#include "Asset.h"
#include "external/imgui/imgui.h"
#include "RandGen.h"

std::vector<CUIParticipant*> CUIParticipantManager::m_uiParticipants;

void CDescriptor::AddDescriptor(CVulkanRHI::DescriptorData p_dData, uint32_t p_id)
{
	m_descList[p_id].descDataList.push_back(p_dData);
}

bool CDescriptor::CreateDescriptors(CVulkanRHI* p_rhi, uint32_t p_varDescCount, uint32_t p_texArrayIndex, uint32_t p_id)
{
	if (!p_rhi->CreateDescriptors(p_varDescCount, m_descList[p_id].descDataList, m_descPool, m_descList[p_id].descLayout, m_descList[p_id].descSet, p_texArrayIndex))
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

CRenderable::CRenderable(uint32_t p_BufferCount)
{
	if (p_BufferCount > 0)
	{
		m_vertexBuffers.resize(p_BufferCount);
		m_indexBuffers.resize(p_BufferCount);
	}
}

void CRenderable::Clear(CVulkanRHI* p_rhi, uint32_t p_idx)
{
	m_vertexBuffers[p_idx].descInfo.offset = 0;
	m_vertexBuffers[p_idx].descInfo.range = 0;
	p_rhi->FreeMemoryDestroyBuffer(m_vertexBuffers[p_idx]);

	m_indexBuffers[p_idx].descInfo.offset = 0;
	m_indexBuffers[p_idx].descInfo.range = 0;
	p_rhi->FreeMemoryDestroyBuffer(m_indexBuffers[p_idx]);
}

void CRenderable::DestroyRenderable(CVulkanRHI* p_rhi)
{
	for (int i = 0; i < m_vertexBuffers.size(); i++)
	{
		m_vertexBuffers[i].descInfo.offset = 0;
		m_vertexBuffers[i].descInfo.range = 0;
		p_rhi->FreeMemoryDestroyBuffer(m_vertexBuffers[i]);

		m_indexBuffers[i].descInfo.offset = 0;
		m_indexBuffers[i].descInfo.range = 0;
		p_rhi->FreeMemoryDestroyBuffer(m_indexBuffers[i]);
	}
}

bool CRenderable::CreateVertexIndexBuffer(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, const MeshRaw* p_meshRaw, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	{
		CVulkanRHI::Buffer vertexStg;
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeof(Vertex) * p_meshRaw->vertexList.size(), vertexStg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)p_meshRaw->vertexList.data(), vertexStg));
		p_stgList.push_back(vertexStg);

		CVulkanRHI::Buffer vertexBuffer;
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(vertexStg.descInfo.range, vertexBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

		RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(vertexStg, vertexBuffer, p_cmdBfr));
		m_vertexBuffers.push_back(vertexBuffer);
	}

	{
		CVulkanRHI::Buffer indxStg;
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeof(uint32_t) * p_meshRaw->indicesList.size(), indxStg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)p_meshRaw->indicesList.data(), indxStg));
		p_stgList.push_back(indxStg);

		CVulkanRHI::Buffer indexBuffer;
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(indxStg.descInfo.range, indexBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

		RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(indxStg, indexBuffer, p_cmdBfr));
		m_indexBuffers.push_back(indexBuffer);
	}

	return true;
}

CBuffers::CBuffers(int p_maxSize)
{
	if(p_maxSize > 0)
		m_buffers.resize(p_maxSize);
}

bool CBuffers::CreateBuffer(CVulkanRHI* p_rhi, uint32_t p_id, VkBufferUsageFlags p_usage, VkMemoryPropertyFlags p_memProp, size_t p_size)
{
	CVulkanRHI::Buffer buffer;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(p_size, buffer, p_usage, p_memProp));

	m_buffers[p_id] = buffer;

	return true;
}

bool CBuffers::CreateBuffer(CVulkanRHI* p_rhi, CVulkanRHI::Buffer& p_stg, void* p_data, size_t p_size, CVulkanRHI::CommandBuffer& p_cmdBfr, uint32_t p_id)
{
	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(p_size, p_stg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)p_data, p_stg));

	CVulkanRHI::Buffer buffer;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(p_size, buffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(p_stg, buffer, p_cmdBfr));

	// Doing this because; if the id is set to -1, then the intent is to grouw the buffer at runtime and not a fixed size
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

bool CTextures::CreateRenderTarget(CVulkanRHI* p_rhi, uint32_t p_id, VkFormat p_format, uint32_t p_width, uint32_t p_height, VkImageLayout p_layout, VkImageUsageFlags p_usage)
{
	CVulkanRHI::Image renderTarget;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateRenderTarget(p_format, p_width, p_height, p_layout, p_usage, renderTarget));

	m_textures[p_id] = renderTarget;

	return true;
}

bool CTextures::CreateTexture(CVulkanRHI* p_rhi, CVulkanRHI::Buffer& p_stg, const ImageRaw* p_rawImg, VkFormat p_format, CVulkanRHI::CommandBuffer& p_cmdBfr, int p_id)
{
	if (p_rawImg->raw != nullptr)
	{
		CVulkanRHI::Image img;

		size_t texSize							= p_rawImg->width * p_rawImg->height * p_rawImg->channels;

		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(texSize, p_stg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)p_rawImg->raw, p_stg));

		img.format								= p_format;//VK_FORMAT_R8G8B8A8_UNORM;
		img.width								= p_rawImg->width;
		img.height								= p_rawImg->height;
		img.viewType							= VK_IMAGE_VIEW_TYPE_2D;

		VkImageCreateInfo imgCrtInfo			= CVulkanCore::ImageCreateInfo();
		imgCrtInfo.extent.width					= p_rawImg->width;
		imgCrtInfo.extent.height				= p_rawImg->height;
		imgCrtInfo.format						= img.format;
		imgCrtInfo.usage						= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		RETURN_FALSE_IF_FALSE(p_rhi->CreateTexture(p_stg, img, imgCrtInfo, p_cmdBfr))

		// Doing this because; if the id is set to -1, then the intent is to grouw the image list at runtime and not a fixed size
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

bool CTextures::CreateCubemap(CVulkanRHI* p_rhi, CVulkanRHI::Buffer& p_stg, const std::vector<ImageRaw>& p_rawList, const CVulkanRHI::SamplerList& p_samplers, CVulkanRHI::CommandBuffer& p_cmdBfr, int p_id)
{
	if (p_rawList.size() != 6)
	{
		std::cout << "Error: Cannot create cubemeap because raw data has slices less/more than 6" << std::endl;
		return false;
	}

	std::vector<unsigned char> megaStg_data;
	for (int i = 0; i< p_rawList.size(); i++)
	{
		int size = p_rawList[i].width * p_rawList[i].height * p_rawList[i].channels;
		megaStg_data.insert(megaStg_data.end(), p_rawList[i].raw, p_rawList[i].raw + size);
	}

	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(megaStg_data.size(), p_stg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)megaStg_data.data(), p_stg));
	CVulkanRHI::Image cubemap;
	cubemap.descInfo.imageLayout					= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	cubemap.descInfo.sampler						= p_samplers[SamplerId::s_Linear].descInfo.sampler;
	cubemap.format									= VK_FORMAT_R8G8B8A8_UNORM;
	cubemap.width									= p_rawList[0].width;
	cubemap.height									= p_rawList[0].height;
	cubemap.layerCount								= 6;
	cubemap.bufOffset								= p_rawList[0].width * p_rawList[0].height * p_rawList[0].channels;
	cubemap.viewType								= VK_IMAGE_VIEW_TYPE_CUBE;

	VkImageCreateInfo imgInfo						= CVulkanCore::ImageCreateInfo();
	imgInfo.extent.width							= p_rawList[0].width;					// assuming all the loaded cubemap images have same width
	imgInfo.extent.height							= p_rawList[0].height;					// assuming all the loaded cubemap images have same height
	imgInfo.arrayLayers								= 6;
	imgInfo.flags									= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imgInfo.usage									= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imgInfo.format									= VK_FORMAT_R8G8B8A8_UNORM;

	RETURN_FALSE_IF_FALSE(p_rhi->CreateTexture(p_stg, cubemap, imgInfo, p_cmdBfr));
	// Doing this because; if the id is set to -1, then the intent is to grouw the texture list at runtime and not a fixed size
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

void CTextures::IssueLayoutBarrier(CVulkanRHI* p_rhi, CVulkanRHI::ImageLayout p_imageLayout, CVulkanRHI::CommandBuffer& p_cmdBfr, uint32_t p_id)
{
	p_rhi->IssueLayoutBarrier(p_imageLayout, m_textures[p_id], p_cmdBfr);
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

CRenderableUI::CRenderableUI()
	:CRenderable(FRAME_BUFFER_COUNT)
{
	m_participantManager = new CUIParticipantManager();
}

CRenderableUI::~CRenderableUI()
{
	delete m_participantManager;
}

bool CRenderableUI::Create(CVulkanRHI* p_rhi, const CVulkanRHI::CommandPool& p_cmdPool)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	CVulkanRHI::CommandBuffer cmdBfr;
	CVulkanRHI::BufferList stgList;

	RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffers(p_cmdPool, &cmdBfr, 1));

	RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, "UI Loading"));

	if (!LoadFonts(p_rhi, stgList, cmdBfr))
	{
		std::cout << "Error: Failed to UI Fonts" << std::endl;
		return false;
	}

	RETURN_FALSE_IF_FALSE(p_rhi->EndCommandBuffer(cmdBfr));

	CVulkanRHI::Queue queue					= p_rhi->GetQueue(0);
	VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType						= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount			= 1;
	submitInfo.pCommandBuffers				= &cmdBfr;
	submitInfo.pSignalSemaphores			= nullptr;
	submitInfo.signalSemaphoreCount			= 0;
	submitInfo.pWaitSemaphores				= nullptr;
	submitInfo.waitSemaphoreCount			= 0;
	submitInfo.pWaitDstStageMask			= &waitstage;
	RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandbuffer(queue, &submitInfo, 1));

	RETURN_FALSE_IF_FALSE(p_rhi->WaitToFinish(queue));

	for (auto& stg : stgList)
		p_rhi->FreeMemoryDestroyBuffer(stg);

	if (!CreateUIDescriptors(p_rhi))
	{
		std::cout << "Error: Failed to create UI Desriptors" << std::endl;
		return false;
	}

	ImGui::GetIO().Fonts->SetTexID(*GetDescriptorSet());

	return true;
}

void CRenderableUI::Destroy(CVulkanRHI* p_rhi)
{
	DestroyRenderable(p_rhi);
	DestroyTextures(p_rhi);
}

bool CRenderableUI::Update(const LoadedUniformData& p_loadedUpdate, CFixedBuffers::PrimaryUniformData& p_primUniData)
{
	ImGuiIO& imguiIO						= ImGui::GetIO();
	imguiIO.DisplaySize						= ImVec2(p_loadedUpdate.screenRes[0], p_loadedUpdate.screenRes[1]);
	imguiIO.DeltaTime						= p_loadedUpdate.timeElapsed;
	imguiIO.MousePos						= ImVec2(p_loadedUpdate.curMousePos[0], p_loadedUpdate.curMousePos[1]);
	imguiIO.MouseDown[0]					= p_loadedUpdate.isLeftMouseDown;
	imguiIO.MouseDown[1]					= p_loadedUpdate.isRightMouseDown;

	ImGui::NewFrame();

	bool showDemo = false;
	if (showDemo)
	{
		ImGui::ShowDemoWindow(&showDemo);
	}
	else
	{
		ShowUI(p_primUniData);
	}

	return true;
}

bool CRenderableUI::LoadFonts(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	ImGuiIO& imguiIO						= ImGui::GetIO();
	
	ImageRaw tex;
	imguiIO.Fonts->GetTexDataAsRGBA32(&tex.raw, &tex.width, &tex.height, &tex.channels);
	size_t texSize							= tex.width * tex.height * tex.channels * sizeof(char); 
	
	if (tex.raw != nullptr)
	{
		CVulkanRHI::Buffer stg;
		RETURN_FALSE_IF_FALSE(CreateTexture(p_rhi, stg, &tex, VK_FORMAT_R8G8B8A8_UNORM, p_cmdBfr));
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

	AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_UI_TexArray, texturesCount, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT,	VK_NULL_HANDLE,	imageInfoList.data() });

	RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, texturesCount, BindingDest::bd_UI_TexArray));

	return true;
}

bool CRenderableUI::ShowUI(CFixedBuffers::PrimaryUniformData& )
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_::ImGuiCond_FirstUseEver);
	ImGui::Begin("VFrame Interface", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::TextUnformatted("VFrame Settings");
	//ImGui::TextUnformatted(deviceProperties.deviceName);
	//ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / lastFPS), lastFPS);

	ImGui::PushItemWidth(110.0f * 1.0f);

	// Here
	m_participantManager->Show();

	ImGui::PopItemWidth();

	ImGui::End();
	ImGui::PopStyleVar();

	return true;
}

bool CRenderableUI::PrepareDraw(CVulkanRHI* p_rhi, uint32_t p_scIdx) 
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
			size_t verteSize				= drawData->TotalVtxCount * sizeof(ImDrawVert);
			size_t indexSize				= drawData->TotalIdxCount * sizeof(ImDrawIdx);

			// if the vertex buffer if already created from previous frame, destroy them and free the asscoiated memroy for this frame's use
			if (m_vertexBuffers[p_scIdx].descInfo.buffer != VK_NULL_HANDLE)
			{
				Clear(p_rhi, p_scIdx);
			}

			// Creating vertex and index buffers and upload all data to single contigeous GPU buffers respectively
			{
				p_rhi->CreateAllocateBindBuffer(verteSize, m_vertexBuffers[p_scIdx], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
				p_rhi->CreateAllocateBindBuffer(indexSize, m_indexBuffers[p_scIdx], VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

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
				p_rhi->UnMapMemory(m_indexBuffers[p_scIdx]);
			}
		}
	}

	return true;
}

CRenderableMesh::CRenderableMesh(uint32_t p_meshId, nm::float4x4 p_modelMat)
	: m_mesh_id(p_meshId)
	, m_modelMatrix(p_modelMat)
{
}

void CRenderableMesh::PopulateSubmeshes(const std::vector<Submesh>& p_rawSubMeshes)
{
	std::vector<CRenderableMesh::SubMesh> meshList;
	for (const auto& rawSubmesh : p_rawSubMeshes)
	{
		CRenderableMesh::SubMesh submesh{ rawSubmesh.firstIndex, rawSubmesh.indexCount, rawSubmesh.materialId };
		meshList.push_back(submesh);
	}

	m_submeshes.resize(p_rawSubMeshes.size());
	std::copy(meshList.begin(), meshList.end(), m_submeshes.begin());
}

bool CScene::Create(CVulkanRHI* p_rhi, const CVulkanRHI::SamplerList* p_samplerList, const CVulkanRHI::CommandPool& p_cmdPool)
{
	CVulkanRHI::CommandBuffer cmdBfr;
	CVulkanRHI::BufferList stgList;

	RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffers(p_cmdPool, &cmdBfr, 1));

	RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, "Scene Loading"));

	if (!LoadDefaultTexture(p_rhi, stgList, cmdBfr))
	{
		std::cout << "Error: Failed to Create Default Texture" << std::endl;
		return false;
	}

	if (!LoadSkybox(p_rhi, p_samplerList, stgList, cmdBfr))
	{
		std::cout << "Error: Failed to Load Skybox" << std::endl;
		return false;
	}

	if (!LoadScene(p_rhi, stgList, cmdBfr))
	{
		std::cout << "Error: Failed to Load Scene" << std::endl;
		return false;
	}

	if (!p_rhi->EndCommandBuffer(cmdBfr))
		return false;

	VkQueue queue							= p_rhi->GetQueue(0);
	VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType						= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount			= 1;
	submitInfo.pCommandBuffers				= &cmdBfr;
	submitInfo.pSignalSemaphores			= nullptr;
	submitInfo.signalSemaphoreCount			= 0;
	submitInfo.pWaitSemaphores				= nullptr;
	submitInfo.waitSemaphoreCount			= 0;
	submitInfo.pWaitDstStageMask			= &waitstage;
	RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandbuffer(queue, &submitInfo, 1));

	RETURN_FALSE_IF_FALSE(p_rhi->WaitToFinish(queue));

	for (auto& stg : stgList)
		p_rhi->FreeMemoryDestroyBuffer(stg);

	RETURN_FALSE_IF_FALSE(CreateMeshUniformBuffer(p_rhi));

	RETURN_FALSE_IF_FALSE(CreateSceneDescriptors(p_rhi));

	return true;
}

void CScene::Destroy(CVulkanRHI* p_rhi)
{
	DestroyDescriptors(p_rhi);
	DestroyTextures(p_rhi);

	for (auto& mesh : m_meshes)
	{
		mesh.DestroyRenderable(p_rhi);
	}

	p_rhi->FreeMemoryDestroyBuffer(m_material_storage);
	p_rhi->FreeMemoryDestroyBuffer(m_meshInfo_uniform);
}

void CScene::Show()
{
	int32_t sceneIndex = 0;
	if(Header("Scene Settings"))
	{
		ComboBox("Scenes", &sceneIndex, m_scenePaths);
	}
}

bool CScene::Update(CVulkanRHI* p_rhi, const LoadedUniformData& p_loadedUpdate)
{
	for (auto& mesh : m_meshes)
	{
		mesh.m_modelMatrix					= nm::float4x4().identity();
		mesh.m_viewNormalTransform			= nm::transpose(nm::inverse(p_loadedUpdate.viewMatrix * mesh.m_modelMatrix));
	}

	{
		std::vector<float> perMeshUniformData;
		for (const auto& mesh : m_meshes)
		{
			const float* modelMat			= &mesh.m_modelMatrix.column[0][0];
			const float* trn_inv_model		= &nm::transpose(nm::inverse(mesh.m_modelMatrix)).column[0][0];

			std::copy(&modelMat[0], &modelMat[16], std::back_inserter(perMeshUniformData));				// model matrix for this mesh
			std::copy(&trn_inv_model[0], &trn_inv_model[16], std::back_inserter(perMeshUniformData));		// Tranpose(inverse(model)) for transforming normal to world space
		}

		uint8_t* data = (uint8_t*)(perMeshUniformData.data());

		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(data, m_meshInfo_uniform));
	}

	return true;
}

bool CScene::LoadDefaultTexture(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	// load default texture to compensate for bad textures
	{
		CVulkanRHI::Buffer stg;
		ImageRaw tex;
		
		RETURN_FALSE_IF_FALSE(LoadRawImage((g_AssetPath + "/tex_not_found.png").c_str(), tex));
		RETURN_FALSE_IF_FALSE(CreateTexture(p_rhi, stg, &tex, VK_FORMAT_B8G8R8A8_SRGB,p_cmdBfr));
		
		FreeRawImage(tex);
		p_stgList.push_back(stg);
	}

	return true;
}

bool CScene::LoadSkybox(CVulkanRHI* p_rhi, const CVulkanRHI::SamplerList* p_samplerList, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	// Load skybox cubemap
	{
		std::string cubemap_path[6]{
			g_AssetPath + "/skybox/Daylight_Box_Pieces/f.png"
		,	g_AssetPath + "/skybox/Daylight_Box_Pieces//b.png"
		,	g_AssetPath + "/skybox/Daylight_Box_Pieces/t.png"
		,	g_AssetPath + "/skybox/Daylight_Box_Pieces/bt.png"
		,	g_AssetPath + "/skybox/Daylight_Box_Pieces/l.png"
		,	g_AssetPath + "/skybox/Daylight_Box_Pieces/r.png"
		};

		std::vector<ImageRaw> cubemap_raw;
		cubemap_raw.resize(6);
		for (int i = 0; i < 6; i++)
		{
			RETURN_FALSE_IF_FALSE(LoadRawImage(cubemap_path[i].c_str(), cubemap_raw[i]));
		}

		CVulkanRHI::Buffer stg;
		RETURN_FALSE_IF_FALSE(CreateCubemap(p_rhi, stg, cubemap_raw, *p_samplerList, p_cmdBfr));

		p_stgList.push_back(stg);
	}

	// Load skybox geometry
	{
		SceneRaw sceneraw;
		ObjLoadData loadData{};
		loadData.flipUV					= false;
		loadData.loadMeshOnly			= true;

		std::string skybox_obj_path		= g_AssetPath + "/skybox/skybox.obj";
		RETURN_FALSE_IF_FALSE(LoadObj(skybox_obj_path.c_str(), sceneraw, loadData));

		MeshRaw meshraw					= sceneraw.meshList[0];
		CRenderableMesh mesh(MeshType::mt_Skybox, nm::float4x4::identity());
		RETURN_FALSE_IF_FALSE(mesh.CreateVertexIndexBuffer(p_rhi, p_stgList, &meshraw, p_cmdBfr));
		m_meshes.push_back(mesh);
	}

	return true;
}

bool CScene::LoadScene(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	m_scenePaths.push_back(g_AssetPath + "/glTFSampleModels/2.0/DragonAttenuation/glTF/DragonAttenuation.gltf");
	m_scenePaths.push_back(g_AssetPath + "/shadow_test_3.gltf");
	m_scenePaths.push_back(g_AssetPath + "/glTFSampleModels/2.0/TransmissionTest/glTF/TransmissionTest.gltf");
	m_scenePaths.push_back(g_AssetPath + "/glTFSampleModels/2.0/NormalTangentMirrorTest/glTF/NormalTangentMirrorTest.gltf");
	m_scenePaths.push_back(g_AssetPath + "/glTFSampleModels/2.0/Suzanne/glTF/Suzanne.gltf");
	m_scenePaths.push_back(g_AssetPath + "/Sponza/glTF/Sponza.gltf");
	m_scenePaths.push_back(g_AssetPath + "DamagedHelmet/glTF/DamagedHelmet.gltf");
	m_scenePaths.push_back(g_AssetPath + "cube/cube.obj");

	std::vector<std::string> paths;
	paths.push_back(m_scenePaths[5]);

	std::vector<bool> flipYList{ false, false, false };

	SceneRaw sceneraw;
	for (unsigned int i = 0; i < paths.size(); i++)
	{
		ObjLoadData loadData{};
		loadData.flipUV = flipYList[i];
		loadData.loadMeshOnly = false;

		RETURN_FALSE_IF_FALSE(LoadGltf(paths[i].c_str(), sceneraw, loadData));
	}
	
	for (auto& meshraw : sceneraw.meshList)
	{
		CRenderableMesh mesh((uint32_t)m_meshes.size(), meshraw.transform);
		mesh.PopulateSubmeshes(meshraw.submeshes);
		RETURN_FALSE_IF_FALSE(mesh.CreateVertexIndexBuffer(p_rhi, p_stgList, &meshraw, p_cmdBfr));
		m_meshes.push_back(mesh);
	}
	
	for (const auto& tex : sceneraw.textureList)
	{
		CVulkanRHI::Image img;
		{
			if (tex.raw != nullptr)
			{
				CVulkanRHI::Buffer stg;
				RETURN_FALSE_IF_FALSE(CreateTexture(p_rhi, stg, &tex, VK_FORMAT_R8G8B8A8_UNORM, p_cmdBfr));
				p_stgList.push_back(stg);
			}
			else
			{
				m_textures.push_back(m_textures[TextureType::tt_default]);

			}
		}
	}

	// storage buffer for materials
	{
		CVulkanRHI::Buffer matStg;
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeof(Material) * sceneraw.materialsList.size(), matStg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)sceneraw.materialsList.data(), matStg));

		p_stgList.push_back(matStg);

		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(matStg.descInfo.range, m_material_storage, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

		RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(matStg, m_material_storage, p_cmdBfr));
	}

	// can binary dump in future to optimised format for faster binary loading
	sceneraw.meshList.clear();
	for(auto& tex : sceneraw.textureList)
		FreeRawImage(tex);

	sceneraw.textureList.clear();

	return true;
}

bool CScene::CreateMeshUniformBuffer(CVulkanRHI* p_rhi)
{
	size_t uniBufize = m_meshes.size() * (
		(sizeof(float) * 16)	// model matrix
	+	(sizeof(float) * 16)	// tranpose(inverse(model)) for transforming normal to world space
		);

	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(uniBufize, m_meshInfo_uniform, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	return true;
}

bool CScene::CreateSceneDescriptors(CVulkanRHI* p_rhi)
{
	std::vector<VkDescriptorImageInfo> imageInfoList;
	uint32_t texturesCount = 0;
	for (int i = TextureType::tt_scene; i < m_textures.size(); i++)
	{
		imageInfoList.push_back(m_textures[i].descInfo);
		texturesCount++;
	}

	VkShaderStageFlags vertex_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Scene_MeshInfo_Uniform,			1,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			vertex_frag,					&m_meshInfo_uniform.descInfo,	VK_NULL_HANDLE });
	AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_CubeMap_Texture,					1,	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	VK_SHADER_STAGE_FRAGMENT_BIT,	VK_NULL_HANDLE,					&m_textures[TextureType::tt_skybox].descInfo });
	AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Material_Storage,					1,	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			VK_SHADER_STAGE_FRAGMENT_BIT,	&m_material_storage.descInfo,	VK_NULL_HANDLE });
	AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SceneRead_TexArray,	texturesCount,	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,			VK_SHADER_STAGE_FRAGMENT_BIT,	VK_NULL_HANDLE,					imageInfoList.data() });

	RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, texturesCount, BindingDest::bd_SceneRead_TexArray));

	return true;
}

CReadOnlyTextures::CReadOnlyTextures()
	:CTextures(TextureReadOnlyId::tr_max)
{
}

bool CReadOnlyTextures::Create(CVulkanRHI* p_rhi, CFixedBuffers& p_fixedBuffers, CVulkanRHI::CommandPool p_commandPool)
{
	CVulkanRHI::CommandBuffer cmdBfr;
	CVulkanRHI::BufferList stgList;

	RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffers(p_commandPool, &cmdBfr, 1));

	RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, "Loading Readonly Textures"));

	CFixedBuffers::PrimaryUniformData& priUnidata		= p_fixedBuffers.GetPrimaryUnifromData();
	RETURN_FALSE_IF_FALSE(CreateSSAOKernelTexture(p_rhi, stgList, priUnidata, cmdBfr));
	p_fixedBuffers.SetPrimaryUniformData(priUnidata);

	if (!p_rhi->EndCommandBuffer(cmdBfr))
		return false;

	VkQueue queue										= p_rhi->GetQueue(0);
	VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType									= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount						= 1;
	submitInfo.pCommandBuffers							= &cmdBfr;
	submitInfo.pSignalSemaphores						= nullptr;
	submitInfo.signalSemaphoreCount						= 0;
	submitInfo.pWaitSemaphores							= nullptr;
	submitInfo.waitSemaphoreCount						= 0;
	submitInfo.pWaitDstStageMask						= &waitstage;
	RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandbuffer(queue, &submitInfo, 1));

	RETURN_FALSE_IF_FALSE(p_rhi->WaitToFinish(queue));

	for (auto& stg : stgList)
		p_rhi->FreeMemoryDestroyBuffer(stg);

	return true;
}

void CReadOnlyTextures::Destroy(CVulkanRHI* p_rhi)
{
	DestroyTextures(p_rhi);
}

bool CReadOnlyTextures::CreateSSAOKernelTexture(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CFixedBuffers::PrimaryUniformData& p_primaryUniformData, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	uint32_t ssaoNoiseDimSquared				= 16;
	p_primaryUniformData.ssaoNoiseScale			= nm::float2((float)p_rhi->GetRenderWidth() / (float)ssaoNoiseDimSquared, (float)p_rhi->GetRenderHeight() / (float)ssaoNoiseDimSquared);

	std::vector<unsigned char> ssaoNoise;
	// introducing good random rotation can reduce number of samples required
	for (uint32_t i = 0; i < ssaoNoiseDimSquared; i++)
	{
		ssaoNoise.push_back((unsigned char)(randf() * 255.0f));
		ssaoNoise.push_back((unsigned char)(randf() * 255.0f));
		ssaoNoise.push_back((unsigned char)0);
		ssaoNoise.push_back((unsigned char)0);// making sure the roation happens along the z axis only
	}
	
	CVulkanRHI::Buffer staging;
	ImageRaw raw{ ssaoNoise.data(), (int)sqrt(ssaoNoiseDimSquared) , (int)sqrt(ssaoNoiseDimSquared), 4 };
	
	RETURN_FALSE_IF_FALSE(CreateTexture(p_rhi, staging, &raw, VK_FORMAT_R8G8B8A8_UNORM, p_cmdBfr, CReadOnlyTextures::tr_SSAONoise));
	
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

	RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffers(p_commandPool, &cmdBfr, 1));

	RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, "Loading Readonly Buffers"));

	CFixedBuffers::PrimaryUniformData& priUnidata		= p_fixedBuffers.GetPrimaryUnifromData();
	RETURN_FALSE_IF_FALSE(CreateSSAONoiseBuffer(p_rhi, stgList, priUnidata, cmdBfr));
	p_fixedBuffers.SetPrimaryUniformData(priUnidata);

	if (!p_rhi->EndCommandBuffer(cmdBfr))
		return false;

	VkQueue queue										= p_rhi->GetQueue(0);
	VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType									= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount						= 1;
	submitInfo.pCommandBuffers							= &cmdBfr;
	submitInfo.pSignalSemaphores						= nullptr;
	submitInfo.signalSemaphoreCount						= 0;
	submitInfo.pWaitSemaphores							= nullptr;
	submitInfo.waitSemaphoreCount						= 0;
	submitInfo.pWaitDstStageMask						= &waitstage;
	RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandbuffer(queue, &submitInfo, 1));

	RETURN_FALSE_IF_FALSE(p_rhi->WaitToFinish(queue));

	for (auto& stg : stgList)
		p_rhi->FreeMemoryDestroyBuffer(stg);

	return true;
}

void CReadOnlyBuffers::Destroy(CVulkanRHI* p_rhi)
{
	DestroyBuffers(p_rhi);
}

bool CReadOnlyBuffers::CreateSSAONoiseBuffer(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CFixedBuffers::PrimaryUniformData& p_primaryUniformData, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	p_primaryUniformData.ssaoKernelSize			= 64.f;
	p_primaryUniformData.ssaoRadius				= 0.5f;

	std::vector<nm::float4> ssaoKernel;
	for (uint32_t i = 0; i < p_primaryUniformData.ssaoKernelSize; i++)
	{
		// creating random values along a semi-sphere oriented along surface normal (Z axis) in tangent space
		nm::float4 sample = {
				randf() * 2.0f - 1.0f
			,	randf() * 2.0f - 1.0f
			,	randf()
			,	0.0f };

		sample = nm::normalize(sample);
		sample = randf() * sample;

		// we also want to make sure the random 3d positions are placed closer to the actual fragment
		float scale = (float)i / 64.0f;
		scale = 0.01f + (scale * scale) * (1.0f - 0.01f);	// exponential curve forcing values close to frag
		sample = scale * sample;

		ssaoKernel.push_back(sample);
	}

	CVulkanRHI::Buffer staging;
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, staging, ssaoKernel.data(), sizeof(float) * 4 * ssaoKernel.size(), p_cmdBfr, CReadOnlyBuffers::br_SSAOKernel));
	p_stgList.push_back(staging);

	return true;
}

CLoadableAssets::CLoadableAssets()
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
	m_ui.Destroy(p_rhi);
	m_readOnlyBuffers.Destroy(p_rhi);
	m_readOnlyTextures.Destroy(p_rhi);
}

bool CLoadableAssets::Update(CVulkanRHI* p_rhi, const LoadedUniformData& p_uniData, CFixedBuffers::PrimaryUniformData& p_primUniData)
{
	RETURN_FALSE_IF_FALSE(m_scene.Update(p_rhi, p_uniData));
	RETURN_FALSE_IF_FALSE(m_ui.Update(p_uniData, p_primUniData));

	return true;
}

CRenderTargets::CRenderTargets()
	:CTextures(CRenderTargets::RenderTargetId::rt_max)
{
}

CRenderTargets::~CRenderTargets()
{
}

bool CRenderTargets::Create(CVulkanRHI* p_rhi)
{
	uint32_t fullResWidth = p_rhi->GetRenderWidth();
	uint32_t fullResHeight = p_rhi->GetRenderHeight();

	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_PrimaryDepth,	VK_FORMAT_D32_SFLOAT_S8_UINT,	fullResWidth, fullResHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_Position,		VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_Normal,			VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_Albedo,			VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_SSAO,			VK_FORMAT_R32_SFLOAT,			fullResWidth, fullResHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_SSAOBlur,		VK_FORMAT_R32_SFLOAT,			fullResWidth, fullResHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_LightDepth,		VK_FORMAT_D32_SFLOAT_S8_UINT,	4096, 4096,					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_DeferredLighting,VK_FORMAT_R16G16B16A16_SFLOAT, fullResWidth, fullResHeight,	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));

	return true;
}

void CRenderTargets::Destroy(CVulkanRHI* p_rhi)
{
	DestroyTextures(p_rhi);
}

void CRenderTargets::SetLayout(RenderTargetId p_id, VkImageLayout p_layout)
{
	m_textures[p_id].descInfo.imageLayout = p_layout;
}

CFixedBuffers::CFixedBuffers()
	:CBuffers(CFixedBuffers::FixedBufferId::fb_max)
{
	m_primaryUniformData.ssaoKernelSize			= 64.0f;
	m_primaryUniformData.ssaoRadius				= 0.5f;
}

CFixedBuffers::~CFixedBuffers()
{
}

bool CFixedBuffers::Create(CVulkanRHI* p_rhi)
{
	size_t primaryUniformBufferSize =
		(sizeof(float) * 1)					// delta time elapsed
		+ (sizeof(float) * 3)				// camera lookfrom
		+ (sizeof(float) * 16)				// camera view projection
		+ (sizeof(float) * 16)				// camera projection
		+ (sizeof(float) * 16)				// camera view
		+ (sizeof(float) * 16)				// inverse camera view
		+ (sizeof(float) * 16)				// skybox model view
		+ (sizeof(float) * 2)				// render resolution
		+ (sizeof(float) * 2)				// mouse position (x,y)
		+ (sizeof(float) * 2)				// SSAO Noise Scale
		+ (sizeof(float) * 1)				// SSAO Kernel Size
		+ (sizeof(float) * 1)				// SSAO Radius
		+ (sizeof(float) * 16)				// sun light View Projection
		+ (sizeof(float) * 3)				// sun light direction in world space
		+ (sizeof(float) * 1)				// unassigned_0
		+ (sizeof(float) * 3)				// sun light direction in view space
		+ (sizeof(float) * 1);				// unassigned_1

	size_t objPickerBufferSize = sizeof(uint32_t) * 1; // selected mesh ID

	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, fb_PrimaryUniform_0,	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, primaryUniformBufferSize));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, fb_PrimaryUniform_1,	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, primaryUniformBufferSize));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, fb_ObjectPickerRead,	VK_BUFFER_USAGE_TRANSFER_DST_BIT,										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,										objPickerBufferSize));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, fb_ObjectPickerWrite,	VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,										objPickerBufferSize));

	return true;
}

void CFixedBuffers::Show()
{
	if (Header("Unfiorms"))
	{
		if (ImGui::TreeNode("General"))
		{
			Text("Time Elapsed: %f", m_primaryUniformData.elapsedTime);
			Text("Render Resolution: %d x %d", (int)m_primaryUniformData.renderRes[0], (int)m_primaryUniformData.renderRes[1]);
			Text("Mouse Position: %d x %d", (int)m_primaryUniformData.mousePos[0], (int)m_primaryUniformData.mousePos[1]);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Camera"))
		{
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("SSAO"))
		{
			Text("Noise Scale: %f, %f", m_primaryUniformData.ssaoNoiseScale[0], m_primaryUniformData.ssaoNoiseScale[1]);
			Text("Kernel Size: %d", (int)m_primaryUniformData.ssaoKernelSize);
			Text("Kernel Radius: %f", m_primaryUniformData.ssaoRadius);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Sunlight"))
		{
			ImGui::TreePop();
		}		
	}
}

void CFixedBuffers::Destroy(CVulkanRHI* p_rhi)
{
	DestroyBuffers(p_rhi);
}

bool CFixedBuffers::Update(CVulkanRHI* p_rhi, uint32_t p_scId)
{
	float* cameraViewProj					= const_cast<float*>(&m_primaryUniformData.cameraViewProj.column[0][0]);
	float* cameraProj						= const_cast<float*>(&m_primaryUniformData.cameraProj.column[0][0]);
	float* cameraView						= const_cast<float*>(&m_primaryUniformData.cameraView.column[0][0]);
	float* cameraInvView					= const_cast<float*>(&m_primaryUniformData.cameraInvView.column[0][0]);
	float* sunViewProj						= const_cast<float*>(&m_primaryUniformData.sunViewProj.column[0][0]);
	float* skyboxModelView					= const_cast<float*>(&m_primaryUniformData.skyboxModelView.column[0][0]);
	// cancelling out translation for skybox rendering
	skyboxModelView[12]						= 0.0;
	skyboxModelView[13]						= 0.0;
	skyboxModelView[14]						= 0.0;
	skyboxModelView[15]						= 1.0;

	std::vector<float> uniformValues;
	uniformValues.push_back(m_primaryUniformData.elapsedTime);																										// time elapsed delata
	std::copy(&m_primaryUniformData.cameraLookFrom[0], &m_primaryUniformData.cameraLookFrom[3], std::back_inserter(uniformValues));				// camera look from x, y, z
	std::copy(&cameraViewProj[0], &cameraViewProj[16], std::back_inserter(uniformValues));														// camera view projection matrix
	std::copy(&cameraProj[0], &cameraProj[16], std::back_inserter(uniformValues));																// camera projection matrix
	std::copy(&cameraView[0], &cameraView[16], std::back_inserter(uniformValues));																// camera view matrix
	std::copy(&cameraInvView[0], &cameraInvView[16], std::back_inserter(uniformValues));															// inverse camera view matrix
	std::copy(&skyboxModelView[0], &skyboxModelView[16], std::back_inserter(uniformValues));														// skybox model view
	uniformValues.push_back((float)m_primaryUniformData.renderRes[0]);	uniformValues.push_back((float)m_primaryUniformData.renderRes[1]);						// render resolution
	uniformValues.push_back((float)m_primaryUniformData.mousePos[0]);	uniformValues.push_back((float)m_primaryUniformData.mousePos[1]);						// mouse pos
	uniformValues.push_back((float)m_primaryUniformData.ssaoNoiseScale[0]); uniformValues.push_back((float)m_primaryUniformData.ssaoNoiseScale[1]);				// ssao noise scale
	uniformValues.push_back((float)m_primaryUniformData.ssaoKernelSize);																							// ssao kernel size
	uniformValues.push_back((float)m_primaryUniformData.ssaoRadius);																								// ssao radius
	std::copy(&sunViewProj[0], &sunViewProj[16], std::back_inserter(uniformValues));																// sun light view projection matrix
	std::copy(&m_primaryUniformData.sunDirWorldSpace[0], &m_primaryUniformData.sunDirWorldSpace[3], std::back_inserter(uniformValues));			// sun light sunLightDirection x, y, z in world space
	uniformValues.push_back(m_primaryUniformData.unassigined_0);																									// unassigned_0
	std::copy(&m_primaryUniformData.sunDirViewSpace[0], &m_primaryUniformData.sunDirViewSpace[3], std::back_inserter(uniformValues));				// sun light sunLightDirection x, y, z in view space
	uniformValues.push_back(m_primaryUniformData.unassigined_1);																									// unassigned_1

	uint8_t* data							= (uint8_t*)(uniformValues.data());
	uint32_t id								= (p_scId == 0) ? fb_PrimaryUniform_0 : fb_PrimaryUniform_1;
	RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(data, m_buffers[id]));
		
	return true;
}

bool CFixedAssets::Create(CVulkanRHI* p_rhi)
{
	RETURN_FALSE_IF_FALSE(m_fixedBuffers.Create(p_rhi));
	RETURN_FALSE_IF_FALSE(m_renderTargets.Create(p_rhi));

	{
		m_samplers.resize(SamplerId::s_max);

		struct SamplerData { uint32_t id; VkFilter filter; VkImageLayout layout; };
		std::vector<SamplerData> samplerDataList
		{
			{s_Linear, VK_FILTER_LINEAR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
		};

		for (const auto& samp : samplerDataList)
		{
			m_samplers[samp.id].filter = samp.filter;
			m_samplers[samp.id].maxAnisotropy = 1.0f;
			m_samplers[samp.id].descInfo.imageLayout = samp.layout;
			m_samplers[samp.id].descInfo.imageView = VK_NULL_HANDLE;
			RETURN_FALSE_IF_FALSE(p_rhi->CreateSampler(m_samplers[samp.id]));
		}
	}

	return true;
}

void CFixedAssets::Destroy(CVulkanRHI* p_rhi)
{
	m_fixedBuffers.Destroy(p_rhi);
	m_renderTargets.Destroy(p_rhi);

	for (auto& sampler: m_samplers)
	{
		p_rhi->DestroySampler(sampler.descInfo.sampler);
	}
	m_samplers.clear();
}

CPrimaryDescriptors::CPrimaryDescriptors()
	:CDescriptor(FRAME_BUFFER_COUNT)
{
}

CPrimaryDescriptors::~CPrimaryDescriptors()
{
}

void CPrimaryDescriptors::SetLayoutForDescriptorCreation(CRenderTargets* p_renderTargets)
{
	p_renderTargets->SetLayout(CRenderTargets::rt_DeferredLighting,		VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_LightDepth,			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_SSAOBlur,				VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_SSAO,					VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_Position,				VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_Albedo,				VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_Normal,				VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_PrimaryDepth,			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

bool CPrimaryDescriptors::Create(CVulkanRHI* p_rhi, CFixedAssets& p_fixedAssets, const CLoadableAssets& p_loadableAssets)
{
	const CReadOnlyTextures* readonlyTex				= p_loadableAssets.GetReadonlyTextures();
	CReadOnlyBuffers* readonlyBuf						= const_cast<CReadOnlyBuffers*>(p_loadableAssets.GetReadonlyBuffers());
	CFixedBuffers* fixedBuf								= p_fixedAssets.GetFixedBuffers();
	CRenderTargets* rendTargets							= p_fixedAssets.GetRenderTargets();
	const CVulkanRHI::SamplerList* samplers				= p_fixedAssets.GetSamplers();
	uint32_t tr_max										= CReadOnlyTextures::tr_max;
	
	SetLayoutForDescriptorCreation(rendTargets);

	// read only tex desc info
	std::vector<VkDescriptorImageInfo> readTexDesInfoList;
	for (int i = 0; i < CReadOnlyTextures::tr_max; i++)
	{
		readTexDesInfoList.push_back(readonlyTex->GetTexture(i).descInfo);
	}

	{
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Gloabl_Uniform,			1,		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	VK_SHADER_STAGE_ALL,			&fixedBuf->GetBuffer(CFixedBuffers::fb_PrimaryUniform_0).descInfo,	VK_NULL_HANDLE }	, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Linear_Sampler,			1,		VK_DESCRIPTOR_TYPE_SAMPLER,			VK_SHADER_STAGE_ALL,			VK_NULL_HANDLE,	&(*samplers)[s_Linear].descInfo }										, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_ObjPicker_Storage,		1,		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_FRAGMENT_BIT,	&fixedBuf->GetBuffer(CFixedBuffers::fb_ObjectPickerWrite).descInfo,	VK_NULL_HANDLE}		, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Depth_Image,			1,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_PrimaryDepth).descInfo }	, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PosGBuf_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Position).descInfo }		, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_NormGBuf_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Normal).descInfo }			, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_AlbedoGBuf_Image,		1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Albedo).descInfo }			, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAO_Image,				1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_SSAO).descInfo }			, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAOBlur_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_SSAOBlur).descInfo }		, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAOKernel_Storage,		1,		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_COMPUTE_BIT,	&readonlyBuf->GetBuffer(CReadOnlyBuffers::br_SSAOKernel).descInfo,	VK_NULL_HANDLE}		, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_LightDepth_Image,		1,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_ALL,			VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_LightDepth).descInfo }		, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_DeferredLighting_Image,	1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_DeferredLighting).descInfo }, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PrimaryRead_TexArray,	tr_max,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	readTexDesInfoList.data() }												, 0);

		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, tr_max, BindingDest::bd_PrimaryRead_TexArray, 0));
	}

	{
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Gloabl_Uniform,			1,		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	VK_SHADER_STAGE_ALL,			&fixedBuf->GetBuffer(CFixedBuffers::fb_PrimaryUniform_1).descInfo,	VK_NULL_HANDLE }	, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Linear_Sampler,			1,		VK_DESCRIPTOR_TYPE_SAMPLER,			VK_SHADER_STAGE_ALL,			VK_NULL_HANDLE,	&(*samplers)[s_Linear].descInfo }										, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_ObjPicker_Storage,		1,		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_FRAGMENT_BIT,	&fixedBuf->GetBuffer(CFixedBuffers::fb_ObjectPickerWrite).descInfo,	VK_NULL_HANDLE}		, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Depth_Image,			1,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_PrimaryDepth).descInfo }	, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PosGBuf_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Position).descInfo }		, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_NormGBuf_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Normal).descInfo }			, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_AlbedoGBuf_Image,		1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Albedo).descInfo }			, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAO_Image,				1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_SSAO).descInfo }			, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAOBlur_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_SSAOBlur).descInfo }		, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAOKernel_Storage,		1,		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_COMPUTE_BIT,	&readonlyBuf->GetBuffer(CReadOnlyBuffers::br_SSAOKernel).descInfo,	VK_NULL_HANDLE}		, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_LightDepth_Image,		1,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_ALL,			VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_LightDepth).descInfo }		, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_DeferredLighting_Image,	1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_DeferredLighting).descInfo }, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PrimaryRead_TexArray,	tr_max,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	readTexDesInfoList.data() }												, 1);

		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, tr_max, BindingDest::bd_PrimaryRead_TexArray, 1));
	}

	UnSetLayoutForDescriptorCreation(rendTargets);

	return true;
}

void CPrimaryDescriptors::UnSetLayoutForDescriptorCreation(CRenderTargets* p_renderTargets)
{
	p_renderTargets->SetLayout(CRenderTargets::rt_DeferredLighting	, VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_LightDepth		, VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_SSAOBlur			, VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_SSAO				, VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_Position			, VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_Albedo			, VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_Normal			, VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_PrimaryDepth		, VK_IMAGE_LAYOUT_UNDEFINED);
}


void CPrimaryDescriptors::Destroy(CVulkanRHI* p_rhi)
{
	DestroyDescriptors(p_rhi);
}

CUIParticipant::CUIParticipant()
{
	CUIParticipantManager::m_uiParticipants.push_back(this);
};
CUIParticipant::~CUIParticipant()
{
	for (int i = 0; i < CUIParticipantManager::m_uiParticipants.size(); i++)
	{
		if (CUIParticipantManager::m_uiParticipants[i] == this)
		{
			CUIParticipantManager::m_uiParticipants.erase(CUIParticipantManager::m_uiParticipants.begin() + i);
			return;
		}
	}
}

bool CUIParticipant::Header(const char* caption)
{
	return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
}

bool CUIParticipant::CheckBox(const char* caption, bool* value)
{
	bool res = ImGui::Checkbox(caption, value);
	if (res) { m_updated = true; };
	return res;
}

bool CUIParticipant::CheckBox(const char* caption, int32_t* value)
{
	bool val = (*value == 1);
	bool res = ImGui::Checkbox(caption, &val);
	*value = val;
	if (res) { m_updated = true; };
	return res;
}

bool CUIParticipant::RadioButton(const char* caption, bool value)
{
	bool res = ImGui::RadioButton(caption, value);
	if (res) { m_updated = true; };
	return res;
}

//bool CUIParticipant::InputFloat(const char* caption, float* value, float step, uint32_t precision)
//{
//	bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
//	if (res) { m_updated = true; };
//	return res;
//}

bool CUIParticipant::SliderFloat(const char* caption, float* value, float min, float max)
{
	bool res = ImGui::SliderFloat(caption, value, min, max);
	if (res) { m_updated = true; };
	return res;
}

bool CUIParticipant::SliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
{
	bool res = ImGui::SliderInt(caption, value, min, max);
	if (res) { m_updated = true; };
	return res;
}

bool CUIParticipant::ComboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items)
{
	if (items.empty()) {
		return false;
	}
	std::vector<const char*> charitems;
	charitems.reserve(items.size());
	for (size_t i = 0; i < items.size(); i++) {
		charitems.push_back(items[i].c_str());
	}
	uint32_t itemCount = static_cast<uint32_t>(charitems.size());
	bool res = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
	if (res) { m_updated = true; };
	return res;
}

bool CUIParticipant::Button(const char* caption)
{
	bool res = ImGui::Button(caption);
	if (res) { m_updated = true; };
	return res;
}

void CUIParticipant::Text(const char* formatstr, ...)
{
	va_list args;
	va_start(args, formatstr);
	ImGui::TextV(formatstr, args);
	va_end(args);
}

CUIParticipantManager::CUIParticipantManager()
{

}

CUIParticipantManager::~CUIParticipantManager()
{
	m_uiParticipants.clear();
}

void CUIParticipantManager::Show()
{
	for (auto& participant : m_uiParticipants)
	{
		participant->Show();
		ImGui::Separator();
	}
}