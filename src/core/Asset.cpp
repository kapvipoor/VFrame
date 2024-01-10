#include "Asset.h"
#include "SceneGraph.h"
#include "RandGen.h"
#include <algorithm>

#include "external/imgui/imgui.h"
#include "external/imguizmo/ImGuizmo.h"

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
	:m_instanceCount(0)
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
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeof(float) * p_meshRaw->vertexList.size(), vertexStg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

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

	// Doing this because; if the id is set to -1, then the intent is to grow the buffer at runtime and not a fixed size
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

bool CTextures::CreateCubemap(CVulkanRHI* p_rhi, CVulkanRHI::Buffer& p_stg, const std::vector<ImageRaw>& p_rawList, const CVulkanRHI::SamplerList& p_samplers, CVulkanRHI::CommandBuffer& p_cmdBfr, int p_id)
{
	if (p_rawList.size() != 6)
	{
		std::cout << "Error: Cannot create cube map because raw data has slices less/more than 6" << std::endl;
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
	imgInfo.extent.width							= p_rawList[0].width;					// assuming all the loaded cube map images have same width
	imgInfo.extent.height							= p_rawList[0].height;					// assuming all the loaded cube map images have same height
	imgInfo.arrayLayers								= 6;
	imgInfo.flags									= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imgInfo.usage									= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imgInfo.format									= VK_FORMAT_R8G8B8A8_UNORM;

	RETURN_FALSE_IF_FALSE(p_rhi->CreateTexture(p_stg, cubemap, imgInfo, p_cmdBfr));
	// Doing this because; if the id is set to -1, then the intent is to grow the texture list at runtime and not a fixed size
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

CRenderableUI::CRenderableUI()
	: CRenderable(FRAME_BUFFER_COUNT)
	, CSelectionListener()
	, m_showImguiDemo(false)
	, m_latestFPS(5)
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
		std::cout << "Error: Failed to create UI Descriptors" << std::endl;
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

bool CRenderableUI::Update(CVulkanRHI* p_rhi, const LoadedUpdateData& p_loadedUpdate, CFixedBuffers::PrimaryUniformData& p_primUniData)
{
	m_latestFPS.Add(p_loadedUpdate.timeElapsed);

	m_primUniforms							= p_primUniData;
	ImGuiIO& imguiIO						= ImGui::GetIO();
	imguiIO.DisplaySize						= ImVec2(p_loadedUpdate.screenRes[0], p_loadedUpdate.screenRes[1]);
	imguiIO.DeltaTime						= p_loadedUpdate.timeElapsed;
	imguiIO.MousePos						= ImVec2(p_loadedUpdate.curMousePos[0], p_loadedUpdate.curMousePos[1]);
	imguiIO.MouseDown[0]					= p_loadedUpdate.isLeftMouseDown;
	imguiIO.MouseDown[1]					= p_loadedUpdate.isRightMouseDown;

	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	ShowUI(p_rhi);
	ShowGuizmo(p_rhi);

	return true;
}

bool CRenderableUI::LoadFonts(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	std::cout << "Loading UI Resources" << std::endl;

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

bool CRenderableUI::ShowGuizmo(CVulkanRHI* p_rhi)
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
		ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
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

		nm::float4x4 camView		= m_primUniforms.cameraView;
		nm::float4x4 camProj		= m_primUniforms.cameraProj;
		nm::Transform transform		= m_selectedEntity->GetTransform();

		if (m_guizmo.type == Guizmo::Translation)
		{
			nm::float4x4 translation = transform.GetTranslate();
			if (ImGuizmo::Manipulate(&camView.column[0][0], &camProj.column[0][0], ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::LOCAL, &translation.column[0][0]))
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
			nm::float4x4 rotation = transform.GetTransform();
			if (ImGuizmo::Manipulate(&camView.column[0][0], &camProj.column[0][0], ImGuizmo::OPERATION::ROTATE, ImGuizmo::LOCAL, &rotation.column[0][0]))
			{
				if (!(transform.GetTransform() == rotation))
				{
					transform.SetTransform(rotation);
					m_selectedEntity->SetTransform(transform);
				}
			}
		}
		else if (m_guizmo.type == Guizmo::Scale)
		{
			nm::float4x4 scaling = transform.GetTransform();
			if (ImGuizmo::Manipulate(&camView.column[0][0], &camProj.column[0][0], ImGuizmo::OPERATION::SCALE, ImGuizmo::LOCAL, &scaling.column[0][0]))
			{
				if (!(transform.GetScale() == scaling))
				{
					transform.SetTransform(scaling);
					m_selectedEntity->SetTransform(transform);
				}
			}
		}
	}

	return true;
}

bool CRenderableUI::ShowUI(CVulkanRHI* p_rhi)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10, 25));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_::ImGuiCond_FirstUseEver);
	ImGui::Begin("VFrame Interface", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
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
	}

	{
		ImGui::TextUnformatted("VFrame Analysis");
		{
			float data[5];
			m_latestFPS.Data(data);
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
				sprintf(overlay, "avg %f", m_latestFPS.Average());
				ImGui::PlotLines("cpu (ms)", data, IM_ARRAYSIZE(data), values_offset, overlay, 0.0f, 32.0f, ImVec2(0, 60.0f));
			}
		}
		ImGui::Separator();
	}

	ImGui::TextUnformatted("VFrame Settings");
	ImGui::PushItemWidth(110.0f * 1.0f);
	m_participantManager->Show();
	ImGui::PopItemWidth();
	ImGui::End();
	ImGui::PopStyleVar();

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
			size_t verteSize				= drawData->TotalVtxCount * sizeof(ImDrawVert);
			size_t indexSize				= drawData->TotalIdxCount * sizeof(ImDrawIdx);

			// if the vertex buffer if already created from previous frame, destroy them and free the associated memory for this frame's use
			if (m_vertexBuffers[p_scIdx].descInfo.buffer != VK_NULL_HANDLE)
			{
				Clear(p_rhi, p_scIdx);
			}

			// Creating vertex and index buffers and upload all data to single continuous GPU buffers respectively
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

CRenderableMesh::CRenderableMesh(std::string p_name, uint32_t p_meshId, nm::float4x4 p_modelMat)
	: CEntity(p_name)
	, m_mesh_id(p_meshId)
{
	CEntity::m_transform.SetTransform(p_modelMat);
}

//void CRenderableMesh::Show()
//{
//	CEntity::Show();
//}

CScene::CScene(CSceneGraph* p_sceneGraph)
	: CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame)
	, m_sceneGraph(p_sceneGraph)
{
	m_sceneTextures = new CTextures();
	m_sceneLights = new CLights();
}

CScene::~CScene()
{
	delete m_sceneLights;
	delete m_sceneTextures;
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
		std::cout << "Error: Failed to Load Sky box" << std::endl;
		return false;
	}

	if (!LoadScene(p_rhi, stgList, cmdBfr))
	{
		std::cout << "Error: Failed to Load Scene" << std::endl;
		return false;
	}

	if (!LoadLights(p_rhi, stgList, cmdBfr))
	{
		std::cout << "Error: Failed to Load Lights" << std::endl;
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
	m_sceneTextures->DestroyTextures(p_rhi);

	for (auto& mesh : m_meshes)
	{
		mesh->DestroyRenderable(p_rhi);
		delete mesh;
	}
	m_meshes.clear();

	p_rhi->FreeMemoryDestroyBuffer(m_material_storage);
	p_rhi->FreeMemoryDestroyBuffer(m_meshInfo_uniform);
}

