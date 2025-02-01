#include "LightingPass.h"
#include "core/Global.h"

CForwardPass::CForwardPass(CVulkanRHI* p_rhi)
	:CDynamicRenderingPass(p_rhi)
{}

CForwardPass::~CForwardPass()
{
}

bool CForwardPass::CreateRenderingInfo(RenderData* p_renderData)
{
	std::vector<VkFormat> colorAttachFormats(AttachId::max, VkFormat::VK_FORMAT_UNDEFINED);	
	m_colorAttachInfos = std::vector<VkRenderingAttachmentInfo>(AttachId::max, CVulkanCore::RenderingAttachinfo());
	// Position
	{
		CVulkanRHI::Image positionRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Position);
		colorAttachFormats[AttachId::Posiiton]				= positionRT.format;
		m_colorAttachInfos[AttachId::Posiiton].imageView	= positionRT.descInfo.imageView;
	}
	// Normal
	{
		CVulkanRHI::Image normalRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PingPong_Normal_0);
		colorAttachFormats[AttachId::Normal]				= normalRT.format;
		m_colorAttachInfos[AttachId::Normal].imageView		= normalRT.descInfo.imageView;
	}
	// Primary Color
	{
		CVulkanRHI::Image colorRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryColor);
		colorAttachFormats[AttachId::Color]					= colorRT.format;
		m_colorAttachInfos[AttachId::Color].imageView		= colorRT.descInfo.imageView;
		m_colorAttachInfos[AttachId::Color].loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD;
	}
	// Rough Metal
	{
		CVulkanRHI::Image rmRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_RoughMetal);
		colorAttachFormats[AttachId::RoughMetal]			= rmRT.format;
		m_colorAttachInfos[AttachId::RoughMetal].imageView	= rmRT.descInfo.imageView;
		m_colorAttachInfos[AttachId::RoughMetal].clearValue	= VkClearValue{ 0.0, 0.0, 0.0, 0.0 };
	}
	// Motion
	{
		CVulkanRHI::Image motionRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Motion);
		colorAttachFormats[AttachId::Motion]				= motionRT.format;
		m_colorAttachInfos[AttachId::Motion].imageView		= motionRT.descInfo.imageView;
		m_colorAttachInfos[AttachId::Motion].clearValue		= VkClearValue{ 0.0, 0.0, 0.0, 0.0 };
	}
	m_pipeline.colorAttachFormats = colorAttachFormats;
	// Primary Depth
	{
		CVulkanRHI::Image depthRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PingPong_Depth_0);
		m_pipeline.depthAttachFormat						= depthRT.format;

		m_depthAttachInfo									= CVulkanCore::RenderingAttachinfo();
		m_depthAttachInfo.imageView							= depthRT.descInfo.imageView;
		m_depthAttachInfo.imageLayout						= VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		m_depthAttachInfo.clearValue						= VkClearValue{ 1.0, 0 };
	}
	
	m_renderingInfo											= CVulkanCore::RenderingInfo();
	m_renderingInfo.renderArea.extent.width					= m_rhi->GetRenderWidth();
	m_renderingInfo.renderArea.extent.height				= m_rhi->GetRenderHeight();
	m_renderingInfo.colorAttachmentCount					= (uint32_t)m_colorAttachInfos.size();
	m_renderingInfo.pColorAttachments						= m_colorAttachInfos.data();
	m_renderingInfo.pDepthAttachment						= &m_depthAttachInfo;

	return true;
}

bool CForwardPass::CreatePipeline(CVulkanRHI::Pipeline p_pipeline)
{
	CVulkanRHI::ShaderPaths fwdShaderpaths{};
	fwdShaderpaths.shaderpath_vertex						= g_EnginePath /"shaders/spirv/Forward.vert.spv";
	fwdShaderpaths.shaderpath_fragment						= g_EnginePath /"shaders/spirv/Forward.frag.spv";
	m_pipeline.pipeLayout									= p_pipeline.pipeLayout;
	m_pipeline.vertexInBinding								= p_pipeline.vertexInBinding;
	m_pipeline.vertexAttributeDesc							= p_pipeline.vertexAttributeDesc;
	m_pipeline.cullMode										= VK_CULL_MODE_BACK_BIT;
	m_pipeline.enableDepthTest								= true;
	m_pipeline.enableDepthWrite								= true;

	if (!m_rhi->CreateGraphicsPipeline(fwdShaderpaths, m_pipeline, "ForwardGfxPipeline"))
	{
		std::cerr << "CForwardPass::CreatePipeline Error: Error Creating Forward Pipeline" << std::endl;
		return false;
	}

	return true;
}

bool CForwardPass::Update(UpdateData*)
{
	return true;
}

