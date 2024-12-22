#include "ScreenSpacePass.h"
#include "core/RandGen.h"

CSSRBlurPass::CSSRBlurPass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
{
}

CSSRBlurPass::~CSSRBlurPass()
{
}

bool CSSRBlurPass::CreatePipeline(CVulkanRHI::Pipeline p_Pipeline)
{
	CVulkanRHI::ShaderPaths ssaoShaderpaths{};
	ssaoShaderpaths.shaderpath_compute = g_EnginePath / "shaders/spirv/SSRBlur.comp.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	if (!m_rhi->CreateComputePipeline(ssaoShaderpaths, m_pipeline, "ColorBlurComputePipleine"))
		return false;

	return true;
}

bool CSSRBlurPass::Update(UpdateData*)
{
	return true;
}

bool CSSRBlurPass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId = p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;

	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	m_rhi->InsertMarker(cmdBfr, "Blur SSR Pass 1/2");
	{
		vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
		vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	}

	//m_rhi->InsertMarker(cmdBfr, "Blur SSR Pass 2/2");
	//{
	//	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
	//	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	//	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	//}

	//// Downsample Blurred Color buffer
	//{
	//	CVulkanRHI::Image colorBlurRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_ColorBlur);
	//	int32_t mipWidth = colorBlurRT.width;
	//	int32_t mipHeight = colorBlurRT.height;
	//	VkImageLayout curLayout = colorBlurRT.curLayout[0];
	//	for (uint32_t i = 1; i < colorBlurRT.GetLevelCount(); i++)
	//	{
	//		m_rhi->InsertMarker(cmdBfr, std::string("Downsampling Color: Res/" + std::to_string(pow(2, i))).c_str());
	//		uint32_t baseMipLevel = i - 1;
	//		m_rhi->IssueLayoutBarrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, colorBlurRT, cmdBfr, i);
	//		m_rhi->IssueLayoutBarrier(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, colorBlurRT, cmdBfr, baseMipLevel);

	//		VkImageBlit imgBlt{};
	//		imgBlt.srcOffsets[0] = { 0, 0, 0 };
	//		imgBlt.srcOffsets[1] = { mipWidth, mipHeight, 1 };
	//		imgBlt.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//		imgBlt.srcSubresource.baseArrayLayer = 0;
	//		imgBlt.srcSubresource.mipLevel = baseMipLevel;
	//		imgBlt.srcSubresource.layerCount = 1;
	//		imgBlt.dstOffsets[0] = { 0, 0, 0 };
	//		imgBlt.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
	//		imgBlt.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//		imgBlt.dstSubresource.mipLevel = i;
	//		imgBlt.dstSubresource.baseArrayLayer = 0;
	//		imgBlt.dstSubresource.layerCount = 1;
	//		m_rhi->BlitImage(cmdBfr, imgBlt, colorBlurRT.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, colorBlurRT.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, colorBlurRT.format);
	//		m_rhi->IssueLayoutBarrier(curLayout, colorBlurRT, cmdBfr, baseMipLevel);
	//		mipWidth = (mipWidth > 1) ? mipWidth / 2 : 1;
	//		mipHeight = (mipHeight > 1) ? mipHeight / 2 : 1;
	//	}
	//	m_rhi->IssueLayoutBarrier(curLayout, colorBlurRT, cmdBfr, colorBlurRT.GetLevelCount() - 1);
	//}
	
	return true;
}


CSSAOComputePass::CSSAOComputePass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
	, CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
	, m_kernelSize(64.0f)
	, m_kernelRadius(0.5f)
	, m_bias(0.015f)
	, m_noiseScale(nm::float2((float)p_rhi->GetRenderWidth() / 4.0f, (float)p_rhi->GetRenderHeight() / 4.0f))
{}