bool draw[5] = { false, true, true, false, false };
void CScene::Show()
{
	bool doubleClickedEntity = false;
	int32_t sceneIndex = 0;
	if(Header("Scene Settings"))
	{
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
				{
					bool drawBBox = entity->IsDebugDrawEnabled();
					std::string str = std::to_string(i) + ". ";
					ImGui::Checkbox(str.c_str(), &drawBBox);
					entity->SetDebugDrawEnable(drawBBox);
					ImGui::SameLine(65);
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
					entity->Show();
					
					// Display sub-mesh data
					if (dynamic_cast<CRenderableMesh*>(m_selectedEntity))
					{				
						CRenderableMesh* mesh = dynamic_cast<CRenderableMesh*>(m_selectedEntity);

						ImGui::Indent();

						bool drawSubBBox = m_selectedEntity->IsSubmeshDebugDrawEnabled();
						ImGui::Checkbox(" ", &drawSubBBox);
						m_selectedEntity->SetSubmeshDebugDrawEnable(drawSubBBox);
						ImGui::SameLine(75);

						std::string treeNodeName = "Sub-meshes (" + std::to_string(mesh->GetSubmeshCount()) + ")";
						if (ImGui::TreeNode(treeNodeName.c_str()))
						{
							for (uint32_t i = 0; i < mesh->GetSubmeshCount(); i++)
							{
								if (ImGui::Selectable(mesh->GetSubmesh(i)->name.c_str(), m_selectedSubMeshIndex == i, ImGuiSelectableFlags_AllowDoubleClick))
								{
									m_selectedSubMeshIndex = i;
								}

								if (m_selectedSubMeshIndex == i)
								{
									ImGui::Indent();

									int matId = mesh->GetSubmesh(i)->materialId;
									Material mat = m_materialsList[matId];
									ImGui::Text("Albedo Id %d", mat.color_id);
									ImGui::Text("Normal Id %d", mat.normal_id);
									ImGui::Text("Metal/Rough Id %d", mat.roughMetal_id);
									ImGui::InputFloat("Metallic ", &mat.metallic);
									ImGui::InputFloat("Roughness ", &mat.roughness);
									ImGui::InputFloat3("Color ", &mat.color[0]);
									ImGui::InputFloat3("PBR Color ", &mat.pbr_color[0]);

									ImGui::Unindent();
								}
							}
							ImGui::TreePop();
						}

						ImGui::Unindent();
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
	m_sceneLights->Update(p_loadedUpdate.cameraData);
	if (m_sceneLights->IsDirty())
	{
		CVulkanRHI::CommandBuffer cmdBfr;
		CVulkanRHI::BufferList stgList;

		RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffers(p_loadedUpdate.commandPool, &cmdBfr, 1));

		RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, "Scene Loading"));

		// Every time any light is dirty(has change in transform, color or intensity), 
		// the raw data is updated and the entire GPU resource is reloaded
		if (!LoadLights(p_rhi, stgList, cmdBfr))
		{
			std::cout << "Error: Failed to Load Lights" << std::endl;
			return false;
		}		

		if (!p_rhi->EndCommandBuffer(cmdBfr))
			return false;

		VkQueue queue = p_rhi->GetQueue(0);
		VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBfr;
		submitInfo.pSignalSemaphores = nullptr;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitDstStageMask = &waitstage;
		RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandbuffer(queue, &submitInfo, 1));

		RETURN_FALSE_IF_FALSE(p_rhi->WaitToFinish(queue));

		for (auto& stg : stgList)
			p_rhi->FreeMemoryDestroyBuffer(stg);

		m_sceneLights->SetDirty(false);
	}

	std::vector<float> perMeshUniformData;
	for (auto& mesh : m_meshes)
	{
		mesh->SetDirty(false);
		mesh->m_viewNormalTransform					= (p_loadedUpdate.viewMatrix * mesh->GetTransform().GetTransform());	// nm::inverse(nm::transpose(p_loadedUpdate.viewMatrix * mesh->GetTransform().GetTransform()));

		const float* modelMat						= &(m_sceneGraph->GetTransform().GetTransform()
														* mesh->GetTransform().GetTransform()).column[0][0];

		const float* trn_inv_model					= &mesh->m_viewNormalTransform.column[0][0];							// this needs to be inverse transpose so as to negate the scaling in the matrix before multiplying with normal. But this isn't working and I do not know why !

		std::copy(&modelMat[0], &modelMat[16], std::back_inserter(perMeshUniformData));										// model matrix for this mesh
		std::copy(&trn_inv_model[0], &trn_inv_model[16], std::back_inserter(perMeshUniformData));							// Transpose(inverse(view * model)) for transforming normal to view space
	}

	uint8_t* data = (uint8_t*)(perMeshUniformData.data());
	RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(data, m_meshInfo_uniform, true));

	return true;
}

bool CScene::LoadDefaultTexture(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	std::cout << "Loading Default resources" << std::endl;

	// load default texture to compensate for bad textures
	{
		CVulkanRHI::Buffer stg;
		ImageRaw tex;
		
		RETURN_FALSE_IF_FALSE(LoadRawImage((g_DefaultPath / "tex_not_found.png").string().c_str(), tex));
		RETURN_FALSE_IF_FALSE(m_sceneTextures->CreateTexture(p_rhi, stg, &tex, VK_FORMAT_B8G8R8A8_SRGB,p_cmdBfr));
		
		FreeRawImage(tex);
		p_stgList.push_back(stg);
	}

	return true;
}

bool CScene::LoadSkybox(CVulkanRHI* p_rhi, const CVulkanRHI::SamplerList* p_samplerList, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	std::cout << "Loading Skybox Resources" << std::endl;

	// Load sky box cube map
	{
		std::filesystem::path cubemap_path[6]{
			g_AssetPath / "skybox/Daylight_Box_Pieces/f.png"
		,	g_AssetPath / "skybox/Daylight_Box_Pieces//b.png"
		,	g_AssetPath / "skybox/Daylight_Box_Pieces/t.png"
		,	g_AssetPath / "skybox/Daylight_Box_Pieces/bt.png"
		,	g_AssetPath / "skybox/Daylight_Box_Pieces/l.png"
		,	g_AssetPath / "skybox/Daylight_Box_Pieces/r.png"
		};

		std::vector<ImageRaw> cubemap_raw;
		cubemap_raw.resize(6);
		for (int i = 0; i < 6; i++)
		{
			RETURN_FALSE_IF_FALSE(LoadRawImage(cubemap_path[i].string().c_str(), cubemap_raw[i]));
		}

		CVulkanRHI::Buffer stg;
		RETURN_FALSE_IF_FALSE(m_sceneTextures->CreateCubemap(p_rhi, stg, cubemap_raw, *p_samplerList, p_cmdBfr));

		p_stgList.push_back(stg);
	}

	// Load skybox geometry
	{
		SceneRaw sceneraw;
		ObjLoadData loadData{};
		loadData.flipUV					= false;
		loadData.loadMeshOnly			= true;

		RETURN_FALSE_IF_FALSE(LoadObj((g_AssetPath / "skybox/skybox.obj").string().c_str(), sceneraw, loadData));

		MeshRaw meshraw					= sceneraw.meshList[0];
		CRenderableMesh* mesh			= new CRenderableMesh("Skybox", MeshType::mt_Skybox, nm::float4x4::identity());
		RETURN_FALSE_IF_FALSE(mesh->CreateVertexIndexBuffer(p_rhi, p_stgList, &meshraw, p_cmdBfr));
		m_meshes.push_back(mesh);
	}

	return true;
}

