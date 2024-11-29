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

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Compute SSR"));

	p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSReflection);
	
	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);

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