CSSAOComputePass::~CSSAOComputePass()
{
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

bool CSSAOComputePass::Update(UpdateData* p_updateData)
{
	p_updateData->uniformData->biasSSAO = m_bias;
	p_updateData->uniformData->enableSSAO = m_isEnabled;
	p_updateData->uniformData->ssaoKernelSize = m_kernelSize;
	p_updateData->uniformData->ssaoNoiseScale = m_noiseScale;
	p_updateData->uniformData->ssaoRadius = m_kernelRadius;

	return true;
}

bool CSSAOComputePass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;
	uint32_t dispatchDim_x									= m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y									= m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	//RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Compute SSAO"));
	m_rhi->InsertMarker(cmdBfr, "SSAO Compute");
	{
		p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSAO_Blur);

		vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
		vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	}
	//RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));

	return true;
}

void CSSAOComputePass::Show(CVulkanRHI* p_rhi)
{
	ImGui::Checkbox(std::to_string(m_passIndex).c_str(), &m_isEnabled);
	ImGui::SameLine(40);
	bool ssaoNode = ImGui::TreeNode("SSAO");
	if (ssaoNode)
	{
		ImGui::SliderFloat("SSAO Bias", &m_bias, 0.00f, 0.1f);
		ImGui::InputFloat2("Noise Scale", &m_noiseScale[0]);
		ImGui::InputFloat("Kernel Size", &m_kernelSize);
		ImGui::InputFloat("Kernel Radius", &m_kernelRadius);
		ImGui::TreePop();
	}
}

CSSAOBlurPass::CSSAOBlurPass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
{}

CSSAOBlurPass::~CSSAOBlurPass()
{}

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

bool CSSAOBlurPass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;

	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	//RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Compute SSAO Blur"));
	m_rhi->InsertMarker(cmdBfr, "SSAO Blur Compute");
	{
		vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
		vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	}
	//RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));

	return true;
}

CSSRComputePass::CSSRComputePass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
	, CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
	, m_maxDistance(5.0f)
	, m_resolution(1.0f)
	, m_thickness(0.11f)
	, m_steps(2)
{}

CSSRComputePass::~CSSRComputePass()
{
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

bool CSSRComputePass::Update(UpdateData* p_updateData)
{
	p_updateData->uniformData->ssrEnable		= IsEnabled();
	p_updateData->uniformData->ssrMaxDistance	= m_maxDistance;
	p_updateData->uniformData->ssrResolution	= m_resolution;
	p_updateData->uniformData->ssrThickness		= m_thickness;
	p_updateData->uniformData->ssrSteps			= m_steps;

	return true;
}

bool CSSRComputePass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId = p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;
	const CScene* scene = p_renderData->loadedAssets->GetScene();
	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	//RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "SSR"));
	{
		m_rhi->InsertMarker(cmdBfr, "SSR Compute");
		{
			p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSReflection);

			vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
			vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);
			vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
		}
	}
	//RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));

	return true;
}

void CSSRComputePass::Show(CVulkanRHI* p_rhi)
{
	ImGui::Checkbox(std::to_string(m_passIndex).c_str(), &m_isEnabled);
	ImGui::SameLine(40);
	bool ssrNode = ImGui::TreeNode("SSR");
	if (ssrNode)
	{
		ImGui::Indent();
		ImGui::SliderFloat("Max Distance", &m_maxDistance, 0.0f, 50.0f);
		ImGui::SliderFloat("Resolution", &m_resolution, 0.0f, 1.0f);
		ImGui::SliderFloat("Steps", &m_steps, 1, 50);
		ImGui::SliderFloat("Thickness", &m_thickness, 0.0f, 5.00f);

		ImGui::Unindent();
		ImGui::TreePop();
	}
}

CCopyComputePass::CCopyComputePass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
{}

CCopyComputePass::~CCopyComputePass()
{
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

bool CCopyComputePass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId = p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;
	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;
	
	//RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Copy Compute"));
	{
		p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSReflection);

		vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
		vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	}
	//RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));
	
	return true;
}