bool CScene::LoadScene(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr, bool p_dumpBinaryToDisk)
{
	m_scenePaths.push_back(g_AssetPath/"glTFSampleModels / 2.0 / DragonAttenuation / glTF / DragonAttenuation.gltf");				//0
	m_scenePaths.push_back(g_AssetPath/"shadow_test_3.gltf");																		//1
	m_scenePaths.push_back(g_AssetPath/"glTFSampleModels/2.0/TransmissionTest/glTF/TransmissionTest.gltf");							//2
	m_scenePaths.push_back(g_AssetPath/"glTFSampleModels/2.0/NormalTangentMirrorTest/glTF/NormalTangentMirrorTest.gltf");			//3
	m_scenePaths.push_back(g_AssetPath/"glTFSampleModels/2.0/Suzanne/glTF/Suzanne.gltf");											//4
	m_scenePaths.push_back(g_AssetPath/"Sponza/glTF/Sponza.gltf");																	//5
	m_scenePaths.push_back(g_AssetPath/"glTFSampleModels/2.0/DamagedHelmet/glTF/DamagedHelmet_withTangents.gltf");					//6
	m_scenePaths.push_back(g_AssetPath/"cube/cube.obj");																			//7
	m_scenePaths.push_back(g_AssetPath/"icosphere.gltf");																			//8
	m_scenePaths.push_back(g_AssetPath/"dragon/dragon.obj");																		//9
	m_scenePaths.push_back(g_AssetPath/"stanford_dragon_pbr/scene.gltf");															//10
	m_scenePaths.push_back(g_AssetPath/"mitsuba/mitsuba.obj");																	//11

	std::vector<std::filesystem::path> paths;
	//paths.push_back(m_scenePaths[5]);
	//paths.push_back(m_scenePaths[2]);
	paths.push_back(m_scenePaths[4]);
	//paths.push_back(m_scenePaths[8]);

	std::vector<bool> flipYList{ false, false, false };

	SceneRaw sceneraw;
	for (unsigned int i = 0; i < paths.size(); i++)
	{
		std::cout << "Loading Scene Resources - " << paths[i] << std::endl;

		ObjLoadData loadData{};
		loadData.flipUV = flipYList[i];
		loadData.loadMeshOnly = false;

		if (paths[i].extension() == ".gltf")
		{
			RETURN_FALSE_IF_FALSE(LoadGltf(paths[i].string().c_str(), sceneraw, loadData));
		}
		else if (paths[i].extension() == ".obj") 
		{
			RETURN_FALSE_IF_FALSE(LoadObj(paths[i].string().c_str(), sceneraw, loadData));
		}
		else
		{
			std::cerr << "Invalid file extension - " << paths[i] << std::endl;
			return false;
		}

		if (p_dumpBinaryToDisk)
		{
			std::string outName = "D:/" + paths[i].stem().string();
			WriteToDisk(std::filesystem::path(outName + "_vertex_float_p3_n3_uv2_t4.charp"), sizeof(Vertex) * sceneraw.meshList[i].vertexList.size(), (char*)sceneraw.meshList[i].vertexList.data());
			WriteToDisk(std::filesystem::path(outName + "_index_uint32.charp"), sizeof(uint32_t) * sceneraw.meshList[i].indicesList.size(), (char*)sceneraw.meshList[i].indicesList.data());
		}
	}
	
	for (auto& meshraw : sceneraw.meshList)
	{
		CRenderableMesh* mesh = new CRenderableMesh(meshraw.name, (uint32_t)m_meshes.size(), meshraw.transform);
		mesh->m_submeshes = meshraw.submeshes;

		mesh->SetBoundingBox(meshraw.bbox);
		for (auto& bbox : meshraw.submeshesBbox)
		{
			mesh->SetSubBoundingBox(bbox);
		}

		RETURN_FALSE_IF_FALSE(mesh->CreateVertexIndexBuffer(p_rhi, p_stgList, &meshraw, p_cmdBfr));
		m_meshes.push_back(mesh);
	}
	
	for (const auto& tex : sceneraw.textureList)
	{
		CVulkanRHI::Image img;
		{
			if (tex.raw != nullptr)
			{
				CVulkanRHI::Buffer stg;
				RETURN_FALSE_IF_FALSE(m_sceneTextures->CreateTexture(p_rhi, stg, &tex, VK_FORMAT_R8G8B8A8_UNORM, p_cmdBfr));
				p_stgList.push_back(stg);
			}
			else
			{
				m_sceneTextures->PushBackPreLoadedTexture(TextureType::tt_default);
			}
		}
	}

	// storage buffer for materials
	{
		m_materialsList = sceneraw.materialsList;
		CVulkanRHI::Buffer matStg;
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(sizeof(Material) * sceneraw.materialsList.size(), matStg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer((uint8_t*)sceneraw.materialsList.data(), matStg));

		p_stgList.push_back(matStg);

		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(matStg.descInfo.range, m_material_storage, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

		RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(matStg, m_material_storage, p_cmdBfr));
	}

	// can binary dump in future to optimized format for faster binary loading
	sceneraw.meshList.clear();

	for(auto& tex : sceneraw.textureList)
		FreeRawImage(tex);

	sceneraw.textureList.clear();

	return true;
}

bool CScene::LoadLights(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer& p_cmdBfr, bool p_dumpBinaryToDisk)
{
	std::cout << "Loading Light resources" << std::endl;

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
		RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(bufferSize, lightStg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(rawData, lightStg));

		p_stgbufferList.push_back(lightStg);

		if (m_light_storage.descInfo.buffer == VK_NULL_HANDLE) 
		{
			RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(lightStg.descInfo.range, m_light_storage, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		}

		RETURN_FALSE_IF_FALSE(p_rhi->UploadFromHostToDevice(lightStg, m_light_storage, p_cmdBfr));

		delete[] rawData;
	}

	return true;
}