bool CForwardPass::Render(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	const CScene* scene										= p_renderData->loadedAssets->GetScene();
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;
	CFixedBuffers::PrimaryUniformData* primaryData			= p_renderData->fixedAssets->GetFixedBuffers()->GetPrimaryUnifromData();

	// Ping pong the depth and normal render targets
	{
		CVulkanRHI::Image normalRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(GetNormalTextureID(primaryData->pingPongIndex));		
		m_colorAttachInfos[AttachId::Normal].imageView = normalRT.descInfo.imageView;

		CVulkanRHI::Image depthRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(GetDepthTextureID(primaryData->pingPongIndex));
		m_depthAttachInfo.imageView = depthRT.descInfo.imageView;
	}

	{
		vkCmdBeginRendering(cmdBfr, &m_renderingInfo);
		{
			m_rhi->SetViewport(cmdBfr, 0.0f, 1.0f, (float)m_rhi->GetRenderWidth(), -(float)m_rhi->GetRenderHeight());
			m_rhi->SetScissors(cmdBfr, 0, 0, m_rhi->GetRenderWidth(), m_rhi->GetRenderHeight());

			vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);

			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

			// Bind Index and Vertices buffers
			VkDeviceSize offsets[1] = { 0 };
			for (unsigned int i = 0; i < scene->GetRenderableMeshCount(); i++)
			{
				const CRenderableMesh* mesh = scene->GetRenderableMesh(i);

				std::vector<VkBuffer> vtxBuffers{ mesh->GetVertexBuffer().descInfo.buffer };
				vkCmdBindVertexBuffers(cmdBfr, 0, (uint32_t)vtxBuffers.size(), vtxBuffers.data(), offsets);
				vkCmdBindIndexBuffer(cmdBfr, mesh->GetIndexBuffer().descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

				for (uint32_t j = 0; j < mesh->GetSubmeshCount(); j++)
				{
					const SubMesh* submesh = mesh->GetSubmesh(j);
					VkPipelineStageFlags pipelineStage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
					CScene::MeshPushConst pc{ mesh->GetMeshId(), submesh->materialId };

					vkCmdPushConstants(cmdBfr, m_pipeline.pipeLayout, pipelineStage, 0, sizeof(CScene::MeshPushConst), (void*)&pc);
					vkCmdDrawIndexed(cmdBfr, submesh->indexCount, 1, submesh->firstIndex, 0, 1);
				}
			}
		}
		vkCmdEndRendering(cmdBfr);
	}
	
	return true;
}
 
void CForwardPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription					= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription						= m_pipeline.vertexInBinding;
}

CSkyboxPass::CSkyboxPass(CVulkanRHI* p_rhi)
	: CDynamicRenderingPass(p_rhi)
{
}

CSkyboxPass::~CSkyboxPass()
{
}

bool CSkyboxPass::CreateRenderingInfo(RenderData* p_renderData)
{
	std::vector<VkFormat> colorAttachFormats(AttachId::max, VkFormat::VK_FORMAT_UNDEFINED);
	m_colorAttachInfos = std::vector<VkRenderingAttachmentInfo>(AttachId::max, CVulkanCore::RenderingAttachinfo());

	// Color
	{
		CVulkanRHI::Image colorRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryColor);
		colorAttachFormats[AttachId::Color]					= colorRT.format;
		m_colorAttachInfos[AttachId::Color].imageView		= colorRT.descInfo.imageView;
		m_colorAttachInfos[AttachId::Color].clearValue		= VkClearValue{ 0.0, 0.0, 0.0, 0.0 };
	}
	m_pipeline.colorAttachFormats = colorAttachFormats;
	// Primary Depth
	{
		CVulkanRHI::Image depthRT							= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PingPong_Depth_0);
		m_pipeline.depthAttachFormat						= depthRT.format;

		m_depthAttachInfo									= CVulkanCore::RenderingAttachinfo();
		m_depthAttachInfo.imageView							= depthRT.descInfo.imageView;
		m_depthAttachInfo.imageLayout						= VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		m_depthAttachInfo.clearValue						= VkClearValue{ 1.0, 0 };
	}

	m_renderingInfo = CVulkanCore::RenderingInfo();
	m_renderingInfo.renderArea.extent.width					= m_rhi->GetRenderWidth();
	m_renderingInfo.renderArea.extent.height				= m_rhi->GetRenderHeight();
	m_renderingInfo.colorAttachmentCount					= (uint32_t)m_colorAttachInfos.size();
	m_renderingInfo.pColorAttachments						= m_colorAttachInfos.data();
	m_renderingInfo.pDepthAttachment						= &m_depthAttachInfo;

	return true;
}

