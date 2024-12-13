#include "LightingPass.h"

CForwardPass::CForwardPass(CVulkanRHI* p_rhi)
	:CPass(p_rhi)
{
	m_frameBuffer.resize(1);
}

CForwardPass::~CForwardPass()
{
}

bool CForwardPass::CreateRenderpass(RenderData* p_renderData)
{
	CVulkanRHI::Renderpass* renderPass						= &m_pipeline.renderpassData;
	CVulkanRHI::Image positionRT							= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Position);
	CVulkanRHI::Image normalRT								= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Normal);
	CVulkanRHI::Image primaryColorRT						= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryColor);
	CVulkanRHI::Image primaryDepthRT						= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryDepth);
	CVulkanRHI::Image rMMMotion								= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_RoughMetal_Motion);

	renderPass->AttachColor(positionRT.format,			VK_ATTACHMENT_LOAD_OP_CLEAR,	VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_GENERAL,							VK_IMAGE_LAYOUT_GENERAL,					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	renderPass->AttachColor(normalRT.format,			VK_ATTACHMENT_LOAD_OP_CLEAR,	VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_GENERAL,							VK_IMAGE_LAYOUT_GENERAL,					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	renderPass->AttachColor(primaryColorRT.format,		VK_ATTACHMENT_LOAD_OP_LOAD,		VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_GENERAL,							VK_IMAGE_LAYOUT_GENERAL,					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	renderPass->AttachColor(rMMMotion.format,			VK_ATTACHMENT_LOAD_OP_CLEAR,		VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_GENERAL,							VK_IMAGE_LAYOUT_GENERAL,					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	renderPass->AttachDepth(primaryDepthRT.format,		VK_ATTACHMENT_LOAD_OP_CLEAR,	VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	if (!m_rhi->CreateRenderpass(*renderPass))
		return false;
		
	renderPass->framebufferWidth							= primaryDepthRT.width;
	renderPass->framebufferHeight							= primaryDepthRT.height;

	std::vector<VkImageView> attachments(5, VkImageView{});
	attachments[0]											= positionRT.descInfo.imageView;
	attachments[1]											= normalRT.descInfo.imageView;
	attachments[2]											= primaryColorRT.descInfo.imageView;
	attachments[3]											= rMMMotion.descInfo.imageView;
	attachments[4]											= primaryDepthRT.descInfo.imageView;
	RETURN_FALSE_IF_FALSE(m_rhi->CreateFramebuffer(renderPass->renderpass, m_frameBuffer[0], attachments.data(), (uint32_t)attachments.size(), renderPass->framebufferWidth, renderPass->framebufferHeight));

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
	CVulkanRHI::Renderpass renderPass						= m_pipeline.renderpassData;
	const CScene* scene										= p_renderData->loadedAssets->GetScene();
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Forward"));
	{
		// x,y  holds roughness and metal and z,w holds velocity x,y
		m_rhi->SetClearColorValue(renderPass, 3, VkClearColorValue{ 0.0f, 0.0f, 0.0f, 0.0f });
		m_rhi->BeginRenderpass(m_frameBuffer[0], renderPass, cmdBfr);
		{

			m_rhi->SetViewport(cmdBfr, 0.0f, 1.0f, (float)renderPass.framebufferWidth, -(float)renderPass.framebufferHeight);
			m_rhi->SetScissors(cmdBfr, 0, 0, renderPass.framebufferWidth, renderPass.framebufferHeight);

			vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);

			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

			// Bind Index and Vertices buffers
			VkDeviceSize offsets[1] = { 0 };
			for (unsigned int i = CScene::MeshType::mt_Scene; i < scene->GetRenderableMeshCount(); i++)
			{
				const CRenderableMesh* mesh = scene->GetRenderableMesh(i);;

				vkCmdBindVertexBuffers(cmdBfr, 0, 1, &mesh->GetVertexBuffer()->descInfo.buffer, offsets);
				vkCmdBindIndexBuffer(cmdBfr, mesh->GetIndexBuffer()->descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

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
		m_rhi->EndRenderPass(cmdBfr);
	}
	m_rhi->EndCommandBuffer(cmdBfr);
	
	return true;
}

void CForwardPass::Destroy()
{
	m_rhi->DestroyFramebuffer(m_frameBuffer[0]);
	m_rhi->DestroyFramebuffer(m_frameBuffer[1]);
	m_rhi->DestroyRenderpass(m_pipeline.renderpassData.renderpass);
	m_rhi->DestroyPipeline(m_pipeline);
}
 
void CForwardPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription					= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription						= m_pipeline.vertexInBinding;
}

CSkyboxPass::CSkyboxPass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
	m_frameBuffer.resize(1);
}

CSkyboxPass::~CSkyboxPass()
{
}

bool CSkyboxPass::CreateRenderpass(RenderData* p_renderData)
{
	CVulkanRHI::Renderpass* renderPass				= &m_pipeline.renderpassData;
	CVulkanRHI::Image primaryColorRT				= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryColor);
	CVulkanRHI::Image primaryDepthRT				= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryDepth);

	renderPass->AttachColor(primaryColorRT.format, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	renderPass->AttachDepth(primaryDepthRT.format, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	if (!m_rhi->CreateRenderpass(*renderPass))
		return false;

	renderPass->framebufferWidth					= primaryDepthRT.width;
	renderPass->framebufferHeight					= primaryDepthRT.height;

	std::vector<VkImageView> attachments(2, VkImageView{});
	attachments[0]									= primaryColorRT.descInfo.imageView;
	attachments[1]									= primaryDepthRT.descInfo.imageView;
	RETURN_FALSE_IF_FALSE(m_rhi->CreateFramebuffer(renderPass->renderpass, m_frameBuffer[0], attachments.data(), (uint32_t)attachments.size(), renderPass->framebufferWidth, renderPass->framebufferHeight));
	
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

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Skybox for Forward"));

	m_rhi->BeginRenderpass(m_frameBuffer[0], renderPass, cmdBfr);

	//InsertMarker(m_vkCmdBfr[p_swapchainIndex], "Skybox");

	m_rhi->SetViewport(cmdBfr, 0.0f, 1.0f, (float)renderPass.framebufferWidth, -(float)renderPass.framebufferHeight);
	m_rhi->SetScissors(cmdBfr, 0, 0, renderPass.framebufferWidth, renderPass.framebufferHeight);

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);

	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

	VkDeviceSize offsets[1]									= { 0 };
	const CRenderableMesh* mesh								= scene->GetRenderableMesh(CScene::MeshType::mt_Skybox);
	vkCmdBindVertexBuffers(cmdBfr, 0, 1, &mesh->GetVertexBuffer()->descInfo.buffer, offsets);
	vkCmdBindIndexBuffer(cmdBfr, mesh->GetIndexBuffer()->descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

	uint32_t count											= (uint32_t)mesh->GetIndexBuffer()->descInfo.range / sizeof(uint32_t);
	vkCmdDrawIndexed(cmdBfr, count, 1, 0, 0, 1);

	m_rhi->EndRenderPass(cmdBfr);
	m_rhi->EndCommandBuffer(cmdBfr);

	return true;
}

void CSkyboxPass::Destroy()
{
	m_rhi->DestroyPipeline(m_pipeline);
}
 
void CSkyboxPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription					= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription						= m_pipeline.vertexInBinding;
}

CDeferredPass::CDeferredPass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
	m_frameBuffer.resize(1);
}

CDeferredPass::~CDeferredPass()
{
}

bool CDeferredPass::CreateRenderpass(RenderData* p_renderData)
{
	CVulkanRHI::Renderpass* renderPass	= &m_pipeline.renderpassData;
	CVulkanRHI::Image positionRT		= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Position);
	CVulkanRHI::Image normalRT			= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Normal);
	CVulkanRHI::Image albedoRT			= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Albedo);
	CVulkanRHI::Image rmMotionRT		= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_RoughMetal_Motion);
	CVulkanRHI::Image primaryDepthRT	= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryDepth);

	renderPass->AttachColor(positionRT.format,		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,		VK_IMAGE_LAYOUT_GENERAL,							VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);																				 
	renderPass->AttachColor(normalRT.format,		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,		VK_IMAGE_LAYOUT_GENERAL,							VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);																				 
	renderPass->AttachColor(albedoRT.format,		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,		VK_IMAGE_LAYOUT_GENERAL,							VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	renderPass->AttachColor(rmMotionRT.format,		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,		VK_IMAGE_LAYOUT_GENERAL,							VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	renderPass->AttachDepth(primaryDepthRT.format,  VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	if (!m_rhi->CreateRenderpass(*renderPass))
		return false;

	std::vector<VkImageView> attachments;
	attachments.push_back(positionRT.descInfo.imageView);
	attachments.push_back(normalRT.descInfo.imageView);
	attachments.push_back(albedoRT.descInfo.imageView);
	attachments.push_back(rmMotionRT.descInfo.imageView);
	attachments.push_back(primaryDepthRT.descInfo.imageView);

	renderPass->framebufferWidth	= positionRT.width;
	renderPass->framebufferHeight	= positionRT.height;
	if (!m_rhi->CreateFramebuffer(renderPass->renderpass, m_frameBuffer[0], attachments.data(), (uint32_t)attachments.size(), positionRT.width, positionRT.height))
		return false;

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

bool CDeferredPass::Update(UpdateData*)
{
	return true;
}

bool CDeferredPass::Render(RenderData* p_renderData)
{
	uint32_t scId												= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr							= p_renderData->cmdBfr;
	CVulkanRHI::Renderpass renderPass							= m_pipeline.renderpassData;
	const CScene* scene											= p_renderData->loadedAssets->GetScene();
	const CPrimaryDescriptors* primaryDesc						= p_renderData->primaryDescriptors;

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Deferred GBuffer"));
	
	m_rhi->BeginRenderpass(m_frameBuffer[0], renderPass, cmdBfr);

	m_rhi->SetViewport(cmdBfr, 0.0f, 1.0f, (float)renderPass.framebufferWidth, -(float)renderPass.framebufferHeight);
	m_rhi->SetScissors(cmdBfr, 0, 0, renderPass.framebufferWidth, renderPass.framebufferHeight);

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);

	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

	// Bind Index and Vertices buffers
	VkDeviceSize offsets[1] = { 0 };
	for (unsigned int i = CScene::MeshType::mt_Scene; i < scene->GetRenderableMeshCount(); i++)
	{
		const CRenderableMesh* mesh	= scene->GetRenderableMesh(i);

		vkCmdBindVertexBuffers(cmdBfr, 0, 1, &mesh->GetVertexBuffer()->descInfo.buffer, offsets);
		vkCmdBindIndexBuffer(cmdBfr, mesh->GetIndexBuffer()->descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

		for (uint32_t j = 0; j < mesh->GetSubmeshCount(); j++)
		{
			const SubMesh* submesh				= mesh->GetSubmesh(j);

			CScene::MeshPushConst pc{ mesh->GetMeshId(), submesh->materialId};

			VkPipelineStageFlags vertex_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			vkCmdPushConstants(cmdBfr, m_pipeline.pipeLayout, vertex_frag, 0, sizeof(CScene::MeshPushConst), (void*)&pc);

			//uint32_t count = (uint32_t)mesh.indexBuffer.descInfo.range / sizeof(uint32_t);
			vkCmdDrawIndexed(cmdBfr, submesh->indexCount, 1, submesh->firstIndex, 0, 1);
		}
	}

	m_rhi->EndRenderPass(cmdBfr);
	m_rhi->EndCommandBuffer(cmdBfr);

	return true;
}

void CDeferredPass::Destroy()
{
	m_rhi->DestroyFramebuffer(m_frameBuffer[0]);
	m_rhi->DestroyFramebuffer(m_frameBuffer[1]);
	m_rhi->DestroyRenderpass(m_pipeline.renderpassData.renderpass);
	m_rhi->DestroyPipeline(m_pipeline);
}

void CDeferredPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription						= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription							= m_pipeline.vertexInBinding;
}

CDeferredLightingPass::CDeferredLightingPass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
	m_frameBuffer.resize(FRAME_BUFFER_COUNT);
}

CDeferredLightingPass::~CDeferredLightingPass()
{
}

bool CDeferredLightingPass::CreateRenderpass(RenderData* p_renderData)
{
	return true;
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

bool CDeferredLightingPass::Render(RenderData* p_renderData)
{
	uint32_t scId												= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr							= p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc						= p_renderData->primaryDescriptors;
	const CScene* scene											= p_renderData->loadedAssets->GetScene();

	uint32_t dispatchDim_x										= m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y										= m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Compute Deferred Lighting"));
	{
		// issue a layout barrier on Primary Depth to set as Shader Read so it can be used in this compute shader
		//CVulkanRHI::Image primaryDepthRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryDepth);
		//m_rhi->IssueLayoutBarrier(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, primaryDepthRT, cmdBfr);

		vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);

		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

		vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	}
	m_rhi->EndCommandBuffer(cmdBfr);

	return true;
}

void CDeferredLightingPass::Destroy()
{
	// Compute shader; so no Framebuffer and Renderpass exists

	m_rhi->DestroyPipeline(m_pipeline);
}

void CDeferredLightingPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription						= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription							= m_pipeline.vertexInBinding;
}