bool CScene::CreateMeshUniformBuffer(CVulkanRHI* p_rhi)
{
	size_t uniBufize = m_meshes.size() * (
		(sizeof(float) * 16)	// model matrix
	+	(sizeof(float) * 16)	// transpose(inverse(model)) for transforming normal to world space
		);

	RETURN_FALSE_IF_FALSE(p_rhi->CreateAllocateBindBuffer(uniBufize, m_meshInfo_uniform, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	return true;
}

bool CScene::CreateSceneDescriptors(CVulkanRHI* p_rhi)
{
	std::vector<VkDescriptorImageInfo> imageInfoList;
	CVulkanRHI::ImageList sceneTexturesList = m_sceneTextures->GetTextures();
	uint32_t texturesCount = 0;
	for (int i = TextureType::tt_scene; i < sceneTexturesList.size(); i++)
	{
		imageInfoList.push_back(sceneTexturesList[i].descInfo);
		texturesCount++;
	}

	VkShaderStageFlags vertex_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Scene_MeshInfo_Uniform,	1,				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			vertex_frag,					&m_meshInfo_uniform.descInfo,	VK_NULL_HANDLE });
	AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_CubeMap_Texture,			1,				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	VK_SHADER_STAGE_FRAGMENT_BIT,	VK_NULL_HANDLE,					&sceneTexturesList[TextureType::tt_skybox].descInfo });
	AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Material_Storage,			1,				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			VK_SHADER_STAGE_FRAGMENT_BIT,	&m_material_storage.descInfo,	VK_NULL_HANDLE });
	AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Scene_Lights,				1,				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			VK_SHADER_STAGE_FRAGMENT_BIT,	&m_light_storage.descInfo,		VK_NULL_HANDLE });
	AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SceneRead_TexArray,		texturesCount,	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,			VK_SHADER_STAGE_FRAGMENT_BIT,	VK_NULL_HANDLE,					imageInfoList.data() });

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

	RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, "Loading Read-only Textures"));

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
	uint32_t ssaoNoiseDim						= 4;
	p_primaryUniformData.ssaoNoiseScale			= nm::float2((float)p_rhi->GetRenderWidth() / (float)ssaoNoiseDim, (float)p_rhi->GetRenderHeight() / (float)ssaoNoiseDim);

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
	ImageRaw raw{ ssaoNoise.data(), (int)ssaoNoiseDim , (int)ssaoNoiseDim, 4 };
	
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

	RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, "Loading Read-only Buffers"));

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
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, staging, ssaoKernel.data(), sizeof(float) * 4 * ssaoKernel.size(), p_cmdBfr, CReadOnlyBuffers::br_SSAOKernel));
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
	m_ui.Destroy(p_rhi);
	m_readOnlyBuffers.Destroy(p_rhi);
	m_readOnlyTextures.Destroy(p_rhi);
}

bool CLoadableAssets::Update(CVulkanRHI* p_rhi, const LoadedUpdateData& p_updateData, CFixedBuffers::PrimaryUniformData& p_primUniData)
{
	RETURN_FALSE_IF_FALSE(m_scene.Update(p_rhi, p_updateData));
	RETURN_FALSE_IF_FALSE(m_ui.Update(p_rhi, p_updateData, p_primUniData));
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

	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_PrimaryDepth,			VK_FORMAT_D32_SFLOAT_S8_UINT,	fullResWidth, fullResHeight,	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_Position,				VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight,	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_Normal,					VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight,	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_Albedo,					VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight,	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_SSAO,					VK_FORMAT_R32_SFLOAT,			fullResWidth, fullResHeight,	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_SSAOBlur,				VK_FORMAT_R32_SFLOAT,			fullResWidth, fullResHeight,	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_DirectionalShadowDepth,	VK_FORMAT_D32_SFLOAT_S8_UINT,	512, 512,						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_PrimaryColor,			VK_FORMAT_R32G32B32A32_SFLOAT,	fullResWidth, fullResHeight,	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
	RETURN_FALSE_IF_FALSE(CreateRenderTarget(p_rhi, rt_DeferredRoughMetal,		VK_FORMAT_R16G16_SFLOAT,		fullResWidth, fullResHeight,	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));

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
	: CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame)
	, CBuffers(CFixedBuffers::FixedBufferId::fb_max)
{
	m_primaryUniformData.ssaoKernelSize			= 64.0f;
	m_primaryUniformData.ssaoRadius				= 0.5f;
	m_primaryUniformData.enableShadowPCF		= 0;
	m_primaryUniformData.sunIntensity			= 20.0f;
	m_primaryUniformData.pbrAmbientFactor		= 0.03f;
	m_primaryUniformData.enableSSAO				= 1;
	m_primaryUniformData.biasSSAO				= 0.015f;
	m_primaryUniformData.unassigned_1			= 0.0f;
}

CFixedBuffers::~CFixedBuffers()
{
}

bool CFixedBuffers::Create(CVulkanRHI* p_rhi)
{
	size_t primaryUniformBufferSize =
		(sizeof(float) * 1)					// delta time elapsed
		+ (sizeof(float) * 3)				// camera look-from
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
		+ (sizeof(int) * 1)					// enabled PCF for shadow
		+ (sizeof(float) * 3)				// sun light direction in view space
		+ (sizeof(float) * 1)				// sun intensity
		+ (sizeof(float) * 1)				// PBR ambient Factor
		+ (sizeof(int) * 1)					// enable SSAO
		+ (sizeof(float) * 1)				// unassigned_0
		+ (sizeof(float) * 1);				// unassigned_1

	size_t objPickerBufferSize = sizeof(uint32_t) * 1; // selected mesh ID
	size_t debugDrawUniformSize = MAX_SUPPORTED_DEBUG_DRAW_ENTITES * ((sizeof(float) * 16)); // storing transforms

	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, fb_PrimaryUniform_0,	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, primaryUniformBufferSize));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, fb_PrimaryUniform_1,	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, primaryUniformBufferSize));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, fb_ObjectPickerRead,	VK_BUFFER_USAGE_TRANSFER_DST_BIT,										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,										objPickerBufferSize));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, fb_ObjectPickerWrite,	VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,										objPickerBufferSize));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, fb_DebugUniform_0,	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, debugDrawUniformSize));
	RETURN_FALSE_IF_FALSE(CreateBuffer(p_rhi, fb_DebugUniform_1,	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, debugDrawUniformSize));

	return true;
}