bool CSkyboxPass::CreatePipeline(CVulkanRHI::Pipeline p_pipeline)
{
	CVulkanRHI::ShaderPaths skyBoxShaderpaths{};
	skyBoxShaderpaths.shaderpath_vertex						= g_EnginePath /"shaders/spirv/Skybox.vert.spv";
	skyBoxShaderpaths.shaderpath_fragment					= g_EnginePath /"shaders/spirv/Skybox.frag.spv";
	m_pipeline.vertexInBinding								= p_pipeline.vertexInBinding;
	m_pipeline.vertexAttributeDesc							= p_pipeline.vertexAttributeDesc;
	m_pipeline.pipeLayout									= p_pipeline.pipeLayout;
	m_pipeline.cullMode										= VK_CULL_MODE_NONE;
	m_pipeline.enableDepthTest								= false;
	m_pipeline.enableDepthWrite								= false;
	if (!m_rhi->CreateGraphicsPipeline(skyBoxShaderpaths, m_pipeline, "SkyboxGfxPipeline"))
	{
		std::cerr << "CSkyboxPass::CreatePipeline Error: Error Creating Skybox Pipeline" << std::endl;
		return false;
	}

	return true;
}

bool CSkyboxPass::Update(UpdateData*)
{
	return true;
}

bool CSkyboxPass::Render(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	const CScene* scene										= p_renderData->loadedAssets->GetScene();
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;
	CVulkanRHI::Renderpass renderPass						= m_pipeline.renderpassData;
	CFixedBuffers::PrimaryUniformData* primaryData			= p_renderData->fixedAssets->GetFixedBuffers()->GetPrimaryUnifromData();

	// Ping pong the depth
	{
		CVulkanRHI::Image depthRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(GetDepthTextureID(primaryData->pingPongIndex));
		m_depthAttachInfo.imageView = depthRT.descInfo.imageView;
	}

	{
		vkCmdBeginRendering(cmdBfr, &m_renderingInfo);
		{
			m_rhi->SetViewport(cmdBfr, 0.0f, 1.0f, (float)m_rhi->GetRenderWidth(), -(float)m_rhi->GetRenderHeight());
			m_rhi->SetScissors(cmdBfr, 0, 0, renderPass.framebufferWidth, renderPass.framebufferHeight);

			vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);

			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

			VkDeviceSize offsets[1] = { 0 };
			const CRenderable* mesh = scene->GetSkyBoxMesh();

			std::vector<VkBuffer> vtxBuffers{ mesh->GetVertexBuffer().descInfo.buffer };
			vkCmdBindVertexBuffers(cmdBfr, 0, (uint32_t)vtxBuffers.size(), vtxBuffers.data(), offsets);
			vkCmdBindIndexBuffer(cmdBfr, mesh->GetIndexBuffer().descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

			uint32_t count = (uint32_t)mesh->GetIndexBuffer().descInfo.range / sizeof(uint32_t);
			vkCmdDrawIndexed(cmdBfr, count, 1, 0, 0, 1);
		}
		vkCmdEndRendering(cmdBfr);
	}
	
	return true;
}
 
void CSkyboxPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription					= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription						= m_pipeline.vertexInBinding;
}

CDeferredPass::CDeferredPass(CVulkanRHI* p_rhi)
	: CDynamicRenderingPass(p_rhi)
	, CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
	, m_ambientFactor(0.01f)
	, m_enableIBL(true)
{}

CDeferredPass::~CDeferredPass()
{
}

bool CDeferredPass::CreateRenderingInfo(RenderData* p_renderData)
{
	std::vector<VkFormat> colorAttachFormats(AttachId::max, VkFormat::VK_FORMAT_UNDEFINED);
	m_colorAttachInfos = std::vector<VkRenderingAttachmentInfo>(AttachId::max, CVulkanCore::RenderingAttachinfo());
	// Position
	{
		CVulkanRHI::Image positionRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Position);
		colorAttachFormats[AttachId::Posiiton]				= positionRT.format;
		m_colorAttachInfos[AttachId::Posiiton].imageView	= positionRT.descInfo.imageView;
	}
	// Normal
	{
		CVulkanRHI::Image normalRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PingPong_Normal_0);
		colorAttachFormats[AttachId::Normal]				= normalRT.format;
		m_colorAttachInfos[AttachId::Normal].imageView		= normalRT.descInfo.imageView;
	}
	// Primary Color
	{
		CVulkanRHI::Image colorRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Albedo);
		colorAttachFormats[AttachId::Albedo]				= colorRT.format;
		m_colorAttachInfos[AttachId::Albedo].imageView		= colorRT.descInfo.imageView;
	}
	// Rough Metal
	{
		CVulkanRHI::Image rmRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_RoughMetal);
		colorAttachFormats[AttachId::RoughMetal]			= rmRT.format;
		m_colorAttachInfos[AttachId::RoughMetal].imageView	= rmRT.descInfo.imageView;
	}
	// Motion
	{
		CVulkanRHI::Image motionRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Motion);
		colorAttachFormats[AttachId::Motion]				= motionRT.format;
		m_colorAttachInfos[AttachId::Motion].imageView		= motionRT.descInfo.imageView;
	}
	m_pipeline.colorAttachFormats = colorAttachFormats;
	// Primary Depth
	{
		CVulkanRHI::Image depthRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PingPong_Depth_0);
		m_pipeline.depthAttachFormat						= depthRT.format;

		m_depthAttachInfo									= CVulkanCore::RenderingAttachinfo();
		m_depthAttachInfo.imageView							= depthRT.descInfo.imageView;
		m_depthAttachInfo.imageLayout						= VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		m_depthAttachInfo.clearValue						= VkClearValue{ 1.0, 0 };
	}
	
	m_renderingInfo											= CVulkanCore::RenderingInfo();
	m_renderingInfo.renderArea.extent.width					= m_rhi->GetRenderWidth();
	m_renderingInfo.renderArea.extent.height				= m_rhi->GetRenderHeight();
	m_renderingInfo.colorAttachmentCount					= (uint32_t)m_colorAttachInfos.size();
	m_renderingInfo.pColorAttachments						= m_colorAttachInfos.data();
	m_renderingInfo.pDepthAttachment						= &m_depthAttachInfo;

	return true;
}

