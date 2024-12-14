#include "ScreenSpacePass.h"
#include "core/RandGen.h"

CSSAOComputePass::CSSAOComputePass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
	m_frameBuffer.resize(1);
}

CSSAOComputePass::~CSSAOComputePass()
{
}

bool CSSAOComputePass::CreateRenderpass(RenderData* p_renderData)
{
	// compute pass, no Renderpass to create
	return true;
}

bool CSSAOComputePass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	CVulkanRHI::ShaderPaths ssaoShaderpaths{};
	ssaoShaderpaths.shaderpath_compute = g_EnginePath /"shaders/spirv/SSAO.comp.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	if (!m_rhi->CreateComputePipeline(ssaoShaderpaths, m_pipeline, "SSAOComputePipeline"))
		return false;

	return true;
}

bool CSSAOComputePass::Update(UpdateData*)
{
	return true;
}

bool CSSAOComputePass::Render(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;
	uint32_t dispatchDim_x									= m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y									= m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Compute SSAO"));

	p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSAO_Blur);

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);

	RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));

	return true;
}

void CSSAOComputePass::Destroy()
{
	m_rhi->DestroyPipeline(m_pipeline);
}

void CSSAOComputePass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription					= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription						= m_pipeline.vertexInBinding;
}

CSSAOBlurPass::CSSAOBlurPass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
}

CSSAOBlurPass::~CSSAOBlurPass()
{
}

bool CSSAOBlurPass::CreateRenderpass(RenderData* p_renderData)
{
	// compute pass, not Renderpass to create
	return true;
}

bool CSSAOBlurPass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	CVulkanRHI::ShaderPaths ssaoBlurShaderpaths{};
	ssaoBlurShaderpaths.shaderpath_compute = g_EnginePath /"shaders/spirv/SSAOBlur.comp.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	if (!m_rhi->CreateComputePipeline(ssaoBlurShaderpaths, m_pipeline, "SSAOBlurComputePipeline"))
		return false;

	return true;
}

bool CSSAOBlurPass::Update(UpdateData*)
{
	return true;
}

bool CSSAOBlurPass::Render(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;

	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Compute SSAO Blur"));

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);

	RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));

	return true;
}

void CSSAOBlurPass::Destroy()
{
	m_rhi->DestroyPipeline(m_pipeline);
}

void CSSAOBlurPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription			= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription				= m_pipeline.vertexInBinding;
}


CSSRComputePass::CSSRComputePass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
	, CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
	, m_maxDistance(5.0f)
	, m_resolution(1.0f)
	, m_thickness(0.11f)
	, m_steps(2)
{
	m_frameBuffer.resize(1);
}

CSSRComputePass::~CSSRComputePass()
{
}

bool CSSRComputePass::CreateRenderpass(RenderData* p_renderData)
{
	// compute pass, not Renderpass to create
	return true;
}

bool CSSRComputePass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	CVulkanRHI::ShaderPaths ssaoShaderpaths{};
	ssaoShaderpaths.shaderpath_compute = g_EnginePath / "shaders/spirv/SSR.comp.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	if (!m_rhi->CreateComputePipeline(ssaoShaderpaths, m_pipeline, "SSRComputePipeline"))
		return false;
	
	return true;
}

bool CSSRComputePass::Update(UpdateData*)
{
	return true;
}

bool CSSRComputePass::Render(RenderData* p_renderData)
{
	uint32_t scId = p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;
	const CScene* scene = p_renderData->loadedAssets->GetScene();
	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "SSR"));

	// Down sample Normal Maps
	{
		CVulkanRHI::Image normalRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Normal);

		int32_t mipWidth = normalRT.width;
		int32_t mipHeight = normalRT.height;
		VkImageLayout curLayout = normalRT.curLayout;
		for (uint32_t i = 1; i < normalRT.levelCount; i++)
		{
			m_rhi->InsertMarker(cmdBfr, std::string("Down sampling Normal: Res/" + std::to_string(pow(2,i))).c_str());
			uint32_t baseMipLevel = i - 1;

			m_rhi->IssueLayoutBarrier(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, normalRT, cmdBfr, baseMipLevel);
			
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
			m_rhi->BlitImage(cmdBfr, imgBlt, normalRT.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, normalRT.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, normalRT.format);

			m_rhi->IssueLayoutBarrier(curLayout, normalRT, cmdBfr, baseMipLevel);

			mipWidth = (mipWidth > 1) ? mipWidth / 2 : 1;
			mipHeight = (mipHeight > 1) ? mipHeight / 2 : 1;	
		}
	}

	m_rhi->InsertMarker(cmdBfr, "Compute");
	{
		p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSReflection);

		vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);
		vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	}

	RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));

	return true;
}

void CSSRComputePass::Destroy()
{
	m_rhi->DestroyPipeline(m_pipeline);
}

void CSSRComputePass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription = m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription = m_pipeline.vertexInBinding;
}

void CSSRComputePass::Show(CVulkanRHI* p_rhi)
{
	bool ssrNode = ImGui::TreeNode("SSR");
	ImGui::SameLine(75);
	bool isSSREnabled = IsEnabled();
	ImGui::Checkbox(" ", &isSSREnabled);
	Enable(isSSREnabled);

	if (ssrNode)
	{
		ImGui::Indent();
		ImGui::SliderFloat("Max Distance", &m_maxDistance, 0.0f, 50.0f);
		ImGui::SliderFloat("Resolution", &m_resolution, 0.0f, 1.0f);
		ImGui::SliderFloat("Steps", &m_steps, 1, 50);
		ImGui::SliderFloat("Thickness", &m_thickness, 0.0f, 4.0f);

		ImGui::Unindent();
		ImGui::TreePop();
	}
}

CCopyComputePass::CCopyComputePass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
	m_frameBuffer.resize(1);
}

CCopyComputePass::~CCopyComputePass()
{
}

bool CCopyComputePass::CreateRenderpass(RenderData* p_renderData)
{
	// compute pass, not Renderpass to create
	return true;
}

bool CCopyComputePass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	CVulkanRHI::ShaderPaths ssaoShaderpaths{};
	ssaoShaderpaths.shaderpath_compute = g_EnginePath / "shaders/spirv/Copy.comp.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	if (!m_rhi->CreateComputePipeline(ssaoShaderpaths, m_pipeline, "CopyComputePipeline"))
		return false;

	return true;
}

bool CCopyComputePass::Update(UpdateData*)
{
	return true;
}

bool CCopyComputePass::Render(RenderData* p_renderData)
{
	uint32_t scId = p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;
	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;
	
	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Copy Compute"));

	p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSReflection);
	
	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	
	RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));
	
	return true;
}

void CCopyComputePass::Destroy()
{
	m_rhi->DestroyPipeline(m_pipeline);
}

void CCopyComputePass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription = m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription = m_pipeline.vertexInBinding;
}