void CFixedBuffers::Show()
{
	if (Header("Uniforms"))
	{
		ImGui::Indent();
		if (ImGui::TreeNode("General"))
		{
			ImGui::InputFloat("Time Elapsed", &m_primaryUniformData.elapsedTime);
			ImGui::InputFloat2("Render Resolution", &m_primaryUniformData.renderRes[0]);
			ImGui::InputFloat2("Mouse Position", &m_primaryUniformData.mousePos[0]);

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Camera"))
		{
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("SSAO"))
		{
			bool enableSSAO = (bool)m_primaryUniformData.enableSSAO;
			ImGui::Checkbox("Enable SSAO ", &enableSSAO);
			m_primaryUniformData.enableSSAO = (int)enableSSAO;

			float ssaoBias = m_primaryUniformData.biasSSAO;
			ImGui::SliderFloat("SSAO Bias", &ssaoBias, 0.00f, 0.1f);
			m_primaryUniformData.biasSSAO = ssaoBias;

			ImGui::InputFloat2("Noise Scale", &m_primaryUniformData.ssaoNoiseScale[0]);
			ImGui::InputFloat("Kernel Size", &m_primaryUniformData.ssaoKernelSize);
			ImGui::InputFloat("Kernel Radius", &m_primaryUniformData.ssaoRadius);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Shadow"))
		{
			bool enablePCF = (bool)m_primaryUniformData.enableShadowPCF;
			ImGui::Checkbox("PCF", &enablePCF);
			m_primaryUniformData.enableShadowPCF = (int)enablePCF;
			ImGui::TreePop();

		}
		if (ImGui::TreeNode("Sunlight"))
		{
			ImGui::InputFloat3("World Space", &m_primaryUniformData.sunDirWorldSpace[0]);
			ImGui::SliderFloat("Intensity", &m_primaryUniformData.sunIntensity, 0.0f, 20.0f);
			ImGui::TreePop();
		}		
		if (ImGui::TreeNode("PBR"))
		{
			ImGui::SliderFloat("Ambient Factor", &m_primaryUniformData.pbrAmbientFactor, 0.00f, 1.0f);
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
	float* cameraProj						= const_cast<float*>(&m_primaryUniformData.cameraProj.column[0][0]);
	float* cameraView						= const_cast<float*>(&m_primaryUniformData.cameraView.column[0][0]);
	float* cameraInvView					= const_cast<float*>(&m_primaryUniformData.cameraInvView.column[0][0]);
	float* sunViewProj						= const_cast<float*>(&m_primaryUniformData.sunViewProj.column[0][0]);
	float* skyboxModelView					= const_cast<float*>(&m_primaryUniformData.skyboxModelView.column[0][0]);
	// canceling out translation for skybox rendering
	skyboxModelView[12]						= 0.0;
	skyboxModelView[13]						= 0.0;
	skyboxModelView[14]						= 0.0;
	skyboxModelView[15]						= 1.0;

	std::vector<float> uniformValues;
	uniformValues.push_back(m_primaryUniformData.elapsedTime);																							// time elapsed delta
	std::copy(&m_primaryUniformData.cameraLookFrom[0], &m_primaryUniformData.cameraLookFrom[3], std::back_inserter(uniformValues));						// camera look from x, y, z
	std::copy(&cameraViewProj[0], &cameraViewProj[16], std::back_inserter(uniformValues));																// camera view projection matrix
	std::copy(&cameraProj[0], &cameraProj[16], std::back_inserter(uniformValues));																		// camera projection matrix
	std::copy(&cameraView[0], &cameraView[16], std::back_inserter(uniformValues));																		// camera view matrix
	std::copy(&cameraInvView[0], &cameraInvView[16], std::back_inserter(uniformValues));																// inverse camera view matrix
	std::copy(&skyboxModelView[0], &skyboxModelView[16], std::back_inserter(uniformValues));															// skybox model view
	uniformValues.push_back((float)m_primaryUniformData.renderRes[0]);	uniformValues.push_back((float)m_primaryUniformData.renderRes[1]);				// render resolution
	uniformValues.push_back((float)m_primaryUniformData.mousePos[0]);	uniformValues.push_back((float)m_primaryUniformData.mousePos[1]);				// mouse pos
	uniformValues.push_back((float)m_primaryUniformData.ssaoNoiseScale[0]); uniformValues.push_back((float)m_primaryUniformData.ssaoNoiseScale[1]);		// SSAO noise scale
	uniformValues.push_back((float)m_primaryUniformData.ssaoKernelSize);																				// SSAO kernel size
	uniformValues.push_back((float)m_primaryUniformData.ssaoRadius);																					// SSAO radius
	std::copy(&sunViewProj[0], &sunViewProj[16], std::back_inserter(uniformValues));																	// sun light view projection matrix
	std::copy(&m_primaryUniformData.sunDirWorldSpace[0], &m_primaryUniformData.sunDirWorldSpace[3], std::back_inserter(uniformValues));					// sun light sunLightDirection x, y, z in world space
	uniformValues.push_back((float)m_primaryUniformData.enableShadowPCF);																				// enable PCF for shadows
	std::copy(&m_primaryUniformData.sunDirViewSpace[0], &m_primaryUniformData.sunDirViewSpace[3], std::back_inserter(uniformValues));					// sun light sunLightDirection x, y, z in view space
	uniformValues.push_back(m_primaryUniformData.sunIntensity);																							// Sunlight Intensity
	uniformValues.push_back(m_primaryUniformData.pbrAmbientFactor);																						// PBR Ambient Factor
	uniformValues.push_back((float)m_primaryUniformData.enableSSAO);																					// enable SSAO
	uniformValues.push_back(m_primaryUniformData.biasSSAO);																								// SSAO Bias
	uniformValues.push_back(m_primaryUniformData.unassigned_1);																							// unassigned_1

	uint8_t* data							= (uint8_t*)(uniformValues.data());
	uint32_t id								= (p_scId == 0) ? fb_PrimaryUniform_0 : fb_PrimaryUniform_1;
	RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(data, m_buffers[id], true));
		
	return true;
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
	m_fixedBuffers.SetPrimaryUniformData(*p_updateData.primaryUniData);
	RETURN_FALSE_IF_FALSE(m_fixedBuffers.Update(p_rhi, p_updateData.swapchainIndex));
	return true;
}

bool CFixedAssets::CreateSamplers(CVulkanRHI* p_rhi)
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

	return true;
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
	p_renderTargets->SetLayout(CRenderTargets::rt_DeferredRoughMetal,		VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_PrimaryColor,				VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_DirectionalShadowDepth,	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_SSAOBlur,					VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_SSAO,						VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_Position,					VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_Albedo,					VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_Normal,					VK_IMAGE_LAYOUT_GENERAL);
	p_renderTargets->SetLayout(CRenderTargets::rt_PrimaryDepth,				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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

	// read only texture desc info
	std::vector<VkDescriptorImageInfo> readTexDesInfoList;
	for (int i = 0; i < CReadOnlyTextures::tr_max; i++)
	{
		readTexDesInfoList.push_back(readonlyTex->GetTexture(i).descInfo);
	}

	{
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Gloabl_Uniform,			1,		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	VK_SHADER_STAGE_ALL,										&fixedBuf->GetBuffer(CFixedBuffers::fb_PrimaryUniform_0).descInfo,	VK_NULL_HANDLE }			, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Linear_Sampler,			1,		VK_DESCRIPTOR_TYPE_SAMPLER,			VK_SHADER_STAGE_ALL,										VK_NULL_HANDLE,	&(*samplers)[s_Linear].descInfo }												, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_ObjPicker_Storage,		1,		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_FRAGMENT_BIT,								&fixedBuf->GetBuffer(CFixedBuffers::fb_ObjectPickerWrite).descInfo,	VK_NULL_HANDLE}				, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Depth_Image,				1,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_PrimaryDepth).descInfo }			, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PosGBuf_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Position).descInfo }				, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_NormGBuf_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Normal).descInfo }					, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_AlbedoGBuf_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Albedo).descInfo }					, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAO_Image,				1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_SSAO).descInfo }					, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAOBlur_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_SSAOBlur).descInfo }				, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAOKernel_Storage,		1,		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_COMPUTE_BIT,								&readonlyBuf->GetBuffer(CReadOnlyBuffers::br_SSAOKernel).descInfo,	VK_NULL_HANDLE}				, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_LightDepth_Image,			1,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_ALL,										VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_DirectionalShadowDepth).descInfo }	, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PrimaryColor_Image,		1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_PrimaryColor).descInfo }			, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PrimaryColor_Texture,		1,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_FRAGMENT_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_PrimaryColor).descInfo }			, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_DeferredRoughMetal_Image,	1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_DeferredRoughMetal).descInfo }		, 0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PrimaryRead_TexArray,	tr_max,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	readTexDesInfoList.data() }														, 0);

		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, tr_max, BindingDest::bd_PrimaryRead_TexArray, 0));
	}

	{
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Gloabl_Uniform,			1,		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	VK_SHADER_STAGE_ALL,										&fixedBuf->GetBuffer(CFixedBuffers::fb_PrimaryUniform_1).descInfo,	VK_NULL_HANDLE }			, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Linear_Sampler,			1,		VK_DESCRIPTOR_TYPE_SAMPLER,			VK_SHADER_STAGE_ALL,										VK_NULL_HANDLE,	&(*samplers)[s_Linear].descInfo }												, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_ObjPicker_Storage,		1,		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_FRAGMENT_BIT,								&fixedBuf->GetBuffer(CFixedBuffers::fb_ObjectPickerWrite).descInfo,	VK_NULL_HANDLE}				, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Depth_Image,				1,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_PrimaryDepth).descInfo }			, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PosGBuf_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Position).descInfo }				, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_NormGBuf_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Normal).descInfo }					, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_AlbedoGBuf_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_Albedo).descInfo }					, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAO_Image,				1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_SSAO).descInfo }					, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAOBlur_Image,			1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_SSAOBlur).descInfo }				, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_SSAOKernel_Storage,		1,		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_COMPUTE_BIT,								&readonlyBuf->GetBuffer(CReadOnlyBuffers::br_SSAOKernel).descInfo,	VK_NULL_HANDLE}				, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_LightDepth_Image,			1,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_ALL,										VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_DirectionalShadowDepth).descInfo }	, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PrimaryColor_Image,		1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_PrimaryColor).descInfo }			, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PrimaryColor_Texture,		1,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_FRAGMENT_BIT,								VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_PrimaryColor).descInfo }			, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_DeferredRoughMetal_Image,	1,		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,	&rendTargets->GetTexture(CRenderTargets::rt_DeferredRoughMetal).descInfo }		, 1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_PrimaryRead_TexArray,	tr_max,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,								VK_NULL_HANDLE,	readTexDesInfoList.data() }														, 1);

		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, tr_max, BindingDest::bd_PrimaryRead_TexArray, 1));
	}

	UnSetLayoutForDescriptorCreation(rendTargets);

	return true;
}