bool CDeferredPass::CreatePipeline(CVulkanRHI::Pipeline p_pipeline)
{
	CVulkanRHI::ShaderPaths dfrdShaderpaths{};
	dfrdShaderpaths.shaderpath_vertex						= g_EnginePath /"shaders/spirv/Deferred_Gbuffer.vert.spv";
	dfrdShaderpaths.shaderpath_fragment						= g_EnginePath /"shaders/spirv/Deferred_Gbuffer.frag.spv";
	m_pipeline.vertexInBinding								= p_pipeline.vertexInBinding;
	m_pipeline.vertexAttributeDesc							= p_pipeline.vertexAttributeDesc;
	m_pipeline.cullMode										= VK_CULL_MODE_BACK_BIT;
	m_pipeline.enableDepthTest								= true;
	m_pipeline.enableDepthWrite								= true;
	m_pipeline.depthCmpOp									= VK_COMPARE_OP_LESS_OR_EQUAL;
	m_pipeline.pipeLayout									= p_pipeline.pipeLayout;
	if (!m_rhi->CreateGraphicsPipeline(dfrdShaderpaths, m_pipeline, "DeferredGfxPipeline"))
	{
		std::cerr << "CDeferredPass::CreatePipeline Error: Error Creating Deferred Pipeline" << std::endl;
		return false;
	}


	return true;
}

bool CDeferredPass::Update(UpdateData* p_updateData)
{
	p_updateData->uniformData->enableIBL = m_enableIBL;
	p_updateData->uniformData->pbrAmbientFactor = m_ambientFactor;
	return true;
}

bool CDeferredPass::Render(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	CScene* scene											= p_renderData->loadedAssets->GetScene();
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;
	CFixedBuffers::PrimaryUniformData* primaryData			= p_renderData->fixedAssets->GetFixedBuffers()->GetPrimaryUnifromData();

	// Ping pong the depth and normal render targets
	{
		CVulkanRHI::Image normalRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(GetNormalTextureID(primaryData->pingPongIndex));
		m_colorAttachInfos[AttachId::Normal].imageView = normalRT.descInfo.imageView;

		CVulkanRHI::Image depthRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(GetDepthTextureID(primaryData->pingPongIndex));
		m_depthAttachInfo.imageView = depthRT.descInfo.imageView;
	}

	{
		vkCmdBeginRendering(cmdBfr, &m_renderingInfo);
		{
			m_rhi->SetViewport(cmdBfr, 0.0f, 1.0f, (float)m_rhi->GetRenderWidth(), -(float)m_rhi->GetRenderHeight());
			m_rhi->SetScissors(cmdBfr, 0, 0, m_rhi->GetRenderWidth(), m_rhi->GetRenderHeight());

			vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);

			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

			// Bind Index and Vertices buffers
			VkDeviceSize offsets[1] = { 0 };
			for (unsigned int i = 0; i < scene->GetRenderableMeshCount(); i++)
			{
				const CRenderableMesh* mesh = scene->GetRenderableMesh(i);

				std::vector<VkBuffer> vtxBuffers{ mesh->GetVertexBuffer().descInfo.buffer };
				vkCmdBindVertexBuffers(cmdBfr, 0, (uint32_t)vtxBuffers.size(), vtxBuffers.data(), offsets);
				vkCmdBindIndexBuffer(cmdBfr, mesh->GetIndexBuffer().descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

				for (uint32_t j = 0; j < mesh->GetSubmeshCount(); j++)
				{
					const SubMesh* submesh = mesh->GetSubmesh(j);

					CScene::MeshPushConst pc{ mesh->GetMeshId(), submesh->materialId };

					VkPipelineStageFlags vertex_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
					vkCmdPushConstants(cmdBfr, m_pipeline.pipeLayout, vertex_frag, 0, sizeof(CScene::MeshPushConst), (void*)&pc);

					vkCmdDrawIndexed(cmdBfr, submesh->indexCount, 1, submesh->firstIndex, 0, 1);
				}
			}
		}
		vkCmdEndRendering(cmdBfr);
	}

	return true;
}