CSkyboxDeferredPass::CSkyboxDeferredPass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
	m_frameBuffer.resize(1);
}

CSkyboxDeferredPass::~CSkyboxDeferredPass()
{
}

bool CSkyboxDeferredPass::CreateRenderpass(RenderData* p_renderData)
{
	CVulkanRHI::Renderpass* renderPass				= &m_pipeline.renderpassData;
	CVulkanRHI::Image primaryColorRT				= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryColor);
	CVulkanRHI::Image primaryDepthRT				= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryDepth);

	renderPass->AttachColor(primaryColorRT.format, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	renderPass->AttachDepth(primaryDepthRT.format, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	
	if (!m_rhi->CreateRenderpass(*renderPass))
		return false;

	renderPass->framebufferWidth					= primaryDepthRT.width;
	renderPass->framebufferHeight					= primaryDepthRT.height;

	std::vector<VkImageView> attachments(2, VkImageView{});
	attachments[0]									= primaryColorRT.descInfo.imageView;
	attachments[1]									= primaryDepthRT.descInfo.imageView;
	RETURN_FALSE_IF_FALSE(m_rhi->CreateFramebuffer(renderPass->renderpass, m_frameBuffer[0], attachments.data(), (uint32_t)attachments.size(), renderPass->framebufferWidth, renderPass->framebufferHeight));
	
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
	const CScene* scene										= p_renderData->loadedAssets->GetScene();
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;
	CVulkanRHI::Renderpass renderPass						= m_pipeline.renderpassData;

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Deferred Skybox"));

	m_rhi->BeginRenderpass(m_frameBuffer[0], renderPass, cmdBfr);

	//InsertMarker(m_vkCmdBfr[p_swapchainIndex], "Skybox");

	m_rhi->SetViewport(cmdBfr, 0.0f, 1.0f, (float)renderPass.framebufferWidth, -(float)renderPass.framebufferHeight);
	m_rhi->SetScissors(cmdBfr, 0, 0, renderPass.framebufferWidth, renderPass.framebufferHeight);

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);

	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);

	VkDeviceSize offsets[1]									= { 0 };
	const CRenderableMesh* mesh								= scene->GetRenderableMesh(CScene::MeshType::mt_Skybox);
	vkCmdBindVertexBuffers(cmdBfr, 0, 1, &mesh->GetVertexBuffer()->descInfo.buffer, offsets);
	vkCmdBindIndexBuffer(cmdBfr, mesh->GetIndexBuffer()->descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

	uint32_t count											= (uint32_t)mesh->GetIndexBuffer()->descInfo.range / sizeof(uint32_t);
	vkCmdDrawIndexed(cmdBfr, count, 1, 0, 0, 1);

	m_rhi->EndRenderPass(cmdBfr);
	m_rhi->EndCommandBuffer(cmdBfr);

	return true;
}

void CSkyboxDeferredPass::Destroy()
{
	m_rhi->DestroyPipeline(m_pipeline);
}

void CSkyboxDeferredPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription = m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription = m_pipeline.vertexInBinding;
}