void CPrimaryDescriptors::UnSetLayoutForDescriptorCreation(CRenderTargets* p_renderTargets)
{
	p_renderTargets->SetLayout(CRenderTargets::rt_DeferredRoughMetal,		VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_PrimaryColor,				VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_DirectionalShadowDepth,	VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_SSAOBlur				,	VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_SSAO					,	VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_Position				,	VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_Albedo				,	VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_Normal				,	VK_IMAGE_LAYOUT_UNDEFINED);
	p_renderTargets->SetLayout(CRenderTargets::rt_PrimaryDepth			,	VK_IMAGE_LAYOUT_UNDEFINED);
}

void CPrimaryDescriptors::Destroy(CVulkanRHI* p_rhi)
{
	DestroyDescriptors(p_rhi);
}

CRenderableDebug::CRenderableDebug()
	: CDescriptor(FRAME_BUFFER_COUNT)
#ifdef INSTANCED_DEBUG_DRAW
	, CRenderable(0)
#else
	, CRenderable(FRAME_BUFFER_COUNT)
#endif
	
{
}

bool CRenderableDebug::Create(CVulkanRHI* p_rhi, const CFixedBuffers* p_fixedBuffers, const CVulkanRHI::CommandPool& p_cmdPool)
{
	CVulkanRHI::CommandBuffer cmdBfr;
	CVulkanRHI::BufferList stgList;
	RETURN_FALSE_IF_FALSE(p_rhi->CreateCommandBuffers(p_cmdPool, &cmdBfr, 1));
	RETURN_FALSE_IF_FALSE(p_rhi->BeginCommandBuffer(cmdBfr, "Debug Resources Loading"));

	RETURN_FALSE_IF_FALSE(CreateGPUBuffers(p_rhi, stgList, cmdBfr))

	RETURN_FALSE_IF_FALSE(p_rhi->EndCommandBuffer(cmdBfr));

	CVulkanRHI::Queue queue = p_rhi->GetQueue(0);
	VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBfr;
	submitInfo.pSignalSemaphores = nullptr;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitDstStageMask = &waitstage;
	RETURN_FALSE_IF_FALSE(p_rhi->SubmitCommandbuffer(queue, &submitInfo, 1));

	RETURN_FALSE_IF_FALSE(p_rhi->WaitToFinish(queue));

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
	uint32_t					currentCount = 0;
	
	m_bBoxDetails.instanceOffset = 0;
	m_bBoxDetails.instanceCount = 0;

	m_bSphereDetails.instanceOffset = 0;
	m_bSphereDetails.instanceCount = 0;

	
	// not rendering scene bounding box for this code path
	// for now
	//if (p_sceneGraph->IsDebugDrawEnabled())
	//{
	//	const float* transform = &(p_sceneGraph->GetBoundingBox()->UnitBBoxTransform().GetTransform()).column[0][0];
	//	std::copy(&transform[0], &transform[16], std::back_inserter(perbBoxTransformData));
	//	++currentBBoxCount;
	//}

	// Loop through the entity list
	for (auto& entity : (*entityList))
	{
		if (entity->IsDebugDrawEnabled() || entity->IsSubmeshDebugDrawEnabled())
		{
			nm::float4x4 entityTransform = entity->GetTransform().GetTransform();

			// Append the entity's bounding sphere to raw CPU buffer if selected
			if (entity->GetDebugDataType() == CDebugData::DebugType::Sphere)
			{
				const float* transform = &(entityTransform * entity->GetTransform().GetTransform()).column[0][0];
				std::copy(&transform[0], &transform[16], std::back_inserter(selectedSphereTransformData));
				++m_bSphereDetails.instanceCount;
			}
			else
			{
				// Append the entity's mesh's primary bbox to raw CPU buffer if selected
				if (entity->IsDebugDrawEnabled())
				{
					const float* transform = &(entityTransform * entity->GetBoundingBox()->GetUnitBBoxTransform().GetTransform()).column[0][0];
					std::copy(&transform[0], &transform[16], std::back_inserter(allSelectedTransformData));
					++m_bBoxDetails.instanceCount;
				}

				// Append the sub-mesh's bbox to raw CPU buffer if selected
				if (entity->IsSubmeshDebugDrawEnabled())
				{
					for (uint32_t subBoxID = 0; subBoxID < entity->GetSubBoundingBoxCount(); subBoxID++)
					{
						BBox* box = entity->GetSubBoundingBox(subBoxID);
						const float* transform = &(entityTransform * box->GetUnitBBoxTransform().GetTransform()).column[0][0];
						std::copy(&transform[0], &transform[16], std::back_inserter(allSelectedTransformData));
						++m_bBoxDetails.instanceCount;
					}
				}
			}
		}
	}

	// Append sphere transform data to the end of all selected transform raw data list
	if (!selectedSphereTransformData.empty())
	{
		std::copy(selectedSphereTransformData.begin(), selectedSphereTransformData.end(), std::back_inserter(allSelectedTransformData));
		m_bSphereDetails.instanceOffset = m_bBoxDetails.instanceCount;
	}

	uint32_t allInstanceCount = m_bBoxDetails.instanceCount + m_bSphereDetails.instanceCount;

	// Heuristic to identify if the uniform buffer is stale and needs rebuilding
	if (allInstanceCount != m_instanceCount ||
		p_sceneGraph->GetSceneStatus() != CSceneGraph::SceneStatus::ss_NoChange)
	{
		m_instanceCount = allInstanceCount;
		bufferStale = true;
	}

	// Irrespective of the swap chain buffer id, nuke both uniform buffers and reload them
	if (bufferStale && !allSelectedTransformData.empty())
	{
		uint8_t* data = (uint8_t*)(allSelectedTransformData.data());
		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(data, p_fixedBuffers->GetBuffer(CFixedBuffers::fb_DebugUniform_0)));
		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(data, p_fixedBuffers->GetBuffer(CFixedBuffers::fb_DebugUniform_1)));
	}

	return m_instanceCount > 0;
}