void CDeferredPass::Show(CVulkanRHI* p_rhi)
{
	ImGui::Checkbox(std::to_string(m_passIndex).c_str(), &m_isEnabled);
	ImGui::SameLine(40);
	bool differedPass = ImGui::TreeNode("Differed Lighting");
	if (differedPass)
	{
		ImGui::Checkbox("IBL", &m_enableIBL);
		ImGui::SliderFloat("Ambient Factor", &m_ambientFactor, 0.00f, 1.0f);
		ImGui::TreePop();
	}
}

void CDeferredPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription						= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription							= m_pipeline.vertexInBinding;
}

CDeferredLightingPass::CDeferredLightingPass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
{}

CDeferredLightingPass::~CDeferredLightingPass()
{
}

bool CDeferredLightingPass::CreatePipeline(CVulkanRHI::Pipeline p_pipeline)
{
	CVulkanRHI::ShaderPaths dfrdLightingShaderpaths{};
	dfrdLightingShaderpaths.shaderpath_compute					= g_EnginePath /"shaders/spirv/Deferred_Lighting.comp.spv";
	m_pipeline.pipeLayout										= p_pipeline.pipeLayout;

	RETURN_FALSE_IF_FALSE(m_rhi->CreateComputePipeline(dfrdLightingShaderpaths, m_pipeline, "DeferredLightingComputePipeline"));

	return true;
}

bool CDeferredLightingPass::Update(UpdateData*)
{
	return true;
}

bool CDeferredLightingPass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId												= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr							= p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc						= p_renderData->primaryDescriptors;
	const CScene* scene											= p_renderData->loadedAssets->GetScene();

	uint32_t dispatchDim_x										= m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y										= m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	{
		// issue a layout barrier on Primary Depth to set as Shader Read so it can be used in this compute shader
		//CVulkanRHI::Image primaryDepthRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryDepth);
		//m_rhi->IssueLayoutBarrier(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, primaryDepthRT, cmdBfr);

		vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);

		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

		vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	}

	return true;
}

CSkyboxDeferredPass::CSkyboxDeferredPass(CVulkanRHI* p_rhi)
	: CDynamicRenderingPass(p_rhi)
{
}

CSkyboxDeferredPass::~CSkyboxDeferredPass()
{
}

bool CSkyboxDeferredPass::CreateRenderingInfo(RenderData* p_renderData)
{
	std::vector<VkFormat> colorAttachFormats(AttachId::max, VkFormat::VK_FORMAT_UNDEFINED);
	m_colorAttachInfos = std::vector<VkRenderingAttachmentInfo>(AttachId::max, CVulkanCore::RenderingAttachinfo());

	// Color
	{
		CVulkanRHI::Image colorRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryColor);
		colorAttachFormats[AttachId::Color]				= colorRT.format;
		m_colorAttachInfos[AttachId::Color].imageView	= colorRT.descInfo.imageView;
		m_colorAttachInfos[AttachId::Color].clearValue	= VkClearValue{ 0.0, 0.0, 0.0, 0.0 };
	}
	m_pipeline.colorAttachFormats = colorAttachFormats;
	// Primary Depth
	{
		CVulkanRHI::Image depthRT						= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PingPong_Depth_0);
		m_pipeline.depthAttachFormat					= depthRT.format;

		m_depthAttachInfo								= CVulkanCore::RenderingAttachinfo();
		m_depthAttachInfo.imageView						= depthRT.descInfo.imageView;
		m_depthAttachInfo.imageLayout					= VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		m_depthAttachInfo.clearValue					= VkClearValue{ 1.0, 0 };
	}

	m_renderingInfo = CVulkanCore::RenderingInfo();
	m_renderingInfo.renderArea.extent.width				= m_rhi->GetRenderWidth();
	m_renderingInfo.renderArea.extent.height			= m_rhi->GetRenderHeight();
	m_renderingInfo.colorAttachmentCount				= (uint32_t)m_colorAttachInfos.size();
	m_renderingInfo.pColorAttachments					= m_colorAttachInfos.data();
	m_renderingInfo.pDepthAttachment					= &m_depthAttachInfo;

	return true;
}

