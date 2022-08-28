#include "UIPass.h"
#include "external/imgui/imgui.h"


CUIPass::CUIPass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
	m_frameBuffer.resize(FRAME_BUFFER_COUNT);
}

CUIPass::~CUIPass()
{
}

bool CUIPass::CreateRenderpass(RenderData* p_renderData)
{
	CVulkanRHI::Image renderTarget			= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_LightDepth);
	CVulkanRHI::Renderpass* renderpass		= &m_pipeline.renderpassData; 

	renderpass->AttachColor(VK_FORMAT_B8G8R8A8_UNORM,		VK_ATTACHMENT_LOAD_OP_LOAD,					VK_ATTACHMENT_STORE_OP_STORE,
															VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
															VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	if (!m_rhi->CreateRenderpass(*renderpass))
		return false;

	renderpass->framebufferWidth			= m_rhi->GetRenderWidth();
	renderpass->framebufferHeight			= m_rhi->GetRenderHeight();
	std::vector<VkImageView> attachments(1, VkImageView{});
	attachments[0] = m_rhi->GetSCImageView(0);
	if (!m_rhi->CreateFramebuffer(renderpass->renderpass, m_frameBuffer[0], attachments.data(), (uint32_t)attachments.size(), renderpass->framebufferWidth, renderpass->framebufferHeight))
		return false;

	attachments[0] = m_rhi->GetSCImageView(1);
	if (!m_rhi->CreateFramebuffer(renderpass->renderpass, m_frameBuffer[1], attachments.data(), (uint32_t)attachments.size(), renderpass->framebufferWidth, renderpass->framebufferHeight))
		return false;

	return true;
}

bool CUIPass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	VkPushConstantRange uiPushrange{};
	uiPushrange.offset							= 0;
	uiPushrange.size							= sizeof(CRenderableUI::UIPushConstant);
	uiPushrange.stageFlags						= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkVertexInputBindingDescription uiVertexInputBinding = {};
	uiVertexInputBinding.binding				= 0;
	uiVertexInputBinding.stride					= sizeof(ImDrawVert);
	uiVertexInputBinding.inputRate				= VK_VERTEX_INPUT_RATE_VERTEX;

	//	layout (location = 0) in vec2 pos;
	//	layout (location = 1) in vec2 uv;
	//	layout (location = 2) in vec4 color;
	std::vector<VkVertexInputAttributeDescription> uiVertexAttributs;
	VkVertexInputAttributeDescription attribDesc{};
	// Attribute location 0: pos
	attribDesc.binding							= 0;
	attribDesc.location							= 0;
	attribDesc.format							= VK_FORMAT_R32G32_SFLOAT;
	attribDesc.offset							= offsetof(ImDrawVert, pos);
	uiVertexAttributs.push_back(attribDesc);

	// Attribute location 1: uv
	attribDesc.binding							= 0;
	attribDesc.location							= 1;
	attribDesc.format							= VK_FORMAT_R32G32_SFLOAT;
	attribDesc.offset							= offsetof(ImDrawVert, uv);
	uiVertexAttributs.push_back(attribDesc);

	// Attribute location 2: color
	attribDesc.binding							= 0;
	attribDesc.location							= 2;
	attribDesc.format							= VK_FORMAT_R8G8B8A8_UNORM;
	attribDesc.offset							= offsetof(ImDrawVert, col);
	uiVertexAttributs.push_back(attribDesc);

	CVulkanRHI::ShaderPaths uiShaderpaths{};
	uiShaderpaths.shaderpath_vertex				= g_EnginePath + "/shaders/spirv/UI.vert.spv";
	uiShaderpaths.shaderpath_fragment			= g_EnginePath + "/shaders/spirv/UI.frag.spv";
	m_pipeline.pipeLayout						= p_Pipeline.pipeLayout;
	m_pipeline.vertexInBinding					= uiVertexInputBinding;
	m_pipeline.vertexAttributeDesc				= uiVertexAttributs;
	m_pipeline.cullMode							= VK_CULL_MODE_NONE;
	m_pipeline.enableBlending					= true;
	m_pipeline.enableDepthTest					= false;
	m_pipeline.enableDepthWrite					= false;
	if (!m_rhi->CreateGraphicsPipeline(uiShaderpaths, m_pipeline))
	{
		std::cout << "Error Creating UI Pipeline" << std::endl;
		return false;
	}


	return true;
}

bool CUIPass::Update(UpdateData* p_updateData)
{
	return true;
}