BBox unitBbox(BBox::Type::Unit);

bool CRenderableDebug::PreDraw(CVulkanRHI* p_rhi, uint32_t p_scIdx, const CFixedBuffers* p_fixedBuffers, const CSceneGraph* p_sceneGraph)
{
	//std::vector<int>			indexData;
	//std::vector<DebugVertex>	vertexData;
	//std::vector<int>			indexTemplate{ 0, 1, 2, 3, 4, 5, 6, 7, 0, 4, 1, 5, 2, 6, 3, 7, 1, 2, 3, 0, 5, 6, 7, 4};	
	//uint32_t					modelMatIndex = 0;
	//uint32_t					displayingBBoxIndex = 0;
	//std::vector<float>			perbBoxTransformData;
	//CSceneGraph::EntityList*	entityList = p_sceneGraph->GetEntities();
	//uint32_t					vertexTemplateSize = 8; // 8 because that is number of vertices of the bounding box

	//if (p_sceneGraph->IsDebugDrawEnabled())
	//{
	//	// this manages the vertices and indices for the bounding box of the scene
	//	const nm::float3* box = p_sceneGraph->GetBoundingBox()->bBox;
	//	for (uint32_t i = 0; i < vertexTemplateSize; i++)
	//	{
	//		DebugVertex dVert{ box[i], (int)modelMatIndex };
	//		vertexData.push_back(dVert);
	//	}
	//
	//	{
	//		auto indices = indexTemplate;
	//		int vertexfactor = (displayingBBoxIndex * vertexTemplateSize);	// vertex offset
	//		std::transform(indices.begin(), indices.end(), indices.begin(), [=](int& c)
	//		{
	//			// adding each index value from the template with the vertex offset
	//			return vertexfactor + c;
	//		});
	//
	//		size_t indexOffset = indexData.size();
	//		indexData.resize((displayingBBoxIndex + 1) * (int)indexTemplate.size());
	//		std::copy(indices.begin(), indices.end(), indexData.begin() + indexOffset);
	//	}

	//	displayingBBoxIndex++;
	//
	//	const float* transform = &nm::float4x4().identity().column[0][0];
	//	std::copy(&transform[0], &transform[16], std::back_inserter(perbBoxTransformData));
	//
	//	modelMatIndex++;
	//}

	//for (auto& entity : (*entityList))
	//{
	//	if (entity->IsDebugDrawEnabled())
	//	{
	//		// Appending vertices the vertex buffer
	//		nm::float3* box = unitBbox.bBox;// entity->GetBoundingBox()->bBox;
	//		for (uint32_t i = 0; i < vertexTemplateSize; i++)
	//		{
	//			DebugVertex dVert{ box[i], (int)modelMatIndex };
	//			vertexData.push_back(dVert);
	//		}
	//		
	//		// Appending indices the index buffer
	//		{
	//			auto indices		= indexTemplate;
	//			int vertexfactor	= (displayingBBoxIndex * vertexTemplateSize);
	//			std::transform(indices.begin(), indices.end(), indices.begin(), [=](int& c)
	//				{
	//					return vertexfactor + c;
	//				});

	//			size_t indexOffset = indexData.size();
	//			indexData.resize((displayingBBoxIndex + 1) * (int)indexTemplate.size());
	//			std::copy(indices.begin(), indices.end(), indexData.begin() + indexOffset);
	//		}

	//		// Appending transform to the transform buffer
	//		{
	//			const float* transform = &(/*p_sceneGraph->GetTransform().GetTransform() **/
	//				entity->GetTransform().GetTransform()
	//				* entity->GetBoundingBox()->GetUnitBBoxTransform().GetTransform()).column[0][0];
	//			std::copy(&transform[0], &transform[16], std::back_inserter(perbBoxTransformData));

	//			modelMatIndex++;
	//		}

	//		displayingBBoxIndex++;
	//	}
	//	if (entity->IsSubmeshDebugDrawEnabled())// && entity->IsDebugDrawEnabled())
	//	{
	//		for (uint32_t subBoxID = 0; subBoxID < entity->GetSubBoundingBoxCount(); subBoxID++)
	//		{
	//			// Appending vertices the vertex buffer
	//			nm::float3* box = unitBbox.bBox; //entity->GetSubBoundingBox(subBoxID)->bBox;
	//			for (uint32_t i = 0; i < vertexTemplateSize; i++)
	//			{
	//				// Adding vertex data and associated transformation index. This is same as per entity bounding box
	//				// because every sub-mesh of the bounding box will have the transform
	//				DebugVertex dVert{ box[i], (int)modelMatIndex };
	//				vertexData.push_back(dVert);
	//			}

	//			// Appending indices the index buffer
	//			{
	//				// making a copy of the index template for this specific bounding box vertices
	//				auto indices = indexTemplate;

	//				// for each index i = i + (number of vertices in bounding box * index of number of bounding boxes added for display )
	//				int vertexfactor = (displayingBBoxIndex * vertexTemplateSize);
	//				std::transform(indices.begin(), indices.end(), indices.begin(), [=](int& c)
	//					{
	//						return vertexfactor + c;
	//					});

	//				// this is number of indices already added to be displayed
	//				size_t indexOffset = indexData.size();

	//				// resizing the index buffer size to accommodate for another bounding box for display
	//				indexData.resize((displayingBBoxIndex + 1) * (int)indexTemplate.size());

	//				// copying the indices data of this bounding box to list of all the bounding boxes to display
	//				std::copy(indices.begin(), indices.end(), indexData.begin() + indexOffset);
	//			}

	//			// Appending transform to the transform buffer
	//			{
	//				const float* transform = &(/*p_sceneGraph->GetTransform().GetTransform() **/
	//					entity->GetTransform().GetTransform()
	//					* entity->GetSubBoundingBox(subBoxID)->GetUnitBBoxTransform().GetTransform()).column[0][0];
	//				std::copy(&transform[0], &transform[16], std::back_inserter(perbBoxTransformData));
	//			
	//				modelMatIndex++;
	//			}

	//			displayingBBoxIndex++;
	//		}
	//	}

	//	//if (entity->IsDebugDrawEnabled() || entity->IsSubmeshDebugDrawEnabled())
	//	//{
	//	//	const float* transform = &(/*p_sceneGraph->GetTransform().GetTransform() **/
	//	//		entity->GetTransform().GetTransform()
	//	//		* entity->GetBoundingBox()->UnitBBoxTransform().GetTransform()).column[0][0];
	//	//	std::copy(&transform[0], &transform[16], std::back_inserter(perbBoxTransformData));

	//	//	modelMatIndex++;
	//	//}
	//}
	//m_bBoxDetails.indexCount = indexData.size();
	//
	////{
	////	// create sphere
	////	RawSphere debugSphere;
	////	GenerateSphere(25, 25, debugSphere, 1.0f);

	////	// create indices for sphere
	////	// let us assume vertices are 0 - 99

	////	// populate m_indexCount
	////	m_indexCount += debugSphere.lineIndices.size();
	////	
	////	for (int i = 0; i < debugSphere.vertices.size(); i++)
	////	{
	////		// populate vertexData
	////		DebugVertex dVert{ debugSphere.vertices[i], (int)modelMatIndex };
	////		vertexData.push_back(dVert);
	////	}

	////	// populate indexData
	////	size_t indexOffset = indexData.size();
	////	indexData.resize(indexOffset + debugSphere.lineIndices.size());
	////	std::copy(debugSphere.lineIndices.begin(), debugSphere.lineIndices.end(), indexData.begin() + indexOffset);
	////			
	////	// populate perbBoxTransformData
	////	const float* transform = &nm::float4x4().identity().column[0][0];
	////	std::copy(&transform[0], &transform[16], std::back_inserter(perbBoxTransformData));
	////}

	//if(!vertexData.empty())
	//{
	//	// if the vertex buffer if already created from previous frame, destroy them and free the associated memory for this frame's use
	//	if (m_vertexBuffers[p_scIdx].descInfo.buffer != VK_NULL_HANDLE)
	//	{
	//		Clear(p_rhi, p_scIdx);
	//	}

	//	{
	//		CVulkanRHI::Buffer uniformBuffer = (p_scIdx == 0) ? p_fixedBuffers->GetBuffer(CFixedBuffers::fb_DebugUniform_0) : p_fixedBuffers->GetBuffer(CFixedBuffers::fb_DebugUniform_1);

	//		uint8_t* data = (uint8_t*)(perbBoxTransformData.data());
	//		RETURN_FALSE_IF_FALSE(p_rhi->WriteToBuffer(data, uniformBuffer));
	//	}
	//	{
	//		// Create vertex / index buffers
	//		size_t vertexSize = vertexData.size() * sizeof(DebugVertex);
	//		size_t indexSize = (size_t)m_bBoxDetails.indexCount * sizeof(int);

	//		// Creating vertex and index buffers and upload all data to single contiguous GPU buffers respectively
	//		{
	//			p_rhi->CreateAllocateBindBuffer(vertexSize, m_vertexBuffers[p_scIdx], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	//			p_rhi->CreateAllocateBindBuffer(indexSize, m_indexBuffers[p_scIdx], VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	//			bool flushMemRanges = true;
	//			std::vector<VkMappedMemoryRange> memRanges;
	//			void* vertexMapped;
	//			void* indexMapped;

	//			if (!p_rhi->MapMemory(m_vertexBuffers[p_scIdx], flushMemRanges, &vertexMapped, &memRanges))
	//				return false;

	//			if (!p_rhi->MapMemory(m_indexBuffers[p_scIdx], flushMemRanges, &indexMapped, &memRanges))
	//				return false;

	//			{
	//				memcpy(vertexMapped, vertexData.data(), vertexSize);
	//				memcpy(indexMapped, indexData.data(), indexSize);
	//			}

	//			if (!p_rhi->FlushMemoryRanges(&memRanges))
	//				return false;

	//			p_rhi->UnMapMemory(m_vertexBuffers[p_scIdx]);
	//			p_rhi->UnMapMemory(m_indexBuffers[p_scIdx]);
	//		}
	//	}

	//	return true;
	//}

	return false;
}