bool CSkyboxDeferredPass::CreatePipeline(CVulkanRHI::Pipeline p_pipeline)
{
	CVulkanRHI::ShaderPaths skyBoxShaderpaths{};
	skyBoxShaderpaths.shaderpath_vertex						= g_EnginePath /"shaders/spirv/Skybox.vert.spv";
	skyBoxShaderpaths.shaderpath_fragment					= g_EnginePath /"shaders/spirv/Skybox.frag.spv";
	m_pipeline.vertexInBinding								= p_pipeline.vertexInBinding;
	m_pipeline.vertexAttributeDesc							= p_pipeline.vertexAttributeDesc;
	m_pipeline.pipeLayout									= p_pipeline.pipeLayout;
	m_pipeline.cullMode										= VK_CULL_MODE_NONE;
	m_pipeline.enableDepthTest								= false;
	m_pipeline.enableDepthWrite								= false;
	if (!m_rhi->CreateGraphicsPipeline(skyBoxShaderpaths, m_pipeline, "SkyboxDeferredGfxPipeline"))
	{
		std::cerr << "CSkyboxDeferredPass::CreatePipeline Error: Error Creating CSkyboxDeferredPass Pipeline" << std::endl;
		return false;
	}

	return true;
}

bool CSkyboxDeferredPass::Update(UpdateData*)
{
	return true;
}

bool CSkyboxDeferredPass::Render(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	CScene* scene											= p_renderData->loadedAssets->GetScene();
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;
	CVulkanRHI::Renderpass renderPass						= m_pipeline.renderpassData;
	CFixedBuffers::PrimaryUniformData* primaryData			= p_renderData->fixedAssets->GetFixedBuffers()->GetPrimaryUnifromData();

	// Ping pong the depth
	{
		CVulkanRHI::Image depthRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(GetDepthTextureID(primaryData->pingPongIndex));
		m_depthAttachInfo.imageView = depthRT.descInfo.imageView;
	}

	{
		vkCmdBeginRendering(cmdBfr, &m_renderingInfo);
		{
			m_rhi->SetViewport(cmdBfr, 0.0f, 1.0f, (float)m_rhi->GetRenderWidth(), -(float)m_rhi->GetRenderHeight());
			m_rhi->SetScissors(cmdBfr, 0, 0, renderPass.framebufferWidth, renderPass.framebufferHeight);

			vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);

			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

			VkDeviceSize offsets[1] = { 0 };
			const CRenderable* mesh = scene->GetSkyBoxMesh();
			std::vector<VkBuffer> vtxBuffers{ mesh->GetVertexBuffer().descInfo.buffer };
			vkCmdBindVertexBuffers(cmdBfr, 0, (uint32_t)vtxBuffers.size(), vtxBuffers.data(), offsets);
			vkCmdBindIndexBuffer(cmdBfr, mesh->GetIndexBuffer().descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

			uint32_t count = (uint32_t)mesh->GetIndexBuffer().descInfo.range / sizeof(uint32_t);
			vkCmdDrawIndexed(cmdBfr, count, 1, 0, 0, 1);
		}
		vkCmdEndRendering(cmdBfr);
	}
	
	return true;
}

void CSkyboxDeferredPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription = m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription = m_pipeline.vertexInBinding;
}

CShadowPass::CShadowPass(CVulkanRHI* p_rhi)
	: CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
	, m_shadowDenoisePass(nullptr)
	, m_rayTraceShadowpass(nullptr)
	, m_staticShadowPass(nullptr)
{
	m_staticShadowPass = new CShadowPass::CStaticShadowPrepass(p_rhi);
	m_rayTraceShadowpass = new CShadowPass::CRayTraceShadowPass(p_rhi);
	m_shadowDenoisePass = new CShadowPass::CShadowDenoisePass(p_rhi);
}

CShadowPass::~CShadowPass()
{
	delete m_shadowDenoisePass;
	delete m_rayTraceShadowpass;
	delete m_staticShadowPass;
}

void CShadowPass::Show(CVulkanRHI* p_rhi)
{
	ImGui::Checkbox(std::to_string(m_staticShadowPass->m_passIndex).c_str(), &m_staticShadowPass->m_isEnabled);
	ImGui::SameLine(40);
	bool shadowNode = ImGui::TreeNode("Shadow");
	if (shadowNode)
	{
		const char* items[] = { "Rasterized", "Ray Traced" };
		int featureState = (int)m_staticShadowPass->m_enableRayTracedShadow;
		ImGui::ListBox("Feature ", &featureState, items, IM_ARRAYSIZE(items), 2);
		m_staticShadowPass->m_enableRayTracedShadow = (bool)featureState;
				
		if (m_staticShadowPass->m_enableRayTracedShadow)
		{
			ImGui::SliderFloat("Temporal Accumulation Weight", &m_rayTraceShadowpass->m_temporalAccumWeight, 0.0f, 1.0f);
		}
		else // This is a rasterized shadow map feature
		{
			ImGui::Checkbox("Rasterized PCF", &m_staticShadowPass->m_enablePCF);
		}

		ImGui::TreePop();
	}
}

CShadowPass::CStaticShadowPrepass::CStaticShadowPrepass(CVulkanRHI* p_rhi)
	: CStaticRenderPass(p_rhi)
	, m_enablePCF(false)
	, m_enableRayTracedShadow(false)
{
	m_frameBuffer.resize(1);
	m_bReuseShadowMap = false;
}