bool CUIPass::Render(RenderData* p_renderData)
{
	uint32_t scId								= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr			= p_renderData->cmdBfr;
	CVulkanRHI::Renderpass renderPass			= m_pipeline.renderpassData;
	const CRenderableUI* ui						= p_renderData->loadedAssets->GetUI();
	const CPrimaryDescriptors* primaryDesc		= p_renderData->primaryDescriptors;
	
	const_cast<CRenderableUI*>(ui)->PreDraw(m_rhi, p_renderData->scIdx);

	ImDrawData* drawData						= ImGui::GetDrawData();
	int fbWidth									= (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	int fbHeight								= (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "UI"));

	m_rhi->SetClearColorValue(renderPass, 0, VkClearColorValue{ 0.45f, 0.55f, 0.60f, 1.00f });
	m_rhi->BeginRenderpass(m_frameBuffer[p_renderData->scIdx], renderPass, cmdBfr);

	m_rhi->SetViewport(cmdBfr, 0.0f, 1.0f, (float)renderPass.framebufferWidth, (float)renderPass.framebufferHeight);

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);

	{
		CRenderableUI::UIPushConstant pc{};
		pc.scale[0]								= (2.0f / drawData->DisplaySize.x);
		pc.scale[1]								= (2.0f / drawData->DisplaySize.y);

		pc.offset[0]							= (-1.0f - drawData->DisplayPos.x * pc.scale[0]);
		pc.offset[1]							= (-1.0f - drawData->DisplayPos.y * pc.scale[1]);

		VkPipelineStageFlags stgFlags			= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		vkCmdPushConstants(cmdBfr, m_pipeline.pipeLayout, stgFlags, 0, sizeof(CRenderableUI::UIPushConstant), (void*)&pc);
	}

	// Bind Vertex And Index Buffer:
	if (drawData->TotalVtxCount > 0)
	{
		VkBuffer vrtxBfrs[1]					= { ui->GetVertexBuffer(p_renderData->scIdx)->descInfo.buffer };
		VkDeviceSize vrtxOffset[1]				= { 0 };
		vkCmdBindVertexBuffers(cmdBfr, 0, 1, vrtxBfrs, vrtxOffset);
		vkCmdBindIndexBuffer(cmdBfr, ui->GetIndexBuffer(p_renderData->scIdx)->descInfo.buffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
	}

	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_UI, 1, ui->GetDescriptorSet(), 0, nullptr);

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clipOff								= drawData->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clipScale							= drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	int globalVtxOffset							= 0;
	int globalIdxOffset							= 0;
	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList* cmdList = drawData->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];

			{
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clipMin((pcmd->ClipRect.x - clipOff.x) * clipScale.x, (pcmd->ClipRect.y - clipOff.y) * clipScale.y);
				ImVec2 clipMax((pcmd->ClipRect.z - clipOff.x) * clipScale.x, (pcmd->ClipRect.w - clipOff.y) * clipScale.y);

				if (clipMin.x < 0.0f) { clipMin.x = 0.0f; }
				if (clipMin.y < 0.0f) { clipMin.y = 0.0f; }
				if (clipMax.x > fbWidth) { clipMax.x = (float)fbWidth; }
				if (clipMax.y > fbHeight) { clipMax.y = (float)fbHeight; }
				if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
					continue;

				// Apply scissor/clipping rectangle
				m_rhi->SetScissors(cmdBfr, (int32_t)(clipMin.x), (int32_t)(clipMin.y), (uint32_t)(clipMax.x - clipMin.x), (uint32_t)(clipMax.y - clipMin.y));
				vkCmdDrawIndexed(cmdBfr, pcmd->ElemCount, 1, pcmd->IdxOffset + globalIdxOffset, pcmd->VtxOffset + globalVtxOffset, 0);
			}
		}
		globalIdxOffset += cmdList->IdxBuffer.Size;
		globalVtxOffset += cmdList->VtxBuffer.Size;
	}

	VkRect2D scissor = { { 0, 0 }, { (uint32_t)fbWidth, (uint32_t)fbHeight } };
	vkCmdSetScissor(cmdBfr, 0, 1, &scissor);

	m_rhi->EndRenderPass(cmdBfr);
	m_rhi->EndCommandBuffer(cmdBfr);

	return true;
}

void CUIPass::Destroy()
{
	m_rhi->DestroyFramebuffer(m_frameBuffer[0]);
	m_rhi->DestroyFramebuffer(m_frameBuffer[1]);
	m_rhi->DestroyRenderpass(m_pipeline.renderpassData.renderpass);
	m_rhi->DestroyPipeline(m_pipeline);
}

void CUIPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription					= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription						= m_pipeline.vertexInBinding;
}

CDebugDrawPass::CDebugDrawPass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
	m_frameBuffer.resize(FRAME_BUFFER_COUNT);
}

CDebugDrawPass::~CDebugDrawPass()
{
}