void CRenderableDebug::Destroy(CVulkanRHI* p_rhi)
{
}

bool CRenderableDebug::CreateDebugDescriptors(CVulkanRHI* p_rhi, const CFixedBuffers* p_fixedBuffers)
{
	VkShaderStageFlags vertex_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	{
		CVulkanRHI::Buffer uniformBuffer = p_fixedBuffers->GetBuffer(CFixedBuffers::fb_DebugUniform_0);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Debug_Transforms_Uniform,	1,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	vertex_frag,	&uniformBuffer.descInfo,	VK_NULL_HANDLE}, 0);
		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, 0, BindingDest::bd_Debug_Transforms_Uniform, 0));
	}

	{
		CVulkanRHI::Buffer uniformBuffer = p_fixedBuffers->GetBuffer(CFixedBuffers::fb_DebugUniform_1);
		AddDescriptor(CVulkanRHI::DescriptorData{ BindingDest::bd_Debug_Transforms_Uniform,	1,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	vertex_frag,	&uniformBuffer.descInfo,	VK_NULL_HANDLE }, 1);
		RETURN_FALSE_IF_FALSE(CreateDescriptors(p_rhi, 0, BindingDest::bd_Debug_Transforms_Uniform, 1));
	}

	return true;
}

bool CRenderableDebug::CreateGPUBuffers(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	std::cout << "Loading Debug Resources" << std::endl;

	MeshRaw allDebugTemplateMeshes;	
	{
		// Bounding Box Template
		allDebugTemplateMeshes.indicesList = BBox::GetIndexTemplate();
		allDebugTemplateMeshes.vertexList = BBox::GetVertexTempate();

		// Populate bbox details
		m_bBoxDetails.indexCount = (uint32_t)allDebugTemplateMeshes.indicesList.size();
		m_bBoxDetails.indexOffset = 0;
		m_bBoxDetails.vertexOffset = 0;
	}

	{
		// Bounding Sphere Template
		// create sphere
		RawSphere debugSphere;
		GenerateSphere(25, 25, debugSphere, 1.0f);

		// populate bSphere details
		m_bSphereDetails.indexCount		= (uint32_t)debugSphere.lineIndices.size();
		m_bSphereDetails.indexOffset	= (uint32_t)allDebugTemplateMeshes.indicesList.size();
		m_bSphereDetails.vertexOffset	= (uint32_t)allDebugTemplateMeshes.vertexList.size() / allDebugTemplateMeshes.vertexList.GetVertexSize();
		
		auto rawVertexData = debugSphere.vertices.getRaw();
		auto rawIndexData = debugSphere.lineIndices;
		
		// adding the vertex offset to the sphere indices
		std::transform(rawIndexData.begin(), rawIndexData.end(), rawIndexData.begin(), [=](int& c)
		{
			return m_bSphereDetails.vertexOffset + c;
		});
		
		std::copy(rawVertexData.begin(), rawVertexData.end(), std::back_inserter(allDebugTemplateMeshes.vertexList.getRaw()));
		std::copy(rawIndexData.begin(), rawIndexData.end(), std::back_inserter(allDebugTemplateMeshes.indicesList));
	}
	
	RETURN_FALSE_IF_FALSE(CreateVertexIndexBuffer(p_rhi, p_stgList, &allDebugTemplateMeshes, p_cmdBfr));

	return true;
}