CShadowPass::CStaticShadowPrepass::~CStaticShadowPrepass()
{
}

bool CShadowPass::CStaticShadowPrepass::CreateRenderpass(RenderData* p_renderData)
{
	CVulkanRHI::Image renderTarget = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_DirectionalShadowDepth);
	CVulkanRHI::Renderpass* renderpass = &m_pipeline.renderpassData;

	renderpass->AttachDepth(renderTarget.format, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	renderpass->framebufferWidth = renderTarget.width;
	renderpass->framebufferHeight = renderTarget.height;
	if (!m_rhi->CreateRenderpass(*renderpass))
		return false;

	std::vector<VkImageView> attachments(1, VkImageView{});
	attachments[0] = renderTarget.descInfo.imageView;
	if (!m_rhi->CreateFramebuffer(renderpass->renderpass, m_frameBuffer[0], attachments.data(), (uint32_t)attachments.size(), renderpass->framebufferWidth, renderpass->framebufferHeight))
		return false;

	return true;
}

bool CShadowPass::CStaticShadowPrepass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	uint32_t offset = 0;

	VkVertexInputBindingDescription vertexInputBinding = {};
	vertexInputBinding.binding = 0;
	vertexInputBinding.stride = 48;// sizeof(Vertex);
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	//	layout (location = 0) in vec3 pos;
	//	layout (location = 1) in vec3 normal;
	//	layout (location = 2) in vec2 uv;
	//	layout (location = 3) in vec3 tangent;
	std::vector<VkVertexInputAttributeDescription> vertexAttributs;
	// Attribute location 0: pos
	VkVertexInputAttributeDescription attribDesc{};
	attribDesc.binding = 0;
	attribDesc.location = 0;
	attribDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
	attribDesc.offset = offset;
	vertexAttributs.push_back(attribDesc);
	offset += 3 * sizeof(float);

	// Attribute location 1: normal
	attribDesc.binding = 0;
	attribDesc.location = 1;
	attribDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
	attribDesc.offset = offset;
	vertexAttributs.push_back(attribDesc);
	offset += 3 * sizeof(float);

	// Attribute location 2: uv
	attribDesc.binding = 0;
	attribDesc.location = 2;
	attribDesc.format = VK_FORMAT_R32G32_SFLOAT;
	attribDesc.offset = offset;
	vertexAttributs.push_back(attribDesc);
	offset += 2 * sizeof(float);

	// Attribute location 3: tangent
	attribDesc.binding = 0;
	attribDesc.location = 3;
	attribDesc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attribDesc.offset = offset;
	vertexAttributs.push_back(attribDesc);

	CVulkanRHI::ShaderPaths shadowPassShaderpaths{};
	shadowPassShaderpaths.shaderpath_vertex = g_EnginePath / "shaders/spirv/LightDepthPrepass.vert.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	m_pipeline.vertexInBinding = vertexInputBinding;
	m_pipeline.vertexAttributeDesc = vertexAttributs;
	m_pipeline.cullMode = VK_CULL_MODE_BACK_BIT;
	m_pipeline.enableDepthTest = true;
	m_pipeline.enableDepthWrite = true;
	if (!m_rhi->CreateGraphicsPipeline(shadowPassShaderpaths, m_pipeline, "StaticShadowGfxPipeline"))
	{
		std::cerr << "CStaticShadowPrepass::CreatePipeline Error: Error Creating Static Shadow Pipeline" << std::endl;
		return false;
	}

	return true;
}

bool CShadowPass::CStaticShadowPrepass::Update(UpdateData* p_updateData)
{
	uint32_t enable_Shadow_RT_PCF = 0;
	enable_Shadow_RT_PCF |= (m_isEnabled * ENABLE_SHADOW);
	enable_Shadow_RT_PCF |= (m_enableRayTracedShadow * ENABLE_RT_SHADOW);
	enable_Shadow_RT_PCF |= (m_enablePCF * ENABLE_PCF);
	p_updateData->uniformData->enable_Shadow_RT_PCF = enable_Shadow_RT_PCF;

	// we are choosing to reuse the shadow map if the scene graph has not gone through any changes 
	m_bReuseShadowMap = (p_updateData->sceneGraph->GetSceneStatus() == CSceneGraph::SceneStatus::ss_NoChange) ? true : false;

	return true;
}