bool CDebugDrawPass::CreateRenderpass(RenderData* p_renderData)
{
	CVulkanRHI::Renderpass* renderPass						= &m_pipeline.renderpassData;
	CVulkanRHI::Image primaryDepthRT						= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryDepth);

	renderPass->AttachColor(VK_FORMAT_B8G8R8A8_UNORM,		VK_ATTACHMENT_LOAD_OP_LOAD,					VK_ATTACHMENT_STORE_OP_STORE,
																VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
																VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	renderPass->AttachDepth(primaryDepthRT.format,			VK_ATTACHMENT_LOAD_OP_LOAD,						VK_ATTACHMENT_STORE_OP_STORE,
															VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
															VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	if (!m_rhi->CreateRenderpass(*renderPass))
		return false;
		
	renderPass->framebufferWidth							= primaryDepthRT.width;
	renderPass->framebufferHeight							= primaryDepthRT.height;

	std::vector<VkImageView> attachments(2, VkImageView{});
	attachments[0]											= m_rhi->GetSCImageView(0);
	attachments[1]											= primaryDepthRT.descInfo.imageView;
	RETURN_FALSE_IF_FALSE(m_rhi->CreateFramebuffer(renderPass->renderpass, m_frameBuffer[0], attachments.data(), (uint32_t)attachments.size(), renderPass->framebufferWidth, renderPass->framebufferHeight));

	attachments[0]											= m_rhi->GetSCImageView(1);
	RETURN_FALSE_IF_FALSE(m_rhi->CreateFramebuffer(renderPass->renderpass, m_frameBuffer[1], attachments.data(), (uint32_t)attachments.size(), renderPass->framebufferWidth, renderPass->framebufferHeight));

	return true;
}

bool CDebugDrawPass::CreatePipeline(CVulkanRHI::Pipeline p_Pipeline)
{
	struct DebugVertex
	{
		nm::float3	pos;
		int			id;
	};

	VkVertexInputBindingDescription vertexInputBinding		= {};
	vertexInputBinding.binding								= 0;
	vertexInputBinding.stride								= sizeof(DebugVertex);
	vertexInputBinding.inputRate							= VK_VERTEX_INPUT_RATE_VERTEX;

	//	layout (location = 0) in vec3 pos;
	//	layout (location = 1) in int entity id;
	std::vector<VkVertexInputAttributeDescription> vertexAttributs;
	// Attribute location 0: pos
	VkVertexInputAttributeDescription attribDesc{};
	attribDesc.binding										= 0;
	attribDesc.location										= 0;
	attribDesc.format										= VK_FORMAT_R32G32B32_SFLOAT;
	attribDesc.offset										= offsetof(DebugVertex, pos);
	vertexAttributs.push_back(attribDesc);

	attribDesc.binding										= 0;
	attribDesc.location										= 1;
	attribDesc.format										= VK_FORMAT_R32_SINT;
	attribDesc.offset										= offsetof(DebugVertex, id);
	vertexAttributs.push_back(attribDesc);

	CVulkanRHI::ShaderPaths shadowPassShaderpaths{};
	shadowPassShaderpaths.shaderpath_vertex					= g_EnginePath + "/shaders/spirv/DebugDisplay.vert.spv";
	shadowPassShaderpaths.shaderpath_fragment				= g_EnginePath + "/shaders/spirv/DebugDisplay.frag.spv";
	m_pipeline.pipeLayout									= p_Pipeline.pipeLayout;
	m_pipeline.vertexInBinding								= vertexInputBinding;
	m_pipeline.vertexAttributeDesc							= vertexAttributs;
	m_pipeline.cullMode										= VK_CULL_MODE_NONE;
	m_pipeline.enableDepthTest								= true;
	m_pipeline.enableDepthWrite								= true;
	m_pipeline.isWireframe									= true;
	if (!m_rhi->CreateGraphicsPipeline(shadowPassShaderpaths, m_pipeline))
	{
		std::cout << "Error Creating Debug Display Pipeline" << std::endl;
		return false;
	}

	return true;
}

bool CDebugDrawPass::Update(UpdateData*)
{
	return true;
}

bool CDebugDrawPass::Render(RenderData* p_renderData)
{
	uint32_t scId												= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr							= p_renderData->cmdBfr;
	CVulkanRHI::Renderpass renderPass							= m_pipeline.renderpassData;
	CRenderableDebug* debugRender								= p_renderData->fixedAssets->GetDebugRenderer();
	const CPrimaryDescriptors* primaryDesc						= p_renderData->primaryDescriptors;

	RETURN_FALSE_IF_FALSE(debugRender->PreDraw(m_rhi, scId, p_renderData->fixedAssets->GetFixedBuffers(), p_renderData->sceneGraph));	

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Debug Draw"));

	m_rhi->BeginRenderpass(m_frameBuffer[p_renderData->scIdx], renderPass, cmdBfr);

	m_rhi->SetViewport(cmdBfr, 0.0f, 1.0f, (float)renderPass.framebufferWidth, -(float)renderPass.framebufferHeight);
	m_rhi->SetScissors(cmdBfr, 0, 0, renderPass.framebufferWidth, renderPass.framebufferHeight);

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);

	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_DebugDisplay, 1, debugRender->GetDescriptorSet(), 0, nullptr);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(cmdBfr, 0, 1, &debugRender->GetVertexBuffer(scId)->descInfo.buffer, offsets);
	vkCmdBindIndexBuffer(cmdBfr, debugRender->GetIndexBuffer(scId)->descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmdBfr, (uint32_t)debugRender->GetIndexBufferCount(), 1, 0, 0, 1);
	
	m_rhi->EndRenderPass(cmdBfr);
	m_rhi->EndCommandBuffer(cmdBfr);
	
	return true;
}

void CDebugDrawPass::Destroy()
{
	// No need to destroy renderpass and frame buffers becuase they have been resused from forward pass.
	m_rhi->DestroyPipeline(m_pipeline);
}

void CDebugDrawPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription = m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription = m_pipeline.vertexInBinding;
}