bool CShadowPass::CStaticShadowPrepass::Render(RenderData* p_renderData)
{
	uint32_t scId = p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
	CVulkanRHI::Renderpass renderPass = m_pipeline.renderpassData;
	const CScene* scene = p_renderData->loadedAssets->GetScene();
	const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;

	{
		m_rhi->BeginRenderpass(m_frameBuffer[0], renderPass, p_renderData->cmdBfr);

		uint32_t fbWidth = renderPass.framebufferWidth;
		uint32_t fbHeight = renderPass.framebufferHeight;
		m_rhi->SetViewport(p_renderData->cmdBfr, 0.0f, 1.0f, (float)fbWidth, (float)fbHeight);
		m_rhi->SetScissors(p_renderData->cmdBfr, 0, 0, fbWidth, fbHeight);

		vkCmdBindPipeline(p_renderData->cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);
		vkCmdBindDescriptorSets(p_renderData->cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
		vkCmdBindDescriptorSets(p_renderData->cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

		// Bind Index and Vertices buffers
		VkDeviceSize offsets[1] = { 0 };
		for (unsigned int i = 0; i < scene->GetRenderableMeshCount(); i++)
		{
			const CRenderableMesh* mesh = scene->GetRenderableMesh(i);
			const CVulkanRHI::Buffer vertex = mesh->GetVertexBuffer();
			const CVulkanRHI::Buffer index = mesh->GetIndexBuffer();

			vkCmdBindVertexBuffers(p_renderData->cmdBfr, 0, 1, &vertex.descInfo.buffer, offsets);
			vkCmdBindIndexBuffer(p_renderData->cmdBfr, index.descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

			for (uint32_t j = 0; j < mesh->GetSubmeshCount(); j++)
			{
				const SubMesh* submesh = mesh->GetSubmesh(j);
				VkPipelineStageFlags vertex_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				CScene::MeshPushConst pc{ mesh->GetMeshId(), submesh->materialId };

				vkCmdPushConstants(p_renderData->cmdBfr, m_pipeline.pipeLayout, vertex_frag, 0, sizeof(CScene::MeshPushConst), (void*)&pc);

				//uint32_t count = (uint32_t)mesh.indexBuffer.descInfo.range / sizeof(uint32_t);
				vkCmdDrawIndexed(p_renderData->cmdBfr, submesh->indexCount, 1, submesh->firstIndex, 0, 1);
			}
		}

		m_rhi->EndRenderPass(p_renderData->cmdBfr);
	}

	return true;
}

void CShadowPass::CStaticShadowPrepass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription = m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription = m_pipeline.vertexInBinding;
}

CShadowPass::CRayTraceShadowPass::CRayTraceShadowPass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
	, m_temporalAccumWeight(0.3f)
{
}

CShadowPass::CRayTraceShadowPass::~CRayTraceShadowPass()
{
}

bool CShadowPass::CRayTraceShadowPass::CreatePipeline(CVulkanRHI::Pipeline p_pipeline)
{
	CVulkanRHI::ShaderPaths dfrdLightingShaderpaths{};
	dfrdLightingShaderpaths.shaderpath_compute = g_EnginePath / "shaders/spirv/RayTraceShadows.comp.spv";
	m_pipeline.pipeLayout = p_pipeline.pipeLayout;

	RETURN_FALSE_IF_FALSE(m_rhi->CreateComputePipeline(dfrdLightingShaderpaths, m_pipeline, "RayTracedShadowComputePipeline"));

	return true;
}

bool CShadowPass::CRayTraceShadowPass::Update(UpdateData* p_updateData)
{
	p_updateData->uniformData->shadowTemporalAccumWeight = m_temporalAccumWeight;
	return true;
}

bool CShadowPass::CRayTraceShadowPass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId = p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;
	const CScene* scene = p_renderData->loadedAssets->GetScene();

	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	{
		vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);

		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

		vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	}

	return true;
}

CShadowPass::CShadowDenoisePass::CShadowDenoisePass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
{
}

CShadowPass::CShadowDenoisePass::~CShadowDenoisePass()
{
}

bool CShadowPass::CShadowDenoisePass::CreatePipeline(CVulkanRHI::Pipeline p_pipeline)
{
	CVulkanRHI::ShaderPaths shaderpaths{};
	shaderpaths.shaderpath_compute = g_EnginePath / "shaders/spirv/ShadowDenoise.comp.spv";
	m_pipeline.pipeLayout = p_pipeline.pipeLayout;
	
	RETURN_FALSE_IF_FALSE(m_rhi->CreateComputePipeline(shaderpaths, m_pipeline, "ShadowDenoiseComputePipeline"));

	return true;
}

bool CShadowPass::CShadowDenoisePass::Update(UpdateData*)
{
	return false;
}

bool CShadowPass::CShadowDenoisePass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId = p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;
	const CScene* scene = p_renderData->loadedAssets->GetScene();
	
	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;
	
	{
		// Issue a memory barrier for the shadow RT pass to finish before the denoise pass can begin
		p_renderData->fixedAssets->GetRenderTargets()->IssueMemoryBarrier(m_rhi,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			cmdBfr, CRenderTargets::rt_RTShadowDenoise);

		vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
	
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
		
		vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	}

	return true;
}
